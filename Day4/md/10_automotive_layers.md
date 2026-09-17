# 실습 10. Automotive 계층 확인 · cmd car_service · 권한 거부 추적

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 3

## 목표

- App → CarService → VHAL 의 세 프로세스와 Binder 서비스를 `service list`·`ps`·`dumpsys` 로 확인하고 Day 2 의 HAL 규칙과 대조한다.
- `cmd car_service` 로 차량 이벤트를 주입해 UI 가 반응하는 것을 본다.
- 권한 거부가 나면 어떤 권한이 필요한지 **로그와 소스로 찾는** 절차를 익힌다.

---

## Step 1. 세 프로세스

```bash
adb shell
service list | grep -E "car_service|vehicle"
#  car_service: [android.car.ICar]                                          ← Java, com.android.car
#  android.hardware.automotive.vehicle.IVehicle/default: [android.hardware.automotive.vehicle.IVehicle]   ← VHAL (vendor)

ps -A -o user,pid,name | grep -E "com.android.car$|vehicle"
# system   1177 com.android.car                                             ← uid system (1000)
# system   ...  android.hardware.automotive.vehicle@V3-default-service      ← Emulator VHAL (uid system 또는 vendor)

dumpsys car_service --hal | head -20                # VHAL 연결 상태 · 지원 속성 수
dumpsys car_service --services CarPropertyService | head -40
```

## Step 2. VHAL 직접 읽기

```bash
dumpsys android.hardware.automotive.vehicle.IVehicle/default --list | head -20
dumpsys android.hardware.automotive.vehicle.IVehicle/default --get 0x11600207          # PERF_VEHICLE_SPEED
dumpsys android.hardware.automotive.vehicle.IVehicle/default --get 0x15600503 -a 49    # HVAC_TEMPERATURE_SET, 좌측 좌석 그룹
dumpsys android.hardware.automotive.vehicle.IVehicle/default --get 0x15600503 -a 68    # 우측
```

`-a` 는 areaId. Emulator 의 HVAC 온도는 좌석 그룹 `0x31(49)`/`0x44(68)` 로 정의되어 있다 (`--list` 출력의 `areaId` 로 확인).

## Step 3. VINTF — Day 2 규칙 대조

```bash
grep -A5 "automotive.vehicle" /vendor/etc/vintf/manifest.xml
# <hal format="aidl"><name>android.hardware.automotive.vehicle</name><version>3</version><fqname>IVehicle/default</fqname>
cat /vendor/etc/init/*vehicle*.rc 2>/dev/null | head       # init.rc 로 뜨는 vendor 데몬 (Day 2 실습 9)
```

| Day 2 규칙 | VHAL 에서 |
|---|---|
| `<pkg>.<Interface>/<instance>` | `android.hardware.automotive.vehicle.IVehicle/default` |
| vendor 는 `libbinder_ndk` | VHAL 은 NDK backend |
| VINTF manifest 등록 | `/vendor/etc/vintf/manifest.xml` |
| App 은 직접 못 부름 | CarService(uid 1000) 만 `IVehicle` 호출 |

## Step 4. cmd car_service — 차 없이 시나리오 재현

```bash
cmd car_service day-night-mode night          # ★ Launcher·SystemUI 가 즉시 다크 테마
cmd car_service day-night-mode day
cmd car_service inject-vhal-event 0x11600207 60         # 속도 60 → 클러스터/Launcher 표시 변화
cmd car_service get-property-value 0x11600207
cmd car_service inject-vhal-event 0x11400400 4          # GEAR_SELECTION = DRIVE(4)
cmd car_service inject-key 3                            # KEYCODE_HOME
cmd car_service -h | head -40                           # 전체 명령
```

`inject-vhal-event` 는 VHAL 을 거치지 않고 CarService 의 `VehicleHal` 에 이벤트를 넣는다. VHAL `--set` 은 VHAL 자체의 값을 바꾼다 — 두 경로의 차이를 실습 12 에서 비교한다.

## Step 5. 권한 거부 추적 절차

Kitchen Sink 로 속도를 읽어 본다 (권한 있음) 후, 권한이 없는 앱 입장을 재현한다:

```bash
# (1) 거부 상황 — 실습 11 앱이 아직 권한이 없을 때 또는 revoke 후
pm revoke com.google.android.car.kitchensink android.car.permission.CAR_SPEED 2>/dev/null
logcat -c
```

Kitchen Sink → Property 탭 → PERF_VEHICLE_SPEED 읽기 → 실패.

```bash
# (2) 로그에서 거부된 속성과 필요한 권한
logcat -d -s CarPropertyService | grep -i "permission" | tail -3
# … Permission denied for property 0x11600207 … requires android.car.permission.CAR_SPEED

# (3) 권한 종류 확인 — pm grant 로 되는가
pm list permissions -g | grep -A2 -i "CAR_SPEED"
dumpsys package com.google.android.car.kitchensink | grep -i car_speed

# (4) 복구
pm grant com.google.android.car.kitchensink android.car.permission.CAR_SPEED
exit
```

소스에서 매핑을 찾을 때 (빌드 서버):

```bash
grep -rn "PERF_VEHICLE_SPEED" ~/android/packages/services/Car/service/src/com/android/car/hal/PropertyHalServiceIds.java | head -3
# → CAR_SPEED 권한 매핑
```

| 권한 종류 | `pm grant` | 예 |
|---|---|---|
| normal / dangerous | 가능 | `CAR_SPEED`(dangerous), `CAR_POWERTRAIN` |
| signature\|privileged | **불가** — `/system/priv-app` + `privapp-permissions` xml 또는 platform 서명 | `CONTROL_CAR_CLIMATE`, `CAR_ENGINE_DETAILED` |

---

## 확인 포인트

- [ ] `car_service` 와 `IVehicle/default` 가 `service list` 에, 프로세스 uid 확인
- [ ] `--get 0x15600503 -a 49` 값 읽힘
- [ ] `day-night-mode night` 로 화면 테마 전환, `inject-vhal-event` 로 속도 표시
- [ ] 거부 로그에서 필요한 권한 이름을 찾고 복구

## 핵심 정리

- Automotive 는 Day 2 의 구조 그대로: Java Service(CarService) 가 AIDL HAL(VHAL) 을 부르고, App 은 CarService 만 본다.
- `cmd car_service` 는 CarService 의 `onShellCommand` — 차량 없이 시나리오를 만드는 표준 도구.
- 권한 문제는 `logcat -s CarPropertyService` → 속성 ID → 권한 이름 → 종류 판단 순서로 푼다.
