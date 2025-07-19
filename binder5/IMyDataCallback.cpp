// IMyDataCallback.cpp

#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

// IMyDataCallback.cpp

#include "IMyDataCallback.h"
#include <binder/Parcel.h>
#include <utils/Log.h>

namespace android {

// Proxy (Client → Server)
class BpMyDataCallback : public BpInterface<IMyDataCallback> {
public:
    explicit BpMyDataCallback(const sp<IBinder>& impl)
        : BpInterface<IMyDataCallback>(impl) {}

    void onDataReceived(const String16& data) override {
        Parcel dataParcel, reply;
        dataParcel.writeInterfaceToken(IMyDataCallback::getInterfaceDescriptor());
        dataParcel.writeString16(data);
        remote()->transact(ON_DATA_RECEIVED, dataParcel, &reply);
    }
};

// Stub (Server)
status_t BnMyDataCallback::onTransact(uint32_t code, const Parcel& data,
                                      Parcel* reply, uint32_t flags) {
    switch (code) {
        case ON_DATA_RECEIVED: {
            CHECK_INTERFACE(IMyDataCallback, data, reply);
            String16 msg = data.readString16();
            onDataReceived(msg);
            return NO_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// 메타 구현
IMPLEMENT_META_INTERFACE(MyDataCallback, "com.example.IMyDataCallback");

} // namespace android


