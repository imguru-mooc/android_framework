#include "foo.h"
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    jclass clazz = env->FindClass("JniFuncMain");

	JNINativeMethod nm[1];
    nm[0].name      = (char*)"foo";
    nm[0].signature = (char*)"()V";
	nm[0].fnPtr     = (void*)foo;

    env->RegisterNatives(clazz, nm, sizeof(nm) / sizeof(JNINativeMethod)); 

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL foo(JNIEnv *env, jobject object)
{
	printf("Native : foo()\n");
}
