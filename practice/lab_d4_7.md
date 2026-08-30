# [D4-7] 🚗 종합 트러블슈팅 워크숍 — HvacSimulator S1~S6

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★★ | Day 4 오후 2교시 · ★전원(조별) | HvacSimulator(장애 스위치판) + Day 3 과제 RRO APK · 🎬 ㉚ 사전 1회 플레이 |

> 🎯 **실습 목표** — 4일간의 모든 계층을 관통해, 6개 사건의 **원인 계층을 도구 근거와 함께 판정**한다. 정답 암기가 아니라 "어디부터 볼 것인가"의 순서가 채점 대상.

## Step 0. 진행 방식

```text
① ㉚ 계층 탐정 게임을 조별 1회 완주 (판정 근육 예열)
② 강사가 HvacSimulator의 장애 스위치를 켠 실기 버전 배포
③ 사건당 10분: 증상 재현 → 도구 수집 → 판정표 기입
④ 조별 발표: "원인 계층 + 결정적 증거 1개 + 첫 조치"
```

**공통 판정표 양식** — 사건마다 이 4칸을 채웁니다:

| 사건 | 원인 계층 | 결정적 증거(도구+출력) | 첫 조치 |
|---|---|---|---|

## S1. 속성 반영 지연 — "온도 버튼이 2초 늦게 먹어요"

```bat
adb shell dumpsys gfxinfo com.example.hvacsimulator | findstr Janky   :: 렌더링?
adb shell dumpsys car_service | findstr /i "HVAC pending dispatch"    :: 디스패치?
:: (필요 시) Perfetto로 Binder 구간 확인
```

💡 판정 힌트: gfxinfo 정상 + car_service 큐 적체 → CarService/VHAL 구간. 반대면 앱.

## S2. 전 속성 DeadObjectException — "차가 통째로 안 잡혀요"

```bat
adb shell pidof com.android.car
adb logcat -d | findstr /i "FATAL com.android.car DeadObject"
adb shell service check car_service
```

💡 판정 힌트: PID 소멸/재기동 흔적 = CarService 크래시. 앱 조치 = linkToDeath+재바인딩(🎬 ㉘).

## S3. 부팅 후 첫 화면 지연 — "시동 후 30초는 먹통"

```bat
adb logcat -d | findstr /i "Displayed StartServices took"
adb logcat -d | findstr /i "SystemServerTiming"
```

💡 판정 힌트: 특정 서비스 onStart 소요 폭증 → SystemServer/CarService init 계층(⑤㉔).

## S4. 팬 애니메이션 jank — "돌다가 뚝뚝 끊겨요"

```bat
adb shell dumpsys gfxinfo com.example.hvacsimulator reset
:: 10초 조작 후
adb shell dumpsys gfxinfo com.example.hvacsimulator | findstr /i "Janky percentile"
```

💡 판정 힌트: Janky↑여도 **원인 계층은 앱**(onClick의 Bitmap 디코딩) — 증상 계층≠원인 계층의 대표 사건.

## S5. RRO 미적용 — "브랜드 색이 일부만 먹어요" (Day 3 과제 APK 사용)

```bat
adb shell cmd overlay list | findstr hvac
adb shell cmd overlay dump <오버레이패키지> | findstr /i "state target"
```

💡 판정 힌트: enable인데 특정 리소스만 원본 → **overlayable 미공개**(㉒) + 앱 재시작 여부.

## S6. 조용한 실패 — "권한 예외도 없는데 값이 안 와요"

```bat
adb logcat -d | findstr /i "avc.*denied"
adb shell dmesg | findstr /i "avc.*denied"
```

💡 판정 힌트: Java 관문 통과 + avc denied 존재 → SELinux(vendor 라벨) 계층. 조치 = D2-11 5단계.

## Step 최종. 발표 & 리캡

✅ **예상 결과:** 6행 판정표 완성. 마지막으로 전원이 함께 사고 흐름을 낭독:

```text
App → Framework → Binder → CarService → HAL/VHAL → Kernel/HW
```

# 🏁 Pass 판정 체크리스트

- [ ] S1~S6 판정표 6행 완성 (계층·증거·첫 조치)
- [ ] "증상 계층 ≠ 원인 계층" 사례를 S4로 설명
- [ ] 조 발표에서 결정적 증거를 실제 출력으로 제시

# 🔧 진행 중 막힐 때

| 상황 | 처방 |
|---|---|
| 어디부터 볼지 모르겠음 | 사고 흐름 사다리의 **App부터** 순서대로 배제 |
| 도구 출력 해석 불가 | 해당 🎬(㉚㉘⑳㉔⑱㉒)로 60초 복습 후 재시도 |
| 조 내 의견 충돌 | "증거 없는 주장 금지" 룰 — 출력 한 줄을 가져올 것 |

# 🚗 현업 활용 포인트

💡 이 판정표 양식(계층/증거/첫 조치)을 **장애 티켓 템플릿**으로 그대로 가져가세요. 4일 과정의 최종 산출물은 지식이 아니라, 이 표를 채우는 속도입니다.

---
*실습 D4-7 (36/36) — 전 과정 실습 완주! 🎉*
