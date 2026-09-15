# 실습 11. Java System Service 들여다보기 — SystemServer · ServiceManager · dumpsys

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 3 System Service
> **환경:** 빌드 서버 (소스 읽기) + Emulator

## 목표

- `system_server` 안에서 Java System Service 가 **등록되는 코드 경로**를 소스에서 찾는다.
- `ServiceManager.addService` (Java) 가 오늘 만든 `defaultServiceManager()->addService` (C++) 와 같은 곳에 도착함을 확인한다.
- `dumpsys` 가 Binder `dump()` Transaction 임을 본다.

---

## Step 1. SystemServer.java 읽기

```bash
cd ~/aosp
grep -n "startBootstrapServices\|startCoreServices\|startOtherServices" frameworks/base/services/java/com/android/server/SystemServer.java | head
```

`run()` 안의 세 단계:

| 단계 | 대표 Service |
|---|---|
| `startBootstrapServices` | ActivityManagerService, PowerManagerService, PackageManagerService |
| `startCoreServices` | BatteryService, UsageStatsService |
| `startOtherServices` | WindowManagerService, InputManagerService, **CarService (Automotive)** … |

```bash
sed -n '/private void startOtherServices/,/^    }/p' frameworks/base/services/java/com/android/server/SystemServer.java | grep -n "mSystemServiceManager.startService\|ServiceManager.addService" | head -20
```

## Step 2. 등록 경로 추적

```bash
# Java → Native
grep -n "public static void addService" frameworks/base/core/java/android/os/ServiceManager.java
grep -n "addService" frameworks/base/core/java/android/os/ServiceManagerNative.java | head -3
grep -rn "getIServiceManager()" frameworks/base/core/java/android/os/ServiceManager.java | head -3
```

경로: `ServiceManager.addService()` → `getIServiceManager()` (`IServiceManager` AIDL Proxy) → Binder handle 0 → **servicemanager 데몬**. C++ 의 `defaultServiceManager()` 와 완전히 같은 목적지다.

```bash
# servicemanager 데몬 자체
ls frameworks/native/cmds/servicemanager/
grep -n "Status ServiceManager::addService" frameworks/native/cmds/servicemanager/ServiceManager.cpp
```

`ServiceManager::addService` 안의 `mAccess->canAdd(ctx, name)` 이 SELinux 검사(실습 9 의 `{ add }`)다.

## Step 3. Emulator 에서 확인

```bash
adb shell
```

```bash
service list | wc -l
service list | grep -E "^ *[0-9]+\s+(activity|window|car_service|led\.auth|rc\.led)"
# activity: [android.app.IActivityManager]        ← Java, system_server
# car_service: [android.car.ICar]                 ← Java, com.android.car 프로세스
# led.auth: [ILedAuthService]                     ← 우리 C++ 서비스 (실습 8 서버가 떠 있으면)

# 같은 테이블에 Java / C++ / vendor HAL 이 나란히 있다
service list | grep -c "\."
```

## Step 4. dumpsys = Binder dump()

```bash
dumpsys activity services | head -20
dumpsys -l | head            # dump 를 지원하는 Service 목록
```

`dumpsys` 는 각 Service 의 `IBinder.dump(fd, args)` — Transaction code `DUMP_TRANSACTION` — 를 호출해 결과를 파이프로 받는다. 우리 C++ 서비스에 `dump()` 를 오버라이드하면 `dumpsys led.auth` 가 동작한다:

```cpp
// LedAuthService 에 추가 (선택)
android::status_t dump(int fd, const android::Vector<android::String16>&) override {
    dprintf(fd, "LedAuthService: allowed uid=%d, calls=%d\n", 1000, calls_);
    return android::NO_ERROR;
}
```

```bash
dumpsys led.auth
# LedAuthService: allowed uid=1000, calls=3
```

## Step 5. Java Service 하나 골라 따라가기 (예: `battery`)

```bash
grep -rn "publishBinderService(\"battery\"\|Context.BATTERY_SERVICE" frameworks/base/services/core/java/com/android/server/BatteryService.java | head
grep -n "class BinderService extends" frameworks/base/services/core/java/com/android/server/BatteryService.java
```

- `publishBinderService(name, new BinderService())` → 내부에서 `ServiceManager.addService`
- `BinderService extends Binder` → C++ `BBinder` 의 Java 판. `dump()` 오버라이드 → `dumpsys battery`
- App 은 `BatteryManager` → `IBatteryStats` Proxy → Binder → 이 객체

---

## 확인 포인트

- [ ] `SystemServer.java` 의 3단계 함수와 `startOtherServices` 의 `ServiceManager.addService` 호출 위치
- [ ] `service list` 에 Java · C++ · vendor HAL 이 한 테이블에 공존
- [ ] `dumpsys -l` 에 등록된 Service, (선택) `dumpsys led.auth` 동작

## 핵심 정리

- system_server 의 Java Service 도 결국 `ServiceManager.addService` → servicemanager 데몬. 오늘 만든 C++ 서비스와 **같은 테이블**에 들어간다.
- `Binder`(Java) ↔ `BBinder`(C++) ↔ `AIBinder`(NDK) 는 같은 Driver 객체의 세 언어 표면이다.
- `dumpsys` 는 Binder `dump()` Transaction. 자기 Service 에 `dump()` 를 구현해 두면 디버깅이 편해진다.
- `mAccess->canAdd()` — servicemanager 가 SELinux 로 등록 권한을 검사하는 지점 (실습 9).
