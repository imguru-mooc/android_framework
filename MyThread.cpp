#include <utils/Thread.h>
#include <stdio.h>
#include <unistd.h>

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
            printf("MyThread::onFirstRef()\n");
            run("MyThread");
        }
        status_t readyToRun()
        {
            printf("MyThread::readyToRun()\n");
            return 0;
        }
        bool threadLoop()
        {
            printf("MyThread::threadLoop()\n");
            sleep(1);
            return true;
        }
};

//---------------------------------------------------------------

int main()
{
    sp<Thread> p = new MyThread;
    p->join();
    return 0;
}
