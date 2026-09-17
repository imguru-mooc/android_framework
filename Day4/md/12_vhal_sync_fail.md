# 실습 12. VHAL 조작 · 동기화 · 실패 재현 · 권한 추적

> **소요시간:** 25분 · **난이도:** ★★★ · **챕터:** Ch 3 · 실습 11 앱 실행 중

## 목표

- VHAL 값을 밖에서 바꿔 실습 11 앱이 따라오는 경로(VHAL → CarService → App)를 로그로 본다.
- 잘못된 값·areaId·Thread 로 실패를 재현해 오류 메시지를 익힌다.
- 권한을 빼앗아 거부 → 필요한 권한을 찾아 복구한다.

---

## Step 1. 로그 준비 (cmd ①)

```bash
adb shell
logcat -c
logcat -s HvacSim CarPropertyService PropertyHalService VehicleHal
```

## Step 2. VHAL --set → 앱 콜백 (cmd ②)

```bash
adb shell
dumpsys android.hardware.automotive.vehicle.IVehicle/default --set 0x15600503 -a 49 -f 25.0
dumpsys android.hardware.automotive.vehicle.IVehicle/default --set 0x15400500 -a 49 -i 3     # FAN 3
```

cmd ① 에:

```text
D VehicleHal: onPropertyEvent … 0x15600503 …
D CarPropertyService: onPropertyChange … → dispatch to 1 listener
I HvacSim: onChangeEvent 15600503 area=49 value=25.0 thread=Binder:7210_2
```

앱 화면의 좌측 온도가 25.0, FAN 이 HIGH 로 바뀐다.

## Step 3. cmd car_service inject 와 비교

```bash
cmd car_service inject-vhal-event 0x15600503 26.0 -a 49     # 형식은 -h 로 확인
dumpsys android.hardware.automotive.vehicle.IVehicle/default --get 0x15600503 -a 49
```

`inject-vhal-event` 는 CarService 에만 이벤트를 넣으므로 **앱은 26.0 으로 바뀌지만 VHAL `--get` 은 이전 값**일 수 있다. `--set` 은 VHAL 의 값을 바꾸고 그 결과가 CarService 로 올라온다. 테스트 시나리오에 따라 골라 쓴다.

## Step 4. Kitchen Sink 동기화

Kitchen Sink → HVAC 탭에서 좌측 온도를 바꾸면 실습 11 앱도 같이 바뀐다. 두 앱 모두 같은 `CarPropertyService` 의 subscriber 다.

```bash
dumpsys car_service --services CarPropertyService | grep -A6 -i "subscri" | head -20
```

## Step 5. 실패 재현

| 실험 | 방법 | 결과 |
|---|---|---|
| 범위 밖 값 | `--set 0x15600503 -a 49 -f 50.0` 또는 앱에서 `setTemp(area, 50f)` | `IllegalArgumentException` / VHAL `INVALID_ARG` — `getMinValue/getMaxValue` 로 사전 검사 |
| 잘못된 areaId | 앱 `setProperty(..., 0 /*GLOBAL*/, 22f)` | `IllegalArgumentException: areaId 0 is not supported` |
| 콜백에서 UI 직접 | `onChangeEvent` 의 `runOnUiThread` 제거 → 재배포 → `--set` | `CalledFromWrongThreadException` (Day 1 실습 9 와 동일) |
| CarService 재시작 | `killall com.android.car` → 앱의 `createCar` 리스너가 `ready=false` 후 다시 `true` | `Car` 객체가 자동 재연결 — Day 1 실습 8 DeathRecipient 의 Car 판 |

각 실험 후 원복한다.

## Step 6. 권한 추적

```bash
# 권한 제거 (priv-app 이라도 xml 에서 빼고 reboot 하면 거부된다 — 빠르게는 appops 대신 revoke 가 안 되므로 xml 방식)
```

```bat
adb root & adb remount
adb shell mv /system/etc/permissions/privapp-permissions-hvacsimulator.xml /data/local/tmp/
adb reboot & adb wait-for-device
```

```bash
adb shell
am start -n com.example.hvacsimulator/.MainActivity
logcat -d -s HvacSim CarPropertyService | grep -iE "SecurityException|permission" | head
# … SecurityException: … requires android.car.permission.CONTROL_CAR_CLIMATE
pm list permissions -g | grep -B1 -A2 CONTROL_CAR_CLIMATE     # protectionLevel: signature|privileged
exit
```

찾은 권한 이름을 xml 에 다시 넣고 복구:

```bat
adb shell mv /data/local/tmp/privapp-permissions-hvacsimulator.xml /system/etc/permissions/
adb reboot
```

빌드 서버에서 매핑 확인: `grep -rn "HVAC_TEMPERATURE_SET" ~/android/packages/services/Car/service/src/com/android/car/hal/PropertyHalServiceIds.java`.

## Step 7. 정리 질문 (각자 답을 로그에서 찾아 적기)

1. 온도 버튼 한 번에 `CarPropertyService` 로그가 몇 줄 찍혔나 (set 1 + change 1)?
2. `onChangeEvent` 의 Thread 이름은? 왜 `runOnUiThread` 가 필요한가?
3. `--set` 과 `inject-vhal-event` 뒤 `--get` 값이 다른 이유는?

→ 실습 13 에서 같은 흐름을 Perfetto 로 본다.

---

## 확인 포인트

- [ ] `--set` → `VehicleHal` → `CarPropertyService` → `HvacSim onChangeEvent` 로그 순서
- [ ] Kitchen Sink 와 동기화
- [ ] 세 가지 실패 메시지 확인 후 원복
- [ ] 권한 제거 → 거부 로그에서 권한 이름 → 복구

## 핵심 정리

- 값의 진실은 VHAL 에 있다. `--set` 은 VHAL 을, `inject-vhal-event` 는 CarService 만 바꾼다.
- 모든 subscriber 는 `CarPropertyService` 한 곳에서 dispatch 된다 — `dumpsys` 로 누가 듣고 있는지 보인다.
- 오류의 절반은 areaId·범위, 나머지 절반은 권한이다. `getCarPropertyConfig` 와 `logcat -s CarPropertyService` 로 푼다.
