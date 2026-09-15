# 실습 13 (데모). Binder 호출 비용 측정 · strace 로 ioctl 관찰

> **소요시간:** 15분 (강사 시연) · **난이도:** ★★☆
> **환경:** Emulator + 실습 5 `my_server_test_4` / `my_client_test_4`

## 목표

- Native Binder 왕복 1회의 실제 비용(µs)을 측정한다.
- `strace` 로 `transact()` 가 결국 `ioctl(BINDER_WRITE_READ)` 임을 눈으로 본다.
- Thread Pool 크기와 동시 호출의 관계를 관찰한다.

---

## Step 1. 왕복 비용 측정

`my_client_test_4` 를 루프로 1000 번 부르는 대신, 클라이언트 코드에 타이머를 넣은 버전을 강사가 준비해 둔다 (`binder4/my_client_bench.cpp`, `Android.bp` 미등록 — Day 1 실습 4 의 직접 clang 빌드 스크립트로 빌드):

```cpp
auto t0 = std::chrono::steady_clock::now();
for (int i = 0; i < 1000; i++) pLed->ledOn(50);
auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count();
printf("1000 calls: %lld us  (%.1f us/call)\n", (long long)us, us / 1000.0);
```

Emulator(x86_64, KVM) 기준 대략 **30~80 µs/call**. 실제 기기는 10~30 µs. Socket 왕복보다 짧고, 함수 호출보다 수천 배 길다 — "IPC 는 공짜가 아니다" 를 숫자로 보여 준다.

## Step 2. strace 로 시스템 콜 보기

```bash
adb shell
strace -f -e trace=ioctl,mmap,openat /data/my_client_test_4 50 2>&1 | grep -E "binder|ioctl" | head -20
```

```text
openat(AT_FDCWD, "/dev/binderfs/binder", O_RDWR|O_CLOEXEC) = 3      ← ProcessState::self()
ioctl(3, BINDER_VERSION, ...) = 0
mmap(NULL, 1040384, PROT_READ, MAP_PRIVATE|MAP_NORESERVE, 3, 0)      ← 1 MB Transaction Buffer (Day 1)
ioctl(3, BINDER_SET_MAX_THREADS, [15]) = 0                          ← Thread Pool 최대 16
ioctl(3, BINDER_WRITE_READ, {write_size=…, read_size=…}) = 0        ← checkService (handle 0)
ioctl(3, BINDER_WRITE_READ, …) = 0                                  ← ★ ledOn() transact
```

`mmap` 크기 1040384 = 1 MB − 8 KB. Day 1 에서 말한 Transaction Buffer 가 실제로 여기서 잡힌다.

## Step 3. Thread Pool 관찰

서버를 `startThreadPool()` 없이(실습 2 형태) 띄우고 클라이언트 3개를 동시에 실행:

```bash
for i in 1 2 3; do /data/my_client_test_4 $i & done; wait
```

`BINDER_SET_MAX_THREADS` 는 15 지만 `startThreadPool()` 을 안 부르면 main Thread 하나만 요청을 받는다 → 서버 로그의 tid 가 모두 같고 순차 처리된다. `startThreadPool()` 을 넣은 실습 4 서버로 바꾸면 tid 가 달라진다.

```bash
SPID=$(pidof my_server_test_4)
cat /dev/binderfs/binder_logs/proc/$SPID | grep -c "^  thread"
```

---

## 정리

| 관찰 | 의미 |
|---|---|
| 30~80 µs/call | Binder 왕복 비용 — 루프 안에서 IPC 를 부르지 말 것 |
| `openat /dev/binderfs/binder` + `mmap 1 MB` | `ProcessState` 생성 시 1회 |
| `BINDER_WRITE_READ` | 모든 `transact()` 의 실체 |
| `BINDER_SET_MAX_THREADS` | Thread Pool 상한. 실제 생성은 `startThreadPool` + Driver 요청 |

Day 3 에서는 이 Binder 위에 JNI 와 SurfaceFlinger 클라이언트를 얹는다.
