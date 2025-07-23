#define LOG_TAG "LedAuthService"
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <BnLedAuthService.h>

using android::binder::Status;
using android::defaultServiceManager;
using android::sp;
using android::ProcessState;

#define AID_SYSTEM 1000 /* system server */

class LedAuthService : public BnLedAuthService {
public:
    Status LEDON() override {
        pid_t pid = android::IPCThreadState::self()->getCallingPid();
        uid_t uid = android::IPCThreadState::self()->getCallingUid();

        static constexpr uid_t kAllowedUid = AID_SYSTEM;   
        if (uid != kAllowedUid) {
            printf("LEDON denied (uid=%d pid=%d)\n", uid, pid);
            return Status::fromExceptionCode(Status::EX_SECURITY,
                                             "Only system UID may toggle LED");
        }
        printf("LED turned ON by uid=%d pid=%d\n", uid, pid);
        return Status::ok();
    }
};

int main(int /*argc*/, char** /*argv*/) {
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    android::sp<LedAuthService> service =
            android::sp<LedAuthService>::make();
    android::status_t status = sm->addService(android::String16("led.auth"), service);
    printf("LedAuthService registered (status %d)\n", status);

    android::ProcessState::self()->startThreadPool();
    android::IPCThreadState::self()->joinThreadPool();
    return 0;
}

