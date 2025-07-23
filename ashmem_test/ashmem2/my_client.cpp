#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

using namespace android;

int main()
{
	sp<IBinder> binder;
	binder = defaultServiceManager()->checkService(String16("ashmem.service"));

	sp<IMemory> base = interface_cast<IMemory>(binder);
	ssize_t offset;
	size_t size;
	sp<IMemoryHeap> heap = base->getMemory(&offset, &size);
	char *p = (char*)heap->getBase();
	printf("%s\n", p+offset );
	return 0;
}

