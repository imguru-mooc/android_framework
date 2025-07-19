// IMyDataService.h

#ifndef ANDROID_IMYDATA_SERVICE_H
#define ANDROID_IMYDATA_SERVICE_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include "IMyDataCallback.h"

namespace android {

class IMyDataService : public IInterface {
public:
    DECLARE_META_INTERFACE(MyDataService);

    virtual void registerCallback(const sp<IMyDataCallback>& callback) = 0;
	enum {
        REGISTER_CALLBACK = IBinder::FIRST_CALL_TRANSACTION,
    };
};

class BnMyDataService : public BnInterface<IMyDataService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
                        Parcel* reply, uint32_t flags = 0);
};

} // namespace android
#endif
