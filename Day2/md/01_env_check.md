# 실습 1. Day 2 환경 점검 · 실습 모듈 일괄 등록

> **소요시간:** 20분 · **난이도:** ★☆☆
> **환경:** Ubuntu 빌드 서버 (`~/aosp`, PuTTY) + 커스텀 Emulator (adb root)

## 목표

- Day 1 에서 만든 빌드 환경과 Emulator 가 정상인지 확인한다.
- Day 2 실습 전체의 `Android.bp` 를 **한 번에** 등록해 Soong 재분석(6분+)을 하루 한 번으로 끝낸다.
- 이후 실습에서는 `.cpp` 만 편집 → `mm` 수십 초.

---

## Step 1. 빌드 환경 확인

```bash
cd ~/aosp
source build/envsetup.sh
lunch sdk_car_x86_64-aosp_current-userdebug
echo $OUT
# /home/<계정>/aosp/out/target/product/emulator_car64_x86_64
```

## Step 2. Emulator 확인 (Windows)

```bat
adb devices
adb root
adb shell getprop ro.build.type       REM userdebug
adb shell getenforce                  REM Permissive (커스텀 이미지)
```

## Step 3. Day 2 실습 디렉토리 생성

실습 2~10 의 디렉토리와 `Android.bp` 를 미리 만든다. 소스 파일은 각 실습에서 작성하지만 `Android.bp` 가 참조하는 파일명이 존재해야 Soong 분석이 통과하므로 **빈 파일**을 먼저 만든다.

```bash
mkdir -p ~/aosp/day2 && cd ~/aosp/day2
mkdir -p binder1 binder2 binder3 binder4 binder5 binder7 auth rc_service led_hal

# 빈 소스 파일 생성 (실습에서 채운다)
touch binder1/my_server.cpp
touch binder2/my_server.cpp binder2/my_client.cpp binder2/ILedService.cpp binder2/ILedService.h
touch binder3/my_server.cpp binder3/my_client.cpp binder3/ILedService.cpp binder3/ILedService.h binder3/LedService.cpp binder3/LedService.h
touch binder4/my_server.cpp binder4/my_client.cpp binder4/ILedService.cpp binder4/ILedService.h binder4/LedService.cpp binder4/LedService.h
touch binder5/server.cpp binder5/client.cpp binder5/IMyDataService.cpp binder5/IMyDataService.h binder5/IMyDataCallback.cpp binder5/IMyDataCallback.h
touch binder7/server.cpp binder7/client.cpp binder7/IMyDataService.aidl binder7/IMyDataCallback.aidl
touch auth/LedAuthService.cpp auth/LedAuthClient.cpp auth/ILedAuthService.aidl
touch rc_service/rc_led_service.cpp
touch led_hal/LedHal.cpp led_hal/LedHalClient.cpp led_hal/ILed.aidl
```

> `.aidl` 은 빈 파일이면 aidl 컴파일이 실패하므로 아래 최소 내용을 넣어 둔다.

```bash
cat > binder7/IMyDataCallback.aidl << 'EOT'
interface IMyDataCallback { void onDataReceived(String data); }
EOT
cat > binder7/IMyDataService.aidl << 'EOT'
import IMyDataCallback;
interface IMyDataService { void registerCallback(IMyDataCallback cb); }
EOT
cat > auth/ILedAuthService.aidl << 'EOT'
interface ILedAuthService { void LEDON(); }
EOT
cat > led_hal/ILed.aidl << 'EOT'
package vendor.example.led;
interface ILed { void setOn(boolean on); boolean isOn(); }
EOT
```

## Step 4. Android.bp 일괄 작성

**`~/aosp/day2/Android.bp`** (한 파일에 전부)

```text
// ---------- binder1 : 최소 서버 ----------
cc_binary { name: "my_server_test_1", srcs: ["binder1/my_server.cpp"], shared_libs: ["liblog","libbinder","libutils"] }

// ---------- binder2 : Client 측 Proxy 수작업 ----------
cc_binary { name: "my_server_test_2", srcs: ["binder2/my_server.cpp"], shared_libs: ["liblog","libbinder","libutils"] }
cc_binary { name: "my_client_test_2", srcs: ["binder2/my_client.cpp", "binder2/ILedService.cpp"], shared_libs: ["liblog","libbinder","libutils"] }

// ---------- binder3 : Bn/Bp 완성 ----------
cc_binary { name: "my_server_test_3", srcs: ["binder3/my_server.cpp", "binder3/ILedService.cpp", "binder3/LedService.cpp"], shared_libs: ["liblog","libbinder","libutils"] }
cc_binary { name: "my_client_test_3", srcs: ["binder3/my_client.cpp", "binder3/ILedService.cpp"], shared_libs: ["liblog","libbinder","libutils"] }

// ---------- binder4 : 인자 전달 ----------
cc_binary { name: "my_server_test_4", srcs: ["binder4/my_server.cpp", "binder4/ILedService.cpp", "binder4/LedService.cpp"], shared_libs: ["liblog","libbinder","libutils"] }
cc_binary { name: "my_client_test_4", srcs: ["binder4/my_client.cpp", "binder4/ILedService.cpp"], shared_libs: ["liblog","libbinder","libutils"] }

// ---------- binder5 : Callback 수작업 ----------
cc_binary { name: "binder_server_5", srcs: ["binder5/server.cpp", "binder5/IMyDataService.cpp", "binder5/IMyDataCallback.cpp"], shared_libs: ["libbinder","libutils"] }
cc_binary { name: "binder_client_5", srcs: ["binder5/client.cpp", "binder5/IMyDataService.cpp", "binder5/IMyDataCallback.cpp"], shared_libs: ["libbinder","libutils"] }

// ---------- binder7 : AIDL cpp backend ----------
aidl_interface {
    name: "my_interface",
    srcs: ["binder7/IMyDataCallback.aidl", "binder7/IMyDataService.aidl"],
    local_include_dir: "binder7",
    backend: { cpp: { enabled: true } },
    unstable: true,
}
cc_binary { name: "binder_server_7", srcs: ["binder7/server.cpp"], shared_libs: ["libbinder","libutils"], static_libs: ["my_interface-cpp"] }
cc_binary { name: "binder_client_7", srcs: ["binder7/client.cpp"], shared_libs: ["libbinder","libutils"], static_libs: ["my_interface-cpp"] }

// ---------- auth : UID 접근 제어 ----------
aidl_interface {
    name: "led_auth_interface",
    srcs: ["auth/ILedAuthService.aidl"],
    local_include_dir: "auth",
    vendor_available: true,
    backend: { cpp: { enabled: true } },
    unstable: true,
}
cc_binary { name: "led_auth_service", srcs: ["auth/LedAuthService.cpp"], shared_libs: ["libbinder","libutils"], static_libs: ["led_auth_interface-cpp"] }
cc_binary { name: "led_auth_client", srcs: ["auth/LedAuthClient.cpp"], shared_libs: ["libbinder","libutils"], static_libs: ["led_auth_interface-cpp"] }

// ---------- rc_service : init.rc 로 자동 시작 ----------
cc_binary {
    name: "rc_led_service",
    srcs: ["rc_service/rc_led_service.cpp"],
    shared_libs: ["libbinder","libutils","liblog"],
    init_rc: ["rc_service/rc_led_service.rc"],
}

// ---------- led_hal : AIDL HAL 구조 ----------
aidl_interface {
    name: "vendor.example.led",
    srcs: ["led_hal/ILed.aidl"],
    local_include_dir: "led_hal",
    vendor_available: true,
    backend: { ndk: { enabled: true }, cpp: { enabled: false }, java: { enabled: false } },
    unstable: true,
}
cc_binary {
    name: "vendor.example.led-service",
    srcs: ["led_hal/LedHal.cpp"],
    shared_libs: ["libbinder_ndk","libbase","liblog"],
    static_libs: ["vendor.example.led-ndk"],
    vendor: true,
}
cc_binary {
    name: "led_hal_client",
    srcs: ["led_hal/LedHalClient.cpp"],
    shared_libs: ["libbinder_ndk","libbase","liblog"],
    static_libs: ["vendor.example.led-ndk"],
}
```

`rc_service/rc_led_service.rc` 도 미리 만들어 둔다 (내용은 실습 9 에서 채움):

```bash
cat > rc_service/rc_led_service.rc << 'EOT'
service rc_led_service /system/bin/rc_led_service
    class main
    user system
    group system
    disabled
EOT
```

## Step 5. Soong 분석 1회 실행

빈 `.cpp` 는 링크 오류가 나므로 **분석만** 통과시키는 것이 목적이다. 모듈 하나만 지정해 빌드하면 분석은 전체, 컴파일은 그 모듈만 한다.

```bash
cd ~/aosp
m my_server_test_1 2>&1 | tail -3
# 분석 (5~6분) → my_server.cpp 가 비어 있으면 링크 오류 — 정상. 분석은 끝났다.
```

이후 실습에서 `.cpp` 를 채운 뒤 `mm` 또는 `m <module>` 을 실행하면 `analyzing Android.bp` 없이 수십 초에 끝난다.

---

## 확인 포인트

- [ ] `echo $OUT` 정상 · `adb shell getenforce` = Permissive
- [ ] `~/aosp/day2/Android.bp` 작성, `m my_server_test_1` 에서 `analyzing Android.bp` 단계 통과
- [ ] 이후 `mm` 실행 시 분석 단계가 다시 나오지 않음

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `error: day2/Android.bp: … file not found` | `touch` 로 빈 파일을 다 만들었는지 확인 |
| aidl 컴파일 오류 | `.aidl` 최소 내용(Step 3)이 들어 있는지 확인 |
| `Killed` (OOM) | Day 1 실습 4 참고 — swap 32GB, 동시 빌드 금지 |
| `.bp` 를 고쳤더니 다시 6분 | 정상. `.bp` 는 하루 한 번만 손댄다 |
