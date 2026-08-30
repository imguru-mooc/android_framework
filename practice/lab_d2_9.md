# [D2-9] JNI_test — First·Second·Third·Fourth (빌드 대기 병행)

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★☆ | Day 2 오후 2교시 · ★전원 필수 | 서버(JDK 11, D0-6) + 배포 `JNI_test/` · 🎬 ⑰ 선시청 |

> 🎯 **실습 목표** — JNI의 4기법(이름 규칙 / native→Java 객체 / RegisterNatives / Invocation API)을 각각 빌드·실행해, `android_util_Binder.cpp`가 낯설지 않은 눈을 만든다.

## Step 1. ① First — 이름 규칙 (Java_클래스_메서드)

```bash
cd ~/JNI_test/First
javac -h . Hello.java                 # Hello.h 생성 (Java_Hello_foo 선언)
g++ -shared -fPIC -o libHello.so Hello.cpp \
    -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux"
java -Djava.library.path=. Hello
```

✅ **예상 결과:** C++ 쪽 `printf` 문구 출력 — JVM이 **이름만으로** .so 심볼을 찾았습니다.

## Step 2. ② Second — native에서 Java 객체 생성

```bash
cd ../Second && javac -h . *.java
g++ -shared -fPIC -o libSecond.so second.cpp -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux"
java -Djava.library.path=. Main
```

핵심 코드 확인: `FindClass → GetMethodID("<init>") → NewObject → CallVoidMethod`

✅ **예상 결과:** native가 만든 Java 객체의 메서드 출력 — **문은 양방향**

## Step 3. ③ Third — RegisterNatives (이름 자유)

```bash
cd ../Third && javac *.java
g++ -shared -fPIC -o libThird.so third.cpp -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux"
java -Djava.library.path=. Main
```

핵심 코드 확인: `JNI_OnLoad`에서 `JNINativeMethod` 테이블 → `env->RegisterNatives(...)` — C 함수명이 아무거나여도 됨

✅ **예상 결과:** 정상 출력 + "오타는 로드 시점에 잡힌다"는 장점 토론

## Step 4. ④ Fourth — Invocation API (C++이 JVM을 만든다)

```bash
cd ../Fourth && javac Main.java
g++ -o invoke invoke.cpp -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" \
    -L"$JAVA_HOME/lib/server" -ljvm
LD_LIBRARY_PATH="$JAVA_HOME/lib/server" ./invoke
```

핵심 코드 확인: `JNI_CreateJavaVM(&jvm, (void**)&env, &args)` → `FindClass("Main")` → static main 호출

✅ **예상 결과:** C++ 실행 파일이 Java main의 출력을 냄 — **app_process/Zygote의 축소판**(🎬 ①⑰)

## Step 5. 총정리 — Framework와의 연결

```bash
grep -n "RegisterNatives\|gBinderProxyMethods" ~/aosp/frameworks/base/core/jni/android_util_Binder.cpp | head -5
```

✅ **예상 결과:** 오늘 배운 ③의 패턴이 그대로 — "Framework JNI = First~Fourth의 총합"

# 🏁 Pass 판정 체크리스트

- [ ] First~Fourth 4개 실행 출력 확보
- [ ] 각 단계 핵심 API를 한 줄씩 설명 (이름규칙/NewObject/RegisterNatives/CreateJavaVM)
- [ ] android_util_Binder.cpp에서 동일 패턴 확인

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| jni.h not found | JAVA_HOME 미설정 | `export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64` |
| UnsatisfiedLinkError | so 이름/경로 불일치 | `System.loadLibrary("Hello")` ↔ `libHello.so`, `-Djava.library.path=.` |
| Fourth 링크 에러(-ljvm) | libjvm 경로 | D0-6에서 기록한 경로를 -L/LD_LIBRARY_PATH에 |

# 🚗 현업 활용 포인트

💡 VHAL·미디어·센서 등 차량 스택의 절반은 JNI 경계 아래에 삽니다. 네이티브 크래시의 `art::JNI` 스택을 볼 때, 오늘의 4형제 중 어느 문에서 터졌는지부터 가늠하세요.

---
*실습 D2-9 (20/36) · 다음: **D2-10 실습 5-2 Binder 보안 검증***
