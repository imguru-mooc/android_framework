# [D3-5] 관찰 3-B — gfxinfo로 프레임 성능 읽기

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 3 Ch 10 · 필수 | 로컬 에뮬레이터 + HvacSimulator · 🎬 ㉗ 선시청 |

> 🎯 **실습 목표** — `dumpsys gfxinfo`의 **percentile·Janky 비율**을 읽고, 의도적 부하 전/후를 비교해 "평균은 거짓말, 꼬리가 진실"을 체득한다.

## Step 1. 기준 측정 (정상 상태)

```bat
adb shell dumpsys gfxinfo com.example.hvacsimulator reset
:: 앱에서 10초간 팬 애니메이션·버튼 조작
adb shell dumpsys gfxinfo com.example.hvacsimulator | findstr /i "Janky percentile Total frames"
```

✅ **예상 결과(예):**

```text
Total frames rendered: 412
Janky frames: 6 (1.4%)
90th percentile: 9ms / 95th: 12ms / 99th: 17ms
```

기록: Janky __% / 99th __ms

## Step 2. 부하 유발 → 재측정

앱의 부하 토글(강사 배포 버전: onClick에서 대형 Bitmap 디코딩)을 켜고 동일하게 10초 조작 → reset 없이 측정 명령 재실행 대신 **reset부터 다시**:

```bat
adb shell dumpsys gfxinfo com.example.hvacsimulator reset
:: 부하 상태로 10초 조작
adb shell dumpsys gfxinfo com.example.hvacsimulator | findstr /i "Janky percentile"
```

✅ **예상 결과:** Janky 수배 상승(예: 15~25%), 99th가 16.6ms를 크게 초과 — 사용자 체감 '끊김'의 숫자 증거

## Step 3. 프레임 버킷 훑기 (선택)

```bat
adb shell dumpsys gfxinfo com.example.hvacsimulator framestats | head -20
```

💡 열 의미는 몰라도 됩니다 — "프레임 단위 타임스탬프가 전부 남는다"는 사실만. 정밀 분석은 Perfetto의 몫.

# 🏁 Pass 판정 체크리스트

- [ ] 정상/부하 2회 측정표 완성 (Janky%, 99th)
- [ ] "평균 대신 percentile을 보는 이유" 1문장
- [ ] reset→조작→측정 순서를 지켰음 (측정 위생)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| No process found | 패키지명 오타/미실행 | 앱 먼저 실행, `pidof` 로 확인 |
| 수치가 0/비어 있음 | reset 후 조작 없음 | 10초 이상 실제 프레임 발생시키기 |

# 🚗 현업 활용 포인트

💡 성능 티켓의 표준 첨부물이 이 두 줄(Janky%, 99th)입니다. 워크숍 S4에서 이 숫자로 시작해 **원인은 앱 코드**임을 밝히는 2단 추적을 완성합니다.

---
*실습 D3-5 (29/36) · 다음: **D4-1 관찰 4-A 두 장부 대응***
