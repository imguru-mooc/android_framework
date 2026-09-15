# 실습 10. AIDL HAL 구조 따라 만들기 — NDK backend · vendor 프로세스 · VINTF

> **소요시간:** 45분 · **난이도:** ★★★ · **챕터:** Ch 4 HAL
> **디렉토리:** `~/aosp/day2/led_hal`

## 목표

- Framework ↔ HAL 경계의 규칙(패키지명, NDK backend, `vendor: true`, `stability: vintf`)을 실제 HAL 과 같은 형태로 만든다.
- `libbinder_ndk`(C API 계열) 로 HAL 서비스와 Client 를 작성한다.
- VINTF manifest 가 왜 필요한지, 실습에서는 왜 `unstable` 로 우회하는지 이해한다.

## 구조

```
Framework (system 파티션)                      Vendor (vendor 파티션)
 led_hal_client  ── AServiceManager_checkService ──▶  vendor.example.led-service
 (system_server 의 역할)      /dev/binder            ILed 구현 (NDK backend BnLed)
                                                       ↑ VINTF manifest 에 선언 (실무)
```

| 실제 HAL | 이번 실습 |
|---|---|
| `android.hardware.light` 패키지 | `vendor.example.led` |
| `stability: "vintf"` + `versions` | `unstable: true` (전체 이미지 리빌드 회피) |
| `hardware/interfaces/…/aidl/` | `day2/led_hal/` |
| `/vendor/etc/vintf/manifest.xml` 에 등록 | 등록 없이 `/data` 에서 실행 |

---

## Step 1. AIDL (패키지 포함)

**`led_hal/ILed.aidl`**

```java
package vendor.example.led;

interface ILed {
    void setOn(boolean on);
    boolean isOn();
}
```

디렉토리도 패키지와 맞춘다: `led_hal/vendor/example/led/ILed.aidl` 로 옮기고 `Android.bp` 의 `srcs` 를 `["led_hal/vendor/example/led/ILed.aidl"]` 로 바꾼다 — **`.bp` 를 고치면 Soong 재분석**이 돌므로, 실습 1 에서 이미 이 경로로 만들어 두었다면 그대로 진행한다.

```bash
cd ~/aosp/day2/led_hal
mkdir -p vendor/example/led && git mv ILed.aidl vendor/example/led/ 2>/dev/null || mv ILed.aidl vendor/example/led/
```

## Step 2. Android.bp 확인 (실습 1 등록분)

```text
aidl_interface {
    name: "vendor.example.led",
    srcs: ["led_hal/vendor/example/led/ILed.aidl"],
    local_include_dir: "led_hal",
    vendor_available: true,                              // vendor 파티션에서 링크 가능
    backend: { ndk: { enabled: true }, cpp: { enabled: false }, java: { enabled: false } },
    unstable: true,                                      // 실무: stability: "vintf", versions: ["1"]
}
cc_binary {
    name: "vendor.example.led-service",
    srcs: ["led_hal/LedHal.cpp"],
    shared_libs: ["libbinder_ndk", "libbase", "liblog"],
    static_libs: ["vendor.example.led-ndk"],
    vendor: true,                                        // ★ /vendor/bin 에 설치되는 vendor 모듈
}
cc_binary {
    name: "led_hal_client",
    srcs: ["led_hal/LedHalClient.cpp"],
    shared_libs: ["libbinder_ndk", "libbase", "liblog"],
    static_libs: ["vendor.example.led-ndk"],
}
```

## Step 3. HAL 서비스 (NDK backend)

**`led_hal/LedHal.cpp`**

```cpp
#define LOG_TAG "vendor.example.led"
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android-base/logging.h>
#include <aidl/vendor/example/led/BnLed.h>          // ★ NDK backend 는 aidl:: 네임스페이스

using aidl::vendor::example::led::BnLed;
using ndk::ScopedAStatus;

class Led : public BnLed {
    bool on_ = false;
public:
    ScopedAStatus setOn(bool on) override {
        on_ = on;
        LOG(INFO) << "setOn(" << on << ")  — 실제 HAL 이라면 여기서 sysfs / i2c 에 쓴다";
        return ScopedAStatus::ok();
    }
    ScopedAStatus isOn(bool* _aidl_return) override {
        *_aidl_return = on_;
        return ScopedAStatus::ok();
    }
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Led> led = ndk::SharedRefBase::make<Led>();

    // ★ HAL 인스턴스 이름 규칙: <패키지>.<인터페이스>/<instance>
    const std::string name = std::string(Led::descriptor) + "/default";
    binder_status_t st = AServiceManager_addService(led->asBinder().get(), name.c_str());
    LOG(INFO) << "addService(" << name << ") = " << st;

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;   // 도달하지 않음
}
```

## Step 4. Client (Framework 측 역할)

**`led_hal/LedHalClient.cpp`**

```cpp
#include <cstdio>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <aidl/vendor/example/led/ILed.h>

using aidl::vendor::example::led::ILed;

int main(int argc, char** argv) {
    const std::string name = std::string(ILed::descriptor) + "/default";
    ndk::SpAIBinder binder(AServiceManager_checkService(name.c_str()));
    if (binder.get() == nullptr) { printf("%s not found\n", name.c_str()); return 1; }

    std::shared_ptr<ILed> led = ILed::fromBinder(binder);     // ★ NDK 의 interface_cast
    bool on = argc > 1 && std::string(argv[1]) == "on";
    led->setOn(on);
    bool now = false;
    led->isOn(&now);
    printf("LED is now %s\n", now ? "ON" : "OFF");
    return 0;
}
```

## Step 5. 빌드 · 배포

```bash
cd ~/aosp/day2 && mm -j$(nproc)
ls $OUT/vendor/bin/vendor.example.led-service $OUT/system/bin/led_hal_client
```

`vendor: true` 라 결과물이 **`$OUT/vendor/bin`** 에 생긴다.

```bat
adb push vendor.example.led-service /data
adb push led_hal_client /data
adb shell chmod 755 /data/vendor.example.led-service /data/led_hal_client
```

## Step 6. 실행

```bash
adb shell /data/vendor.example.led-service &
adb shell service list | grep vendor.example
# vendor.example.led.ILed/default: [vendor.example.led.ILed]        ← ★ HAL 이름 규칙
adb shell /data/led_hal_client on      # LED is now ON
adb shell /data/led_hal_client off     # LED is now OFF
adb shell logcat -s vendor.example.led -d | tail -3
```

실제 Light HAL 과 비교:

```bash
adb shell service list | grep -E "hardware\.(light|vibrator|power)"
# android.hardware.light.ILights/default: [android.hardware.light.ILights]
```

## Step 7. VINTF — 왜 필요한가 (읽기)

```bash
adb shell cat /vendor/etc/vintf/manifest.xml | grep -A4 "android.hardware.light"
```

```xml
<hal format="aidl">
    <name>android.hardware.light</name>
    <version>2</version>
    <fqname>ILights/default</fqname>
</hal>
```

- **manifest**(vendor) = "이 기기는 이 HAL 을 이 버전으로 제공한다"
- **compatibility matrix**(system) = "Framework 는 이 HAL 이 이 버전 범위여야 부팅한다"
- 부팅 시 두 파일을 대조하고, `stability: "vintf"` 인 인터페이스는 `AServiceManager_addService` 때 manifest 에 없으면 **등록이 거부**된다.
- 우리 실습은 `unstable: true` 라 검사 대상이 아니다. 실제 HAL 로 만들려면: `stability: "vintf"`, `versions: ["1"]`, `aidl_api/` 동결(`m vendor.example.led-freeze-api`), `manifest_led.xml` 을 `vintf_fragments` 로 추가, `vendor` 이미지 리빌드.

---

## 확인 포인트

- [ ] `$OUT/vendor/bin` 에 서비스, `$OUT/system/bin` 에 client
- [ ] `service list` 에 `vendor.example.led.ILed/default` (패키지.인터페이스/인스턴스)
- [ ] `on/off` 동작, logcat 에 HAL 로그
- [ ] `/vendor/etc/vintf/manifest.xml` 에서 실제 HAL 항목 찾기

## 핵심 정리

| 항목 | cpp backend (실습 7) | ndk backend (HAL) |
|---|---|---|
| 네임스페이스 | `android::` | `aidl::<pkg>::` |
| 라이브러리 | `libbinder` | `libbinder_ndk` (vendor 에서 링크 가능) |
| 상태 타입 | `binder::Status` | `ndk::ScopedAStatus` |
| 객체 참조 | `sp<>` | `std::shared_ptr` + `SharedRefBase::make` |
| Service 등록 | `defaultServiceManager()->addService` | `AServiceManager_addService` |
| Proxy 획득 | `interface_cast<>` | `I::fromBinder` |

vendor 프로세스는 `libbinder`(system 전용) 를 쓸 수 없어 **NDK backend 가 HAL 의 표준**이다.

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `aidl/vendor/example/led/BnLed.h: No such file` | `.aidl` 경로가 패키지와 일치하는지 (`led_hal/vendor/example/led/`), `local_include_dir` 확인 |
| `library "libbinder" ... vendor` 링크 오류 | vendor 모듈은 `libbinder_ndk` 만 링크 가능 |
| `addService = 1` (STATUS_PERMISSION) | `setenforce 0`, 또는 실제 HAL 이름과 충돌하지 않는지 확인 |
