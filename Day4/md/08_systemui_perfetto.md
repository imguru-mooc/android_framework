# 실습 8. SystemUI Perfetto — UI Thread · RenderThread · Jank

> **소요시간:** 20분 · **난이도:** ★★☆ · **챕터:** Ch 2 · 저장소 SystemUI 실습 5 · Day 3 실습 13 절차 재사용

## 목표

- Java App(SystemUI) 의 프레임 파이프라인 `Choreographer#doFrame` → UI Thread → `RenderThread` → SF 를 Perfetto 로 본다.
- Day 3 의 Native(Thread 1개) 와 다른 점(UI + Render 2개 Thread)을 확인한다.
- `gfxinfo` 숫자와 대조한다.

---

## Step 1. cfg (Windows)

`C:\aosp16\day4_sysui.cfg`:

```text
buffers: { size_kb: 63488 fill_policy: RING_BUFFER }
buffers: { size_kb: 2048  fill_policy: RING_BUFFER }
data_sources: {
  config {
    name: "linux.ftrace"
    ftrace_config {
      ftrace_events: "sched/sched_switch"
      ftrace_events: "sched/sched_wakeup"
      ftrace_events: "power/cpu_frequency"
      atrace_categories: "gfx"
      atrace_categories: "view"
      atrace_categories: "wm"
      atrace_categories: "am"
      atrace_categories: "input"
      atrace_apps: "com.android.systemui"
    }
  }
}
data_sources: { config { name: "linux.process_stats" process_stats_config { scan_all_processes_on_start: true } } }
data_sources: { config { name: "android.surfaceflinger.frametimeline" } }
duration_ms: 10000
```

```bat
adb push C:\aosp16\day4_sysui.cfg /data/local/tmp/
```

## Step 2. 캡처 — 10초 동안 QS 패널을 열고 닫는다

cmd ①:

```bash
adb shell
perfetto -c /data/local/tmp/day4_sysui.cfg --txt -o /data/misc/perfetto-traces/day4_sysui.perfetto-trace
```

cmd ② (바로):

```bash
adb shell
input swipe 500 0 500 1500 300      # QS 열기
sleep 1
input swipe 500 1500 500 0 300      # 닫기
sleep 1
input swipe 500 0 500 1500 300
sleep 1
input swipe 500 1500 500 0 300
exit
```

```bat
adb pull /data/misc/perfetto-traces/day4_sysui.perfetto-trace C:\aosp16\trace\
```

## Step 3. UI 읽기

https://ui.perfetto.dev → 열기 → `com.android.systemui` 핀, `surfaceflinger` 핀.

| 트랙 | 보는 것 |
|---|---|
| `com.android.systemui` main (`ndroid.systemui`) | **`Choreographer#doFrame`** → 안에 `traversal` → `measure` / `layout` / `draw` — UI Thread |
| `RenderThread` | **`DrawFrames`** / `DrawFrame` — GPU 명령 생성 (Day 3 Native 에는 없던 두 번째 Thread) |
| `input` 카테고리 | `deliverInputEvent` — swipe 터치가 들어온 시점 |
| `wm` | `relayoutWindow` — 패널 창 크기 변경 |
| `Expected/Actual Timeline` (SystemUI Layer) | 프레임 막대, Jank 색 |
| `surfaceflinger` | `onMessageRefresh` — Day 3 와 같다 |

한 프레임을 확대해 순서를 확인: `deliverInputEvent` → `Choreographer#doFrame`(UI) → `DrawFrames`(Render) → SF `onMessageRefresh` → Actual Timeline.

## Step 4. gfxinfo 와 대조

```bash
adb shell
dumpsys gfxinfo com.android.systemui reset
input swipe 500 0 500 1500 300; sleep 2
dumpsys gfxinfo com.android.systemui | grep -A6 "Janky"
dumpsys gfxinfo com.android.systemui framestats | head -20      # 프레임별 타임스탬프
exit
```

## Step 5. 해석표

| 증상 | Perfetto 위치 | 원인 후보 |
|---|---|---|
| UI Thread 느림 | `Choreographer#doFrame` > 16 ms | 복잡한 레이아웃 · Main Thread 블로킹 |
| RenderThread 느림 | `DrawFrames` > 8 ms | 오버드로우 · GPU |
| 입력 지연 | `deliverInputEvent` 뒤 doFrame 이 늦음 | Main Thread 점유 |
| Binder 대기 | `binder transaction` 이 길다 | IPC 병목 (Day 2 실습 13) |
| SF 지연 | `onMessageRefresh` > 4 ms | Layer 과다 |

---

## 확인 포인트

- [ ] `Choreographer#doFrame` 과 `DrawFrames` 가 다른 Thread 에 있음
- [ ] swipe 동안 프레임 막대가 촘촘히, 일부 Jank
- [ ] `gfxinfo` Janky 비율과 Perfetto 의 빨간 프레임 수가 비슷

## 핵심 정리

Java App 은 한 프레임을 UI Thread(measure/layout/draw) 와 RenderThread(GPU) 로 나눠 처리하고, 결과 버퍼는 Day 3 와 같은 BufferQueue 로 SF 에 간다. Jank 의 원인이 어느 Thread 인지 이 두 트랙으로 가른다.
