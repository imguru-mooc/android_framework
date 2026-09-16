# 실습 13 (데모). Perfetto 로 SurfaceFlinger 합성 파이프라인 · Jank 보기

> **소요시간:** 15분 (강사 시연) · **난이도:** ★★☆
> **환경:** Emulator + 실습 10 `sf_vsync_anim` + https://ui.perfetto.dev

## 목표

- VSync → `handleEvent` → `Transaction.apply` → SurfaceFlinger `onMessageRefresh` → HWC 합성이 시간축에서 어떻게 이어지는지 본다.
- 일부러 프레임 예산(16.67 ms) 을 넘겨 Jank 가 frame timeline 에 어떻게 표시되는지 본다.

---

## Step 1. 트레이스 캡처

```bash
adb shell /data/sf_vsync_anim &
adb shell perfetto -c - --txt -o /data/misc/perfetto-traces/day3.perfetto-trace <<EOF
buffers: { size_kb: 65536 fill_policy: RING_BUFFER }
data_sources: { config { name: "linux.ftrace" ftrace_config {
  ftrace_events: "sched/sched_switch" ftrace_events: "sched/sched_wakeup"
  atrace_categories: "gfx" atrace_categories: "view" atrace_categories: "sf"
  atrace_apps: "*" } } }
data_sources: { config { name: "android.surfaceflinger.frametimeline" } }
duration_ms: 6000
EOF
adb pull /data/misc/perfetto-traces/day3.perfetto-trace .
```

## Step 2. UI 에서 보기

1. https://ui.perfetto.dev → Open trace file
2. 검색 `vsync_rect` → 우리 Layer 의 **Actual Timeline / Expected Timeline** 트랙
3. `surfaceflinger` 프로세스 → `onMessageRefresh` · `composite` · `HWC` 슬라이스 — 각 VSync 마다 반복
4. `sf_vsync_anim` 프로세스 → main Thread 의 `handleEvent` 가 VSync 직후 짧게 실행 → `Transaction.apply`

## Step 3. Jank 재현

`vsync_anim.cpp` 의 `step()` 에 `usleep(20000)` 을 넣고 재빌드 후 다시 캡처.

- Actual Timeline 에 **빨간 프레임** (App Deadline Missed / Buffer Stuffing)
- `handleEvent` 슬라이스가 16.67 ms 를 넘어 다음 VSync 를 침범 → 프레임 드롭
- SurfaceFlinger 는 제때 합성하지만 새 Transaction 이 없어 같은 위치 → 화면이 툭툭 끊김

## 정리

| 구간 | 예산 | 넘기면 |
|---|---|---|
| App (UI Thread → RenderThread) | ~16.67 ms − SF 시간 | App Jank (빨강) |
| SurfaceFlinger 합성 | 수 ms | SF Jank |
| HWC / Display | VSync 경계 | 프레임 표시 지연 |

Day 4 SystemUI 실습에서 같은 방법으로 Java App(`com.android.systemui`) 의 UI Thread / RenderThread Jank 를 분석한다.
