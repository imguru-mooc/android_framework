# 실습 8. JNI Invocation API + Framework JNI 읽기

> **소요시간:** 25분 · **난이도:** ★★★ · **챕터:** Ch 2 JNI
> **디렉토리:** `~/android/day3/jni_test/Fourth` (호스트 JDK) + `frameworks/base/core/jni` (읽기)

## 목표

- Native 프로세스가 `JNI_CreateJavaVM` 으로 VM 을 **만들고** Java `main` 을 호출한다 — `app_process` 가 Zygote 를 띄우는 방식.
- Framework JNI 에서 Java 객체 ↔ C++ 객체가 이어지는 세 지점을 소스에서 찾는다: `JavaBBinder`, `NativeMessageQueue`, `Surface.mNativeObject`.

---

## Step 1. Invocation API

**`Fourth/JniFuncMain.java`**

```java
class JniFuncMain {
    public static void main(String args[]) {
        System.out.println("Hello java: " + (args.length > 0 ? args[0] : ""));
    }
}
```

**`Fourth/invocation.cpp`**

```cpp
#include <jni.h>
#include <iostream>

int main() {
    JavaVM* vm = nullptr; JNIEnv* env = nullptr;
    JavaVMInitArgs vm_args; JavaVMOption options[1];
    options[0].optionString = (char*)"-Djava.class.path=.";
    vm_args.version = JNI_VERSION_1_8;
    vm_args.options = options; vm_args.nOptions = 1;
    vm_args.ignoreUnrecognized = JNI_TRUE;

    if (JNI_CreateJavaVM(&vm, (void**)&env, &vm_args) != JNI_OK) {     // ★ VM 생성 — Process 당 1개
        std::cerr << "Failed to create Java VM\n"; return -1;
    }
    std::cout << "JVM created successfully.\n";

    jclass cls = env->FindClass("JniFuncMain");
    jmethodID mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");
    if (cls == nullptr || mid == nullptr) { env->ExceptionDescribe(); vm->DestroyJavaVM(); return -1; }

    jstring jstr = env->NewStringUTF("Hello from C++ Invocation API!!");
    jobjectArray args = env->NewObjectArray(1, env->FindClass("java/lang/String"), jstr);
    env->CallStaticVoidMethod(cls, mid, args);                             // ★ Java main 호출
    if (env->ExceptionOccurred()) { env->ExceptionDescribe(); env->ExceptionClear(); }

    vm->DestroyJavaVM();
    std::cout << "JVM destroyed.\n";
    return 0;
}
```

```bash
cd ~/android/day3/jni_test/Fourth
javac JniFuncMain.java
g++ -o invocation invocation.cpp \
    -I$JAVA_HOME/include -I$JAVA_HOME/include/linux \
    -L$JAVA_HOME/lib/server -ljvm -Wl,-rpath,$JAVA_HOME/lib/server
./invocation
```

```text
JVM created successfully.
Hello java: Hello from C++ Invocation API!!
JVM destroyed.
```

## Step 2. app_process 와 비교

```bash
cd ~/android/frameworks/base/cmds/app_process
vim app_main.cpp
/runtime.start                # AppRuntime::start("com.android.internal.os.ZygoteInit", args, zygote)
:tag AndroidRuntime::start    # → startVm() → JNI_CreateJavaVM → startReg() (RegisterNatives 전부) → CallStaticVoidMethod(main)
```

Day 1 의 `init.zygote64_32.rc` 가 실행하는 `app_process64` 가 바로 이 코드다. `AndroidRuntime::startVm` 이 ART 를 `JNI_CreateJavaVM` 으로 만들고, `startReg` 에서 Framework JNI 수백 개를 `RegisterNatives` 로 등록한 뒤 `ZygoteInit.main` 을 부른다. 실습 7 Third + 실습 8 Step 1 을 합친 것이 Zygote 다.

## Step 3. Framework JNI 세 지점

```bash
cd ~/android/frameworks/base/core/jni
```

**① Java `Binder` ↔ C++ `BBinder`**

```bash
:tag JavaBBinder
```

```cpp
class JavaBBinder : public BBinder {                       // ★ Day 2 의 BBinder
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) override {
        JNIEnv* env = javavm_to_jnienv(mVM);
        jboolean res = env->CallBooleanMethod(mObject, gBinderOffsets.mExecTransact,   // ★ Java Binder.execTransact 호출
                                              code, reinterpret_cast<jlong>(&data), reinterpret_cast<jlong>(reply), flags);
        ...
    }
    jobject const mObject;                                 // ★ Java Binder 객체의 Global ref
};
```

Day 1 의 Java `Stub.onTransact` 가 불리는 경로: Driver → Binder Thread → `JavaBBinder::onTransact` → JNI → `Binder.execTransact` → `onTransact`.

**② Java `MessageQueue` ↔ C++ `Looper`**

```bash
:tag android_os_MessageQueue_nativePollOnce
:tag NativeMessageQueue::NativeMessageQueue     # mLooper = Looper::getForThread() ?: new Looper(false)
```

**③ Java `Surface` ↔ C++ `sp<Surface>`**

```bash
:tag android_view_Surface_lockCanvas
```

```cpp
static jlong nativeLockCanvas(JNIEnv* env, jclass clazz, jlong nativeObject, jobject canvasObj, jobject dirtyRectObj) {
    sp<Surface> surface(reinterpret_cast<Surface*>(nativeObject));   // ★ Java 의 long mNativeObject 가 C++ 포인터
    ...
    surface->lock(&outBuffer, dirtyRectPtr);                           // 실습 9 의 ANativeWindow_lock
```

Java 객체가 C++ 객체를 `long` 필드로 들고, JNI 함수가 그것을 `sp<>` 로 감싸 쓴다. `incStrong` 은 Java 쪽 `nativeCreate` 때 한 번, `nativeRelease` 때 `decStrong`.

---

## 확인 포인트

- [ ] `./invocation` 이 Java `main` 을 호출하고 종료
- [ ] `AndroidRuntime::start` 에서 `JNI_CreateJavaVM` → `startReg` → `main` 순서 확인
- [ ] `JavaBBinder::onTransact` 가 `CallBooleanMethod(execTransact)` 를 부르는 줄
- [ ] `nativeLockCanvas` 의 `reinterpret_cast<Surface*>(nativeObject)`

## 핵심 정리

| Java | C++ | 연결 방식 |
|---|---|---|
| `Binder` | `JavaBBinder : BBinder` | C++ 이 Java 객체를 Global ref 로 보관, `onTransact` 에서 JNI 역호출 |
| `MessageQueue` | `NativeMessageQueue` (`sp<Looper>`) | `mPtr` long 필드 |
| `Surface` | `sp<Surface>` | `mNativeObject` long 필드 |
| `app_process` | `AndroidRuntime` | `JNI_CreateJavaVM` + `RegisterNatives` |

"Java 객체 안의 `long` 하나가 C++ 객체 포인터" — Framework JNI 의 공통 패턴이다.
