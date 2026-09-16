# 실습 12. Graphics · Memory 분석 + LMKD 관찰

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 3·4
> **환경:** Emulator (실습 9~11 프로세스 실행 중이면 더 좋다)

## 목표

- `dumpsys SurfaceFlinger` 로 Layer 트리와 BufferQueue 상태를 읽는다.
- `dumpsys meminfo` · `procrank` · `showmap` 으로 PSS / USS 를 해석하고 Zygote 공유 페이지, 실습 11 의 ashmem/memfd 항목을 찾는다.
- `oom_score_adj` 와 LMKD 가 캐시된 프로세스를 죽이는 순서를 관찰한다.

---

## Step 1. Graphics — Layer 와 BufferQueue

```bash
adb shell /data/surface_client_test &          # 실습 9 Layer 띄워 두기
adb shell dumpsys SurfaceFlinger --list | head -30
adb shell dumpsys SurfaceFlinger | grep -B2 -A25 '"My Cpp Surface"' | head -40
```

| 항목 | 의미 |
|---|---|
| `z=2147483647` | `setLayer` 값 |
| `pos=(100,100)  size=400x400` | Transaction 결과 |
| `format=RGBA_8888` · `activeBuffer=[400x400:448, 1]` | 현재 합성 중인 버퍼 — `:448` 이 stride (400 아님) |
| `mQueuedFrames / mFrameCounter` | BufferQueue 에 queue 된 프레임 |

```bash
adb shell dumpsys SurfaceFlinger --latency 'My Cpp Surface' | head -5     # VSync 주기 + 프레임 타임스탬프 3열
adb shell dumpsys gfxinfo com.android.car.carlauncher | grep -A12 "Janky"   # App 측 프레임 통계
```

## Step 2. Memory — PSS / USS

```bash
adb shell dumpsys meminfo com.android.car | head -40
```

| 항목 | 의미 |
|---|---|
| `Native Heap` | `malloc` — C/C++ 할당 |
| `Dalvik Heap` | Java 객체 |
| `.so mmap` / `.dex mmap` | 라이브러리 — 대부분 **shared clean** (Zygote 와 공유) |
| `Ashmem` / `Other mmap` | 실습 11 의 공유 메모리 · `CursorWindow` |
| `Graphics` | GraphicBuffer (dmabuf) — 실습 9·10 |
| `TOTAL PSS` | 공유 페이지를 공유 수로 나눠 배분한 값 — 실제 부담 |

```bash
adb shell procrank | head -15          # VSS RSS PSS USS 순 정렬
adb shell showmap $(adb shell pidof ashmem1_server) | grep -E "ashmem|memfd|TOTAL"
```

Zygote 공유 확인:

```bash
Z=$(adb shell pidof zygote64); A=$(adb shell pidof com.android.car)
adb shell showmap $Z | tail -1; adb shell showmap $A | tail -1
# shared clean 이 크고 private dirty 가 작다 → COW 로 Framework 를 공유
```

| 지표 | 정의 | 언제 보나 |
|---|---|---|
| VSS | 가상 주소 공간 크기 | 거의 안 봄 |
| RSS | 물리 페이지 (공유 포함 중복 계산) | 대략 |
| **PSS** | 공유 페이지를 n 분의 1 로 배분 | 프로세스 비교·합산 |
| **USS** | 이 프로세스만의 페이지 | 죽이면 돌아오는 양 |

## Step 3. oom_score_adj

```bash
adb shell dumpsys activity oom | grep -E "Proc #|oom:" | head -30
for p in com.android.car.carlauncher com.android.car.settings; do
  echo -n "$p: "; adb shell cat /proc/$(adb shell pidof $p)/oom_score_adj; done
```

| 상태 | oom_score_adj (대략) |
|---|---|
| system_server | -900 |
| Persistent (car service) | -800 |
| Foreground | 0 |
| Visible | 100~200 |
| Service | 500 |
| Cached / Empty | 900~999 ← **먼저 죽는다** |

Settings 앱을 열었다 홈으로 나가면 값이 0 → 900 대로 바뀐다.

## Step 4. LMKD 관찰

메모리 압박 앱 `MemHog` (Android Studio 프로젝트, 버튼 1회당 `byte[]` 50 MB 를 `ArrayList` 에 보관):

```java
List<byte[]> hog = new ArrayList<>();
findViewById(R.id.btn).setOnClickListener(v -> { hog.add(new byte[50 * 1024 * 1024]); tv.setText(hog.size() * 50 + " MB"); });
```

```bash
adb shell getprop | grep ro.lmk          # lmkd 파라미터 (psi, kill_timeout 등)
adb shell cat /proc/pressure/memory       # PSI — some/full avg10
adb logcat -s lmkd &
```

MemHog 버튼을 계속 누르면:

```text
I lmkd: Kill 'com.android.car.settings' (6720), uid 1000, oom_score_adj 985 to free 48212kB rss ...
I lmkd: Kill 'com.android.car.messenger' (6644), uid 1000, oom_score_adj 900 ...
```

캐시된(점수 높은) 프로세스부터 죽고, 계속 누르면 MemHog 자신(Foreground, 0) 이 마지막에 죽거나 OOM 이 난다. Emulator RAM 을 2 GB 로 줄이면 빨리 재현된다.

---

## 확인 포인트

- [ ] `dumpsys SurfaceFlinger` 에서 내 Layer 의 z · pos · stride 확인
- [ ] `meminfo` 의 `Ashmem`/`Other mmap` 에 실습 11 페이지, `.so mmap` 이 shared clean
- [ ] Foreground 0 → 백그라운드 900 대로 `oom_score_adj` 변화
- [ ] `logcat -s lmkd` 에 `Kill '…' oom_score_adj 9xx` 순서

## 핵심 정리

- PSS 로 비교하고 USS 로 회수량을 본다. Zygote COW 덕에 App 의 PSS 는 작다.
- 공유 메모리(ashmem/memfd/dmabuf) 는 `meminfo` 의 별도 항목 — Native Heap 이 아니다.
- LMKD 는 `oom_score_adj` 가 높은 캐시 프로세스부터, PSI 압박 시점에 죽인다. Foreground 는 마지막.
