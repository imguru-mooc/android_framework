# [D3-1] 관찰 3-A — AMS 상태와 콜드/웜 스타트 실측

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 3 Ch 7 · 필수 | 로컬 에뮬레이터 · 🎬 ⑥㉑ 선시청 |

> 🎯 **실습 목표** — dumpsys로 Activity 스택과 프로세스 oom_adj를 읽고, 같은 앱의 **콜드 vs 웜 스타트 시간을 숫자로 비교**한다.

## Step 1. Activity 스택 읽기

```bat
adb shell dumpsys activity activities | findstr /i "ResumedActivity topResumed task"
```

✅ **예상 결과:** 현재 최상단 Activity와 Task 정보 — ㉕ 트리의 AMS쪽 시점

## Step 2. 프로세스 등급표 (oom_adj)

```bat
adb shell dumpsys activity processes | findstr /i "oom_adj PERCEPTIBLE CACHED foreground"
adb shell "cat /proc/$(pidof com.example.hvacsimulator)/oom_score_adj"
```

✅ **예상 결과:** 포그라운드 앱 = 0 근처, 홈으로 보내면 값 상승 — 홈↔복귀 반복하며 **숫자 변화 2회 기록**(🎬 ㉑)

## Step 3. 콜드 vs 웜 실측

```bat
:: 콜드: 완전 종료 후 실행
adb shell am force-stop com.example.hvacsimulator
adb shell am start -W com.example.hvacsimulator/.MainActivity
:: 출력의 TotalTime 기록 → ____ ms

:: 웜: 홈 → 재실행
adb shell input keyevent KEYCODE_HOME
adb shell am start -W com.example.hvacsimulator/.MainActivity
:: TotalTime 기록 → ____ ms
```

✅ **예상 결과:** 콜드 ≫ 웜 (수 배 차이). logcat의 `Displayed` 라인과 교차 확인:

```bat
adb logcat -d | findstr Displayed
```

# 🏁 Pass 판정 체크리스트

- [ ] Resumed Activity/Task 라인 확보
- [ ] oom_score_adj 변화 2회 기록 (포그라운드↔백그라운드)
- [ ] 콜드/웜 TotalTime 실측치 기록 + 차이 이유 1문장 (fork·attach 생략)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| am start -W 가 안 끝남 | 컴포넌트 경로 오타 | `dumpsys package <pkg> | findstr Activity` 로 정확한 이름 확인 |
| Displayed 라인 없음 | 로그 밀림 | `-t 200` 최근 로그 또는 재실행 직후 확인 |

# 🚗 현업 활용 포인트

💡 성능 회귀 리포트의 표준 증거가 `am start -W`와 `Displayed` 페어입니다. "느려졌다"를 접수하면 **먼저 콜드/웜을 분리 측정** — 부팅 직후 이슈(S3)와 상시 이슈를 가르는 첫 칼질입니다.

---
*실습 D3-1 (25/36) · 다음: **D3-2 Lab 3-1 RRO 1차***
