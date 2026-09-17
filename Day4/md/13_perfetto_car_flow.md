# 실습 13. Perfetto 로 온도 버튼 한 번 따라가기 — App → CarService → VHAL → 콜백 → 화면

> **소요시간:** 25분 · **난이도:** ★★☆ · **챕터:** Ch 3 · 4일 총정리 · Day 3 실습 13 절차 재사용

## 목표

- 실습 11 앱의 `setProperty` 1회를 캡처해 4일 동안 배운 구간을 한 트레이스의 flow 로 잇는다.
- SQL 로 Binder 왕복 수와 시간을 뽑아 "버튼 한 번의 비용" 을 기록한다.

---

## Step 1. cfg — Day 3 cfg + aidl + 앱

`C:\aosp16\day4_car.cfg` (Day 3 `day3_trace.cfg` 를 복사해 아래 두 줄만 추가):

```text
      atrace_categories: "aidl"
      atrace_apps: "com.example.hvacsimulator"
      atrace_apps: "com.android.car"
```

전체:

```text
buffers: { size_kb: 131072 fill_policy: RING_BUFFER }
data_sources: {
  config {
    name: "linux.ftrace"
    ftrace_config {
      ftrace_events: "sched/sched_switch"
      ftrace_events: "sched/sched_wakeup"
      ftrace_events: "sched/sched_waking"
      ftrace_events: "binder/binder_transaction"
      ftrace_events: "binder/binder_transaction_received"
      atrace_categories: "gfx"
      atrace_categories: "view"
      atrace_categories: "binder_driver"
      atrace_categories: "aidl"
      atrace_apps: "com.example.hvacsimulator"
      atrace_apps: "com.android.car"
    }
  }
}
data_sources: { config { name: "android.surfaceflinger.frametimeline" } }
data_sources: { config { name: "linux.process_stats" process_stats_config { scan_all_processes_on_start: true } } }
duration_ms: 8000
```

```bat
adb push C:\aosp16\day4_car.cfg /data/local/tmp/
```

## Step 2. 캡처 — 8초 동안 온도 버튼 3번

cmd ①:

```bash
adb shell
perfetto -c /data/local/tmp/day4_car.cfg --txt -o /data/misc/perfetto-traces/day4_car.perfetto-trace
```

Emulator 에서 HvacSimulator 의 온도 ▲ 를 1초 간격으로 3번 누른다.

```bat
adb pull /data/misc/perfetto-traces/day4_car.perfetto-trace C:\aosp16\trace\
```

## Step 3. 핀 4개

https://ui.perfetto.dev → 열기 → 검색·📌: `hvacsimulator` · `com.android.car` · `automotive.vehicle` · `surfaceflinger`.

## Step 4. flow 따라가기 (Day 3 실습 13 Step 5 방식)

`hvacsimulator` main Thread 에서 `binder transaction` 을 하나 클릭하고 화살표를 따라간다.

| 순서 | 어디 | 슬라이스 | 4일 중 어디서 배웠나 |
|---|---|---|---|
| ① | `hvacsimulator` main | `deliverInputEvent` → `performClick` | Day 4 실습 8 (input) |
| ② | 같은 Thread | **`binder transaction`** — aidl 이름 `ICarProperty::setProperty` | Day 1 (Java AIDL Proxy) |
| ③ | `com.android.car` `binder:PID_N` | **`binder reply`** — `CarPropertyService.setProperty` | Day 2 실습 11 (Java System Service) |
| ④ | 같은 Thread 안쪽 | `binder transaction` — `IVehicle::setValues` | Day 2 실습 10 (AIDL HAL, NDK backend) |
| ⑤ | `automotive.vehicle…` `binder:PID_N` | `binder reply` — VHAL 처리 | Day 2 (vendor 프로세스) |
| ⑥ | VHAL → `com.android.car` | `binder transaction` — `IVehicleCallback::onPropertyEvent` (역방향) | Day 1 실습 9 · Day 2 실습 6 (Callback) |
| ⑦ | `com.android.car` → `hvacsimulator` `binder:PID_N` | `ICarPropertyEventListener::onEvent` → 앱 `onChangeEvent` | Day 3 실습 6 (Binder Thread, Looper 없음) |
| ⑧ | `hvacsimulator` main | `runOnUiThread` → **`Choreographer#doFrame`** → `RenderThread` `DrawFrames` | Day 3 실습 5 (Looper) · Day 4 실습 8 |
| ⑨ | `surfaceflinger` | `onMessageRefresh` → HWC | Day 3 실습 9·10 |

하단 **Flow events** 탭에서 ②→③→④→⑤→⑥→⑦ 의 화살표 목록을 확인한다. `M`/`Shift+M` 으로 ② 시작부터 ⑨ 까지 잰다 — 보통 수 ms.

## Step 5. SQL — 버튼 한 번의 비용

```sql
SELECT s.ts, s.dur/1e3 AS dur_us, s.name, t.name AS thread, p.name AS process
FROM slice s JOIN thread_track tt ON s.track_id = tt.id
JOIN thread t USING(utid) JOIN process p USING(upid)
WHERE s.name LIKE 'binder%'
  AND (p.name LIKE '%hvacsimulator%' OR p.name LIKE '%com.android.car%' OR p.name LIKE '%vehicle%')
ORDER BY s.ts;
```

한 번의 클릭 구간에서 `binder transaction`/`binder reply` 쌍이 몇 개인지, `dur_us` 합이 얼마인지 적는다. (aidl 카테고리가 켜져 있으면 `AIDL::java::ICarProperty::setProperty::server` 같은 슬라이스도 함께 보인다 — `name LIKE 'AIDL%'` 로 뽑아 본다.)

```sql
SELECT name, COUNT(*) n, ROUND(AVG(dur)/1e3,1) avg_us FROM slice WHERE name LIKE 'AIDL::%' GROUP BY name ORDER BY n DESC;
```

## Step 6. 기록

| 항목 | 값 |
|---|---|
| 클릭 1회의 Binder 왕복 수 | (예: 4 — set, setValues, onPropertyEvent, onEvent) |
| ②~⑦ 총 시간 | ms |
| ⑧ 첫 프레임까지 | ms |
| 가장 긴 슬라이스 | 이름 · Thread |

---

## 확인 포인트

- [ ] 앱 → CarService → VHAL → 콜백 → 앱 → SF 가 화살표로 이어짐
- [ ] Flow events 탭에 6개 이상의 flow
- [ ] SQL 로 Binder 쌍과 시간 추출

## 핵심 정리

이 트레이스 한 장이 4일의 지도다. Java AIDL(Day 1) → Java System Service 와 AIDL HAL(Day 2) → Binder Thread·Looper·SurfaceFlinger(Day 3) → Automotive 계층(Day 4) 이 버튼 한 번 안에 다 들어 있다. 현장에서 "느리다" 는 말이 나오면 이 그림에서 어느 구간인지부터 찾는다.
