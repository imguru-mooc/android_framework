#if 1
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static pthread_key_t gTLS = 0;

void foo(const char *func)
{
	int *count = (int*)pthread_getspecific(gTLS);
    if (!count) 
	{
		count = new int;
    	pthread_setspecific(gTLS, count);
	}
	++*count;
	printf("foo() : %s => count=%d\n", func, *count);
}

void * handler(void* data)
{
	foo("handler()");
	foo("handler()");
	foo("handler()");
	return 0;
}

void threadDestructor(void *st)
{
    int* count = (int*)(st);
    delete count;
}

int main()
{
	pthread_t thread;
	pthread_key_create(&gTLS, threadDestructor);
    pthread_create(&thread, 0, handler, 0);
	foo("main()");
	pthread_join(thread, 0 );
	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void foo(const char *func)
{
	static int count=0;
	++count;
	printf("foo() : %s => count=%d\n", func, count);
}

void * handler(void* data)
{
	foo("handler()");
	foo("handler()");
	foo("handler()");
	return 0;
}
int main()
{
	pthread_t thread;
    pthread_create(&thread, 0, handler, 0);
	foo("main()");
	pthread_join(thread, 0 );
	return 0;
}
#endif


