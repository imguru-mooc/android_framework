# [D2-4] 실습 5-1 — Java→커널 호출 스택 추적 (소스 grep)

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 2 Ch 5 · 필수 | 빌드 서버(PuTTY) `~/aosp` |

> 🎯 **실습 목표** — `transact()` 한 번이 지나가는 4개 파일을 서버 소스에서 직접 grep으로 추적해, **빈칸 호출 스택 다이어그램**을 완성한다.

## Step 1. Java층 — BinderProxy.transact

```bash
cd ~/aosp
grep -n "transactNative" frameworks/base/core/java/android/os/BinderProxy.java | head -5
```

✅ **예상 결과:** `public native boolean transactNative(...)` 선언 + transact()에서 호출 — **Java의 끝은 native 선언**

## Step 2. JNI 다리 — android_util_Binder.cpp

```bash
grep -n "transactNative\|android_os_BinderProxy_transact" frameworks/base/core/jni/android_util_Binder.cpp | head -5
```

✅ **예상 결과:** gBinderProxyMethods 테이블에 `"transactNative"` ↔ C++ 함수 매핑 — **Third(RegisterNatives)의 실전형**(🎬 ⑰)

## Step 3. Native 프록시 — BpBinder::transact

```bash
grep -n "IPCThreadState::self()->transact" frameworks/native/libs/binder/BpBinder.cpp | head -3
```

✅ **예상 결과:** BpBinder::transact가 IPCThreadState로 위임하는 한 줄

## Step 4. 커널 문 앞 — IPCThreadState

```bash
grep -n "talkWithDriver\|ioctl(.*BINDER_WRITE_READ" frameworks/native/libs/binder/IPCThreadState.cpp | head -5
```

✅ **예상 결과:** `talkWithDriver()` 안의 `ioctl(mDriverFD, BINDER_WRITE_READ, &bwr)` — **유저 공간의 마지막 줄**

## Step 5. 빈칸 다이어그램 완성

```text
BinderProxy.transact (Java, [파일: BinderProxy.java])
   ↓ native
[① android_util_Binder.cpp ] — JNI 매핑 테이블
   ↓
BpBinder::transact ([② BpBinder.cpp ])
   ↓
IPCThreadState::transact → [③ talkWithDriver() ]
   ↓
[④ ioctl(BINDER_WRITE_READ) ] → 커널 Binder 드라이버
```

# 🏁 Pass 판정 체크리스트

- [ ] 4개 파일에서 각 grep 히트 라인번호 기록
- [ ] 빈칸 ①~④ 정확히 완성
- [ ] "Java에서 커널까지 몇 개의 층을 지나는가"를 한 문장으로

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 파일 경로 없음 | AOSP 버전 차이 | `find frameworks -name BpBinder.cpp` 로 위치 재확인 |
| grep 히트 0 | 심볼명 변경 | 함수명 일부(예: talkWith)로 완화 검색 |

# 🚗 현업 활용 포인트

💡 네이티브 크래시 스택에서 `IPCThreadState`·`BpBinder`가 보이면 "Binder 경계에서 터졌다"는 뜻입니다. 오늘 만든 지도가 그 스택을 읽는 범례가 됩니다.

---
*실습 D2-4 (15/36) · 다음: **D2-5 실습 6-1 AIDL 정의***
