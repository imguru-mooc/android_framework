# 실습 5. SystemUI 분석 — dumpsys statusbar · notification · gfxinfo

> **소요시간:** 15분 · **난이도:** ★☆☆ · **챕터:** Ch 2 · 저장소 SystemUI 실습 1

## 목표

- 상태바·알림·QS 의 현재 상태를 `dumpsys` 로 읽는다.
- 알림 발생·Wi-Fi 토글 전후를 비교해 "상태 변화가 어디에 기록되는지" 익힌다.

---

## Step 1. 상태바

```bash
adb shell
dumpsys statusbar | head -30
# StatusBarService state:
#   mDisabled1=0x0  mDisabled2=0x0
#   ...
dumpsys statusbar | grep -A5 "Notifications"
dumpsys statusbar | grep -i tile | head
dumpsys statusbar | grep -i icon | head
```

## Step 2. 알림 전후

```bash
dumpsys notification | grep -c "NotificationRecord"           # 현재 알림 수
am start -a android.intent.action.VIEW -d "https://example.com" # 브라우저 → 다운로드/알림이 뜰 수 있음
dumpsys notification | grep -A3 "NotificationRecord" | head -20
```

## Step 3. Wi-Fi 토글 전후 아이콘

```bash
dumpsys statusbar | grep -i wifi
svc wifi disable; sleep 2; dumpsys statusbar | grep -i wifi
svc wifi enable
```

## Step 4. 프레임 통계 · 메모리 · 포커스

```bash
dumpsys gfxinfo com.android.systemui | grep -A8 "Janky frames"
# Total frames rendered: 412   Janky frames: 9 (2.18%)   90th percentile: 11ms
dumpsys meminfo com.android.systemui | grep -E "TOTAL PSS|Graphics"     # Day 3 실습 12
dumpsys activity activities | grep mResumedActivity
dumpsys activity top | grep -E "ACTIVITY|View Hierarchy" | head
exit
```

QS 패널을 펼친 상태와 접은 상태에서 `dumpsys statusbar | grep -i "panel\|expanded"` 를 비교해 본다.

---

## 확인 포인트

- [ ] `mDisabled1/2`, Tiles, Icons 섹션 위치
- [ ] 알림 발생 후 `NotificationRecord` 증가
- [ ] Wi-Fi off 후 아이콘 항목 변화, `gfxinfo` 의 Janky frames 비율

## 핵심 정리

| 명령 | 보는 것 |
|---|---|
| `dumpsys statusbar` | StatusBarService 상태 — disabled 플래그 · 아이콘 · QS 타일 |
| `dumpsys notification` | NotificationManagerService — 활성 알림 레코드 |
| `dumpsys gfxinfo <pkg>` | 프레임 통계 (Janky 5% 미만·90th < 16.67 ms 이면 양호) — 실습 8 의 숫자 버전 |
| `dumpsys activity activities` | 포커스 Activity — "어느 창이 위인가" |

SystemUI 는 Java Service 들의 클라이언트다. `dumpsys` 는 그 Service 들의 `dump()` (Day 2 실습 11) 를 부르는 것이다.
