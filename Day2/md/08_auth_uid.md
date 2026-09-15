# 실습 8. UID 기반 접근 제어 — getCallingUid · EX_SECURITY · su 1000

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 2 AIDL Native · 보안
> **디렉토리:** `~/aosp/day2/auth`

## 목표

- `IPCThreadState::getCallingUid()` 로 호출자를 확인하고 system UID(1000) 만 허용한다.
- 거부 시 `Status::EX_SECURITY` 를 반환해 Client 에 예외로 전달한다.
- 같은 바이너리를 `uid 0`(root) 와 `uid 1000`(system) 으로 실행해 결과 차이를 본다.

---

## Step 1. AIDL

**`auth/ILedAuthService.aidl`**

```java
interface ILedAuthService {
    void LEDON();
}
```

## Step 2. Service

**`auth/LedAuthService.cpp`**

```cpp
#define LOG_TAG "LedAuthService"
#include <cstdio>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <BnLedAuthService.h>

using android::binder::Status;
using android::defaultServiceManager;
using android::sp;
using android::ProcessState;
using android::IPCThreadState;

#define AID_SYSTEM 1000   /* system_server / system 계정 */

class LedAuthService : public BnLedAuthService {
public:
    Status LEDON() override {
        pid_t pid = IPCThreadState::self()->getCallingPid();     // ★ Driver 가 붙여 준 값
        uid_t uid = IPCThreadState::self()->getCallingUid();

        static constexpr uid_t kAllowedUid = AID_SYSTEM;
        if (uid != kAllowedUid) {
            printf("LEDON denied  (uid=%d pid=%d)\n", uid, pid); fflush(stdout);
            return Status::fromExceptionCode(Status::EX_SECURITY, "Only system UID may toggle LED");
        }
        printf("LED turned ON by uid=%d pid=%d\n", uid, pid); fflush(stdout);
        return Status::ok();
    }
};

int main() {
    sp<android::IServiceManager> sm = defaultServiceManager();
    sp<LedAuthService> service = sp<LedAuthService>::make();
    android::status_t st = sm->addService(android::String16("led.auth"), service);
    printf("LedAuthService registered (status %d)\n", st); fflush(stdout);

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

## Step 3. Client

**`auth/LedAuthClient.cpp`**

```cpp
#include <cstdio>
#include <unistd.h>
#include <binder/IServiceManager.h>
#include <ILedAuthService.h>

int main() {
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    android::sp<ILedAuthService> svc =
        android::interface_cast<ILedAuthService>(sm->checkService(android::String16("led.auth")));
    if (svc == nullptr) { printf("Service not found\n"); return 1; }

    printf("client uid=%d calling LEDON()\n", getuid());
    android::binder::Status st = svc->LEDON();
    if (!st.isOk()) {
        printf("LEDON() failed: %s (exception=%d)\n", st.toString8().c_str(), st.exceptionCode());
        return 1;
    }
    printf("LEDON() succeeded\n");
    return 0;
}
```

## Step 4. 빌드 · 배포

```bash
cd ~/aosp/day2 && mm -j$(nproc)
```

```bat
adb push led_auth_service /data & adb push led_auth_client /data
adb shell chmod 755 /data/led_auth_service /data/led_auth_client
```

## Step 5. 실행 — UID 를 바꿔 가며

터미널 ① 서버:

```bash
adb shell /data/led_auth_service
```

터미널 ②:

```bash
adb shell
```

```bash
# (1) root (uid 0) 로 호출 → 거부
/data/led_auth_client
# client uid=0 calling LEDON()
# LEDON() failed: Status(-8, EX_SECURITY): 'Only system UID may toggle LED' (exception=-1)

# (2) system (uid 1000) 으로 호출 → 허용
su 1000 /data/led_auth_client
# client uid=1000 calling LEDON()
# LEDON() succeeded

# (3) 일반 App 계정(uid 10050 등) 으로 → 거부
su 10050 /data/led_auth_client
```

서버 로그:

```text
LEDON denied  (uid=0 pid=6201)
LED turned ON by uid=1000 pid=6215
LEDON denied  (uid=10050 pid=6230)
```

## Step 6. 위조 시도

Client 코드에서 uid 를 속일 방법이 있는가? `setuid(1000)` 은 root 에서만 되고, 그 경우 실제로 uid 가 1000 이 된다. Parcel 에 "나는 1000" 이라고 써 넣어도 서버는 그 값을 읽지 않는다. `getCallingUid()` 는 **커널 Binder Driver 가 송신 Process 의 cred 에서 직접 채우는 값**이라 사용자 공간에서 조작할 수 없다.

---

## 확인 포인트

- [ ] uid 0 → `EX_SECURITY`, uid 1000 → 성공, uid 10050 → `EX_SECURITY`
- [ ] 서버 로그의 `uid/pid` 가 Client 의 실제 값과 일치
- [ ] `st.exceptionCode()` 가 `EX_SECURITY`(-1)

## 핵심 정리

| 항목 | 내용 |
|---|---|
| `getCallingUid()` | 커널이 보증. Stub Method(Binder Thread) 안에서만 유효 |
| `Status::EX_SECURITY` | Java 의 `SecurityException` 으로 변환되어 App 에 전달 |
| Android 권한 모델 | `checkPermission()` 도 결국 이 UID 로 패키지 권한을 조회한다 |
| `su <uid>` | userdebug 의 `su` 는 임의 uid 로 실행 가능 — 테스트 전용 |

Day 1 실습 7 의 Java `Binder.getCallingUid()` 와 같은 값이다. Java 든 C++ 이든 Driver 는 하나다.
