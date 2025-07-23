#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>

using namespace android;

int main()
{
	sp<IMemoryHeap> heap = new MemoryHeapBase(4096);
	sp<IMemory>  base = new MemoryBase(heap, 2048, 1024);
	defaultServiceManager()->addService(
			 String16("ashmem.service"), 
			 IMemory::asBinder(base));

	char *p = (char*)heap->getBase();
	sprintf(p+2048 , "Hello Client !!\n");
	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();
	return 0;
}

