# 실습 11. HvacSimulator → CarPropertyManager 연동

> **소요시간:** 35분 · **난이도:** ★★★ · **챕터:** Ch 3 · 저장소 `HvacSimulator.zip` 확장

## 목표

- 저장소의 가짜 HVAC UI 를 `CarPropertyManager` 로 **실제 VHAL 속성**(`HVAC_TEMPERATURE_SET` · `HVAC_FAN_SPEED` · `HVAC_POWER_ON`) 에 연결한다.
- `subscribePropertyEvents` 콜백으로 외부(VHAL·Kitchen Sink) 변경을 UI 에 반영한다.
- privileged 권한(`CONTROL_CAR_CLIMATE`) 앱을 `/system/priv-app` 에 설치하는 법을 익힌다.

---

## Step 1. 원본 실행

`C:\aosp16\day4\HvacSimulator` 를 Android Studio 로 열고 Run. 좌/우 온도 ±, FAN, 단위(F/C). 값은 앱 안의 변수일 뿐이다.

## Step 2. android.car 라이브러리

**`app/build.gradle.kts`**

```kotlin
android {
    namespace = "com.example.hvacsimulator"
    compileSdk = 36
    defaultConfig { applicationId = "com.example.hvacsimulator"; minSdk = 34; targetSdk = 36; versionCode = 1; versionName = "1.0" }
    useLibrary("android.car")                       // ★ Automotive SDK 의 optional 라이브러리
    compileOptions { sourceCompatibility = JavaVersion.VERSION_11; targetCompatibility = JavaVersion.VERSION_11 }
}
```

`useLibrary` 가 안 잡히면 직접 참조:

```kotlin
dependencies {
    compileOnly(files("${android.sdkDirectory}/platforms/android-35/optional/android.car.jar"))
}
```

**`AndroidManifest.xml`**

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.example.hvacsimulator">
    <uses-permission android:name="android.car.permission.CONTROL_CAR_CLIMATE"/>
    <application android:label="HVAC Simulator" android:theme="@style/Theme.AppCompat.DayNight.DarkActionBar">
        <uses-library android:name="android.car" android:required="true"/>     <!-- ★ -->
        <activity android:name=".MainActivity" android:exported="true">
            <intent-filter><action android:name="android.intent.action.MAIN"/><category android:name="android.intent.category.LAUNCHER"/></intent-filter>
        </activity>
    </application>
</manifest>
```

## Step 3. MainActivity — Car 연결과 속성 I/O

기존 `MainActivity.java` 에 다음을 추가·교체한다 (UI 코드는 그대로).

```java
import android.car.Car;
import android.car.VehicleAreaSeat;
import android.car.VehiclePropertyIds;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyManager;
import android.util.Log;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "HvacSim";
    private Car car;
    private CarPropertyManager pm;
    private int areaLeft = VehicleAreaSeat.SEAT_ROW_1_LEFT;      // 실제 areaId 는 config 에서 다시 읽는다
    private int areaRight = VehicleAreaSeat.SEAT_ROW_1_RIGHT;
    private float leftTemp = 22f, rightTemp = 22f;              // ℃ (VHAL 은 섭씨)
    private int fanModeIndex = 0;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        setContentView(R.layout.activity_main);
        // … 기존 findViewById · Spinner 설정 …

        // ★ CarService 연결 (비동기) — 준비되면 콜백
        car = Car.createCar(this, null, Car.CAR_WAIT_TIMEOUT_WAIT_FOREVER, (c, ready) -> {
            if (!ready) { pm = null; return; }
            pm = (CarPropertyManager) c.getCarManager(Car.PROPERTY_SERVICE);
            runOnUiThread(this::initFromCar);
        });

        findViewById(R.id.temp_up_left).setOnClickListener(v -> setTemp(areaLeft, leftTemp + 0.5f));
        findViewById(R.id.temp_down_left).setOnClickListener(v -> setTemp(areaLeft, leftTemp - 0.5f));
        findViewById(R.id.temp_up_right).setOnClickListener(v -> setTemp(areaRight, rightTemp + 0.5f));
        findViewById(R.id.temp_down_right).setOnClickListener(v -> setTemp(areaRight, rightTemp - 0.5f));
        fanButton.setOnClickListener(v -> setFan((fanModeIndex + 1) % fanModes.length));
    }

    /** areaId·범위를 config 에서 읽고 현재값으로 UI 초기화, 변경 구독 */
    private void initFromCar() {
        CarPropertyConfig<?> cfg = pm.getCarPropertyConfig(VehiclePropertyIds.HVAC_TEMPERATURE_SET);
        if (cfg == null) { Log.e(TAG, "HVAC_TEMPERATURE_SET not supported"); return; }
        int[] areas = cfg.getAreaIds();                          // Emulator: [49, 68]
        Log.i(TAG, "temp areaIds=" + java.util.Arrays.toString(areas) + " min=" + cfg.getMinValue(areas[0]) + " max=" + cfg.getMaxValue(areas[0]));
        areaLeft = areas[0]; areaRight = areas.length > 1 ? areas[1] : areas[0];

        leftTemp  = pm.getProperty(Float.class, VehiclePropertyIds.HVAC_TEMPERATURE_SET, areaLeft).getValue();
        rightTemp = pm.getProperty(Float.class, VehiclePropertyIds.HVAC_TEMPERATURE_SET, areaRight).getValue();
        int fanArea = pm.getCarPropertyConfig(VehiclePropertyIds.HVAC_FAN_SPEED).getAreaIds()[0];
        fanModeIndex = Math.min(pm.getProperty(Integer.class, VehiclePropertyIds.HVAC_FAN_SPEED, fanArea).getValue(), fanModes.length - 1);
        updateDisplays();

        // ★ 외부 변경 구독 — 콜백은 Binder Thread 에서 온다
        pm.subscribePropertyEvents(VehiclePropertyIds.HVAC_TEMPERATURE_SET, callback);
        pm.subscribePropertyEvents(VehiclePropertyIds.HVAC_FAN_SPEED, callback);
    }

    private final CarPropertyManager.CarPropertyEventCallback callback = new CarPropertyManager.CarPropertyEventCallback() {
        @Override public void onChangeEvent(CarPropertyValue value) {
            Log.i(TAG, "onChangeEvent " + Integer.toHexString(value.getPropertyId()) + " area=" + value.getAreaId()
                    + " value=" + value.getValue() + " thread=" + Thread.currentThread().getName());
            if (value.getPropertyId() == VehiclePropertyIds.HVAC_TEMPERATURE_SET) {
                if (value.getAreaId() == areaLeft) leftTemp = (Float) value.getValue();
                else if (value.getAreaId() == areaRight) rightTemp = (Float) value.getValue();
            } else if (value.getPropertyId() == VehiclePropertyIds.HVAC_FAN_SPEED) {
                fanModeIndex = Math.min((Integer) value.getValue(), fanModes.length - 1);
            }
            runOnUiThread(MainActivity.this::updateDisplays);       // ★ Day 1 실습 9 · Day 3 실습 6
        }
        @Override public void onErrorEvent(int propId, int zone) { Log.e(TAG, "error prop=" + Integer.toHexString(propId)); }
    };

    private void setTemp(int areaId, float v) {
        if (pm == null) return;
        try {
            pm.setProperty(Float.class, VehiclePropertyIds.HVAC_TEMPERATURE_SET, areaId, v);   // ★ Binder → CarService → VHAL
        } catch (Exception e) { Log.e(TAG, "setProperty failed", e); }
        // UI 는 onChangeEvent 로 갱신된다 (VHAL 이 값을 확정한 뒤)
    }

    private void setFan(int idx) {
        if (pm == null) return;
        int fanArea = pm.getCarPropertyConfig(VehiclePropertyIds.HVAC_FAN_SPEED).getAreaIds()[0];
        pm.setProperty(Integer.class, VehiclePropertyIds.HVAC_FAN_SPEED, fanArea, idx);
        pm.setProperty(Boolean.class, VehiclePropertyIds.HVAC_POWER_ON, fanArea, idx > 0);
    }

    @Override protected void onDestroy() {
        if (pm != null) pm.unsubscribePropertyEvents(callback);
        if (car != null) car.disconnect();
        super.onDestroy();
    }
}
```

`updateDisplays()` 의 `formatTemp` 는 섭씨 기준으로 바꾼다 (`isCelsius` 이면 그대로, 아니면 `*9/5+32`).

## Step 4. privileged 권한 — /system/priv-app 설치

`CONTROL_CAR_CLIMATE` 는 signature|privileged 라 `pm grant` 가 안 된다. APK 를 priv-app 으로 넣고 허용 목록을 준다.

Generate APKs → `C:\aosp16\bin\hvacsimulator.apk`.

`C:\aosp16\privapp-permissions-hvacsimulator.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<permissions>
    <privapp-permissions package="com.example.hvacsimulator">
        <permission name="android.car.permission.CONTROL_CAR_CLIMATE"/>
    </privapp-permissions>
</permissions>
```

```bat
adb root
adb remount
adb shell mkdir -p /system/priv-app/HvacSimulator
adb push C:\aosp16\bin\hvacsimulator.apk /system/priv-app/HvacSimulator/HvacSimulator.apk
adb push C:\aosp16\privapp-permissions-hvacsimulator.xml /system/etc/permissions/
adb reboot
adb wait-for-device
```

```bash
adb shell
dumpsys package com.example.hvacsimulator | grep -A3 "runtime permissions\|install permissions" | grep CONTROL_CAR_CLIMATE
# android.car.permission.CONTROL_CAR_CLIMATE: granted=true
am start -n com.example.hvacsimulator/.MainActivity
logcat -s HvacSim
# I HvacSim: temp areaIds=[49, 68] min=16.0 max=32.0
```

> 이후 코드를 고쳐 다시 배포할 때는 `adb push` 로 덮어쓰고 `am force-stop` 후 재시작하면 된다 (재부팅 불필요). `adb install` 로 같은 패키지를 올리면 priv-app 이 아니게 되므로 쓰지 않는다.

## Step 5. 동작 확인

- 온도 ▲ → 로그에 `onChangeEvent 15600503 area=49 value=22.5 thread=Binder:…` → UI 갱신
- FAN → `HVAC_FAN_SPEED` 변경, Kitchen Sink 의 HVAC 탭에서 같은 값이 보이는지

```bash
dumpsys car_service --services CarPropertyService | grep -B2 -A4 "com.example.hvacsimulator" | head
exit
```

---

## 확인 포인트

- [ ] `areaIds=[49, 68]`, min/max 로그
- [ ] 버튼 → `setProperty` → `onChangeEvent`(Binder Thread) → UI
- [ ] `dumpsys package` 에 `CONTROL_CAR_CLIMATE granted=true`
- [ ] Kitchen Sink 와 값 동기화

## 핵심 정리

| API | 역할 |
|---|---|
| `Car.createCar(ctx, handler, timeout, listener)` | CarService(`ICar`) 에 Binder 연결. 준비되면 콜백 |
| `getCarManager(Car.PROPERTY_SERVICE)` | `CarPropertyManager` (`ICarProperty` Proxy) |
| `getCarPropertyConfig` | areaIds · min/max · changeMode — **호출 전 확인 습관** |
| `getProperty / setProperty(Class, propId, areaId, v)` | 동기 Binder → `CarPropertyService` → `PropertyHalService` → `IVehicle` |
| `subscribePropertyEvents` | 역방향 콜백 (Binder Thread) — UI 는 `runOnUiThread` |
| privileged 권한 | `/system/priv-app` + `privapp-permissions-*.xml` |

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `package android.car does not exist` | `useLibrary("android.car")` 또는 `android.car.jar` compileOnly · Automotive 플랫폼(API 35) 설치 |
| 앱 시작 시 `Car service not ready` | `createCar` 콜백 안에서만 `pm` 사용 |
| `SecurityException` | priv-app 설치·xml 이름·패키지명 확인, reboot |
| `IllegalArgumentException: areaId` | `getCarPropertyConfig().getAreaIds()` 값 사용 (1 이 아니라 49/68) |
| `INSTALL_FAILED…` on reboot | `logcat -s PackageManager` — priv-app 권한 xml 누락 시 부팅 중 앱 비활성화 |
