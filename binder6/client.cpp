// client.cpp
#include <BnMyDataCallback.h>
#include <IMyDataService.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <stdio.h>

using namespace android;
using android::binder::Status;

class MyCallback : public BnMyDataCallback {
public:
    Status onDataReceived(const String16& data) override {
        printf("Client received: %s\n", String8(data).string());
		return Status::ok();
    }
};

int main() {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->checkService(String16("my.data.service"));
    sp<IMyDataService> service = interface_cast<IMyDataService>(binder);

    sp<MyCallback> callback = new MyCallback();
    service->registerCallback(callback);

    // Keep client alive to wait for callback
    sleep(2);
    return 0;
}

