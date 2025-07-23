#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>

using namespace android;

int main()
{
	sp<IMemoryHeap> heap = new MemoryHeapBase(4096);
	defaultServiceManager()->addService(
			 String16("ashmem.service"), 
			 IMemoryHeap::asBinder(heap));

	char *p = (char*)heap->getBase();
	sprintf(p , "Hello Client !!\n");
	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();
	return 0;
}

