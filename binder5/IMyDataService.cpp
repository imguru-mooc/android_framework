// IMyDataService.cpp
#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

// IMyDataService.cpp

#include "IMyDataService.h"
#include <binder/Parcel.h>
#include <utils/Log.h>

namespace android {

// Proxy (Client → Server)
class BpMyDataService : public BpInterface<IMyDataService> {
public:
    explicit BpMyDataService(const sp<IBinder>& impl)
        : BpInterface<IMyDataService>(impl) {}

    void registerCallback(const sp<IMyDataCallback>& callback){
        Parcel data, reply;
        data.writeInterfaceToken(IMyDataService::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(callback));
        remote()->transact(REGISTER_CALLBACK, data, &reply);
    }
};

// 메타 구현
IMPLEMENT_META_INTERFACE(MyDataService, "android.my.IMyDataService");

// Stub (Server)
status_t BnMyDataService::onTransact(uint32_t code, const Parcel& data,
                                     Parcel* reply, uint32_t flags) {
    switch (code) {
        case REGISTER_CALLBACK: {
            CHECK_INTERFACE(IMyDataService, data, reply);
            sp<IBinder> binder = data.readStrongBinder();
            sp<IMyDataCallback> cb = interface_cast<IMyDataCallback>(binder);
            registerCallback(cb);
            return NO_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}


} // namespace android


