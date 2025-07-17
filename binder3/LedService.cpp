#include <stdio.h> 
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>
#include <utils/threads.h>

#include <cutils/properties.h>

#include "LedService.h"

namespace android 
{
	void LedService::ledOn(void)
	{
		printf("LedService::ledOn()\n");
	}
};
