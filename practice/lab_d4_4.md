# [D4-4] 실습 N-1 — Native Binder 서비스 (BBinder 직접 구현)

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★★ | Day 4 오전 2교시 · ★전원 필수 | 서버+WinSCP+로컬 · 🎬 ②⑰ 복습 |

> 🎯 **실습 목표** — Java 없이 **C++ libbinder만으로** 서비스를 만들어 `hello.native`로 등록하고 `service call`로 호출한다 — "Binder는 언어가 아니라 커널 메커니즘".

## Step 1. main.cpp 작성 (WinSCP: vendor/edu/native_hello/)

```cpp
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/Log.h>
#include <utils/String16.h>

using namespace android;

class HelloService : public BBinder {                 // ★ Java Stub과 같은 자리
    status_t onTransact(uint32_t code, const Parcel& data,
                        Parcel* reply, uint32_t flags) override {
        switch (code) {
        case 1: {                                     // TRANSACTION code = 1
            String16 name = data.readString16();
            String16 out(String16("hello, ") + name);
            reply->writeString16(out);
            return NO_ERROR;
        }
        default: return BBinder::onTransact(code, data, reply, flags);
        }
    }
};

int main() {
    sp<ProcessState> ps = ProcessState::self();
    defaultServiceManager()->addService(String16("hello.native"),
                                        new HelloService());   // ★ 장부 등록
    ps->startThreadPool();
    IPCThreadState::self()->joinThreadPool();          // ★ 없으면 즉시 종료!
    return 0;
}
```

## Step 2. Android.bp

```text
cc_binary {
    name: "native_hello",
    srcs: ["main.cpp"],
    shared_libs: ["libbinder", "libutils", "liblog"],
}
```

## Step 3. 빌드 → 전송 → 배포

```bash
# (서버) 해당 디렉토리에서
mm
```

🖱 WinSCP **OUT** 북마크 → `system/bin/native_hello` 다운로드 → 로컬에서:

```bat
adb root
adb push native_hello /data/local/tmp/
adb shell chmod +x /data/local/tmp/native_hello
adb shell /data/local/tmp/native_hello &
```

## Step 4. 검증 — 장부 확인 + 수동 transact

```bat
adb shell service list | findstr hello.native
adb shell service call hello.native 1 s16 "world"
```

✅ **예상 결과:**

```text
hello.native: [ ]                       ← 장부 등록!
Result: Parcel(... 'h.e.l.l.o.,. .w.o.r.l.d' ...)   ← reply의 UTF-16 hex
```

⚠️ addService 거부/denied 시 (개발 검증용):

```bat
adb shell setenforce 0
:: 재실행 → 성공 확인 → "정식 해결은 sepolicy(D2-11)" 토론 후
adb shell setenforce 1
```

## Step 5. (선택) client.cpp — Java Proxy가 하던 일을 손으로

```cpp
sp<IBinder> b = defaultServiceManager()->getService(String16("hello.native"));
Parcel data, reply;
data.writeString16(String16("client"));
b->transact(1, data, &reply);
ALOGI("%s", String8(reply.readString16()).c_str());
```

# 🏁 Pass 판정 체크리스트

- [ ] service list에 hello.native 등장
- [ ] service call 응답 Parcel에서 "hello, world" 판독
- [ ] joinThreadPool 주석 처리 → 즉시 종료 재현 후 복구 (역할 체득)
- [ ] SELinux 임시 우회를 썼다면 setenforce 1 복구 + 정식 경로 1문장

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 실행 직후 종료 | joinThreadPool 누락 | Step 1 마지막 두 줄 확인 |
| addService 실패 | 권한/SELinux | root 셸 실행 + setenforce 실험(위) |
| service call 결과가 깨짐 | code/타입 불일치 | code=1, s16 타입 유지 (계약!) |
| String16 연산 에러 | 헤더 누락 | utils/String16.h 포함 |

# 🚗 현업 활용 포인트

💡 **VHAL 데몬이 정확히 이 골격**입니다 — `hardware/interfaces/automotive/vehicle/`의 서비스 등록부와 오늘 코드를 나란히 열어 보세요. 이제 vendor 데몬의 크래시 스택(BBinder::transact...)이 남의 코드가 아닙니다.

---
*실습 D4-4 (33/36) · 다음: **D4-5 N-2 surface_test***
