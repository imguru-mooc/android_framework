#include "jnifunc.h"

JNIEXPORT jobject JNICALL Java_JniFuncMain_createJniObject
  (JNIEnv *env, jclass clazz)
{
	jclass targetClass = 0;
	jobject newObject;
	jmethodID mid;
	targetClass = env->FindClass("JniTest");
	mid = env->GetMethodID( targetClass, "<init>" , "(I)V" );
	newObject = env->NewObject( targetClass, mid, 100);
	return newObject;
}

