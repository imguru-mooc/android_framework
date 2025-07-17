#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <utils/Debug.h>
#include <utils/Log.h>
#include <binder/IPCThreadState.h>
#include <binder/Parcel.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <unistd.h>

#undef LOG_TAG
#define LOG_TAG "LedService"
#include "ILedService.h"
namespace android {
// -----------------------------------------------------
class BpLedService : public BpInterface<ILedService>
{
public:
    BpLedService(const sp<IBinder>& impl)
        : BpInterface<ILedService>(impl)
    {
    }

    virtual void ledOn( void ) 
    {
        Parcel data, reply;
        remote()->transact( LED_ON, data, &reply);
    }
};

IMPLEMENT_META_INTERFACE(LedService, "android.my.ILedService");
}; // namespace android
