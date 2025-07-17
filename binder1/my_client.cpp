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

using namespace android;

int main()
{
	sp<ProcessState> proc(ProcessState::self());
	sp<IServiceManager> sm = defaultServiceManager();
	sp<IBinder> p = sm->checkService( String16("my.service") );
	Parcel msg, reply;
	std::optional<int32_t> handle = ((BpBinder*)p.get())->getDebugBinderHandle();
	if (handle.has_value()) {
		printf("mHandle=%d\n", handle.value());
	} else {
		printf("mHandle=none\n");
	}
	p->transact( handle.value(), msg , &reply );
}




