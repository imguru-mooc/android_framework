#ifndef ANDROID_ILED_SERVICE_H
#define ANDROID_ILED_SERVICE_H

#include <binder/IInterface.h>
#include <binder/IPermissionController.h>
#include <utils/Vector.h>
#include <utils/String16.h>

namespace android {

// ----------------------------------------------------------------------

class ILedService : public IInterface {
public:
    DECLARE_META_INTERFACE(LedService);
    virtual void   ledOn( int ) = 0;
    enum {
        LED_ON = IBinder::FIRST_CALL_TRANSACTION,
    };
};


// ----------------------------------------------------------------------

class BnLedService : public BnInterface<ILedService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

// -----------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_ILED_SERVICE_H

