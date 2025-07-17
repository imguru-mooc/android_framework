#ifndef ANDROID_LED_SERVICE_H
#define ANDROID_LED_SERVICE_H

#include <stdint.h>
#include <sys/types.h>
#include <limits.h>

#include <utils/Atomic.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include <utils/SortedVector.h>
#include <utils/Vector.h>

#include <binder/BinderService.h>
#include <binder/MemoryDealer.h>

#include "ILedService.h"

namespace android {
// -------------------------------------------------------------------

class LedService : 
    public BinderService<LedService>,
	public BnLedService
{
    friend class BinderService<LedService>;
public:
    static char const* getServiceName() { return "my.led_service"; }
    virtual void ledOn( void );
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_AUDIO_FLINGER_H
