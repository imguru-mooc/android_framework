# Day 2 Worksheet

> **형식:** 객관식 8 · 단답형 5 · 빈칸 채우기 4 · 순번 지정 2 · 선잇기 1 (총 20문제) · **소요시간:** 25분
> **범위:** libbinder · 수작업 Bn/Bp · Parcel · AIDL cpp/ndk · UID 접근 제어 · init.rc · SELinux · AIDL HAL · VINTF

이름: ______________________

---

## 객관식 (Q1 ~ Q8)

**Q1.** Native Binder 에서 Process 당 하나만 존재하며 `/dev/binder` 를 열고 1 MB 를 mmap 하는 클래스는?

① IPCThreadState
② ProcessState
③ BBinder
④ IServiceManager

**Q2.** Server 측에서 요청을 받아 code 로 분기하는 `onTransact()` 를 가진 기본 클래스는?

① BpBinder
② BBinder
③ BpInterface
④ IInterface

**Q3.** `interface_cast&lt;ILedService&gt;(binder)` 가 내부적으로 호출하는 것은?

① ILedService::asInterface(binder)
② new BnLedService(binder)
③ binder->transact(0)
④ defaultServiceManager()->getService()

**Q4.** AIDL Method 의 첫 번째 Transaction code 값(`FIRST_CALL_TRANSACTION`)은?

① 0
② 1
③ 0x5f444d50
④ 100

**Q5.** Callback 처럼 IBinder 를 Parcel 에 실어 보낼 때 Proxy 가 사용하는 Parcel API 는?

① writeInt32
② writeString16
③ writeStrongBinder
④ writeFileDescriptor

**Q6.** vendor 파티션의 AIDL HAL 프로세스가 링크할 수 있는 Binder 라이브러리는?

① libbinder
② libbinder_ndk
③ libhwbinder
④ libutils

**Q7.** SELinux 가 permissive 인 상태에서 정책 위반이 발생하면?

① 즉시 차단되고 프로세스가 종료된다
② avc: denied 로그만 남고 동작은 허용된다
③ 아무 로그 없이 허용된다
④ 커널 패닉이 발생한다

**Q8.** init.rc 의 `service` 로 등록한 데몬을 시작하는 명령은?

① systemctl start <name>
② start <name>
③ adb shell run <name>
④ exec /system/bin/<name>

---

## 단답형 (Q9 ~ Q13)

**Q9.** Native Binder Server 의 main() 마지막 줄에서 현재 Thread 를 요청 대기 루프로 만드는 `IPCThreadState` 의 Method 이름은?

답: ______________________

**Q10.** Stub Method 안에서 호출자의 UID 를 얻는 `IPCThreadState` 의 Method 이름은?

답: ______________________

**Q11.** system_server 와 system 계정이 사용하는 UID 값(`AID_SYSTEM`)은?

답: ______________________

**Q12.** Soong 에서 `.aidl` 파일로 cpp/ndk/java backend 라이브러리를 생성하는 모듈 타입 이름은?

답: ______________________

**Q13.** Binder Service 이름과 handle 을 관리하며 `addService` 시 SELinux 검사(`canAdd`)를 수행하는 Native 데몬의 이름은?

답: ______________________

---

## 빈칸 채우기 (Q14 ~ Q17)

**Q14.** 수작업 인터페이스에서 헤더의 ( 1 ) 매크로가 asInterface/descriptor 를 선언하고, .cpp 의 ( 2 ) 매크로가 그 본문과 descriptor 문자열을 정의한다.

(1) __________  (2) __________

**Q15.** Proxy 는 Parcel 맨 앞에 ( 1 ) 로 인터페이스 토큰을 쓰고, Stub 은 ( 2 ) 매크로로 그 토큰을 검증한 뒤 인자를 읽는다.

(1) __________  (2) __________

**Q16.** AIDL cpp backend 가 생성한 Method 는 예외를 ( 1 ) 타입으로 반환하고, 반환값은 ( 2 ) 이름의 out 포인터 인자로 전달한다.

(1) __________  (2) __________

**Q17.** AIDL HAL 인스턴스 이름은 ( 1 ).( 2 )/default 형식이다. 예: android.hardware.light.ILights/default 에서 [1]=android.hardware.light, [2]=ILights.

(1) __________  (2) __________

---

## 순번 지정 (Q18 ~ Q19)

**Q18.** Client 의 <code>led->ledOn(70)</code> 호출이 처리되는 순서대로 번호를 매기시오.

- ( &nbsp; ) BnLedService::onTransact 가 readInt32 로 인자를 읽고 ledOn() 호출
- ( &nbsp; ) BpLedService::ledOn 이 Parcel 에 토큰과 70 을 쓰고 transact()
- ( &nbsp; ) Driver 가 서버의 Binder Thread 를 깨운다
- ( &nbsp; ) Proxy 가 reply.readInt32() 로 반환값을 돌려준다
- ( &nbsp; ) IPCThreadState::transact 가 ioctl(BINDER_WRITE_READ) 호출

**Q19.** init.rc 로 등록한 데몬이 <code>start</code> 명령으로 실행되어 Service 를 등록하기까지의 순서를 매기시오.

- ( &nbsp; ) servicemanager 가 SELinux canAdd 검사 후 테이블에 등록
- ( &nbsp; ) init 이 .rc 정의를 찾아 fork → setuid(user) → exec
- ( &nbsp; ) 데몬이 defaultServiceManager()->addService() 호출
- ( &nbsp; ) start <name> (setprop ctl.start) 실행
- ( &nbsp; ) exec 시 SELinux 도메인 전환 (전용 .te 없으면 init 도메인 유지)

---

## 선잇기 (Q20)

**Q20.** C++ Native Binder 클래스와 Day 1 Java 클래스를 알맞게 연결하시오.

| 구성 요소 | | 역할 |
|---|---|---|
| BBinder | · &nbsp;&nbsp;&nbsp; · | Stub.Proxy |
| BpBinder | · &nbsp;&nbsp;&nbsp; · | I.Stub.asInterface() |
| BnInterface<I> | · &nbsp;&nbsp;&nbsp; · | Binder |
| BpInterface<I> | · &nbsp;&nbsp;&nbsp; · | BinderProxy |
| interface_cast<I> | · &nbsp;&nbsp;&nbsp; · | I.Stub |

---
---

# Day 2 Worksheet — 정답 및 해설

**1. ②**

ProcessState::self() 가 /dev/binder open, mmap, Thread Pool 관리를 담당한다. IPCThreadState 는 Thread 마다 하나로 transact/joinThreadPool 을 맡는다.

**2. ②**

BBinder 가 로컬 Binder 객체의 기본 클래스이며 onTransact 를 오버라이드해 요청을 처리한다. BpBinder 는 원격 handle 을 감싼 Proxy 다.

**3. ①**

interface_cast 는 I::asInterface() 를 호출한다. asInterface 는 queryLocalInterface 로 로컬이면 객체 자체를, 원격이면 new BpX(binder) 를 반환한다.

**4. ②**

IBinder::FIRST_CALL_TRANSACTION = 1 이다. 0x5f444d50 은 DUMP_TRANSACTION 등 예약된 특수 code 의 형태다.

**5. ③**

writeStrongBinder 로 실린 IBinder 는 Driver 를 지나며 송신 측 포인터가 수신 측 handle 로 변환된다.

**6. ②**

vendor 는 system 전용 libbinder 를 쓸 수 없고 안정 ABI 인 libbinder_ndk(NDK backend) 만 사용한다.

**7. ②**

permissive 는 기록만 하고 허용한다(로그에 permissive=1). enforcing 으로 바꾸면 같은 위반이 실제로 차단된다.

**8. ②**

start <name> (= setprop ctl.start <name>) 으로 init 에게 요청한다. 상태는 getprop init.svc.<name> 으로 확인한다.

**9. joinThreadPool()**

IPCThreadState::self()->joinThreadPool() 은 반환하지 않고 Binder 요청을 처리한다. startThreadPool() 은 추가 Thread 를 만드는 것으로 별개다.

**10. getCallingUid()**

Driver 가 Transaction 에 붙인 sender_euid 를 반환한다. 커널 cred 기반이라 사용자 공간에서 위조할 수 없다.

**11. 1000**

AID_SYSTEM = 1000. root 는 0, 일반 App 은 10000 번대(AID_APP_START) 부터 하나씩 받는다.

**12. aidl_interface**

aidl_interface { name, srcs, backend { cpp/ndk/java } } 가 <name>-cpp, <name>-ndk 라이브러리를 만든다.

**13. servicemanager**

frameworks/native/cmds/servicemanager. Java 의 ServiceManager.addService 와 C++ 의 defaultServiceManager()->addService 모두 여기(handle 0)에 도착한다.

**14. (1) DECLARE_META_INTERFACE / (2) IMPLEMENT_META_INTERFACE**

DECLARE_META_INTERFACE(X) 는 선언, IMPLEMENT_META_INTERFACE(X, "desc") 는 구현. AIDL 은 이 쌍을 자동 생성한다.

**15. (1) writeInterfaceToken / (2) CHECK_INTERFACE**

토큰이 없거나 다르면 CHECK_INTERFACE 가 PERMISSION_DENIED 를 반환한다. service call 에 s16 "descriptor" 를 붙여야 하는 이유다.

**16. (1) binder::Status / (2) _aidl_return**

Status f(in…, T* _aidl_return). Status::fromExceptionCode(EX_SECURITY) 등이 Parcel 예외 헤더로 전달되어 Java 예외가 된다.

**17. (1) 패키지 / (2) 인터페이스**

<패키지>.<인터페이스>/<인스턴스>. vendor manifest 의 <fqname>ILights/default</fqname> 와 대응한다.

**18. BpLedService::ledOn 이 Parcel 에 토큰과 70 을 쓰고 transact() → IPCThreadState::transact 가 ioctl(BINDER_WRITE_READ) 호출 → Driver 가 서버의 Binder Thread 를 깨운다 → BnLedService::onTransact 가 readInt32 로 인자를 읽고 ledOn() 호출 → Proxy 가 reply.readInt32() 로 반환값을 돌려준다**

Proxy 직렬화 → IPCThreadState ioctl → Driver 가 Binder Thread 깨움 → Stub 역직렬화·호출 → reply 로 반환.

**19. start <name> (setprop ctl.start) 실행 → init 이 .rc 정의를 찾아 fork → setuid(user) → exec → exec 시 SELinux 도메인 전환 (전용 .te 없으면 init 도메인 유지) → 데몬이 defaultServiceManager()->addService() 호출 → servicemanager 가 SELinux canAdd 검사 후 테이블에 등록**

start → init fork/exec → 도메인 전환 → addService → servicemanager canAdd 검사·등록.

**20. BBinder ↔ Binder, BpBinder ↔ BinderProxy, BnInterface<I> ↔ I.Stub, BpInterface<I> ↔ Stub.Proxy, interface_cast<I> ↔ I.Stub.asInterface()**

BBinder↔Binder, BpBinder↔BinderProxy, BnInterface↔Stub, BpInterface↔Stub.Proxy, interface_cast↔asInterface. 같은 Driver 객체의 두 언어 표면이다.
