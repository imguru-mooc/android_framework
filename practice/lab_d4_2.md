# [D4-2] Lab 4-1 — FloatingOverlayDemo: 시스템 오버레이 창

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 4 Ch 11 · 필수 | 배포 `FloatingOverlayDemo` |

> 🎯 **실습 목표** — 앱 밖에 뜨는 **TYPE_APPLICATION_OVERLAY** 창을 띄우고, appops 권한과 dumpsys에서의 정체를 확인한다.

## Step 1. 권한 부여 (appops)

```bat
adb install FloatingOverlayDemo.apk
adb shell appops set com.example.floatingoverlaydemo SYSTEM_ALERT_WINDOW allow
```

## Step 2. 실행 — 플로팅 뷰 확인

앱 실행 → "오버레이 시작" → 홈으로 나가도 떠 있는 작은 뷰 확인. 핵심 코드 리딩:

```kotlin
val params = WindowManager.LayoutParams(
    WRAP_CONTENT, WRAP_CONTENT,
    WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,   // ★ 창 타입
    FLAG_NOT_FOCUSABLE, PixelFormat.TRANSLUCENT)
windowManager.addView(floatingView, params)                 // Activity 없이 창 추가!
```

✅ **예상 결과:** 다른 앱 위에도 떠 있는 뷰 — `Settings.canDrawOverlays()` 가 true

## Step 3. dumpsys에서 정체 확인

```bat
adb shell dumpsys window windows | findstr /i "floatingoverlay TYPE_APPLICATION_OVERLAY 2038"
```

✅ **예상 결과:** 해당 Window의 타입이 APPLICATION_OVERLAY(2038) — ㉕ 트리에서 **Task 밖**에 사는 창

## Step 4. 권한 회수 대조 실험

```bat
adb shell appops set com.example.floatingoverlaydemo SYSTEM_ALERT_WINDOW deny
:: 앱에서 다시 시작 시도
```

✅ **예상 결과:** addView 시점 예외/미표시 — 권한이 창 타입을 지배함

# 🏁 Pass 판정 체크리스트

- [ ] 플로팅 뷰가 타 앱 위에 표시
- [ ] dumpsys에서 타입 2038 라인 확보
- [ ] deny 시 실패 확인 후 allow 복구

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| addView에서 BadTokenException | 권한 미부여 | Step 1 appops / 설정 화면 허용 |
| 뷰가 터치를 다 먹음 | FLAG 설정 | NOT_FOCUSABLE/NOT_TOUCH_MODAL 조합 검토 |

# 🚗 활용 포인트

💡 차량의 **경고 팝업·음성 어시스턴트 오버레이**가 이 창 타입 계열입니다. 단 Automotive에선 주행 중 표시 정책(운전자 주의 분산)이 겹치므로, 타입+권한+정책 3중 관문으로 기억하세요.

---
*실습 D4-2 (31/36) · 다음: **D4-3 Lab 4-2 SplitScreenDemo***
