#if 1
#include <stdio.h>
#include <stdlib.h>
#include <utils/Thread.h>

using namespace android;

class MyThread : public Thread
{
	public:
    virtual bool  threadLoop() 
	{
		sleep(1);
		printf("\t\tMyThread::threadLoop()\n");
		return true;
	}
    virtual void   onFirstRef()
	{
		run("MyThread");  
	}
};
//-----------------------------------------------------------

int main()
{
	sp<Thread> thread = new MyThread;

	while(1)
	{
		sleep(1);
		printf("main()\n");
	}
	return 0;
}
#endif

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

class Thread
{
	pthread_t mThread;
	public:
	virtual void handler(void) = 0;
	static void * __handler(void* data)
	{
		Thread *self = (Thread*)data;
		self->handler();
		return 0;
	}
	void run()
	{
		pthread_create(&mThread, 0, __handler, this);
	}
	void join()
	{
		pthread_join(mThread, 0 );
	}
};

class MyThread : public Thread
{
	public:
	void handler(void)
	{
		printf("MyThread\n");
	}
};
//-----------------------------------------------------------

int main()
{
	{
		MyThread thread;
		thread.run();  // pthread_create
		thread.join(); // pthread_join
	}  

	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

class Thread
{
	pthread_t mThread;
	int mData;
	public:
	void handler(void)
	{
		mData=10;
		printf("child\n");
	}
	static void * __handler(void* data)
	{
		Thread *self = (Thread*)data;
		self->handler();
		return 0;
	}
	void run()
	{
		pthread_create(&mThread, 0, __handler, this);
	}
	void join()
	{
		pthread_join(mThread, 0 );
	}
};

//-----------------------------------------------------------

int main()
{
	{
		Thread thread;
		thread.run();  // pthread_create
		thread.join(); // pthread_join
	}  

	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void * foo(void* data)
{
	printf("child\n");
	return 0;
}

class Thread
{
	pthread_t mThread;
	public:
		void run()
		{
    		pthread_create(&mThread, 0, foo, 0);
		}
		void join()
		{
			pthread_join(mThread, 0 );
		}
};

//-----------------------------------------------------------

int main()
{
	{
		Thread thread;
		thread.run();  // pthread_create
		thread.join(); // pthread_join
	}  

	return 0;
}
#endif
