#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <grp.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include "ILedService.h"
#include "LedService.h"

using namespace android;

int main()
{
	LedService::instantiate();
    IPCThreadState::self()->joinThreadPool();
}




