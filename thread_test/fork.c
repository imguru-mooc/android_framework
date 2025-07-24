#if 1
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
int global=6;
void * foo(void* data)
{
	printf("child  : global=%d, local=%d\n", global, ++*(int*)data);
	return 0;
}
int main()
{
	int local=10;
	pthread_t thread;

    pthread_create(&thread, 0, foo, &local);
//	foo(&local);
	pthread_join(thread, 0 );
	printf("parent : global=%d, local=%d\n", global, local);
	return 0;
}
#endif

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int global=6;
int main()
{
	pid_t pid;

	pid = fork();

	if( pid == 0 )
	{
		printf("child  : global=%d\n", ++global);
		exit(0);
	}

	wait(0);
	printf("parent : global=%d\n", global);
	return 0;
}
#endif
