#include <sys/types.h>
#include <unistd.h>
#include <grp.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <utils/Debug.h>
#include <binder/Parcel.h>
#include <binder/BpBinder.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>

#include "ILedService.h"

using namespace android;

int main()
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
	sp<IBinder> p = sm->getService( String16("led.service") );
	sp<ILedService> pLed = interface_cast<ILedService>(p);
	pLed->ledOn();
}




