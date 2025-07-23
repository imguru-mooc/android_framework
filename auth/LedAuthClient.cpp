#include <binder/IServiceManager.h>
#include <ILedAuthService.h>
#include <stdio.h>

int main() {
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    android::sp<ILedAuthService> svc =
        android::interface_cast<ILedAuthService>(sm->checkService(android::String16("led.auth")));

    if (svc == nullptr) {
        printf("Service not found\n");
        return 1;
    }
    android::binder::Status st = svc->LEDON();
    if (!st.isOk()) {
        printf("LEDON() failed: %s\n", st.toString8().c_str());
        return 1;
    }
    printf("LEDON() succeeded\n");
    return 0;
}

