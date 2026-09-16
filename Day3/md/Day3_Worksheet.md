# Day 3 Worksheet

> **형식:** 객관식 8 · 단답형 5 · 빈칸 채우기 4 · 순번 지정 2 · 선잇기 1 (총 20문제) · **소요시간:** 25분
> **범위:** RefBase · sp/wp · Thread · Looper · Binder Thread vs Looper · JNI · SurfaceFlinger · 공유 메모리 (pipe · ashmem · memfd)

이름: ______________________

---

## 객관식 (Q1 ~ Q8)

**Q1.** Android `sp&lt;T&gt;` 가 참조 카운트를 저장하는 위치는?

① sp 객체 내부의 static 변수
② 별도의 control block
③ T 가 상속한 RefBase 객체 내부 (intrusive)
④ Binder Driver

**Q2.** `wp&lt;T&gt;` 가 가리키는 객체에 접근하려면?

① wp-&gt;method() 로 직접 호출
② promote() 로 sp 를 얻은 뒤 null 검사
③ get() 으로 raw pointer 획득
④ incStrong() 을 직접 호출

**Q3.** `android::Thread::threadLoop()` 이 `true` 를 반환하면?

① Thread 가 종료된다
② threadLoop 이 다시 호출된다 (반복)
③ readyToRun 이 다시 불린다
④ join() 이 반환된다

**Q4.** `Looper::pollOnce()` 가 내부적으로 사용하는 시스템 콜은?

① select
② poll
③ epoll_wait
④ ioctl(BINDER_WRITE_READ)

**Q5.** Binder Thread 에서 Java UI 를 직접 갱신할 수 없는 근본 이유는?

① 권한이 없어서
② Binder Thread 에는 Looper 가 없어 UI Thread 의 메시지 큐에 속하지 않기 때문
③ Binder Thread 는 커널 Thread 라서
④ JNIEnv 가 없어서

**Q6.** JNI 에서 `Java_Hello_foo` 같은 이름 규칙 없이 C 함수를 매핑하는 방법은?

① System.load 에 함수 이름 전달
② JNI_OnLoad 에서 RegisterNatives
③ extern "C" 선언
④ javac -h 로 헤더 생성

**Q7.** SurfaceFlinger 와 App 이 GraphicBuffer 를 공유하는 방식은?

① Parcel 에 픽셀을 복사
② Socket 으로 전송
③ dmabuf FD 를 Binder 로 전달 — 픽셀 복사 없음
④ ashmem 에 매 프레임 memcpy

**Q8.** memfd 에 `F_SEAL_WRITE` 를 걸면 수신 프로세스에서 일어나는 일은?

① 읽기도 불가능해진다
② PROT_WRITE mmap 이 EPERM 으로 실패한다
③ fd 가 자동으로 닫힌다
④ ashmem 으로 변환된다

---

## 단답형 (Q9 ~ Q13)

**Q9.** RefBase 에서 첫 `sp` 가 붙는 순간(mStrong 0→1) 한 번 호출되는 훅 함수 이름은? (실습 4 에서 Thread::run 을 호출한 곳)

답: ______________________

**Q10.** 다른 Thread 에서 `Looper` 를 깨워 `pollOnce` 가 `POLL_WAKE` 를 반환하게 하는 Method 이름은?

답: ______________________

**Q11.** `ProcessState::spawnPooledThread` 가 만드는, `Thread` 를 상속하고 `threadLoop` 에서 `joinThreadPool` 을 부르는 클래스 이름은?

답: ______________________

**Q12.** Native 프로세스가 JVM/ART 를 직접 생성할 때 호출하는 JNI 함수 이름은? (app_process 가 Zygote 를 띄울 때 사용)

답: ______________________

**Q13.** ashmem 또는 memfd 를 mmap 하고 `IMemoryHeap` Binder 객체로 감싸 주는 libbinder 클래스 이름은?

답: ______________________

---

## 빈칸 채우기 (Q14 ~ Q17)

**Q14.** 멤버 함수를 pthread 엔트리로 넘기려면 ( 1 ) 멤버 함수를 만들어 `void*` 인자로 ( 2 ) 를 전달하고 그 안에서 원래 객체를 복원한다.

(1) __________  (2) __________

**Q15.** `Looper` 는 ( 1 ) 로 등록한 fd 의 이벤트 루프와, ( 2 ) 로 넣은 Message 를 `MessageHandler::handleMessage` 로 전달하는 메시지 큐를 한 객체에 가진다.

(1) __________  (2) __________

**Q16.** Java `Binder` 객체는 C++ 에서 ( 1 ) 클래스(BBinder 상속)로 감싸지고, Java `Surface` 의 `long` 필드 ( 2 ) 가 `sp&lt;Surface&gt;` 포인터를 담는다.

(1) __________  (2) __________

**Q17.** SurfaceFlinger 클라이언트는 ( 1 ) 으로 setLayer·setPosition 을 모아 `apply()` 하고, 60 Hz 에서 한 프레임의 예산은 약 ( 2 ) ms 이다.

(1) __________  (2) __________

---

## 순번 지정 (Q18 ~ Q19)

**Q18.** <code>sp&lt;Thread&gt; p = new MyThread; p-&gt;join();</code> 의 생명주기 순서를 매기시오.

- ( &nbsp; ) threadLoop() 반복 (true 반환 동안)
- ( &nbsp; ) onFirstRef() → run("name")
- ( &nbsp; ) 생성자 MyThread()
- ( &nbsp; ) readyToRun() 1회
- ( &nbsp; ) threadLoop() 이 false 반환 → join() 반환

**Q19.** Client 가 그린 픽셀이 화면에 나오기까지 순서를 매기시오.

- ( &nbsp; ) SurfaceFlinger 가 다음 VSync 에 acquire → composite
- ( &nbsp; ) HWC / Display 출력 후 release → FREE
- ( &nbsp; ) ANativeWindow_lock — BufferQueue 에서 dequeue
- ( &nbsp; ) 픽셀 write 후 unlockAndPost — queueBuffer
- ( &nbsp; ) createSurface + Transaction.apply 로 Layer 준비

---

## 선잇기 (Q20)

**Q20.** 공유 메모리 방식과 특징을 알맞게 연결하시오.

| 구성 요소 | | 역할 |
|---|---|---|
| pipe | · &nbsp;&nbsp;&nbsp; · | GPU·디스플레이·코덱이 같은 버퍼 접근 |
| ashmem / MemoryHeapBase | · &nbsp;&nbsp;&nbsp; · | FD 참조를 수신 프로세스 fd 테이블에 dup |
| memfd + F_SEAL | · &nbsp;&nbsp;&nbsp; · | 복사 기반 순차 스트림, 1회 읽기 |
| dmabuf (GraphicBuffer) | · &nbsp;&nbsp;&nbsp; · | 수신 측이 내용·크기를 못 바꾸도록 커널이 보장 |
| Binder writeFileDescriptor | · &nbsp;&nbsp;&nbsp; · | 같은 물리 페이지를 mmap, unpin 으로 회수 가능 |

---
---

# Day 3 Worksheet — 정답 및 해설

**1. ③**

RefBase 가 mStrong/mWeak 를 가지며 sp 는 incStrong/decStrong 만 부른다. shared_ptr 은 control block 을 쓰는 non-intrusive 방식이다.

**2. ②**

wp 에는 -> 가 없다. promote() 는 객체가 살아 있으면 sp 를, 아니면 nullptr 를 반환하므로 반드시 검사한다.

**3. ②**

true = 반복, false = 종료. while(1) 을 직접 쓰지 않고 반환값으로 루프를 제어한다.

**4. ③**

Looper 는 epoll fd 를 만들고 wake 용 eventfd 와 addFd 로 등록한 fd 들을 epoll_wait 로 기다린다.

**5. ②**

Binder Thread 는 joinThreadPool 로 Driver 를 기다리는 루프이고 Main Thread 는 Looper 루프다. Handler.post 로 Looper 큐에 넣어 넘긴다.

**6. ②**

RegisterNatives 는 {Java 이름, 시그니처, 함수 포인터} 테이블을 등록한다. Framework JNI 는 전부 이 방식이다.

**7. ③**

BufferQueue 의 버퍼는 dmabuf 이며 FD 만 Binder 로 넘어간다. 400×400 픽셀은 한 번도 복사되지 않는다.

**8. ②**

seal 은 커널이 보장한다. 받는 쪽이 내용·크기를 바꿀 수 없으므로 신뢰 경계를 넘는 버퍼 공유에 쓴다.

**9. onFirstRef()**

생성자 안에서는 sp<this> 를 만들 수 없으므로 onFirstRef 에서 run() 을 부른다.

**10. wake()**

wake() 는 eventfd 에 1 을 write 한다. Java Handler.post 의 nativeWake 가 이것이다.

**11. PoolThread**

class PoolThread : public Thread — Binder Thread 의 정체가 android::Thread 임을 보여 준다.

**12. JNI_CreateJavaVM**

Invocation API. AndroidRuntime::startVm 이 이것을 부른 뒤 startReg 로 Framework JNI 를 등록한다.

**13. MemoryHeapBase**

sp<IMemoryHeap> heap = new MemoryHeapBase(size). Client 는 interface_cast<IMemoryHeap> 으로 받으면 FD 수신 + mmap 이 자동으로 된다.

**14. (1) static / (2) this**

pthread 는 C 함수 포인터만 받는다. static 트램폴린 + this 전달은 Looper 콜백·JNI·PoolThread 에도 반복되는 패턴이다.

**15. (1) addFd / (2) sendMessage / sendMessageDelayed**

③④ 가 fd 이벤트 루프(VSync fd 가 여기로), ⑤ 가 메시지 큐(Java Handler 의 실체).

**16. (1) JavaBBinder / (2) mNativeObject**

JavaBBinder::onTransact 가 Java execTransact 를 역호출한다. "Java 객체 안의 long 하나가 C++ 객체" 가 Framework JNI 의 공통 패턴이다.

**17. (1) Transaction / (2) 16.67**

Transaction 은 한 VSync 에 원자 적용된다. handleEvent 가 16.67 ms 를 넘기면 다음 프레임을 놓쳐 Jank 가 된다.

**18. 생성자 MyThread() → onFirstRef() → run("name") → readyToRun() 1회 → threadLoop() 반복 (true 반환 동안) → threadLoop() 이 false 반환 → join() 반환**

생성자 → onFirstRef(sp 에 담기는 순간) → run → readyToRun → threadLoop 반복 → false → join 반환.

**19. createSurface + Transaction.apply 로 Layer 준비 → ANativeWindow_lock — BufferQueue 에서 dequeue → 픽셀 write 후 unlockAndPost — queueBuffer → SurfaceFlinger 가 다음 VSync 에 acquire → composite → HWC / Display 출력 후 release → FREE**

Layer 준비 → dequeue → 그리기·queue → SF acquire·합성 → HWC 출력·release.

**20. pipe ↔ 복사 기반 순차 스트림, 1회 읽기, ashmem / MemoryHeapBase ↔ 같은 물리 페이지를 mmap, unpin 으로 회수 가능, memfd + F_SEAL ↔ 수신 측이 내용·크기를 못 바꾸도록 커널이 보장, dmabuf (GraphicBuffer) ↔ GPU·디스플레이·코덱이 같은 버퍼 접근, Binder writeFileDescriptor ↔ FD 참조를 수신 프로세스 fd 테이블에 dup**

pipe=복사 스트림, ashmem=페이지 공유·회수, memfd=봉인, dmabuf=HW 공유, writeFileDescriptor=dup 전달. 전달 메커니즘은 하나이고 file 객체 종류만 다르다.
