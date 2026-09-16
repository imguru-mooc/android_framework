# 실습 7. JNI Native Method — Java → C · C → Java 객체 생성 · RegisterNatives

> **소요시간:** 35분 · **난이도:** ★★☆ · **챕터:** Ch 2 JNI
> **디렉토리:** `~/android/day3/jni_test/{First,Second,Third}` · **호스트 JDK** 로 빌드 (Emulator 불필요)

## 목표

- `native` 선언 → `javac -h` 헤더 → `Java_Class_method` 규칙으로 C 함수를 연결한다 (First).
- C 에서 `FindClass / GetMethodID / NewObject` 로 **Java 객체를 만들어 반환**한다 (Second).
- `JNI_OnLoad` + `RegisterNatives` 로 이름 규칙 없이 매핑한다 (Third) — Framework JNI 방식.

---

## First — Java → C

**`First/Hello.java`**

```java
class Hello {
    native void foo();
    public static void main(String args[]) {
        System.out.println("Hello java");
        System.loadLibrary("Hello");        // libHello.so
        Hello hello = new Hello();
        hello.foo();
    }
}
```

**`First/Hello.c`**

```c
#include <stdio.h>
#include "Hello.h"

JNIEXPORT void JNICALL Java_Hello_foo(JNIEnv* env, jobject object) {   // ★ Java_<Class>_<method>
    printf("NATIVE : Java_Hello_foo()\n");
}
```

```bash
cd ~/android/day3/jni_test/First
javac -h . Hello.java                  # Hello.class + Hello.h 생성
cat Hello.h                            # JNIEXPORT void JNICALL Java_Hello_foo(JNIEnv *, jobject);
gcc -shared -fPIC -o libHello.so Hello.c \
    -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
LD_LIBRARY_PATH=. java Hello
```

```text
Hello java
NATIVE : Java_Hello_foo()
```

`$JAVA_HOME` 이 비어 있으면 `export JAVA_HOME=$(dirname $(dirname $(readlink -f $(which javac))))`.

## Second — C 에서 Java 객체 생성

**`Second/JniFuncMain.java`**

```java
class JniFuncMain {
    static { System.loadLibrary("jnifunc"); }
    public static native JniTest createJniObject();
    public static void main(String args[]) {
        System.out.println("Hello java");
        JniTest jniObj = createJniObject();
    }
}
class JniTest {
    public JniTest(int num) { System.out.println("JniTest.JniTest() , num=" + num); }
}
```

**`Second/jnifunc.c`**

```c
#include <stdio.h>
#include "JniFuncMain.h"

JNIEXPORT jobject JNICALL Java_JniFuncMain_createJniObject(JNIEnv* env, jclass clazz) {
    jclass    cls = (*env)->FindClass(env, "JniTest");                 // ★ 클래스 찾기
    jmethodID mid = (*env)->GetMethodID(env, cls, "<init>", "(I)V");   // ★ 생성자 = "<init>", (int)void
    if (cls == NULL || mid == NULL) return NULL;
    jobject obj = (*env)->NewObject(env, cls, mid, 100);               // ★ new JniTest(100)
    printf("NATIVE : JniTest object created\n");
    return obj;                                                         // Local ref — 반환하면 Java 가 받는다
}
```

```bash
cd ../Second
javac -h . JniFuncMain.java
gcc -shared -fPIC -o libjnifunc.so jnifunc.c -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
LD_LIBRARY_PATH=. java JniFuncMain
```

```text
Hello java
JniTest.JniTest() , num=100
NATIVE : JniTest object created
```

시그니처 문자열 규칙: `I`=int, `J`=long, `Z`=boolean, `V`=void, `Ljava/lang/String;`=객체, `[I`=int[]. `javap -s JniTest` 로 확인할 수 있다.

## Third — JNI_OnLoad + RegisterNatives

**`Third/JniFuncMain.java`**

```java
class JniFuncMain {
    static { System.loadLibrary("jnimap"); }
    public native void foo();
    public static void main(String args[]) {
        System.out.println("Hello java");
        new JniFuncMain().foo();
    }
}
```

**`Third/foo.h` / `foo.cpp`**

```cpp
#include <jni.h>
#include <stdio.h>

static void foo(JNIEnv* env, jobject obj) {               // ★ 이름 규칙 무관 (static 가능)
    printf("Native : foo()\n");
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {              // ★ System.loadLibrary 직후 1회
    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;

    jclass clazz = env->FindClass("JniFuncMain");
    JNINativeMethod nm[] = {
        { (char*)"foo", (char*)"()V", (void*)foo },        // ★ Java 이름 · 시그니처 · C 함수
    };
    env->RegisterNatives(clazz, nm, sizeof(nm) / sizeof(nm[0]));
    return JNI_VERSION_1_6;
}
```

```bash
cd ../Third
javac JniFuncMain.java                 # -h 불필요 (헤더 안 씀)
g++ -shared -fPIC -o libjnimap.so foo.cpp -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
LD_LIBRARY_PATH=. java JniFuncMain
```

`RegisterNatives` 는 심볼 이름을 노출하지 않아도 되고(`static`), 한 번에 여러 Method 를 테이블로 매핑한다. `frameworks/base/core/jni` 의 모든 파일이 이 방식이다 (`gMethods[]` + `RegisterMethodsOrDie`).

---

## 확인 포인트

- [ ] First: `javac -h` 가 만든 헤더의 함수 이름 = `Java_Hello_foo`
- [ ] Second: C 가 `new JniTest(100)` 을 만들어 Java 생성자 로그가 찍힘
- [ ] Third: `-h` 없이 `RegisterNatives` 로 연결, `foo` 가 `static`
- [ ] `javap -s` 로 시그니처 문자열 확인

## 핵심 정리

| 항목 | 내용 |
|---|---|
| `JNIEnv*` | Thread 마다 다른 포인터. 다른 Thread 에 넘기지 말 것 |
| `jclass` / `jobject` / `jmethodID` | Local ref — 함수 반환 시 무효. 보관하려면 `NewGlobalRef` |
| 이름 규칙 vs `RegisterNatives` | 앱은 규칙, Framework 는 `RegisterNatives` |
| C 와 C++ 차이 | C: `(*env)->Fn(env, …)` / C++: `env->Fn(…)` |
| Android 에서 | `.so` 를 `System.loadLibrary` 로 로드하는 것은 동일. ART 가 JVM 역할 |

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `jni.h: No such file` | `-I$JAVA_HOME/include -I$JAVA_HOME/include/linux` |
| `UnsatisfiedLinkError: no Hello in java.library.path` | `LD_LIBRARY_PATH=.` 또는 `-Djava.library.path=.` |
| `UnsatisfiedLinkError: 'void Hello.foo()'` | 함수 이름·시그니처 불일치 → 헤더 다시 생성 |
