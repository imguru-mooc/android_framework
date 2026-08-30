# [D4-3] Lab 4-2 — SplitScreenDemo: 화면 분할의 두 방식

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 4 Ch 11 · 필수 | 배포 `SplitScreenDemo` |

> 🎯 **실습 목표** — 앱 내부 레이아웃 분할(한 창)과 시스템 멀티윈도(여러 창)의 차이를 dumpsys로 구분한다 — 차량 센터 디스플레이 분할 UI의 축소판.

## Step 1. 실행 — 좌우 분할 UI

앱 실행: 좌(리스트)/우(미디어) FrameLayout 2분할 화면. 코드 리딩:

```xml
<LinearLayout orientation="horizontal">
    <FrameLayout id="@+id/left"  layout_weight="1"/>
    <FrameLayout id="@+id/right" layout_weight="1"/>
</LinearLayout>
```

## Step 2. 창의 개수 확인

```bat
adb shell dumpsys window windows | findstr /i "splitscreendemo"
```

✅ **예상 결과:** WindowState **1개** — 화면은 둘로 보여도 WMS 시점에선 **한 창**(뷰 분할)

## Step 3. 시스템 분할과 대조 (개념 확인)

```text
뷰 분할(오늘):    Window 1개 · 앱이 내부에서 영역 배분 · 다른 앱 불가
시스템 멀티윈도:  Task/Window 여러 개 · WMS가 배치 · 다른 앱 조합 가능
차량 멀티디스플레이(㉖): DisplayContent 자체가 여러 개
```

✅ **예상 결과:** 세 방식의 차이를 각 1문장으로 구분 설명

💡 확장 실험(선택): `am start --windowingMode` 지원 여부를 확인해 프리폼/분할 모드로 두 앱을 띄워 Window 수 변화를 관찰.

# 🏁 Pass 판정 체크리스트

- [ ] 분할 UI 실행 + WindowState 1개 확인
- [ ] 세 방식(뷰/시스템/디스플레이) 구분 설명 완성

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| findstr 미검출 | 패키지명 상이 | dumpsys 전체에서 앱 이름 검색 |

# 🚗 활용 포인트

💡 요구사항 "화면을 나눠 주세요"를 들으면 먼저 물어야 합니다 — **한 앱 안 분할인가, 앱 간 분할인가, 디스플레이 분리인가?** 구현 계층이 완전히 달라집니다(뷰 ↔ WMS ↔ DisplayContent).

---
*실습 D4-3 (32/36) · 다음: **D4-4 N-1 Native Binder 서비스***
