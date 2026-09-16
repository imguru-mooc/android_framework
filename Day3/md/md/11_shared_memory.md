# 실습 11. 공유 메모리 3단계 — pipe → ashmem(MemoryHeapBase/MemoryBase) → memfd + F_SEAL

> **소요시간:** 30분 · **난이도:** ★★★ · **챕터:** Ch 4 공유 메모리
> **디렉토리:** `~/android/day3/{pipe_test, ashmem1, ashmem2, memfd_test}`

## 목표

- Day 1 실습 11 의 "FD 만 Binder 로" 를 Native 에서 세 방식으로 다시 만든다.
- **pipe**(복사 스트림) · **ashmem**(같은 페이지 공유) · **memfd**(봉인 가능한 익명 파일) 의 차이를 코드와 `/proc/<pid>/fd` 로 확인한다.
- `MemoryHeapBase` / `MemoryBase` 가 ashmem 을 Binder 객체(`IMemoryHeap` / `IMemory`) 로 감싼 것임을 본다.

## FD 가 Binder 를 타는 원리

```
Server: reply->writeFileDescriptor(fd)        Parcel 안: flat_binder_object { type = BINDER_TYPE_FD, handle = fd }
   → Driver: 수신 Process 에 dup → 새 번호      Client: reply.readFileDescriptor() → 자기 fd 테이블의 번호
```

숫자가 아니라 **커널 file 객체에 대한 참조**가 넘어간다. pipe · ashmem · memfd · dmabuf 모두 이 한 가지 메커니즘이다.

---

## ① pipe — 스트림, 복사

**`pipe_test/pipe_server.cpp`**

```cpp
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <utils/Thread.h>
using namespace android;

class Writer : public Thread {                     // ★ 실습 4 — 별도 Thread 에서 write
    int mFd; size_t mSize;
public:
    Writer(int fd, size_t size) : mFd(fd), mSize(size) {}
    bool threadLoop() override {
        char chunk[64 * 1024]; memset(chunk, 'P', sizeof(chunk));
        size_t left = mSize;
        while (left > 0) { ssize_t n = write(mFd, chunk, left < sizeof(chunk) ? left : sizeof(chunk)); if (n <= 0) break; left -= n; }
        close(mFd);
        printf("[server] wrote %zu bytes via pipe\n", mSize);
        return false;
    }
};

class PipeService : public BBinder {
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t) override {
        if (code != 1) return UNKNOWN_TRANSACTION;
        int32_t size = data.readInt32();
        int fd[2]; pipe(fd);
        reply->writeFileDescriptor(fd[0], true /*takeOwnership*/);   // ★ 읽기 쪽 FD 만 Parcel 에
        (new Writer(fd[1], size))->run("pipe_writer");
        printf("[server] pipe fd[0]=%d fd[1]=%d sent to client pid=%d\n", fd[0], fd[1], IPCThreadState::self()->getCallingPid());
        return NO_ERROR;
    }
};

int main() {
    defaultServiceManager()->addService(String16("pipe.service"), new PipeService);
    printf("pipe.service registered\n"); fflush(stdout);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
```

**`pipe_test/pipe_client.cpp`**

```cpp
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
using namespace android;

int main(int argc, char** argv) {
    int32_t size = argc > 1 ? atoi(argv[1]) : 8 * 1024 * 1024;        // 8 MB — Binder 로는 불가능
    sp<IBinder> b = defaultServiceManager()->checkService(String16("pipe.service"));
    if (b == nullptr) { printf("not found\n"); return 1; }
    Parcel data, reply; data.writeInt32(size);
    b->transact(1, data, &reply);
    int fd = dup(reply.readFileDescriptor());                           // ★ 서버와 다른 번호 (dup 된 것)
    printf("[client] got fd=%d\n", fd);
    char buf[64 * 1024]; size_t total = 0; ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) total += n;            // ★ 데이터는 복사되어 온다
    printf("[client] read %zu bytes\n", total);
    close(fd); return 0;
}
```

```bash
m pipe_server pipe_client
adb push $OUT/system/bin/pipe_server $OUT/system/bin/pipe_client /data && adb shell chmod 755 /data/pipe_server /data/pipe_client
adb shell /data/pipe_server &
adb shell /data/pipe_client 8388608
# [server] pipe fd[0]=7 fd[1]=8 sent to client pid=6410
# [client] got fd=5          ← ★ 번호가 다르다 = dup
# [client] read 8388608 bytes
```

## ② ashmem — 같은 페이지 공유 (`MemoryHeapBase`)

**`ashmem1/my_server.cpp`**

```cpp
#include <cstdio>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
using namespace android;

int main() {
    sp<IMemoryHeap> heap = new MemoryHeapBase(4096);                 // ★ ashmem 4 KB + mmap
    defaultServiceManager()->addService(String16("ashmem.service"), IMemoryHeap::asBinder(heap));   // ★ Binder 객체로 등록

    char* p = (char*)heap->getBase();
    sprintf(p, "Hello Client !!  (server pid %d)\n", getpid());
    printf("heap fd=%d base=%p size=%zu\n", heap->getHeapID(), p, heap->getSize());
    fflush(stdout);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
```

**`ashmem1/my_client.cpp`**

```cpp
#include <cstdio>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IMemory.h>
using namespace android;

int main() {
    sp<IBinder> binder = defaultServiceManager()->checkService(String16("ashmem.service"));
    sp<IMemoryHeap> heap = interface_cast<IMemoryHeap>(binder);      // ★ BpMemoryHeap — 생성 시 FD 를 받아 mmap
    char* p = (char*)heap->getBase();                                // ★ 서버와 같은 물리 페이지
    printf("[client] base=%p : %s", p, p);
    return 0;
}
```

```bash
m ashmem1_server ashmem1_client
adb push … && adb shell /data/ashmem1_server &
adb shell /data/ashmem1_client
# [client] base=0x7c1e...  : Hello Client !!  (server pid 6501)
adb shell ls -l /proc/$(adb shell pidof ashmem1_server)/fd | grep -E "ashmem|memfd"
# lrwx------ ... 4 -> /dev/ashmem     (또는 /memfd:MemoryHeapBase)
```

`BpMemoryHeap` 은 `interface_cast` 시점에 서버에 `HEAP_ID` Transaction 을 보내 **FD 를 받아 자기 주소 공간에 mmap** 한다. 이후 `getBase()` 는 IPC 없이 로컬 포인터. 서버가 문자열을 바꾸면 Client 는 다음 실행에서 바로 본다 — 복사가 없다.

**`ashmem2` — heap 의 일부만 공개 (`MemoryBase`)**

```cpp
// server
sp<IMemoryHeap> heap = new MemoryHeapBase(4096);
sp<IMemory> base = new MemoryBase(heap, 2048, 1024);                   // ★ offset 2048, 1 KB 만
defaultServiceManager()->addService(String16("ashmem.service"), IMemory::asBinder(base));
sprintf((char*)heap->getBase() + 2048, "Hello Client !! (offset 2048)\n");

// client
sp<IMemory> base = interface_cast<IMemory>(binder);
ssize_t offset; size_t size;
sp<IMemoryHeap> heap = base->getMemory(&offset, &size);               // heap + offset + size
printf("[client] offset=%zd size=%zu : %s", offset, size, (char*)heap->getBase() + offset);
// 또는 base->unsecurePointer()
```

AudioFlinger 가 `AudioTrack` 마다 큰 heap 의 일부를 `MemoryBase` 로 잘라 주는 구조다.

## ③ memfd — 봉인 가능한 익명 파일

**`memfd_test/memfd_server.cpp`**

```cpp
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
using namespace android;

static int gFd = -1;

class MemfdService : public BBinder {
    status_t onTransact(uint32_t code, const Parcel&, Parcel* reply, uint32_t) override {
        if (code != 1) return UNKNOWN_TRANSACTION;
        reply->writeFileDescriptor(gFd, false);                        // ★ 같은 FD 를 여러 Client 에 (dup)
        return NO_ERROR;
    }
};

int main() {
    gFd = memfd_create("shared_page", MFD_ALLOW_SEALING);              // ★ 익명 tmpfs 파일
    ftruncate(gFd, 4096);
    char* p = (char*)mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, gFd, 0);
    strcpy(p, "Hello from memfd (sealed)\n");

    // ★ 봉인 — 크기 변경 · 쓰기 금지 (서버 자신도 이후 write 불가)
    fcntl(gFd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL);
    printf("memfd fd=%d seals=0x%x\n", gFd, fcntl(gFd, F_GET_SEALS)); fflush(stdout);

    defaultServiceManager()->addService(String16("memfd.service"), new MemfdService);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
```

**`memfd_test/memfd_client.cpp`**

```cpp
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
using namespace android;

int main() {
    sp<IBinder> b = defaultServiceManager()->checkService(String16("memfd.service"));
    Parcel data, reply; b->transact(1, data, &reply);
    int fd = dup(reply.readFileDescriptor());
    printf("[client] fd=%d seals=0x%x\n", fd, fcntl(fd, F_GET_SEALS));

    char* ro = (char*)mmap(nullptr, 4096, PROT_READ, MAP_SHARED, fd, 0);
    printf("[client] read: %s", ro);

    void* rw = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);   // ★ F_SEAL_WRITE → EPERM
    printf("[client] PROT_WRITE mmap: %s\n", rw == MAP_FAILED ? strerror(errno) : "OK (sealed 아님!)");
    if (ftruncate(fd, 8192) < 0) printf("[client] ftruncate: %s\n", strerror(errno));   // F_SEAL_GROW → EPERM
    return 0;
}
```

```bash
m memfd_server memfd_client
adb push … && adb shell /data/memfd_server &
adb shell /data/memfd_client
# [client] fd=5 seals=0xf
# [client] read: Hello from memfd (sealed)
# [client] PROT_WRITE mmap: Operation not permitted
# [client] ftruncate: Operation not permitted
adb shell ls -l /proc/$(adb shell pidof memfd_server)/fd | grep memfd
# 3 -> /memfd:shared_page (deleted)
```

Android 11+ 의 Java `SharedMemory` 와 `MemoryHeapBase` 는 내부적으로 memfd 를 쓴다(`ashmem` 은 커널에서 제거 추세). `F_SEAL_WRITE` 로 "받는 쪽이 내용을 못 바꾼다" 를 커널이 보장하므로 신뢰 경계를 넘는 버퍼 공유에 쓴다.

---

## 비교표 (실습 후 작성)

| | pipe | ashmem / MemoryHeapBase | memfd + seal | dmabuf (실습 9) |
|---|---|---|---|---|
| 데이터 이동 | **복사** (커널 버퍼 경유) | 없음 — 같은 페이지 | 없음 | 없음 |
| 접근 | 순차, 1회 | 임의 (`mmap`) | 임의 | GPU/HW 도 접근 |
| 크기 | 무제한 스트림 | 고정 (`mmap` 크기) | 고정, `SEAL_GROW/SHRINK` 로 봉인 | 고정 |
| 보호 | — | `PROT_MASK` 로 강등 | `F_SEAL_WRITE` — 커널 보장 | — |
| 회수 | — | `unpin` 시 커널이 회수 가능 | — | — |
| 사용처 | 로그 · 스트리밍 · `ParcelFileDescriptor.createPipe` | AudioFlinger · Camera · `CursorWindow` | `SharedMemory` · 신뢰 경계 버퍼 | GraphicBuffer · Codec |

## 확인 포인트

- [ ] ① Client fd 번호 ≠ 서버 fd 번호, 8 MB 수신
- [ ] ② Client 가 서버가 쓴 문자열을 `getBase()` 로 읽음, `/proc/<pid>/fd` 에 ashmem/memfd
- [ ] ② ashmem2: `offset=2048 size=1024`
- [ ] ③ seals=0xf, `PROT_WRITE` mmap 과 `ftruncate` 가 EPERM

## 핵심 정리

- Binder 로 넘어가는 것은 **FD 참조** 하나. pipe · ashmem · memfd · dmabuf 는 그 FD 가 가리키는 커널 객체의 종류만 다르다.
- `MemoryHeapBase` = ashmem/memfd + mmap 을 `IMemoryHeap` Binder 객체로, `MemoryBase` = 그 일부를 `IMemory` 로. `interface_cast` 가 FD 를 받아 mmap 까지 해 준다.
- 실습 12 의 `dumpsys meminfo` 에서 이 페이지들이 `Ashmem` / `Other mmap` 항목에 잡힌다.
