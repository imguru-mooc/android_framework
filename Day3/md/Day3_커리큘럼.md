# Day 3 — sp/wp · Thread · Looper · JNI · Graphics · SurfaceFlinger · 공유 메모리 · Memory

**과정:** Android Framework 16 심화 교육 (4일 / 32시간)
**환경:** Ubuntu 24 빌드 서버 (`~/android`, AOSP `sdk_car_x86_64`, 호스트 JDK) + Windows (Android Studio, WinSCP, adb) + 커스텀 Emulator (userdebug, permissive)
**범위:** libutils 3종 세트(`RefBase`·`sp/wp`, `Thread`, `Looper`)를 **단계별로 직접 구현** → Binder Thread Pool 에서의 동작 → JNI → SurfaceFlinger Client (VSync + Looper) → 공유 메모리 (pipe · ashmem · memfd · `MemoryHeapBase`) → Memory / LMKD
**총 소요시간:** 8시간 (환경·등록 실습 15m / 강의 1h 55m / 실습 5h 15m / 데모·Worksheet 35m)
**실습 번호:** 실습 1 ~ 13 (Part 0 부터 연속 번호)

---

## 학습 목표

1. `sp<>` 를 없는 상태에서 4단계로 만들어 `RefBase` 구조에 도달하고, `wp<>`·`promote()`·순환 참조를 설명할 수 있다.
2. `pthread` 에서 시작해 `android::Thread` (`run` / `readyToRun` / `threadLoop` / `join`) 를 단계별로 만들고, `onFirstRef` 로 `sp` 와 Thread 가 결합되는 이유를 안다.
3. `Looper` 를 `pollOnce` → `wake` → `addFd` 콜백 → `MessageHandler` 순으로 사용해 **fd 기반 이벤트 루프**와 **메시지 큐**를 구분한다.
4. Binder Thread Pool 이 `ProcessState::PoolThread : Thread` + `joinThreadPool` 로 만들어지고, Binder 콜백이 `Looper/Handler` 로 Main Thread 에 전달되는 경로를 소스에서 찾는다.
5. JNI 로 Java ↔ C 를 양방향(Native Method · C 에서 Java 객체 생성 · `RegisterNatives`)으로 연결하고, Invocation API 로 Native 프로세스에서 VM 을 띄운다.
6. `SurfaceComposerClient` 로 Layer 를 만들고 `ANativeWindow` 로 그린 뒤, `DisplayEventReceiver` 의 fd 를 **실습 5 의 Looper 에 addFd** 해 VSync 애니메이션을 만든다.
7. Process 간 데이터 공유 3가지 — **pipe**(스트림, 복사) · **ashmem**(공유 페이지, `MemoryHeapBase`/`IMemory` 로 Binder 전달) · **memfd**(`memfd_create` + `F_SEAL`, Android 11+ 의 ashmem 대체) — 를 직접 만들어 FD 가 Binder 를 타는 실체를 확인한다.
8. `dumpsys meminfo` / `procrank` 로 PSS·USS 를 읽고 `oom_score_adj` 와 LMKD 의 kill 순서를 관찰한다.

---

## Part 0. 환경·등록 실습 (총 15분)

| # | 실습명 | 난이도 | 소요시간 |
|---|---|---|---|
| 1 | Day 3 환경 점검 · `day3/Android.bp` 일괄 등록 (sp_test · thread_test · looper_test · surface1 · vsync_anim · pipe_test · ashmem1/2 · memfd_test) · 호스트 JDK 확인 | ★☆☆ | 15분 |

---

## Part 1. 강의 (총 115분)

### Chapter 1. libutils — RefBase · sp/wp · Thread · Looper — 40분

| 주제 | 핵심 내용 |
|---|---|
| 왜 이 셋인가 | Binder 객체·Surface·Thread 는 여러 Thread 가 공유 → **소유권(sp)** · **실행 단위(Thread)** · **이벤트 루프(Looper)** 가 libutils 의 기본 골격. Binder·SurfaceFlinger·Input 이 전부 이 위에서 돈다 |
| `RefBase` · `sp<T>` | intrusive 참조 카운트 · `incStrong / decStrong` → 0 이면 `delete this` · `onFirstRef` · `sp` 4단계 발전 (raw delete → static 카운트 → 객체 내 카운트 → `RefBase` 분리) |
| `wp<T>` | 강한 카운트에 무관 · `promote()` → 살아 있으면 `sp`, 아니면 `nullptr` · 순환 참조는 한쪽을 `wp` 로 (Callback · Listener) |
| `Thread` 단계별 | ① `pthread_create` 직접 → ② `Thread` 클래스로 감싸기 (`run/join`, `static __handler(void*)` 로 `this` 전달) → ③ 순수 가상 `handler()` 로 파생 → ④ `android::Thread`: `run(name)` · `readyToRun()` · `threadLoop()` 반환값 (true = 반복, false = 종료) · `requestExit` · `join` |
| `sp` × `Thread` | `Thread : RefBase` — `run()` 이 자기 자신을 `sp` 로 잡아 실행 중 소멸 방지 · `onFirstRef()` 에서 `run()` 하는 패턴 · `this` 를 raw 로 넘기면 안 되는 이유 |
| `Looper` 단계별 | ① `pollOnce(timeout)` → `POLL_TIMEOUT` → ② 다른 Thread 에서 `wake()` → `POLL_WAKE` → ③ `addFd(fd, ident, events, callback, data)` — pipe 로 `POLL_CALLBACK` → ④ 콜백을 클래스로 (`static` 트램폴린 + 가상 함수 = `LooperCallback`) → ⑤ `sendMessage / sendMessageDelayed` + `MessageHandler::handleMessage` |
| Looper 의 두 얼굴 | **fd 이벤트 루프** (epoll) 와 **메시지 큐** (`Message.what`) 가 한 객체. Java `Looper/Handler/MessageQueue` 는 이 C++ Looper 위의 JNI wrapper (`android_os_MessageQueue.cpp`) |
| Binder 에서의 동작 | `ProcessState::startThreadPool` → `spawnPooledThread` → `PoolThread : Thread` → `threadLoop() { IPCThreadState::self()->joinThreadPool(mIsMain) }` · Binder Thread 는 Looper 가 없다 → Java 콜백이 Main Thread 로 가려면 `Handler.post` (= `Looper.sendMessage`) · `DeathRecipient` · `RemoteCallbackList` 도 같은 경로 |

### Chapter 2. JNI — Java ↔ Native 경계 — 20분

| 주제 | 핵심 내용 |
|---|---|
| 구조 | `JavaVM`(Process 당 1) · `JNIEnv`(Thread 당 1) · `System.loadLibrary` → `JNI_OnLoad` · `Java_pkg_Class_method` 규칙 vs `RegisterNatives` · 시그니처 문자열 |
| 참조 · Thread | Local / Global / Weak global · Native Thread(예: `android::Thread`) 에서 Java 를 부르려면 `AttachCurrentThread` + Global ref |
| Invocation API | `JNI_CreateJavaVM` — `app_process` 가 Zygote 를 띄우는 방식 (`app_main.cpp`) |
| Framework JNI | `android_os_Binder.cpp` (`JavaBBinder : BBinder`) · `android_os_MessageQueue.cpp` (Java Looper ↔ C++ Looper) · `android_view_Surface.cpp` (`mNativeObject` = `sp<Surface>`) |

### Chapter 3. Graphics — SurfaceFlinger · BufferQueue · VSync — 25분

| 주제 | 핵심 내용 |
|---|---|
| 파이프라인 | App(UI Thread → RenderThread) → BufferQueue → SurfaceFlinger 합성 → HWC/Gralloc → Display · Layer 트리 · `dumpsys SurfaceFlinger` |
| Client API | `SurfaceComposerClient`(Binder) · `createSurface` → `sp<SurfaceControl>` · `Transaction { setLayer · setPosition · show · apply }` · `sp<Surface>` = `ANativeWindow` · `lock / unlockAndPost` · stride |
| 버퍼 공유 | GraphicBuffer = `dmabuf` FD → Binder 로 FD 만 전달, 픽셀 복사 없음 · dequeue/queue/acquire/release · Triple buffering |
| VSync | `DisplayEventReceiver` 의 fd → **`Looper::addFd`** (실습 5 의 ③④ 그대로) · `requestNextVsync` · 16.67 ms 예산 · Jank |

### Chapter 4. 공유 메모리 · Memory — pipe · ashmem · memfd · 측정 · LMKD — 30분

| 주제 | 핵심 내용 |
|---|---|
| FD 가 Binder 를 타는 원리 | `writeFileDescriptor` → Parcel 의 `flat_binder_object` (`BINDER_TYPE_FD`) → Driver 가 수신 Process 의 fd 테이블에 **복제**(`dup`) — Day 1 실습 11 · Day 2 Callback(IBinder) 과 같은 메커니즘 |
| pipe | `pipe(fd)` → 커널 링 버퍼(64 KB) · **복사** 기반 스트림 · 단방향 · 한 번 읽으면 사라짐 · `ParcelFileDescriptor.createPipe()` · 대용량 순차 전달에 적합 |
| ashmem | `/dev/ashmem` → `ashmem_create_region(name, size)` → `mmap` → **같은 물리 페이지를 양쪽이 매핑** · 복사 없음 · 임의 접근 · `ashmem_pin/unpin` (메모리 압박 시 회수 가능) · `ASHMEM_SET_PROT_MASK` 로 읽기 전용 강등 |
| libbinder wrapper | `MemoryHeapBase(size)` = ashmem + mmap 을 `IMemoryHeap`(Binder 객체) 로 · `MemoryBase(heap, offset, size)` = heap 의 부분 영역 `IMemory` · Client 는 `interface_cast<IMemoryHeap>` → `getBase()` 로 같은 페이지 접근 · AudioFlinger · Camera 의 버퍼 공유가 이 구조 |
| memfd | `memfd_create(name, MFD_ALLOW_SEALING)` — 익명 tmpfs 파일, ashmem 의 커널 상류 대체 (Android 11+ `SharedMemory` 내부) · `F_ADD_SEALS(F_SEAL_SHRINK|GROW|WRITE)` 로 상대가 크기·내용을 못 바꾸게 고정 · `ashmem` 은 `CONFIG_ASHMEM` 제거 추세 |
| 선택 기준 | 순차 스트림 → pipe · 구조화된 공유 버퍼 → ashmem/memfd · 그래픽 버퍼 → dmabuf(Gralloc) · 표: 복사 여부 / 크기 / 회수 가능성 / API 수준 |
| 지표 · LMKD | VSS / RSS / PSS / USS · `dumpsys meminfo` 항목 · `procrank` · `showmap` · Zygote COW · `oom_score_adj` · `lmkd` + PSI · `ro.lmk.*` · `logcat -s lmkd` |

---

## Part 2. 실습 (총 315분)

| # | 실습명 | 난이도 | 소요시간 | 챕터 |
|---|---|---|---|---|
| 2 | `sp<>` 직접 구현 4단계 — raw delete → static 카운트 → 객체 내 카운트 → `RefBase` 분리 | ★★☆ | 25분 | Ch 1 |
| 3 | Android `RefBase` · `wp<>` · `promote()` · 순환 참조 실험 | ★★☆ | 20분 | Ch 1 |
| 4 | `Thread` 단계별 구현 — `pthread` → `Thread` 클래스 → 가상 `handler` → `android::Thread` (`onFirstRef` · `readyToRun` · `threadLoop`) | ★★☆ | 30분 | Ch 1 |
| 5 | `Looper` 단계별 — `pollOnce` → `wake` → `addFd` 콜백 함수 → 콜백 클래스 → `MessageHandler` / `sendMessageDelayed` | ★★★ | 40분 | Ch 1 |
| 6 | Binder 에서의 동작 — `PoolThread::threadLoop` → `joinThreadPool` 읽기 · `startThreadPool` 유무 실험 · Java 콜백을 `Handler` 로 Main Thread 에 전달 | ★★☆ | 20분 | Ch 1 |
| 7 | JNI Native Method — First(Java → C, `javac -h`) · Second(C 에서 Java 객체 생성 `NewObject`) · Third(`JNI_OnLoad` + `RegisterNatives`) — 호스트 JDK | ★★☆ | 35분 | Ch 2 |
| 8 | JNI Invocation API + Framework JNI 읽기 — `invocation.cpp` 로 VM 생성 · `app_process` 비교 · `:tag JavaBBinder` / `nativePollOnce` / `Surface.mNativeObject` | ★★★ | 25분 | Ch 2 |
| 9 | SurfaceFlinger Client — `SurfaceComposerClient` · `Transaction` · `ANativeWindow_lock/unlockAndPost` 빨간 사각형 | ★★★ | 30분 | Ch 3 |
| 10 | VSync 애니메이션 — `DisplayEventReceiver` fd 를 `Looper::addFd` + `LooperCallback` 으로 매 프레임 `setPosition` (실습 5 재사용) | ★★★ | 30분 | Ch 3 |
| 11 | 공유 메모리 3단계 — pipe → ashmem(`MemoryHeapBase` / `MemoryBase` 를 Binder 로) → memfd + `F_SEAL` | ★★★ | 30분 | Ch 4 |
| 12 | Graphics · Memory 분석 + LMKD — `dumpsys SurfaceFlinger` · `meminfo` / `procrank` / `showmap` · `oom_score_adj` · `MemHog` 로 kill 순서 | ★★☆ | 30분 | Ch 3·4 |

> 실습 2 → 6 은 **libutils 를 밑바닥부터** 쌓는 순서다. 실습 5 에서 만든 `Looper + LooperCallback` 이 실습 10 의 VSync 핸들러로 그대로 재사용되고, 실습 4 의 `Thread` 가 실습 6 에서 Binder Thread Pool 의 정체로 드러난다. 실습 11 은 Day 1 실습 11 의 "FD 만 Binder 로" 를 pipe · ashmem · memfd 세 방식으로 Native 에서 다시 만들어, 실습 9·10 의 GraphicBuffer(dmabuf) 도 같은 FD 전달임을 잇는다.

### 실습별 요점

**실습 2. `sp<>` 직접 구현 4단계 — 25분** (`sp_test/sp.cpp`, `#if` 전환)
- ① `sp` 소멸자에서 `delete mPtr` → 복사하면 double free
- ② `static int mRefs` → 모든 객체가 카운트를 공유하는 오류
- ③ `AAA` 안에 `mRefs` + `incStrong/decStrong` → 동작하지만 클래스마다 반복
- ④ `class RefBase { incStrong; decStrong { if(--mRefs==0) delete this; } }` 분리 → `AAA : RefBase` — Android 구조

**실습 3. Android `RefBase` · `wp` — 20분** (`sp_test/my_sp.cpp`)
- `sp<AAA> p = new AAA(); sp<AAA> q = p;` → 소멸 1회 · `wp<AAA> p = new AAA();` 만 → 즉시 소멸 → `promote()` 가 `nullptr`
- `sp` 가 살아 있는 동안 `promote()` 성공 · A↔B `sp` 상호 참조 → 소멸 안 됨 → 한쪽 `wp`

**실습 4. `Thread` 단계별 구현 — 30분** (`thread_test/thread.cpp`, `my_thread.cpp`)
- ① `pthread_create(&t, 0, foo, 0)` 를 `Thread { run(); join(); }` 로 감싸기
- ② `static void* __handler(void* data)` 로 `this` 를 넘겨 멤버 `handler()` 호출 — 멤버 함수를 pthread 에 넘기는 규칙
- ③ `virtual void handler() = 0` → `MyThread` 파생 — 프레임워크가 정하는 실행 골격
- ④ `android::Thread`: `run("name")` · `readyToRun()` 1회 · `threadLoop()` 반복(true)/종료(false) · `join()` · `requestExit()`
- ⑤ `sp<Thread> p = new MyThread;` + `onFirstRef() { run(); }` — `sp` 에 담기는 순간 실행. `Thread` 가 `RefBase` 인 이유(실행 중 자기 참조 유지)

**실습 5. `Looper` 단계별 — 40분** (`Looper_test/Looper.cpp`, `#if` 전환)
- ① `new Looper(true)` → `pollOnce(1000)` → `POLL_TIMEOUT`
- ② 실습 4 의 `MyThread::threadLoop` 에서 3초 후 `looper->wake()` → `POLL_WAKE`
- ③ `pipe(fd)` → `addFd(fd[0], 0, EVENT_INPUT, foo, data)` → 다른 Thread 가 `write` → `POLL_CALLBACK` + `foo(fd, events, data)`
- ④ 콜백을 클래스로: `static staticHandler(fd, events, void* data)` 트램폴린 → 가상 `handler()` → `MyHandler : RefBase` (= Android `LooperCallback`)
- ⑤ `sendMessage(handler, Message(1))` · `sendMessageDelayed(3s, …)` → `MessageHandler::handleMessage(msg.what)` — Java `Handler.sendMessageDelayed` 의 실체
- 정리: fd 이벤트(③④) 와 메시지 큐(⑤) 가 한 Looper 안에 있다

**실습 6. Binder 에서의 동작 — 20분**
- `:tag ProcessState::spawnPooledThread` → `PoolThread : Thread` → `threadLoop() { IPCThreadState::self()->joinThreadPool(mIsMain); return false; }` — Day 2 의 Binder Thread 가 실습 4 의 `Thread` 였다
- Day 2 `my_server_test_4` 로 `startThreadPool()` 유무에 따라 `binder_logs/proc/<PID>` 의 thread 수와 동시 호출 시 tid 비교
- Binder Thread 에는 Looper 가 없다 → Day 1 `StockLab` 콜백의 `runOnUiThread` = `Handler.post` = Main Looper `sendMessage`. `:tag android_os_MessageQueue_nativePollOnce` 로 Java Looper 가 C++ Looper 의 `pollOnce` 임을 확인
- `IPCThreadState::joinThreadPool` 의 `talkWithDriver` 루프 vs `Looper::pollOnce` 의 epoll 루프 — 둘 다 "대기 루프" 지만 대기 대상이 다르다 (Binder Driver vs fd)

**실습 7. JNI Native Method — 35분** (`jni_test/First`, `Second`, `Third`, 호스트 JDK)
- First: `native void foo()` → `javac -h` 헤더 → `Java_Hello_foo` → `gcc -shared` → `LD_LIBRARY_PATH=. java Hello`
- Second: C 에서 `FindClass("JniTest")` → `GetMethodID("<init>", "(I)V")` → `NewObject` → Java 로 반환. 시그니처 문자열 규칙 (`javap -s`)
- Third: `JNI_OnLoad` 에서 `RegisterNatives` — 이름 규칙 없이 `static` 함수 매핑. Framework JNI 방식

**실습 8. Invocation API + Framework JNI — 25분**
- `invocation.cpp` (`JNI_CreateJavaVM` → `FindClass("JniFuncMain")` → `CallStaticVoidMethod`) · `-ljvm` 링크 · `AndroidRuntime::start` (`app_main.cpp`) 와 비교 — Zygote 가 뜨는 방식
- `:tag JavaBBinder` (Java `Binder` ↔ `BBinder`, `execTransact` 역호출) · `:tag android_os_MessageQueue_nativePollOnce` (Java Looper = C++ Looper) · `:tag android_view_Surface_lockCanvas` (`mNativeObject` = `sp<Surface>`)

**실습 9. SurfaceFlinger Client — 30분** (`surface1/main.cpp`)
- `SurfaceComposerClient` → `createSurface(400×400, RGBA_8888)` → `Transaction.setLayer.setPosition(100,100).show().apply()` → `ANativeWindow_lock` → stride 로 픽셀 → `unlockAndPost`
- Layer 수명 = `sp<SurfaceControl>` 수명 (실습 3 연결)

**실습 10. VSync 애니메이션 — 30분** (`vsync_anim/vsync_anim.cpp`)
- `DisplayEventReceiver` (`setVsyncRate(1)` · `requestNextVsync`) → `Looper::prepare` → `addFd(recv->getFd(), …, new VSyncHandler)` — 실습 5 ④ 의 `LooperCallback` 그대로
- `handleEvent` 에서 `getEvents` → `DISPLAY_EVENT_VSYNC` 마다 `setPosition` → `apply` · `sp<SurfaceControl>`(RefBase) vs `unique_ptr<DisplayEventReceiver>`(non-RefBase)
- `pollAll(-1)` 10초 루프 · `setVsyncRate(2)` 로 30 fps 비교

**실습 11. 공유 메모리 3단계 — pipe → ashmem → memfd — 30분** (`pipe_test`, `ashmem_test/ashmem1`, `ashmem2`, `memfd_test`)
- ① **pipe**: Day 2 형태의 Binder 서버가 `pipe(fd)` 를 만들고 `reply->writeFileDescriptor(fd[0])` → Client 가 `reply.readFileDescriptor()` 로 `read` — 8 MB 스트리밍 · 서버는 별도 Thread 에서 `write`(실습 4 의 `Thread`) · `strace` 로 Client fd 번호가 서버와 다른 것(`dup`) 확인
- ② **ashmem1**: `sp<IMemoryHeap> heap = new MemoryHeapBase(4096)` → `addService("ashmem.service", IMemoryHeap::asBinder(heap))` → 서버가 `heap->getBase()` 에 문자열 write · Client 는 `interface_cast<IMemoryHeap>(checkService(...))` → `getBase()` 로 **같은 페이지** 읽기 → 서버가 내용을 바꾸면 Client 도 즉시 보임 (복사 없음)
- ② **ashmem2**: `sp<IMemory> base = new MemoryBase(heap, 2048, 1024)` — heap 의 offset 2048 부터 1 KB 만 `IMemory` 로 공개 · Client 의 `pointer()` 가 offset 반영 · `ls -l /proc/<pid>/fd` 에서 `/dev/ashmem` 확인 · `ASHMEM_SET_PROT_MASK` 로 읽기 전용 강등 실험
- ③ **memfd**: `memfd_create("shared", MFD_ALLOW_SEALING)` → `ftruncate(4096)` → `mmap` → `fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK|F_SEAL_GROW|F_SEAL_WRITE)` → `writeFileDescriptor` 로 전달 · Client 가 `mmap` 후 `write` 시도 → `SIGBUS`/`EPERM` 으로 seal 확인 · `/proc/<pid>/fd` 에 `/memfd:shared` · Java `SharedMemory` 의 내부가 이것
- 비교표 작성: 복사 여부 · 크기 제한 · 임의 접근 · 회수/봉인 · 사용처 (pipe → 로그·스트림 / ashmem·memfd → 버퍼 공유·`CursorWindow` / dmabuf → GraphicBuffer)

**실습 12. Graphics · Memory 분석 + LMKD — 30분**
- `dumpsys SurfaceFlinger | grep -A20 "My Cpp Surface"` · `--list` · `--latency`
- `dumpsys meminfo com.android.car` 항목 (실습 11 의 ashmem/memfd 가 어느 항목에 잡히는지 — `Ashmem` / `Other mmap`) · `procrank` · `showmap <pid>` 의 Zygote 공유(shared clean) 페이지
- `/proc/<pid>/oom_score_adj` Foreground/Cached 비교 · `dumpsys activity oom` · `MemHog` 로 압박 → `logcat -s lmkd` kill 순서 · `/proc/pressure/memory`

---

## Part 3. 데모 & 정리 (총 35분)

### 실습 13 (데모). Perfetto 로 SurfaceFlinger 합성 파이프라인 · Jank — 15분 (강사 시연)
- 실습 10 실행 중 `perfetto` 캡처 (`gfx`, `sf`, frame timeline) → 우리 Layer 의 VSync → `onMessageRefresh` → HWC · `handleEvent` 에 `usleep(20000)` 으로 Jank 재현 → Day 4 SystemUI/Perfetto 로 연결

### Day 3 Worksheet — 20분
- 객관식 · 단답형 · 빈칸 · 순번 · 선잇기 20문제, 별도 정답·해설

**출제 키워드**
```text
RefBase / incStrong / decStrong / sp / wp / promote / 순환 참조 / onFirstRef
pthread_create / static 트램폴린 / this 전달
android::Thread / run / readyToRun / threadLoop 반환값 / join / requestExit
Looper / pollOnce / POLL_TIMEOUT / POLL_WAKE / POLL_CALLBACK / wake / addFd
LooperCallback / MessageHandler / sendMessage / sendMessageDelayed / Message.what
epoll / fd 이벤트 루프 vs 메시지 큐 / Java Looper·Handler·MessageQueue (nativePollOnce)
ProcessState::spawnPooledThread / PoolThread / joinThreadPool / talkWithDriver
Binder Thread 에는 Looper 없음 / Handler.post / runOnUiThread
JavaVM / JNIEnv / JNI_OnLoad / RegisterNatives / Local·Global ref / AttachCurrentThread / JNI_CreateJavaVM
JavaBBinder / android_os_MessageQueue / mNativeObject
SurfaceFlinger / Layer / SurfaceComposerClient / SurfaceControl / Transaction / ANativeWindow_lock / stride
DisplayEventReceiver / VSync / 16.67 ms / Jank / BufferQueue / dmabuf
pipe / writeFileDescriptor / BINDER_TYPE_FD / dup
ashmem / ashmem_create_region / MemoryHeapBase / MemoryBase / IMemoryHeap / IMemory / pin·unpin
memfd_create / F_SEAL_WRITE / SharedMemory / CursorWindow
VSS / RSS / PSS / USS / dumpsys meminfo / procrank / showmap / COW
oom_score_adj / LMKD / PSI
```

---

## 준비물 체크리스트

**강사 / 사전 준비**
- [ ] `~/android/day3/Android.bp` 완성본 + 빈 소스 트리 (sp_test · thread_test · looper_test · jni_invocation · surface1 · vsync_anim · pipe_test · ashmem1 · ashmem2 · memfd_test · memhog)
- [ ] 실습 2·4·5 의 `#if` 단계별 소스 (`sp.cpp`, `thread.cpp`, `my_thread.cpp`, `Looper.cpp`) 배포본
- [ ] 빌드 서버에 `default-jdk` 설치 (실습 7·8) — `javac -version`
- [ ] 실습 11 memfd 는 커널 `CONFIG_MEMFD_CREATE`(GKI 기본 포함), ashmem 은 `/dev/ashmem` 존재 확인 (`ls -l /dev/ashmem`) · 실습 12 용 2 GB RAM AVD 안내 · 실습 13 Perfetto config
- [ ] Day 3 Worksheet 인쇄본 + 정답지

**수강생**
- [ ] Day 2 ctags(`android_native_tags`) 유지 — 실습 6·8 의 `:tag ProcessState::spawnPooledThread`, `JavaBBinder`
- [ ] Day 2 `my_server_test_4` 바이너리 유지 (실습 6 에서 재사용)
- [ ] `m <모듈명>` → WinSCP → `adb push` 배포 절차 숙지
