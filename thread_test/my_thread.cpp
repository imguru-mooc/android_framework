#if 1
#include <stdio.h>
#include <unistd.h>
#include <utils/Thread.h>

using namespace android;

class MyThread : public Thread
{
	public:
    MyThread()
	{
		printf("MyThread::MyThread()\n");
	}

	void onFirstRef()
	{
		run("my_thread");
		printf("MyThread::onFirstRef()\n");
	}

    status_t readyToRun()
	{
		printf("MyThread::readyToRun()\n");
		return NO_ERROR;
	}
    bool threadLoop()
	{
		printf("MyThread::threadLoop()\n");
		sleep(1);
		return true;
	}
};

int main()
{
	sp<Thread> p = new MyThread;
	p->join();
	printf("parent\n");
	return 0;
}
#endif
