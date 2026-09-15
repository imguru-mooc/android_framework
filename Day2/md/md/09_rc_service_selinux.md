# 실습 9. init.rc 로 Service 자동 시작 + SELinux 기초

> **소요시간:** 40분 · **난이도:** ★★★ · **챕터:** Ch 3 System Service · SELinux
> **디렉토리:** `~/aosp/day2/rc_service`, `system/sepolicy` 읽기

## 목표

- Native Service 를 `/data` 에서 손으로 띄우지 않고 **init 이 부팅 시 자동 실행**하도록 `.rc` 를 등록한다.
- `init_rc` 속성 → `/system/etc/init/*.rc` 설치 → `start/stop` 명령을 익힌다.
- SELinux `enforcing` 으로 바꿨을 때 어떤 `avc: denied` 가 나는지 읽고, `.te` 정책이 무엇을 허용해야 하는지 이해한다.

---

## Step 1. Service 소스

**`rc_service/rc_led_service.cpp`**

```cpp
#define LOG_TAG "rc_led_service"
#include <unistd.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <log/log.h>

using namespace android;

class RcLedService : public BBinder {
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) override {
        (void)data; (void)flags;
        ALOGI("onTransact code=%u from uid=%d", code, IPCThreadState::self()->getCallingUid());
        if (code == 1) { reply->writeInt32(1); return NO_ERROR; }
        return BBinder::onTransact(code, data, reply, flags);
    }
};

int main() {
    ALOGI("starting, uid=%d pid=%d", getuid(), getpid());   // ★ stdout 이 아니라 logcat
    status_t st = defaultServiceManager()->addService(String16("rc.led"), new RcLedService);
    ALOGI("addService = %d", st);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

init 이 띄우는 데몬은 터미널이 없으므로 `printf` 대신 `ALOGI` 를 쓴다.

## Step 2. rc 파일

**`rc_service/rc_led_service.rc`**

```text
service rc_led_service /system/bin/rc_led_service
    class main
    user system
    group system
    disabled            # 부팅 시 자동 시작 안 함 — start 명령으로만 (실습용)
```

`Android.bp` 의 `init_rc: ["rc_service/rc_led_service.rc"]` (실습 1) 가 이 파일을 `/system/etc/init/` 에 설치한다.

## Step 3. 빌드 · 설치 (`/system` 에 써야 하므로 remount)

```bash
cd ~/aosp/day2 && mm -j$(nproc)
ls $OUT/system/bin/rc_led_service $OUT/system/etc/init/rc_led_service.rc
```

```bat
adb root
adb remount                       REM 최초 1회 "reboot required" 면 adb reboot 후 다시
adb push rc_led_service /system/bin/
adb push rc_led_service.rc /system/etc/init/
adb shell chmod 755 /system/bin/rc_led_service
adb shell chcon u:object_r:system_file:s0 /system/bin/rc_led_service
adb reboot                        REM init 이 새 .rc 를 읽게 한다
```

## Step 4. init 으로 시작·정지

```bash
adb root
adb shell
```

```bash
getprop | grep rc_led            # [init.svc.rc_led_service]: [disabled]
start rc_led_service             # ★ init 에게 시작 요청 (setprop ctl.start 와 동일)
getprop init.svc.rc_led_service  # running
ps -A | grep rc_led              # system  6301  1  rc_led_service  ← PPID 1 = init 의 자식, uid system
logcat -s rc_led_service -d
# I rc_led_service: starting, uid=1000 pid=6301
# I rc_led_service: addService = 0
service list | grep rc.led
service call rc.led 1            # Result: Parcel(00000000 00000001)

stop rc_led_service
getprop init.svc.rc_led_service  # stopped
```

`.rc` 에서 `disabled` 를 지우면 부팅 시 자동 시작된다.

## Step 5. SELinux — enforcing 으로 바꿔 보기

커스텀 이미지는 `androidboot.selinux=permissive` 로 부팅했다. 정책 위반이 **기록만** 되고 차단되지 않는 상태다.

```bash
getenforce                        # Permissive
logcat -b all -d | grep avc | grep rc_led | head
```

permissive 에서도 `avc: denied` 로그는 남는다. 예:

```text
avc: denied { call } for pid=6301 comm="rc_led_service" scontext=u:r:init:s0 tcontext=u:r:servicemanager:s0 tclass=binder permissive=1
avc: denied { add } for pid=6301 comm="rc_led_service" name="rc.led" scontext=u:r:init:s0 tcontext=u:object_r:default_android_service:s0 tclass=service_manager permissive=1
```

| 필드 | 의미 |
|---|---|
| `{ call }` / `{ add }` | 거부된 권한 (binder 호출 / 서비스 등록) |
| `scontext=u:r:init:s0` | 주체 도메인 — **우리 데몬이 init 도메인으로 돌고 있다** (전용 도메인이 없어서) |
| `tcontext=…servicemanager` / `default_android_service` | 객체 — servicemanager 와 서비스 이름 라벨 |
| `permissive=1` | 기록만 됨 |

enforcing 으로 바꾸면:

```bash
setenforce 1
stop rc_led_service; start rc_led_service
logcat -s rc_led_service -d | tail -2     # addService = -1  (PERMISSION_DENIED)
setenforce 0                              # 다시 permissive
```

## Step 6. 필요한 정책 읽기 (작성은 데모)

제대로 하려면 전용 도메인을 만들어야 한다. `system/sepolicy/private/` 에 추가하는 최소 정책:

**`rc_led_service.te`**

```text
type rc_led_service, domain;
type rc_led_service_exec, exec_type, file_type, system_file_type;
init_daemon_domain(rc_led_service)          # init 이 exec 할 때 이 도메인으로 전환
binder_use(rc_led_service)                  # servicemanager 와 binder 통신
add_service(rc_led_service, rc_led_service_service)
```

**`service.te`** 에 `type rc_led_service_service, service_manager_type;`
**`service_contexts`** 에 `rc.led  u:object_r:rc_led_service_service:s0`
**`file_contexts`** 에 `/system/bin/rc_led_service  u:object_r:rc_led_service_exec:s0`

정책은 `system/sepolicy` 전체 리빌드 + 이미지 교체가 필요하므로 오늘은 강사 시연으로 대체한다. Day 4 APEX 실습의 `file_contexts` 가 같은 규칙이다.

```bash
# 거부 로그 → 정책 규칙 자동 변환 (참고)
adb logcat -b all -d | grep avc | audit2allow -p $OUT/vendor/etc/selinux/precompiled_sepolicy
```

---

## 확인 포인트

- [ ] `start rc_led_service` 후 `ps` 에서 PPID 1, uid system
- [ ] `logcat -s rc_led_service` 에 `addService = 0`
- [ ] `avc: denied` 로그의 `scontext` 가 `u:r:init:s0` 임을 읽을 수 있다
- [ ] `setenforce 1` 에서 `addService = -1`, `setenforce 0` 복귀

## 핵심 정리

- `init_rc` 속성 → `/system/etc/init/<name>.rc` → `start/stop <name>` 으로 제어. `getprop init.svc.<name>` 이 상태.
- `user/group` 으로 데몬의 UID 를 정한다 → 실습 8 의 UID 검사가 여기서 의미를 갖는다.
- SELinux 는 **도메인(scontext) × 객체(tcontext) × 권한** 삼중 검사. permissive 는 기록, enforcing 은 차단.
- 전용 도메인 없이 init 도메인으로 돌면 거의 모든 것이 거부된다 → `.te` + `service_contexts` + `file_contexts` 세트가 필요.

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `adb remount` 실패 | `adb root` 후 `adb remount` → "reboot required" 면 `adb reboot` 후 재시도 |
| `start` 해도 running 이 안 됨 | `.rc` 가 `/system/etc/init/` 에 있는지, reboot 했는지, `logcat -s init` 확인 |
| `addService = -1` (permissive 인데도) | `adb shell getenforce` 확인. Enforcing 이면 `setenforce 0` |
| logcat 에 아무것도 없음 | `ALOGI` 는 `liblog` 링크 필요 — `Android.bp` 의 `shared_libs` 확인 |
