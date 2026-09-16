# 실습 1. Day 3 환경 점검 · `day3/Android.bp` 일괄 등록 · JDK 확인

> **소요시간:** 15분 · **난이도:** ★☆☆
> **환경:** Ubuntu 빌드 서버 (`~/android`, PuTTY) + 커스텀 Emulator (adb root)

## 목표

- Day 3 실습 전체의 `Android.bp` 를 **한 번에** 등록해 Soong 재분석을 하루 한 번으로 끝낸다.
- JNI 실습(7·8) 은 빌드 서버의 **호스트 JDK** 로 진행하므로 `javac` 를 확인한다.
- 이후 실습은 소스 디렉토리에서 `m <모듈명>` 으로 빌드한다 (Day 2 와 동일).

---

## Step 1. 빌드 환경 · Emulator 확인

```bash
cd ~/android
source build/envsetup.sh
lunch sdk_car_x86_64-aosp_current-userdebug
echo $OUT
```

```bat
adb root
adb shell getenforce                REM Permissive
adb shell ls -l /dev/ashmem         REM 실습 11 ashmem
adb shell "grep memfd /proc/kallsyms | head -1"   REM 실습 11 memfd (커널 지원)
```

## Step 2. 호스트 JDK (실습 7·8)

```bash
sudo apt install default-jdk -y
javac -version        # javac 17.x 이상
java -version
```

## Step 3. Day 3 실습 디렉토리 + 빈 소스 파일

```bash
mkdir -p ~/android/day3 && cd ~/android/day3
mkdir -p sp_test thread_test looper_test surface1 vsync_anim pipe_test ashmem1 ashmem2 memfd_test jni_test

touch sp_test/sp.cpp sp_test/my_sp.cpp
touch thread_test/thread.cpp thread_test/my_thread.cpp
touch looper_test/Looper.cpp
touch surface1/main.cpp vsync_anim/vsync_anim.cpp
touch pipe_test/pipe_server.cpp pipe_test/pipe_client.cpp
touch ashmem1/my_server.cpp ashmem1/my_client.cpp
touch ashmem2/my_server.cpp ashmem2/my_client.cpp
touch memfd_test/memfd_server.cpp memfd_test/memfd_client.cpp
```

`jni_test/` 는 호스트 JDK 로 빌드하므로 `Android.bp` 에 넣지 않는다.

## Step 4. Android.bp 일괄 작성

**`~/android/day3/Android.bp`**

```text
// ---------- Ch 1 libutils ----------
cc_binary { name: "sp_test",     srcs: ["sp_test/sp.cpp"],        shared_libs: ["libutils","liblog"] }
cc_binary { name: "my_sp_test",  srcs: ["sp_test/my_sp.cpp"],     shared_libs: ["libutils","liblog"] }
cc_binary { name: "thread_test", srcs: ["thread_test/thread.cpp"], shared_libs: ["libutils","liblog"] }
cc_binary { name: "my_thread_test", srcs: ["thread_test/my_thread.cpp"], shared_libs: ["libutils","liblog"] }
cc_binary { name: "looper_test", srcs: ["looper_test/Looper.cpp"], shared_libs: ["libutils","liblog"] }

// ---------- Ch 3 Graphics ----------
cc_binary {
    name: "surface_client_test",
    srcs: ["surface1/main.cpp"],
    shared_libs: ["libgui","libui","libutils","liblog","libbinder","libnativewindow"],
    cflags: ["-Wall","-Werror"],
}
cc_binary {
    name: "sf_vsync_anim",
    srcs: ["vsync_anim/vsync_anim.cpp"],
    shared_libs: ["libgui","libui","libutils","liblog","libbinder","libandroid"],
    cflags: ["-Wall","-Werror"],
}

// ---------- Ch 4 공유 메모리 ----------
cc_binary { name: "pipe_server",  srcs: ["pipe_test/pipe_server.cpp"],  shared_libs: ["libbinder","libutils","liblog"] }
cc_binary { name: "pipe_client",  srcs: ["pipe_test/pipe_client.cpp"],  shared_libs: ["libbinder","libutils","liblog"] }
cc_binary { name: "ashmem1_server", srcs: ["ashmem1/my_server.cpp"], shared_libs: ["libbinder","libutils","liblog"] }
cc_binary { name: "ashmem1_client", srcs: ["ashmem1/my_client.cpp"], shared_libs: ["libbinder","libutils","liblog"] }
cc_binary { name: "ashmem2_server", srcs: ["ashmem2/my_server.cpp"], shared_libs: ["libbinder","libutils","liblog"] }
cc_binary { name: "ashmem2_client", srcs: ["ashmem2/my_client.cpp"], shared_libs: ["libbinder","libutils","liblog"] }
cc_binary { name: "memfd_server",  srcs: ["memfd_test/memfd_server.cpp"], shared_libs: ["libbinder","libutils","liblog"] }
cc_binary { name: "memfd_client",  srcs: ["memfd_test/memfd_client.cpp"], shared_libs: ["libbinder","libutils","liblog"] }
```

## Step 5. Soong 분석 1회

```bash
m sp_test 2>&1 | tail -3      # 분석 5~6분 → 빈 .cpp 링크 오류는 정상
```

이후 실습에서는 `.cpp` 를 채운 뒤 소스 디렉토리에서 `m <모듈명>` 만 실행한다. `.bp` 는 오늘 이 한 번만 손댄다.

---

## 확인 포인트

- [ ] `adb shell getenforce` = Permissive, `/dev/ashmem` 존재
- [ ] `javac -version` 출력
- [ ] `m sp_test` 에서 `analyzing Android.bp` 통과
- [ ] `vim` 에서 `:tag Looper::pollOnce` 로 이동 (Day 2 tags 유지)

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `file not found` (bp) | Step 3 의 `touch` 확인 |
| `/dev/ashmem` 없음 | 커스텀 커널이 `CONFIG_ASHMEM` 없이 빌드됨 → 실습 11 ② 는 `MemoryHeapBase` 가 내부적으로 memfd 를 쓰므로 그대로 진행 가능 |
| `javac: command not found` | `sudo apt install default-jdk` |
