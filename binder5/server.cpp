// MyDataService.cpp
#include "IMyDataService.h"
#include <binder/BinderService.h>

using namespace android;

class MyDataService : public BnMyDataService {
public:
    void registerCallback(const sp<IMyDataCallback>& callback) {
		printf("MyDataService::registerCallback()\n");
        callback->onDataReceived(String16("Hello from C++ Binder Server"));
    }
};

int main() {
    defaultServiceManager()->addService(String16("my.data.service"), new MyDataService());
    IPCThreadState::self()->joinThreadPool();
    return 0;
}

