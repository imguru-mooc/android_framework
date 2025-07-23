#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

using namespace android;

int main()
{
	sp<IBinder> binder;
	binder = defaultServiceManager()->checkService(String16("ashmem.service"));

	sp<IMemoryHeap> heap = interface_cast<IMemoryHeap>(binder);
	char* p = (char*)heap->getBase();
	printf("%s\n", p );
	return 0;
}

