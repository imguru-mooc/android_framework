#include <stdio.h>
#include "Hello.h"

JNIEXPORT void JNICALL Java_Hello_foo (JNIEnv *env, jobject object)
{
	printf("NATIVE : Java_Hello_foo()\n");
}
