# [D1-3] Lab 1-1 — CalcServiceApp: 첫 AIDL 서비스

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 1 Ch 2 · 필수 | Android Studio + 배포 프로젝트 `CalcServiceApp` |

> 🎯 **실습 목표** — `ICalculatorService.aidl` → Stub 구현 → bindService 클라이언트의 3단 구조를 직접 완성하고, **컴파일러가 생성한 Stub/Proxy 코드를 눈으로 확인**한다.

## Step 1. 프로젝트 열기 & 구조 파악

🖱 Android Studio → Open → 배포된 `CalcServiceApp` → Gradle Sync

```text
app/src/main/aidl/.../ICalculatorService.aidl   ← 계약서 (일부 비어 있음)
app/src/main/java/.../CalculatorService.kt      ← Stub 구현 (TODO)
app/src/main/java/.../ClientActivity.kt         ← bindService (TODO)
AndroidManifest.xml                              ← service 선언
```

## Step 2. AIDL 계약 작성

```java
// ICalculatorService.aidl
package com.example.calcservice;

interface ICalculatorService {
    int add(int a, int b);
    int subtract(int a, int b);
}
```

🖱 **Build → Make Project** — aidl이 Java 코드를 생성합니다.

✅ **예상 결과:** 빌드 성공. `app/build/generated/aidl_source_output_dir/.../ICalculatorService.java` 생성

## Step 3. 생성 코드 열람 (오늘의 핵심 관찰)

🖱 생성된 `ICalculatorService.java` 를 열어 다음 3가지를 찾아 형광펜:

```text
① abstract class Stub extends Binder implements ICalculatorService
② static final int TRANSACTION_add = FIRST_CALL_TRANSACTION + 0
③ static class Proxy ... mRemote.transact(TRANSACTION_add, ...)
```

✅ **예상 결과:** "내가 add를 부르면 실제로는 번호(②)가 Binder를 건넌다" — 애니메이션 ⑪의 실물 확인

## Step 4. Stub 구현 (서버)

```kotlin
class CalculatorService : Service() {
    private val binder = object : ICalculatorService.Stub() {
        override fun add(a: Int, b: Int) = a + b
        override fun subtract(a: Int, b: Int) = a - b
    }
    override fun onBind(intent: Intent): IBinder = binder
}
```

## Step 5. 클라이언트 (bindService)

```kotlin
private var calc: ICalculatorService? = null
private val conn = object : ServiceConnection {
    override fun onServiceConnected(n: ComponentName, b: IBinder) {
        calc = ICalculatorService.Stub.asInterface(b)
        result.text = "3 + 4 = ${calc?.add(3, 4)}"
    }
    override fun onServiceDisconnected(n: ComponentName) { calc = null }
}
// onStart:
bindService(Intent(this, CalculatorService::class.java), conn, BIND_AUTO_CREATE)
```

✅ **예상 결과:** 앱 실행 → 화면에 `3 + 4 = 7` (그리고 subtract 결과)

💡 지금은 같은 앱(같은 프로세스)이라 `asInterface`가 **Stub 자신**을 돌려줍니다(IPC 없음). Manifest의 service에 `android:process=":remote"` 를 붙이면 진짜 IPC로 바뀝니다 — 붙여서 로그로 PID 차이를 확인해 보세요.

# 🏁 Pass 판정 체크리스트

- [ ] add/subtract 결과가 UI에 표시
- [ ] 생성 코드에서 Stub/TRANSACTION_/Proxy 3요소 확인
- [ ] `:remote` 프로세스 분리 후에도 동작 (PID 상이 확인)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| aidl 클래스가 빨간 줄 | Make 이전 / 패키지 불일치 | Build→Make, aidl 파일 package와 폴더 경로 일치 확인 |
| onServiceConnected 안 옴 | Manifest service 미선언/오타 | `<service android:name=".CalculatorService"/>` 확인 |
| :remote에서 NPE | 연결 전 calc 사용 | 반드시 onServiceConnected 이후 호출 |

# 🚗 현업 활용 포인트

💡 CarPropertyManager도 구조는 이것과 동일한 3단(계약→Stub→클라)입니다. 오늘의 손맛이 Day 2 커스텀 System Service(6-x)의 축소 리허설입니다.

---
*실습 D1-3 (10/36) · 다음: **D1-4 Lab 1-2 콜백 서비스***
