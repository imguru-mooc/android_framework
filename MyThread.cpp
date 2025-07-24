#include <utils/Thread.h>
#include <stdio.h>
#include <unistd.h>

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
