# Day 4 — APEX · RRO / SRO · SystemUI 커스터마이징 · WindowManager · Android Automotive (CarService · CarPropertyManager · VHAL)

**과정:** Android Framework 16 심화 교육 (4일 / 32시간)
**환경:** Ubuntu 24 빌드 서버 (`~/android`, AOSP `sdk_car_x86_64`, 호스트 JDK) + Windows 11 (Android Studio, WinSCP, adb, `aapt`) + 커스텀 Emulator (userdebug, permissive, `-writable-system`)
**소스 저장소:** https://github.com/imguru-mooc/android/tree/main/04_day — `apex_build_tutorial_android16.md` · `RRO_example_android16.md` · `SRO_product_flavors_android16.md` · `systemui_customizing_lab_android16.md`(실습 1~5) · `RRO_test/overlay_list`(ListOverlays) · `FloatingOverlayDemo.zip` · `SplitScreenDemo.zip` · `HvacSimulator.zip` · `MemoAppJava.zip`
**범위:** 시스템 이미지를 바꾸는 세 가지 방법(APEX 모듈 · RRO 런타임 오버레이 · AOSP 소스 수정) + 빌드 변형(SRO) → SystemUI 분석·커스터마이징(RRO · QS 타일 · 상태바 커스텀 뷰 · Perfetto) → WindowManager 창 타입(오버레이 · 분할 화면) → Android Automotive 계층 (CarService → CarPropertyManager → VHAL) 로 HVAC 시뮬레이터를 실제 차량 속성에 연결
**총 소요시간:** 8시간 (환경·등록 실습 15m / 강의 1h 30m / 실습 5h 35m / 총정리·Worksheet 40m) — 데모·선택 과제 없이 **전원이 실습 1~13 을 직접 수행**
**설계 원칙 (현장 반영):** ① "내 변경이 왜 안 먹는가" 를 스스로 진단할 수 있는 절차를 모든 실습에 넣는다 ② 실무에서 매일 쓰는 명령(`dumpsys` · `cmd overlay` · `cmd car_service` · VHAL `--set`) 을 손에 익힌다 ③ 이론보다 "바꾸고 → 눈으로 확인" 의 왕복 횟수를 늘린다 ④ App 개발 성격이 강한 항목(SRO · 분할 화면) 은 짧게, Framework·Automotive 항목은 길게
**배포 절차:** 빌드 서버 `m <모듈명>` → WinSCP 로 결과물(`.apex` · `.apk` · `.jar`) 을 `C:\aosp16\bin\` 로 다운로드 → Windows cmd `adb push` / `adb install` → `adb shell` 에서 실행·확인 (Day 1~3 과 동일). Android Studio 프로젝트는 Windows 에서 빌드 → `adb install`
**실습 번호:** 실습 1 ~ 13 (Part 0 부터 연속 번호)

---

## 학습 목표

1. **APEX** 모듈(`apex_manifest.json` · `apex_key`/인증서 · **`file_contexts` 는 `system/sepolicy/apex/`**)을 만들어 `adb install` 하고 `/apex/` 마운트 구조와 `updatable`/`compressible` 규칙을 설명할 수 있다.
2. **RRO** 의 `<overlayable>`/`<overlay>` 계약으로 문자열·색상·drawable 을 런타임에 바꾸고, `cmd overlay` 와 **`IOverlayManager` hidden API(app_process 도구)** 로 상태를 읽으며, **SRO(Product Flavors)** 와의 차이를 안다.
3. `dumpsys statusbar / notification / gfxinfo` 로 **SystemUI** 를 분석하고, **Car SystemUI** 가 `<overlayable>` 을 선언하지 않아 `/system/app/` 설치(2단계 remount)가 필요한 이유와 `aapt` 로 실제 리소스 이름을 찾는 법을 안다.
4. AOSP SystemUI 소스에 **QS 타일(`QSTileImpl`)** 을 추가해 `m SystemUI` 로 교체하고, Perfetto(`view/wm/am/input`) 로 UI Thread / RenderThread Jank 를 본다.
5. **WindowManager** 의 창 타입(`TYPE_APPLICATION_OVERLAY` + `SYSTEM_ALERT_WINDOW`/appops) 과 분할 화면(Fragment 2분할 · Automotive 멀티 디스플레이) 을 Day 3 의 SurfaceFlinger Layer 와 연결해 설명할 수 있다.
6. Android Automotive 의 계층 — App → `Car` API → **CarService** (`car_service`, `com.android.car`) → **VHAL** (`IVehicle/default`, AIDL HAL) → Vehicle Property — 를 Day 2 의 Binder·HAL·VINTF 규칙으로 설명하고, **HvacSimulator** UI 를 `CarPropertyManager` 로 실제 `HVAC_*` 속성에 연결한다.
7. 4일 과정 전체를 "App → Binder → system_server → HAL → Kernel / SurfaceFlinger / Memory" 한 그림으로 정리한다.

---

## Part 0. 환경·등록 실습 (총 15분)

| # | 실습명 | 난이도 | 소요시간 |
|---|---|---|---|
| 1 | Day 4 환경 점검 — Emulator `-writable-system` 부팅 · **2단계 `adb remount`** (verity 해제 → reboot → `remount succeeded`, 실습 6·7 의 전제) · `aapt` PATH · 저장소 zip 3종(Floating/SplitScreen/Hvac) 배포 · `~/android/simple_apex/`(키 포함), `RRO_test/overlay_list/` 등록 · Kitchen Sink 존재 확인 | ★☆☆ | 15분 |

---

## Part 1. 강의 (총 90분)

### Chapter 1. 시스템을 바꾸는 세 가지 방법 — APEX · RRO / SRO · 소스 수정 — 30분

| 주제 | 핵심 내용 |
|---|---|
| 왜 세 가지인가 | 전체 이미지 리빌드 없이 배포 → 모듈(APEX) · 리소스(RRO) · 빌드 변형(SRO). 소스 수정(`m SystemUI`) 은 마지막 수단. "델타 배포" 판단표 |
| APEX | Android 10+ 모듈 단위 (`/apex/<name>@<ver>/`) · `apex {}` (`manifest` · `file_contexts` · `key` · `certificate` · `binaries/native_shared_libs/apps` · `updatable: false` · `compressible` 규칙) · `apex_key {}` + `android_app_certificate {}` (`.pem/.pk8/.x509.pem/.avbpubkey`) · **`file_contexts` 는 반드시 `system/sepolicy/apex/<name>-file_contexts`** (다른 곳이면 `should be under system/sepolicy` 오류) · `adb install x.apex` → reboot → `dumpsys apexservice` · Mainline 과의 관계 |
| RRO | `<overlayable name><policy type="public"><item type name/>` (Android 11+ 의무) · `<overlay targetPackage targetName priority isStatic/>` · `hasCode="false"` · `cmd overlay list/enable/disable/dump` 의 `[x] [ ] ---` · `OverlayManagerService` (`service list \| grep overlay`) · `IOverlayManager.getOverlayInfosForTarget()` hidden API · AOSP `runtime_resource_overlay {}` |
| Car SystemUI 특수성 | `<overlayable>` 미선언 → 사용자 설치(`adb install`) 오버레이는 `---` → **`/system/app/` 설치(policy system)** 만 가능 → `targetName` 제거 · `pm path` → `adb pull` → `aapt dump resources \| findstr color` 로 실제 이름 · 투명 배경 리소스(`system_bar_background_transparent`) 함께 오버레이 |
| SRO | Gradle Product Flavors `productFlavors { original; themed }` · `src/<flavor>/res` · 병합 우선순위 (라이브러리 < main < flavor < buildType) · Build Variant 전환 · `Analyze APK` 의 `resources.arsc` · SRO(빌드 시, White Label) vs RRO(런타임, 테마) 표 |
| 소스 수정 | `frameworks/base/packages/SystemUI` · `m SystemUI` → `/system_ext/priv-app/CarSystemUI/` (또는 `/system/priv-app/SystemUI/`) 교체 · `killall com.android.systemui` · 언제 `mm`/`adb push` 로 되고 언제 `m emu_img_zip` 이 필요한가 |
| **델타 배포 판단표** (수업 핵심) | 바꾸는 것 → 필요한 절차: Native 바이너리/`.so` → `m` + `adb push` · Java 리소스(색·문자열·drawable) → **RRO** (재빌드 없음) · 시스템 앱 코드 → `m <앱>` + `adb push` + `killall` · 독립 배포 모듈 → **APEX** · 커널/기본 탑재 RRO/VHAL/sepolicy → `m emu_img_zip` + Emulator 재시작. 실무의 첫 질문 "이거 이미지 다시 구워야 하나요?" 에 답하는 표 |
| **변경이 안 먹을 때 체크리스트** | ① `adb remount` 가 `succeeded` 였나 (2단계) ② `cmd overlay list` 에서 `---` 인가 (overlayable/targetName) ③ `[ ]` 인데 `enable` 을 안 했나 ④ `killall com.android.systemui` / reboot 를 했나 ⑤ 리소스 이름이 `aapt dump` 결과와 같은가 ⑥ 우선순위가 더 높은 다른 RRO 가 덮고 있나 (`cmd overlay dump` 의 priority) ⑦ `logcat -s AndroidRuntime` 에 크래시가 있나 |

### Chapter 2. SystemUI · WindowManager — 구조 · 분석 · 커스터마이징 — 25분

| 주제 | 핵심 내용 |
|---|---|
| SystemUI 구조 | `com.android.systemui` 프로세스 · StatusBar / NavigationBar / QS / Notification / Keyguard · Car 는 `CarSystemUI`(`system_ext/priv-app`) + `CarSystemUIRRO` · `status_bar.xml` 레이아웃 (시계·알림 아이콘 start / 시스템 아이콘·배터리 end) |
| 분석 도구 | `dumpsys statusbar` (mDisabled · Tiles · Icons) · `dumpsys notification` · `dumpsys activity activities \| grep mResumed` · `dumpsys activity top` · `dumpsys gfxinfo com.android.systemui` (Janky frames · 90th percentile) · `svc wifi` · `am start -a VIEW` |
| QS 타일 | `QSTileImpl<BooleanState>` · `@Inject` 생성자 (QSHost · Looper · Handler · FalsingManager …) · `newTileState / handleClick / handleUpdateState / getLongClickIntent / getTileLabel` · Dagger `@Binds @IntoMap @StringKey(TILE_SPEC)` · `config.xml` `quick_settings_tiles_default` · `ResourceIcon` |
| 상태바 커스텀 뷰 | `TextView` 상속 `BatteryTemperatureView` · `onAttachedToWindow/onDetachedFromWindow` + `Handler.postDelayed` 주기 갱신 (Day 3 Looper) · `BatteryManager.BATTERY_PROPERTY_TEMPERATURE` · `status_bar.xml` 에 배치 |
| 렌더링 | Java App 프레임: `Choreographer#doFrame` → UI Thread `measure/layout/draw` → `RenderThread` → BufferQueue → SF (Day 3) · Perfetto `gfx/view/wm/am/input` · `gfxinfo` 와 대조 |
| WindowManager | 창 타입 (`TYPE_APPLICATION` / `TYPE_APPLICATION_OVERLAY` / 시스템 창) · `SYSTEM_ALERT_WINDOW` 권한 + `Settings.canDrawOverlays` / `appops set … SYSTEM_ALERT_WINDOW allow` · `WindowManager.addView(view, LayoutParams)` · WMS → SF Layer (Day 3 `dumpsys SurfaceFlinger --list` 의 `StatusBar#0`, 앱 창) · `dumpsys window windows` · 분할 화면·멀티 디스플레이는 개념만 (`dumpsys display`, `am start --display`) |
| **현장에서 자주 묻는 것** | "SystemUI 만 바꿨는데 왜 부팅 후 원래대로?" → `/system` 이 overlayfs 인지·`m emu_img_zip` 필요 여부 · "QS 타일이 안 보여요" → Dagger 등록 누락(크래시 로그) 또는 `config.xml` 미반영 · "RRO 두 개가 충돌" → priority 와 `cmd overlay dump` · "상태바에 우리 회사 아이콘" → 실습 7 ② 패턴 그대로 |

### Chapter 3. Android Automotive — CarService · CarPropertyManager · VHAL — 35분

| 주제 | 핵심 내용 |
|---|---|
| 계층 | App → `android.car.Car` (`car-lib`, `uses-library android.car`) → **CarService** (`com.android.car` 프로세스, `ICar`, `service list \| grep car_service`) → **VHAL** (`android.hardware.automotive.vehicle.IVehicle/default`, vendor, AIDL HAL) → 차량 버스 (Emulator 는 `FakeVehicleHardware`) |
| Day 2 규칙 그대로 | VHAL = AIDL HAL (`stability: vintf`, `/vendor/etc/vintf/manifest.xml` 에 등록) · CarService(uid system) 만 `IVehicle` 호출 · App 은 직접 못 부름 — 권한은 `CarPropertyService` 가 검사 (Day 2 실습 8 의 UID 검사) |
| CarService | `CarServiceImpl` · `CarPropertyService` · `CarPowerManagementService` · `VehicleHal` / `PropertyHalService` (Java, JNI 없이 AIDL Java backend) → `IVehicle.getValues/setValues/subscribe` · `dumpsys car_service` · `--services CarPropertyService` |
| Vehicle Property | `VehiclePropertyIds` (`PERF_VEHICLE_SPEED` 0x11600207 · `GEAR_SELECTION` 0x11400400 · `HVAC_TEMPERATURE_SET` 0x15600503 · `HVAC_FAN_SPEED` 0x15400500 · `HVAC_POWER_ON` · `NIGHT_MODE` · `IGNITION_STATE`) · ID 비트 구조 (VehiclePropertyGroup · VehicleArea(GLOBAL/SEAT/…) · VehiclePropertyType) · `CarPropertyConfig` (access · changeMode · areaIds · min/max) · `CarPropertyValue` |
| CarPropertyManager | `Car.createCar(ctx, handler, timeout, (car, ready) -> …)` → `getCarManager(Car.PROPERTY_SERVICE)` → `getProperty(Class, propId, areaId)` / `setProperty` / `subscribePropertyEvents(cb, propId, rate)` · 권한 (`CAR_SPEED` · `CONTROL_CAR_CLIMATE` · `CAR_POWERTRAIN`) · Emulator 에서 `pm grant` 가 되는 권한과 privileged 권한 구분 |
| VHAL 에뮬레이터 | `FakeVehicleHardware` + `DefaultProperties.json` · `dumpsys android.hardware.automotive.vehicle.IVehicle/default --list / --get <id> / --set <id> -i/-f <v> [-a <areaId>]` · Kitchen Sink · 값 변경 → `PropertyHalService` → `CarPropertyService` → App 콜백 경로 (`logcat -s CarPropertyService VehicleHal`) |
| 확장 | 새 Vehicle Property (`VehicleProperty.aidl` vendor 그룹 → `DefaultProperties.json` → VHAL 리빌드 → `PERMISSION_VENDOR_EXTENSION`) — Day 2 실습 10 의 AIDL HAL 절차 재사용 |
| **`cmd car_service` 실무 명령** | `cmd car_service inject-vhal-event <propId> <value>` (VHAL 경유 없이 CarService 에 이벤트 주입) · `day-night-mode night/day` · `inject-key` · `get-property-value` · `dumpsys car_service --hal` (HAL 연결 상태) · `--services CarPropertyService` 의 subscriber 목록 — 차량 없이 데스크에서 시나리오 재현 |
| **권한 거부 추적** | App `SecurityException` → `logcat -s CarPropertyService` 의 `Permission denied for property 0x…` → `CarPropertyService` 의 `mPropToPermission` 매핑 → 필요한 `android.car.permission.*` → `pm grant` 가능(normal/dangerous) vs privileged(`privapp-permissions-*.xml` + platform 서명) 판단표 — 현장 질문 1위 |

---

## Part 2. 실습 (총 335분 · 전원 수행)

| # | 실습명 | 난이도 | 소요시간 | 챕터 |
|---|---|---|---|---|
| 2 | APEX 만들기 — `simple_apex` (`binary/main.cpp` · `Android.bp` · `apex_manifest.json` · 키 4종(강사가 미리 생성해 배포) · `system/sepolicy/apex/com.example.simple-file_contexts`) → `m com.example.simple` → `adb install` → `/apex/com.example.simple/bin/` 실행 · `dumpsys apexservice` · `file_contexts` 위치 오류 재현 | ★★☆ | 35분 | Ch 1 |
| 3 | RRO — `RROTarget`(`overlayable.xml`) + `RROOverlay`(`<overlay>`, `hasCode=false`) 로 문자열·색상 → drawable(`background_shape` gradient · `ic_target_icon` rocket) 교체 · `cmd overlay enable/disable/dump` | ★★☆ | 35분 | Ch 1 |
| 4 | ListOverlays 도구 + RRO 우선순위 충돌 — `RRO_test/overlay_list` (`java_binary`, `IOverlayManager` hidden API) 를 `mm` → `.jar` WinSCP → `app_process /system/bin ListOverlays` 로 오버레이 상태 출력 · `RROOverlay2`(priority 2) 로 충돌 실험 · SRO 는 실습 3 프로젝트에 `themed` flavor 만 추가해 Build Variant 전환 1회 | ★★☆ | 20분 | Ch 1 |
| 5 | SystemUI 분석 — `dumpsys statusbar / notification / gfxinfo / meminfo` · 알림 발생(`am start -a VIEW`)·Wi-Fi 토글(`svc wifi`) 전후 비교 · `mResumedActivity` | ★☆☆ | 15분 | Ch 2 |
| 6 | Car SystemUI RRO — `cmd overlay list \| grep systemui` 의 `---` → `dumpsys overlay` 에서 `mTargetOverlayableName: null` → `adb pull CarSystemUI.apk` → `aapt dump resources` 로 색상 이름 → `SystemUIOverlay`(targetName 제거, minSdk 34) → (실습 1 에서 끝낸) remount 상태에서 `/system/app/SystemUIOverlay/` push → reboot → `enable` → `killall com.android.systemui` → 시계 핑크·아이콘 노랑 확인 | ★★★ | 30분 | Ch 1·2 |
| 7 | SystemUI 소스 수정 — `CpuInfoTile` (`QSTileImpl` · `ic_qs_cpu_info` · Dagger 등록 · `config.xml`) → `m SystemUI` → WinSCP → remount 상태에서 `/system_ext/priv-app/CarSystemUI/` 교체 → `killall` → QS 타일 확인 · **일부러 Dagger 등록을 빼고 빌드해 크래시 로그 읽기** (`BatteryTemperatureView` 는 문서에 참고 코드로만) | ★★★ | 40분 | Ch 2 |
| 8 | SystemUI Perfetto — 저장소 cfg (`gfx/view/wm/am/input`, `atrace_apps: com.android.systemui`, 10초) · 알림 패널 열고 닫기 캡처 · `Choreographer#doFrame` → UI Thread → `RenderThread` → SF · Jank 프레임 · `gfxinfo framestats` 대조 (Day 3 실습 13 방법 그대로) | ★★☆ | 20분 | Ch 2 |
| 9 | WindowManager 앱 — `FloatingOverlayDemo` (`TYPE_APPLICATION_OVERLAY` · `SYSTEM_ALERT_WINDOW` · `appops set … allow` · `WindowManager.addView`) → `dumpsys window windows` / `dumpsys SurfaceFlinger --list` 에서 새 창·Layer 확인 · `appops` 를 `deny` 로 바꿔 `BadTokenException` 재현 · `SplitScreenDemo` 설치·실행 후 `--list` 로 Layer 가 하나임을 각자 확인 | ★★☆ | 25분 | Ch 2 |
| 10 | Automotive 계층 확인 + `cmd car_service` — `service list \| grep -E "car_service\|vehicle"` · `ps -A` (uid system vs vendor) · `dumpsys car_service --hal` · `--services CarPropertyService` · `dumpsys …IVehicle/default --list / --get` · `/vendor/etc/vintf/manifest.xml` (Day 2 규칙 대조) · **`cmd car_service day-night-mode night`** 로 화면 테마 즉시 전환 · `inject-vhal-event 0x11600207 60` 으로 클러스터 속도 · `get-property-value` | ★★☆ | 30분 | Ch 3 |
| 11 | HvacSimulator → CarPropertyManager 연동 — 저장소 `HvacSimulator` UI(좌/우 온도 · FAN · 단위) 에 `Car.createCar` → `HVAC_TEMPERATURE_SET`(좌석 areaId) `get/set` · `HVAC_FAN_SPEED` · `HVAC_POWER_ON` · `subscribePropertyEvents` 로 외부 변경 반영 · `pm grant` | ★★★ | 35분 | Ch 3 |
| 12 | VHAL 조작·동기화·실패 재현 — `dumpsys …IVehicle/default --set 0x15600503 -a 1 -f 22.0` 로 값을 바꾸면 실습 11 UI 가 따라오는지 · Kitchen Sink HVAC 탭과 동기화 · `logcat -s CarPropertyService PropertyHalService` 로 VHAL → CarService → App 경로 · 범위 밖 값·잘못된 areaId·`runOnUiThread` 누락 오류 재현 · 권한 거부 추적 (`SecurityException` → `mPropToPermission`) | ★★★ | 25분 | Ch 3 |
| 13 | Perfetto 로 온도 버튼 한 번 따라가기 — Day 3 `day3_trace.cfg` + `atrace_categories: "aidl"` 로 실습 11 앱의 `setProperty` 1회 캡처 → App UI Thread → `ICarProperty` Binder → `com.android.car` `CarPropertyService` → `IVehicle.setValues` → VHAL 프로세스 → `onChangeEvent` 콜백 → `Choreographer#doFrame` → SF 를 flow 화살표로 각자 잇기 · SQL 로 Binder 쌍 추출 (Day 3 실습 13 방법) | ★★☆ | 25분 | Ch 3·전체 |

> 실습 2·3·4 = "이미지 리빌드 없이 바꾸는 세 가지" 를 각각 한 번씩. 실습 6·7 = 같은 SystemUI 를 리소스(RRO)·코드(소스) 로 바꿔 경계 체감. 실습 9 = Day 3 의 SurfaceFlinger Layer 가 WMS 창으로 어떻게 보이는가. 실습 10~12 = Day 2 의 HAL·VINTF·UID 규칙이 `car_service`·`IVehicle/default` 에 그대로 적용되고, 저장소의 HvacSimulator(가짜 UI) 가 실제 차량 속성에 연결되는 순서. 실습 13 = 전원이 자기 앱의 버튼 한 번을 Perfetto 로 캡처해 4일 전체를 한 트레이스로 잇는다. 강사 시연·선택 과제는 없다.

### 실습별 요점

**실습 2. APEX — 35분** (`~/android/simple_apex/`, 저장소 `apex_build_tutorial_android16.md`)
- `binary/main.cpp` ("Hello from Simple APEX on Android 16") + `binary/Android.bp` (`cc_binary { name: "simple_apex_bin", apex_available: ["com.example.simple"] }`)
- `Android.bp`: `apex { name: "com.example.simple", manifest: "apex_manifest.json", file_contexts: ":com.example.simple-file_contexts", key: "com.example.simple.key", certificate: ":com.example.simple.certificate", binaries: ["simple_apex_bin"], updatable: false }` · `apex_key { public_key: "com.example.simple.avbpubkey", private_key: "com.example.simple.pem" }` · `android_app_certificate { certificate: "com.example.simple" }`
- 키 4종은 강사가 미리 만들어 각 계정 `simple_apex/` 에 넣어 둔다 (명령은 문서에 참고로: `openssl genrsa` → `avbtool extract_public_key` → `development/tools/make_key`) — 수강생은 `ls *.pem *.pk8 *.avbpubkey` 로 확인만
- `system/sepolicy/apex/com.example.simple-file_contexts` (`(/.*)? u:object_r:system_file:s0` · `/bin(/.*)? u:object_r:system_file:s0`) + `prebuilt_etc`/`filegroup` 등록 — 위치가 틀리면 `should be under system/sepolicy`
- `m com.example.simple` → `$OUT/system/apex/com.example.simple.apex` → WinSCP → `adb install com.example.simple.apex` → `adb reboot` → `adb shell` 에서 `ls /apex/com.example.simple/bin` → `/apex/com.example.simple/bin/simple_apex_bin` → `dumpsys apexservice \| grep simple` · `compressible: true` 를 넣으면 나는 오류 재현

**실습 3. RRO — 35분** (Android Studio, 저장소 `RRO_example_android16.md`)
- `RROTarget`: `hello_world` 문자열 · `background_color/text_color` · **`overlayable.xml`** (`RROTargetTheme`, `policy public`) · minSdk 34 (API 36 으로 하면 `INSTALL_FAILED_OLDER_SDK`)
- `RROOverlay`: `<overlay targetPackage="com.example.rrotarget" targetName="RROTargetTheme" priority="1" isStatic="false"/>` + `hasCode="false"` · 같은 이름 리소스만 · `Generate APKs` → `adb install` 두 개 → `cmd overlay list \| grep rro` (`[ ]`) → `enable` → 앱 재시작 → `dump` → `disable`
- drawable: `RROTarget` 에 `background_shape.xml`(회색 사각) · `ic_target_icon`(Android) + ImageView → `RROOverlay` 에 **같은 파일명** gradient shape · rocket 아이콘 → `install -r` → 확인
- 실험: `overlayable.xml` 에서 `hello_world` 를 빼면 활성화돼도 변경 없음 (계약)

**실습 4. SRO + ListOverlays — 25분** (저장소 `SRO_product_flavors_android16.md`, `RRO_test/overlay_list`)
- `RROTarget` 에 `flavorDimensions("theme")` + `productFlavors { create("original"); create("themed") }` → `src/themed/res/values/{strings,colors}.xml` (덮어쓸 것만) → **Build > Select Build Variant** `themedDebug` → 보라 배경 · main 수정 없음 · `Analyze APK` 로 `resources.arsc` 값 비교 · SRO vs RRO 표
- SRO 는 10분: `productFlavors { original; themed }` + `src/themed/res` → `themedDebug` → `Analyze APK`. "빌드 시 vs 런타임" 한 문장으로 정리하고 넘어간다 (App 개발 영역)
- `ListOverlays.java` (`IOverlayManager.Stub.asInterface(ServiceManager.getService("overlay"))` → `getOverlayInfosForTarget(pkg, userId)`) + `Android.bp` (`java_binary`, `libs: ["framework","services"]`, `sdk_version: "core_platform"`) → `mm` → `$OUT/system/framework/list_overlays.jar` → WinSCP → `adb push /data` → `adb shell` `CLASSPATH=/data/list_overlays.jar app_process /system/bin ListOverlays [pkg]` → `ENABLED/disabled` 목록. Day 3 실습 8 의 `app_process` + Day 2 hidden API 접근이 합쳐진 도구 — "SDK 에 없는 API 를 Framework 안에서 쓰는 법" 으로 반응이 좋다
- **우선순위 충돌 실험**: `RROOverlay2` 를 `priority="2"` 로 하나 더 만들어 같은 `hello_world` 를 다른 값으로 → 둘 다 enable → 어느 것이 보이는지 → `cmd overlay dump` 의 priority · `ListOverlays` 출력 순서 — 현장에서 "RRO 를 넣었는데 다른 RRO 가 덮는" 사고를 미리 겪게 한다

**실습 5. SystemUI 분석 — 20분** (저장소 SystemUI 실습 1)
- `dumpsys statusbar` 섹션 (mDisabled1/2 · Notifications · Quick Settings Tiles · Icons) · `am start -a android.intent.action.VIEW -d https://example.com` 전후 `dumpsys notification` · `svc wifi disable/enable` 전후 아이콘 · `dumpsys gfxinfo com.android.systemui` Janky frames · `dumpsys activity activities \| grep mResumedActivity` · QS 패널 펼침/접힘 비교

**실습 6. Car SystemUI RRO — 30분** (저장소 SystemUI 실습 2) — remount 2단계는 실습 1 에서 이미 끝낸 상태
- `cmd overlay list \| grep systemui` → `[x] [ ] ---` 읽기 → `dumpsys overlay \| grep -A5 com.android.systemui.car.rro` 의 `mTargetOverlayableName: null`
- `pm path com.android.systemui` → `adb pull /system_ext/priv-app/CarSystemUI/CarSystemUI.apk` → `aapt dump resources CarSystemUI.apk \| findstr /i color` → `system_bar_clock_text_color` · `system_bar_icon_color` · `car_nav_icon_fill_color` …
- `SystemUIOverlay` (targetName **없음**, minSdk 34, `hasCode=false`) · `colors.xml` 에 Car 이름으로
- `adb root; adb remount` (실습 1 에서 verity 해제·reboot 완료 → `remount succeeded`) → `mkdir /system/app/SystemUIOverlay` → `adb push systemuioverlay.apk /system/app/SystemUIOverlay/` → `reboot` → `cmd overlay list \| grep systemuioverlay` (`[ ]`) → `enable` → `killall com.android.systemui` → 결과표 (시계·아이콘 ✅, 바 배경 ⚠️ 투명) · `screencap` 전후 비교 · 트러블슈팅 7항목

**실습 7. SystemUI 소스 수정 — QS 타일 — 40분** (저장소 SystemUI 실습 3, 4 는 참고)
- `qs/tiles/CpuInfoTile.java` (`QSTileImpl<BooleanState>`, `@Inject` 9개 인자, `handleClick` 토글 → `handleUpdateState` 에 `/sys/class/thermal/thermal_zone0/temp`, `TILE_SPEC="cpuinfo"`) · `res/drawable/ic_qs_cpu_info.xml` · Dagger `@Binds @IntoMap @StringKey(CpuInfoTile.TILE_SPEC)` · `config.xml` `quick_settings_tiles_default` 에 `cpuinfo`
- **실패 재현(반응 좋음)**: Dagger `@Binds @IntoMap` 등록을 빼고 빌드·교체 → SystemUI 가 죽는다 → `logcat -s AndroidRuntime` 에서 `No tile for spec cpuinfo` / `IllegalArgumentException` 을 찾아 원인을 스스로 진단 → 등록 후 재빌드
- 참고 코드(문서에 수록, 시간 내 진행하지 않음): `statusbar/BatteryTemperatureView.java` + `status_bar.xml` — "우리 회사 로고/상태를 상태바에" 요청의 표준 패턴
- `m SystemUI` (증분 수 분) → `$OUT/system_ext/priv-app/CarSystemUI/CarSystemUI.apk` (Car 타겟 모듈명 확인 `CarSystemUI`) → WinSCP → remount 상태 `adb push` → `killall com.android.systemui` → QS 패널 타일 · 상태바 온도 · `dumpsys statusbar \| grep -i temperature` · Dagger 누락 시 크래시 로그(`logcat -s AndroidRuntime`) 읽기

**실습 8. SystemUI Perfetto — 20분** (저장소 SystemUI 실습 5)
- 저장소 cfg (buffers 63488+2048 · `sched/*` · `power/cpu_frequency` · `gfx/view/wm/am/input` · `atrace_apps: com.android.systemui` · `duration_ms: 10000`) 를 `C:\aosp16\day4_sysui.cfg` 로 push → 캡처 중 알림 패널 열고 닫기·QS 펼치기 반복
- UI: `com.android.systemui` 핀 → main Thread `Choreographer#doFrame` → `measure/layout/draw` → `RenderThread` `DrawFrames` → `surfaceflinger` `onMessageRefresh` · Actual Timeline Jank · `wm` 카테고리의 `relayoutWindow` · `input` 의 터치 이벤트에서 프레임까지 · `dumpsys gfxinfo com.android.systemui framestats` 대조 · Day 3 Native(Thread 1개) 와 다른 점(UI+Render 2개)

**실습 9. WindowManager 앱 — 25분** (저장소 `FloatingOverlayDemo.zip` · `SplitScreenDemo.zip`)
- `FloatingOverlayDemo`: `MainActivity` 가 `Settings.canDrawOverlays` 확인 → `ACTION_MANAGE_OVERLAY_PERMISSION` / Emulator 는 `appops set com.example.floatingoverlaydemo SYSTEM_ALERT_WINDOW allow` → `FloatingService.onCreate` 에서 `WindowManager.LayoutParams(WRAP, WRAP, TYPE_APPLICATION_OVERLAY, FLAG_NOT_FOCUSABLE, TRANSLUCENT)` + `addView` → 다른 앱 위에 떠 있는 창 → `dumpsys window windows \| grep -A3 floating` · `dumpsys SurfaceFlinger --list` 에서 새 Layer (Day 3 실습 9 의 `createSurface` 를 WMS 가 대신 한 것) · `onDestroy` `removeView`
- `appops set … SYSTEM_ALERT_WINDOW deny` 후 다시 실행 → `BadTokenException: permission denied for window type 2038` 재현 → 권한이 WMS 에서 검사됨을 확인 (Day 2 UID 검사의 WMS 판)
- `SplitScreenDemo` 5분: 저장소 zip 을 열어 Run → 좌우 Fragment → `dumpsys SurfaceFlinger --list` 로 앱 창 Layer 가 **하나**임을 각자 확인 · `dumpsys display` 로 Automotive 디스플레이 목록 (멀티 디스플레이는 디스플레이마다 Layer 스택이 다르다는 것만)

**실습 10. Automotive 계층 확인 + `cmd car_service` — 30분**
- `service list \| grep -E "car_service\|vehicle"` (`car_service: [android.car.ICar]` · `android.hardware.automotive.vehicle.IVehicle/default`) · `ps -A \| grep -E "com.android.car\|vehicle"` (uid system vs vendor) · `dumpsys car_service --hal` (VHAL 연결·지원 속성 수) · `dumpsys car_service --services CarPropertyService \| head -40`
- `dumpsys android.hardware.automotive.vehicle.IVehicle/default --list \| head` → 속성 ID · `--get 0x15600503 -a 1` (운전석 HVAC 온도) · `/vendor/etc/vintf/manifest.xml` 의 `android.hardware.automotive.vehicle` 항목 — Day 2 실습 10 표와 대조 · `pm list packages \| grep kitchensink`
- **`cmd car_service` 실무 명령 (반응 좋음)**: `cmd car_service day-night-mode night` → Launcher·SystemUI 가 즉시 다크 테마로 → `day` 로 복귀 · `cmd car_service inject-vhal-event 0x11600207 60` → 클러스터 속도 60 · `cmd car_service get-property-value 0x11600207` · `inject-key 3`(HOME) — "차 없이 시나리오 재현" 이 실무 가치의 핵심임을 설명
- **권한 거부 추적 절차**: Kitchen Sink 또는 실습 11 앱에서 권한 없이 `getProperty(PERF_VEHICLE_SPEED)` → `SecurityException` → `logcat -s CarPropertyService` 의 `Permission denied for property` → `CarPropertyService` 소스의 `mPropToPermission` (`:tag` 또는 grep) → `android.car.permission.CAR_SPEED` → `pm grant` 로 되는지 / privileged 인지 판단표

**실습 11. HvacSimulator → CarPropertyManager 연동 — 35분** (저장소 `HvacSimulator.zip` 확장)
- 저장소 앱 그대로 실행(가짜 값) → `build.gradle.kts` 에 `useLibrary("android.car")` · Manifest `<uses-library android:name="android.car"/>` · `<uses-permission android:name="android.car.permission.CONTROL_CAR_CLIMATE"/>`
- `Car.createCar(this, null, Car.CAR_WAIT_TIMEOUT_WAIT_FOREVER, (car, ready) -> { pm = (CarPropertyManager) car.getCarManager(Car.PROPERTY_SERVICE); … })`
- 온도 버튼 → `pm.setProperty(Float.class, VehiclePropertyIds.HVAC_TEMPERATURE_SET, SEAT_ROW_1_LEFT(0x0001)/RIGHT(0x0004), value)` · FAN 버튼 → `HVAC_FAN_SPEED`(Integer) · `HVAC_POWER_ON`(Boolean) · 시작 시 `getProperty` 로 현재값 · `getCarPropertyConfig` 의 `areaIds`/`minValue/maxValue` 로 단위·범위
- `subscribePropertyEvents(callback, HVAC_TEMPERATURE_SET, SENSOR_RATE_ONCHANGE)` → `onChangeEvent` 에서 UI 갱신 (Binder Thread → `runOnUiThread`, Day 1 실습 9 · Day 3 실습 6)
- `adb shell pm grant com.example.hvacsimulator android.car.permission.CONTROL_CAR_CLIMATE` (강사가 사전 확인한 방식 — 안 되면 배포된 platform 서명본 설치)

**실습 12. VHAL 조작 · 동기화 · 실패 재현 · 권한 추적 — 25분**
- `dumpsys android.hardware.automotive.vehicle.IVehicle/default --set 0x15600503 -a 1 -f 22.0` → 실습 11 앱 UI 가 22.0 으로 · Kitchen Sink HVAC 탭에서 바꿔도 동기화 · `logcat -s CarPropertyService PropertyHalService` 로 VHAL → CarService → App 경로 · `dumpsys car_service --services CarPropertyService \| grep -A3 Subscri` 에 우리 앱의 subscriber
- **실패 재현**: `setProperty` 에 `minValue/maxValue` 밖 값(온도 50.0) → `IllegalArgumentException` · areaId 를 GLOBAL(0) 로 → `IllegalArgumentException: areaId` · 콜백에서 `runOnUiThread` 를 빼면 `CalledFromWrongThreadException`
- **권한 거부 추적**: `CONTROL_CAR_CLIMATE` 를 `pm revoke` → `SecurityException` → `logcat -s CarPropertyService` 의 `Permission denied for property` → `CarPropertyService` 소스의 `mPropToPermission` (grep) → 다시 `pm grant` — "권한 거부 → 어떤 권한이 필요한가" 를 스스로 찾는 절차
- 정리 질문: "이 앱은 CarService 의 무엇을 거쳐 VHAL 에 갔나" 를 `dumpsys`·`logcat` 출력으로 답하게 한다 → 실습 13 의 예습

**실습 13. Perfetto 로 온도 버튼 한 번 따라가기 — 25분** (전원, Day 3 실습 13 절차 재사용)
- Day 3 `day3_trace.cfg` 에 `atrace_categories: "aidl"` 과 `atrace_apps: "com.example.hvacsimulator"` 를 더한 `day4_car.cfg` 를 push → 캡처 중 실습 11 앱의 온도 버튼을 3번 누른다 → `adb pull`
- UI 에서 핀: `com.example.hvacsimulator` · `com.android.car` · `android.hardware.automotive.vehicle…` · `surfaceflinger`
- flow 따라가기 (Day 3 실습 13 Step 5 방식): 앱 UI Thread `binder transaction`(`ICarProperty::setProperty`) ─▶ `com.android.car` `binder:PID_N` `binder reply` (CarPropertyService, Day 2 실습 11) ─▶ 같은 Thread 에서 `IVehicle::setValues` `binder transaction` ─▶ VHAL 프로세스 `binder reply` (AIDL HAL, Day 2 실습 10) ─▶ 역방향 `onChangeEvent` 콜백 (Day 1 실습 9 · Day 3 실습 6) ─▶ 앱 `runOnUiThread` → `Choreographer#doFrame` → `RenderThread` → `surfaceflinger` `onMessageRefresh` (Day 3)
- SQL (Day 3 실습 13 ③ 재사용) 로 `hvacsimulator`/`com.android.car`/`vehicle` 의 `binder transaction`·`binder reply` 쌍과 `dur_us` 추출 → "버튼 한 번에 Binder 왕복 몇 번, 총 몇 ms" 를 각자 기록
- 이 한 장의 트레이스가 4일 과정의 요약이다 — 총정리에서 그림과 대조

---

## Part 3. 총정리 & Worksheet (총 40분)

### 4일 과정 총정리 — 20분
- 실습 13 에서 각자 캡처한 트레이스를 띄워 놓고: Day 1 Binder/AIDL(Java) → Day 2 Native Binder·HAL·SELinux → Day 3 libutils·JNI·SurfaceFlinger·공유 메모리 → Day 4 시스템 커스터마이징·WindowManager·Automotive 가 그 flow 의 어느 구간인지 대응
- **델타 배포 판단표** (Ch 1 의 표를 4일 전체로 확장): 바이너리/`.so` → `m` + `adb push` · Java 리소스 → RRO · 시스템 앱 코드 → `m <앱>` + push + `killall` · 독립 모듈 → APEX · Native Service 등록/init.rc/sepolicy → Day 2 절차 · 커널/기본 탑재 RRO/VHAL 속성 추가 → `m emu_img_zip`
- **"안 될 때 보는 순서" 카드**: `logcat -s AndroidRuntime` → `dumpsys <service>` → `cmd overlay list` / `service list` → `getenforce`·`avc: denied` → `adb remount` 상태 → Perfetto. 4일간 쓴 도구를 진단 순서로 재배열
- 현장 적용 제안: ① 우리 제품의 SystemUI 색·아이콘 RRO 1개 ② 우리 HAL 속성 1개를 `CarPropertyManager` 로 읽는 앱 ③ 서비스 하나를 Perfetto 로 캡처해 왕복 시간 기록
- 질의응답

### Day 4 Worksheet — 20분
- 객관식 · 단답형 · 빈칸 · 순번 · 선잇기 20문제, 별도 정답·해설

**출제 키워드**
```text
APEX / apex_manifest.json / apex_key / android_app_certificate / file_contexts 는 system/sepolicy/apex / /apex 마운트 / adb install .apex / updatable / compressible
RRO / overlayable / policy public / overlay targetPackage·targetName·priority·isStatic / hasCode=false / cmd overlay list·enable·disable·dump / [x] [ ] ---
OverlayManagerService / IOverlayManager / app_process 로 hidden API
SRO / Product Flavors / src/<flavor>/res / 병합 우선순위 / 빌드 시 vs 런타임
Car SystemUI / overlayable 미선언 / /system/app 설치 / 2단계 remount / aapt dump resources / 투명 배경
dumpsys statusbar / notification / gfxinfo / QSTileImpl / handleClick·handleUpdateState / Dagger @IntoMap / config.xml / BatteryTemperatureView / status_bar.xml
Choreographer#doFrame / UI Thread / RenderThread / Jank / atrace view·wm·am·input
WindowManager / TYPE_APPLICATION_OVERLAY / SYSTEM_ALERT_WINDOW / appops / addView / dumpsys window / Fragment 분할
Car API / Car.createCar / CarService / car_service / ICar / com.android.car / CarPropertyService
CarPropertyManager / getProperty / setProperty / subscribePropertyEvents / VehiclePropertyIds / areaId (SEAT) / CarPropertyConfig
VHAL / IVehicle/default / AIDL HAL / VINTF manifest / FakeVehicleHardware / dumpsys --list --get --set / Kitchen Sink
Vehicle Property ID 구조 (Group·Area·Type) / HVAC_TEMPERATURE_SET / HVAC_FAN_SPEED / 권한 CONTROL_CAR_CLIMATE
```

---

## 준비물 체크리스트

**강사 / 사전 준비**
- [ ] 저장소 `04_day` 의 zip 3종(`FloatingOverlayDemo` · `SplitScreenDemo` · `HvacSimulator`) 과 md 4종 배포 · `RRO_test/overlay_list` 를 각 계정 `~/android/RRO_test/` 에 복사
- [ ] Emulator `-writable-system` 부팅에서 **2단계 remount 가 되는지** 사전 확인 (`remount succeeded`) — 실습 1 에서 전원이 끝내고 시작
- [ ] `~/android/simple_apex/` 골격 + `system/sepolicy/apex/com.example.simple-file_contexts` + **서명 키 4종을 각 계정에 미리 생성·복사** (실습 2 는 소스·bp 작성과 빌드·설치만)
- [ ] `m SystemUI`(CarSystemUI) 각 계정 사전 1회 빌드 (실습 7 증분용) · `CpuInfoTile.java` · `BatteryTemperatureView.java` · `ic_qs_cpu_info.xml` 완성본
- [ ] `RROTarget` / `RROOverlay` / `SystemUIOverlay` / HvacSimulator+Car 연동본 Android Studio 완성 프로젝트 zip · Automotive 시스템 이미지(API 35)로 `android.car` 참조 가능한지 확인
- [ ] `aapt` 경로 (`%LOCALAPPDATA%\Android\Sdk\build-tools\35.0.0\aapt.exe`) PATH 안내
- [ ] HvacSimulator 의 `CONTROL_CAR_CLIMATE` 가 `pm grant` 로 되는지 확인 — 안 되면 platform 서명본 준비
- [ ] Kitchen Sink 존재 확인 (`pm list packages \| grep kitchensink`) · VHAL `--list` · `--get 0x15600503 -a 1` 출력 확인
- [ ] 실습 8 cfg(`day4_sysui.cfg`) · 실습 13 cfg(`day4_car.cfg` = Day 3 cfg + `aidl` + hvacsimulator) 배포본 · 강사용 예시 트레이스 1개 (수강생 캡처 실패 시 대조용)
- [ ] Day 4 Worksheet 인쇄본 + 정답지 · 4일 총정리 그림 한 장

**수강생**
- [ ] Day 3 `day3_trace.cfg` 가 Emulator `/data/local/tmp/` 에 남아 있을 것 (실습 8 비교용)
- [ ] `C:\aosp16\bin\` 배포 절차 · `adb install` 절차 숙지
- [ ] Day 2 ctags(`android_native_tags`) 유지 (선택: `frameworks/base/services` · `packages/services/Car` Java tags)
