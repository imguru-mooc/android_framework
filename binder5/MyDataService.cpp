// MyDataService.cpp
#include "IMyDataService.h"
#include <binder/BinderService.h>
#include <thread>
#include <chrono>

using namespace android;

class MyDataService : public BnMyDataService {
public:
    void requestData(const sp<IMyDataCallback>& callback) override {
        std::thread([callback]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            callback->onDataReceived(String16("Hello from C++ Binder Server"));
        }).detach();
    }

    static void publishAndJoin() {
        defaultServiceManager()->addService(String16("my.data.service"), new MyDataService());
        ProcessState::self()->startThreadPool();
        IPCThreadState::self()->joinThreadPool();
    }
};

int main() {
    MyDataService::publishAndJoin();
    return 0;
}

