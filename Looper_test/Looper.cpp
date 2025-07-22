#if 1
#include <stdio.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread 
{
	sp<Looper> mLooper;
	sp<MessageHandler> mHandler;
public:
	MyThread( sp<Looper> looper , sp<MessageHandler> handler) 
		: mLooper(looper), mHandler(handler) {}
	bool threadLoop(void)
	{
		mLooper->sendMessageDelayed( 3*1000*1000*1000UL, mHandler, Message(1));
		printf("after\n");
		return false;
	}
};

class MyMessageHandler : public MessageHandler{

public:
    virtual void handleMessage(const Message& message) 
	{
		printf("MyMessageHandler::handleMessage(%d)\n", message.what);
	}
};

//--------------------------------------------------------------

int main()
{
	sp<Looper> looper = new Looper(true);
	sp<MessageHandler> handler = new MyMessageHandler();
	sp<Thread> p = new MyThread( looper, handler );

	p->run("MyThread");

	while(1)
	{
		int result = looper->pollOnce(-1);

		if( result == Looper::POLL_TIMEOUT )
			printf("POLL_TIMEOUT\n");
		if( result == Looper::POLL_WAKE )
			printf("POLL_WAKE\n");
		if( result == Looper::POLL_CALLBACK )
			printf("POLL_CALLBACK\n");
	}

	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread 
{
	sp<Looper> mLooper;
	sp<MessageHandler> mHandler;
public:
	MyThread( sp<Looper> looper , sp<MessageHandler> handler) 
		: mLooper(looper), mHandler(handler) {}
	bool threadLoop(void)
	{
		sleep(3);
		mLooper->sendMessage( mHandler, Message(1));
		mLooper->sendMessage( mHandler, Message(2));
		mLooper->sendMessage( mHandler, Message(3));
		return false;
	}
};

class MyMessageHandler : public MessageHandler{

public:
    virtual void handleMessage(const Message& message) 
	{
		printf("MyMessageHandler::handleMessage(%d)\n", message.what);
	}
};

//--------------------------------------------------------------

int main()
{
	sp<Looper> looper = new Looper(true);
	sp<MessageHandler> handler = new MyMessageHandler();
	sp<Thread> p = new MyThread( looper, handler );

	p->run("MyThread");

	while(1)
	{
		int result = looper->pollOnce(-1);

		if( result == Looper::POLL_TIMEOUT )
			printf("POLL_TIMEOUT\n");
		if( result == Looper::POLL_WAKE )
			printf("POLL_WAKE\n");
		if( result == Looper::POLL_CALLBACK )
			printf("POLL_CALLBACK\n");
	}

	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread 
{
	sp<Looper> mLooper;
	int mFd;
public:
	MyThread( sp<Looper> looper , int fd) : mLooper(looper), mFd(fd) {}
	bool threadLoop(void)
	{
		sleep(3);
		write( mFd, "W", 1 );
		return false;
	}
};

class MyHandler : public RefBase
{
	static int staticHandler(int fd, int events, void* data)
	{
		((MyHandler*)data)->handler(fd, events);
		return 0;
	}
public : 
	virtual int handler(int fd, int events) = 0;
	void setCallback( sp<Looper> &looper , int fd, int events )
	{
		looper->addFd(fd, 0, events, staticHandler, this);
	}
};
class StubMyHandler : public MyHandler
{
public : 
	virtual int handler(int fd, int events)
	{
		printf("StubMyHanlder::handler(%d, %d)\n", fd, events);
		return 0;
	}
};
//--------------------------------------------------------------

int main()
{
	int fd[2];
	pipe(fd);

	sp<Looper> looper = new Looper(true);
	sp<Thread> p = new MyThread(looper, fd[1] );
	sp<MyHandler> handler = new StubMyHandler;
	handler->setCallback(looper, fd[0], Looper::EVENT_INPUT);

	p->run("MyThread");

	while(1)
	{
		int result = looper->pollOnce(-1);

		if( result == Looper::POLL_TIMEOUT )
			printf("POLL_TIMEOUT\n");
		if( result == Looper::POLL_WAKE )
			printf("POLL_WAKE\n");
		if( result == Looper::POLL_CALLBACK )
			printf("POLL_CALLBACK\n");
	}

	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread 
{
	sp<Looper> mLooper;
	int mFd;
public:
	MyThread( sp<Looper> looper , int fd) : mLooper(looper), mFd(fd) {}
	bool threadLoop(void)
	{
		sleep(3);
		write( mFd, "W", 1 );
		return false;
	}
};

class MyHandler : public RefBase
{
	static int staticHandler(int fd, int events, void* data)
	{
		((MyHandler*)data)->handler(fd, events);
		return 0;
	}
public : 
	int handler(int fd, int events)
	{
		printf("MyHanlder::handler(%d, %d)\n", fd, events);
		return 0;
	}
	void setCallback( sp<Looper> &looper , int fd, int events )
	{
		looper->addFd(fd, 0, events, staticHandler, this);
	}
};
//--------------------------------------------------------------

int main()
{
	int fd[2];
	pipe(fd);

	sp<Looper> looper = new Looper(true);
	sp<Thread> p = new MyThread(looper, fd[1] );
	sp<MyHandler> handler = new MyHandler;
	handler->setCallback(looper, fd[0], Looper::EVENT_INPUT);

	p->run("MyThread");

	while(1)
	{
		int result = looper->pollOnce(-1);

		if( result == Looper::POLL_TIMEOUT )
			printf("POLL_TIMEOUT\n");
		if( result == Looper::POLL_WAKE )
			printf("POLL_WAKE\n");
		if( result == Looper::POLL_CALLBACK )
			printf("POLL_CALLBACK\n");
	}

	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread 
{
	sp<Looper> mLooper;
	int mFd;
public:
	MyThread( sp<Looper> looper , int fd) : mLooper(looper), mFd(fd) {}
	bool threadLoop(void)
	{
		sleep(3);
		write( mFd, "W", 1 );
		return false;
	}
};

int foo(int fd, int events, void* data)
{
	printf("foo(%d, %d, %p)\n", fd, events, data);
	return 0;
}

int main()
{
	int fd[2];
	pipe(fd);

	sp<Looper> looper = new Looper(true);
	sp<Thread> p = new MyThread(looper, fd[1] );

	looper->addFd(fd[0], 0, Looper::EVENT_INPUT, foo, (void*)0x1234);

	p->run("MyThread");

	while(1)
	{
		int result = looper->pollOnce(-1);

		if( result == Looper::POLL_TIMEOUT )
			printf("POLL_TIMEOUT\n");
		if( result == Looper::POLL_WAKE )
			printf("POLL_WAKE\n");
		if( result == Looper::POLL_CALLBACK )
			printf("POLL_CALLBACK\n");
	}

	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread 
{
	sp<Looper> mLooper;
public:
	MyThread( sp<Looper> looper ) : mLooper(looper) {}
	bool threadLoop(void)
	{
		sleep(3);
		mLooper->wake();
		return false;
	}
};

int main()
{
	sp<Looper> looper = new Looper(true);
	sp<Thread> p = new MyThread(looper);

	p->run("MyThread");

	int result = looper->pollOnce(-1);

	if( result == Looper::POLL_TIMEOUT )
		printf("POLL_TIMEOUT\n");
	if( result == Looper::POLL_WAKE )
		printf("POLL_WAKE\n");

	return 0;
}
#endif

#if 0
#include <stdio.h>
#include <utils/Looper.h>

using namespace android;

int main()
{
	sp<Looper> p = new Looper(true);

	int result = p->pollOnce(1000);

	if( result == Looper::POLL_TIMEOUT )
		printf("POLL_TIMEOUT\n");

	return 0;
}
#endif
