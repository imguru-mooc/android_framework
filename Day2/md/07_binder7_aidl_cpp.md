# 실습 7. AIDL cpp backend 로 전환 — Bn/Bp 자동 생성 · binder::Status

> **소요시간:** 40분 · **난이도:** ★★☆ · **챕터:** Ch 2 AIDL Native
> **디렉토리:** `~/aosp/day2/binder7`

## 목표

- 실습 6 의 수작업 Bn/Bp 6개 파일을 **`.aidl` 2개 + `aidl_interface`** 로 대체한다.
- 생성된 `BnMyDataService.h`, `BpMyDataService.h`, `IMyDataService.h` 를 열어 실습 4~6 에서 손으로 쓴 코드와 비교한다.
- `binder::Status` 로 예외를 전달한다.

---

## Step 1. AIDL 파일

**`binder7/IMyDataCallback.aidl`**

```java
interface IMyDataCallback {
    void onDataReceived(String data);
}
```

**`binder7/IMyDataService.aidl`**

```java
import IMyDataCallback;

interface IMyDataService {
    void registerCallback(IMyDataCallback cb);
    int add(int a, int b);
}
```

> 패키지 선언 없이 최상위에 두었다 (`local_include_dir: "binder7"`). 실무에서는 `android.hardware.x` 같은 패키지를 쓴다.

## Step 2. Android.bp 확인

실습 1 에서 등록한 부분:

```text
aidl_interface {
    name: "my_interface",
    srcs: ["binder7/IMyDataCallback.aidl", "binder7/IMyDataService.aidl"],
    local_include_dir: "binder7",
    backend: { cpp: { enabled: true } },
    unstable: true,                    // VINTF 안정성 검사 제외 (실습용)
}
cc_binary { name: "binder_server_7", ..., static_libs: ["my_interface-cpp"] }
```

`aidl_interface` 하나가 `my_interface-cpp` 라이브러리를 만들고, 그 안에 Bn/Bp/I 헤더와 구현이 들어간다.

## Step 3. Server

**`binder7/server.cpp`**

```cpp
#include <cstdio>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <BnMyDataService.h>           // ★ 자동 생성
#include <IMyDataCallback.h>

using namespace android;
using android::binder::Status;

class MyDataService : public BnMyDataService {
public:
    Status registerCallback(const sp<IMyDataCallback>& cb) override {
        printf("server: registerCallback()\n"); fflush(stdout);
        Status st = cb->onDataReceived(String16("Hello from AIDL cpp backend"));
        printf("server: callback status=%s\n", st.toString8().c_str()); fflush(stdout);
        return Status::ok();
    }
    Status add(int32_t a, int32_t b, int32_t* _aidl_return) override {
        if (a < 0 || b < 0)
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT, "negative not allowed");
        *_aidl_return = a + b;
        return Status::ok();
    }
};

int main() {
    defaultServiceManager()->addService(String16("my.data.service"), new MyDataService());
    printf("my.data.service (AIDL) registered\n"); fflush(stdout);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

## Step 4. Client

**`binder7/client.cpp`**

```cpp
#include <cstdio>
#include <unistd.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include <BnMyDataCallback.h>          // ★ 자동 생성
#include <IMyDataService.h>

using namespace android;
using android::binder::Status;

class MyCallback : public BnMyDataCallback {
public:
    Status onDataReceived(const String16& data) override {
        printf("client: onDataReceived(\"%s\") tid=%d\n", String8(data).c_str(), gettid()); fflush(stdout);
        return Status::ok();
    }
};

int main() {
    sp<ProcessState> proc(ProcessState::self());
    sp<IBinder> b = defaultServiceManager()->checkService(String16("my.data.service"));
    if (b == nullptr) { printf("not found\n"); return 1; }
    sp<IMyDataService> svc = interface_cast<IMyDataService>(b);   // ★ 생성된 asInterface

    int32_t r = 0;
    Status st = svc->add(40, 2, &r);
    printf("client: add(40,2) -> %d, status=%s\n", r, st.toString8().c_str());

    st = svc->add(-1, 2, &r);                                        // ★ 예외 전달
    printf("client: add(-1,2) -> status=%s exception=%d\n", st.toString8().c_str(), st.exceptionCode());

    svc->registerCallback(new MyCallback());
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

## Step 5. 빌드 · 생성 코드 보기

```bash
cd ~/aosp/day2 && mm -j$(nproc)

# ★ 자동 생성된 파일 찾기
find out/soong/.intermediates/day2/my_interface-cpp-source -name "*.h" | head
find out/soong/.intermediates/day2/my_interface-cpp-source -name "*.cpp" | head
```

`IMyDataService.cpp` (생성본) 를 열어 다음을 찾아 본다.

| 실습 4~6 에서 손으로 쓴 것 | 생성 코드에서의 이름 |
|---|---|
| `class BpLedService : BpInterface` | `class BpMyDataService : public ::android::BpInterface<IMyDataService>` |
| `writeInterfaceToken` | `_aidl_data.writeInterfaceToken(getInterfaceDescriptor())` |
| `writeStrongBinder(asBinder(cb))` | `_aidl_data.writeStrongBinder(cb)` |
| `CHECK_INTERFACE` | `if (!(_aidl_data.checkInterface(this))) return BAD_TYPE;` |
| `case LED_ON:` | `case BnMyDataService::TRANSACTION_registerCallback:` |
| `IMPLEMENT_META_INTERFACE` | `DO_NOT_DIRECTLY_USE_ME_IMPLEMENT_META_INTERFACE(MyDataService, "IMyDataService")` |
| 반환값 `reply->writeInt32` | `_aidl_reply->writeInt32(_aidl_return)` + `Status` 헤더 |

## Step 6. 실행

```bash
adb shell /data/binder_server_7 &
adb shell /data/binder_client_7
```

```text
client: add(40,2) -> 42, status=No error
client: add(-1,2) -> status=Status(-8, EX_ILLEGAL_ARGUMENT): 'negative not allowed' exception=-3
server: registerCallback()
client: onDataReceived("Hello from AIDL cpp backend") tid=6120
server: callback status=No error
```

```bash
adb shell service list | grep my.data
# my.data.service: [IMyDataService]
```

---

## 확인 포인트

- [ ] 소스 파일 4개(`.aidl` 2 + `.cpp` 2)로 실습 6 과 같은 동작
- [ ] `out/soong/.intermediates/…/my_interface-cpp-source` 에서 생성된 Bn/Bp 확인
- [ ] `add(-1, 2)` 가 `EX_ILLEGAL_ARGUMENT` Status 로 돌아옴

## 핵심 정리

- `aidl_interface` = AIDL 컴파일 + 라이브러리 패키징. `-cpp`, `-ndk`, `-java` 접미사로 backend 별 라이브러리를 만든다.
- 생성된 Method 시그니처는 항상 `Status f(in..., out* _aidl_return)`. 반환값은 out 포인터, 예외는 `Status`.
- `unstable: true` 는 실습용. 실제 HAL 은 `stability: "vintf"` + `versions` 로 버전을 고정한다 (실습 10).
