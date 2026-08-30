# [D4-1] 관찰 4-A — 두 장부 대응: window 트리 ↔ SurfaceFlinger Layer

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 4 Ch 11 · 필수 | 로컬 에뮬레이터 · 🎬 ㉕⑨ 선시청 |

> 🎯 **실습 목표** — 같은 화면을 WMS(`dumpsys window`)와 SF(`dumpsys SurfaceFlinger`) 두 시점에서 읽고, WindowState↔Layer 대응 스케치를 완성한다.

## Step 1. 장부 A — window 트리

```bat
adb shell dumpsys window windows | findstr /i "Window #"
adb shell dumpsys window | findstr mCurrentFocus
```

✅ **예상 결과:** StatusBar / NavigationBar / 앱 창 등의 `Window #N` 목록 + 현재 포커스 창

## Step 2. 장부 B — SurfaceFlinger Layer

```bat
adb shell dumpsys SurfaceFlinger --list | findstr /v "^$" 
```

✅ **예상 결과:** 비슷한 이름의 Layer 목록(앱 Activity 경로명, StatusBar 등)

## Step 3. 대응 스케치 (지면)

```text
[window]                                  [SurfaceFlinger]
Window{... StatusBar}          ◀──────▶  StatusBar#0
Window{... hvac/.MainActivity} ◀──────▶  com.example.hvac/...MainActivity#0
Window{... NavigationBar}      ◀──────▶  NavigationBar#0
(대응이 안 보이는 항목이 있으면 ✍ 메모 — 예: SF 전용 Layer)
```

✅ **예상 결과:** 최소 3쌍 매핑 + "WindowState 1개 ≈ Layer 1개, 단 SF에는 시스템 전용 Layer가 더 있다" 관찰

# 🏁 Pass 판정 체크리스트

- [ ] Window # 목록·mCurrentFocus 확보
- [ ] Layer 목록 확보
- [ ] 3쌍 이상 매핑 스케치 + 예외 항목 메모

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| --list 출력이 김/빈 줄 다수 | 정상 | findstr /v 로 정리하거나 상단만 관찰 |
| 이름이 서로 달라 못 찾음 | 축약 표기 | 패키지명 일부로 눈맞춤 |

# 🚗 현업 활용 포인트

💡 "화면에 안 보여요" 티켓의 2단 점검 — **window에 존재? → SF에 Layer? →** 둘 다 있으면 z-order/가시성, 한쪽에만 있으면 그 경계가 용의자입니다.

---
*실습 D4-1 (30/36) · 다음: **D4-2 Lab 4-1 FloatingOverlayDemo***
