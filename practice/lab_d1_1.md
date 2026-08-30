# [D1-1] 관찰 1-A — 시스템 지도: 프로세스 족보와 서비스 장부

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★☆☆☆☆ | Day 1 Ch 0 직후 · 필수 | Automotive Emulator |

> 🎯 **실습 목표** — `ps -A`로 **Zygote 족보**(모든 앱의 PPID)를 눈으로 확인하고, `service list`로 ServiceManager 장부에서 **car_* 서비스 군단**을 찾는다. 애니메이션 ①에서 본 그림을 실기로 검증한다.

## Step 1. 프로세스 족보 — Zygote가 정말 모두의 부모인가

```bat
adb shell ps -A -o USER,PID,PPID,NAME | findstr /i "zygote system_server"
```

✅ **예상 결과:** `zygote64`의 PID(예: 512)가 `system_server`의 **PPID**와 일치

이어서 아무 앱(예: HVAC)을 실행한 뒤:

```bat
adb shell ps -A -o USER,PID,PPID,NAME | findstr /i "hvac car"
```

✅ **예상 결과:** 앱들의 PPID도 전부 zygote64의 PID — **족보 완성**. 기록: zygote PID = ____

## Step 2. 시스템 속성 훑기

```bat
adb shell getprop ro.build.version.sdk
adb shell getprop ro.product.model
adb shell getprop | findstr /i "boot_completed"
```

✅ **예상 결과:** `36` / 모델명 / `[sys.boot_completed]: [1]` — getprop은 "시스템의 상태판"입니다.

## Step 3. ServiceManager 장부 — 차가 달린 Android

```bat
adb shell service list | findstr /i car
adb shell service list | find /c ":"
```

✅ **예상 결과:** `car_service` 등 car_* 10여 개 + 전체 서비스 200개 안팎의 개수 — 이 장부가 애니메이션 ②의 '전화번호부'입니다.

💡 `service check car_service` 로 개별 생존 확인도 가능 — Day 4 워크숍 S2의 첫 명령이 됩니다.

# 🏁 Pass 판정 체크리스트

- [ ] zygote PID = system_server·앱들의 PPID 확인 (숫자 기록)
- [ ] boot_completed=1 확인
- [ ] car_* 서비스 목록 확보 (개수 기록)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| findstr 결과 없음 | 일반 폰 AVD 사용 중 | **Automotive AVD**로 전환 (D0-1 Step 5) |
| ps 열이 안 맞음 | -o 옵션 오타 | 옵션 없이 `ps -A` 후 눈으로 열 찾기도 OK |

# 🚗 현업 활용 포인트

💡 장애 접수 1분 안에 치는 명령이 정확히 이 세 개입니다: **살아있나(ps) → 상태는(getprop) → 등록됐나(service list)**. 계층 탐정(㉚)의 0단계.

---
*실습 D1-1 (8/36) · 다음: **D1-2 관찰 1-B Binder 흔적***
