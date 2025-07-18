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
	int ratio;
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
	sp<IBinder> p = sm->checkService( String16("my.led_service") );
	sp<ILedService> pLed = interface_cast<ILedService>(p);
	while(1)
	{
		scanf("%d", &ratio);
		pLed->ledOn(ratio);
	}
}




