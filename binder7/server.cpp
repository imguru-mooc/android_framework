// MyDataService.cpp
#include <BnMyDataService.h>
#include <IMyDataCallback.h>
#include <binder/BinderService.h>

using namespace android;
using android::binder::Status;

class MyDataService : public BnMyDataService {
public:
    Status registerCallback(const sp<IMyDataCallback>& callback) {
		printf("MyDataService::registerCallback()\n");
        callback->onDataReceived(String16("Hello from C++ Binder Server"));
		return Status::ok();
    }
};

int main() {
    defaultServiceManager()->addService(String16("my.data.service"), new MyDataService());
    IPCThreadState::self()->joinThreadPool();
    return 0;
}

