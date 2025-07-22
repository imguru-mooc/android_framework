
#include <jni.h>
#include <iostream>

int main() {
    // 1. JVM 포인터 및 JNI 환경 포인터 선언
    JavaVM* vm = nullptr;
    JNIEnv* env = nullptr;
    JavaVMInitArgs vm_args;
    JavaVMOption options[1];

    // 2. JVM 옵션 설정
    options[0].optionString = (char*)"-Djava.class.path=.";
    vm_args.version = JNI_VERSION_1_8; // 최신 버전을 명시적으로 사용
    vm_args.options = options;
    vm_args.nOptions = 1;
    vm_args.ignoreUnrecognized = JNI_TRUE;

    // 3. JVM 생성
    jint res = JNI_CreateJavaVM(&vm, (void**)&env, &vm_args);
    if (res != JNI_OK) {
        std::cerr << "Failed to create Java VM. Error code: " << res << std::endl;
        return -1;
    }
    std::cout << "JVM created successfully." << std::endl;

    // 4. Java 클래스 찾기
    jclass cls = env->FindClass("JniFuncMain");
    if (cls == nullptr) {
        std::cerr << "ERROR: class not found!" << std::endl;
        if(env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        }
        vm->DestroyJavaVM();
        return -1;
    }

    // 5. 정적(static) 메소드 ID 가져오기
    jmethodID mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");
    if (mid == nullptr) {
        std::cerr << "ERROR: method not found!" << std::endl;
        if(env->ExceptionOccurred()) {
            env->ExceptionDescribe();
        }
        vm->DestroyJavaVM();
        return -1;
    }
    std::cout << "Method ID found: " << mid << std::endl;

    // 6. Java 메소드에 전달할 인자(argument) 생성
    jstring jstr = env->NewStringUTF("Hello from C++ Invocation API!!");
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray args = env->NewObjectArray(1, stringClass, jstr);

    // 7. 정적 메소드 호출
    env->CallStaticVoidMethod(cls, mid, args);

    // 메소드 호출 후 발생할 수 있는 예외 확인
    if (env->ExceptionOccurred()) {
        std::cerr << "Exception occurred during Java method execution." << std::endl;
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    // 8. JVM 소멸
    vm->DestroyJavaVM();
    std::cout << "JVM destroyed." << std::endl;

    return 0;
}
