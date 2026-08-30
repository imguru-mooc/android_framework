# [D2-5] 실습 6-1 — 커스텀 서비스 ①: AIDL 인터페이스 정의

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 2 Ch 6 · ★전원 필수 | WinSCP(북마크 custom) + 서버 |

> 🎯 **실습 목표** — AOSP 트리에 `IDeviceInfoService.aidl`을 추가하고 빌드 시스템(filegroup)에 등록한다. 이후 6-2~6-5의 출발점. 🎬 ⑤의 '① 계약서' 단계.

## Step 1. 디렉토리·파일 생성 (WinSCP 편집기 연동)

🖱 WinSCP 북마크 **custom** 이동 → 없으면 서버에서:

```bash
mkdir -p ~/aosp/frameworks/base/core/java/android/os/custom/
```

🖱 새 파일 `IDeviceInfoService.aidl` 생성 → 더블클릭 → 아래 내용 입력 → **Ctrl+S(자동 업로드)**

```java
package android.os.custom;

interface IDeviceInfoService {
    String getDeviceModel();
    long getUptimeSeconds();
    boolean setCustomProperty(String key, String value);   // 권한 필요
    String getCustomProperty(String key);
}
```

## Step 2. 빌드 등록 위치 찾기

```bash
grep -n "framework-aidl-export\|srcs.*aidl" ~/aosp/frameworks/base/Android.bp | head -5
```

🖱 해당 filegroup의 `srcs`에 경로 패턴이 우리 파일을 포함하는지 확인 — 포함 안 되면 `"core/java/android/os/custom/*.aidl"` 추가

⚠️ Android.bp 수정 위치는 버전마다 다릅니다 — 강사 화면과 대조 후 저장.

## Step 3. 문법 검증 (가벼운 부분 빌드)

```bash
cd ~/aosp && source build/envsetup.sh && lunch sdk_car_x86_64-userdebug
m framework-minus-apex -j8 2>&1 | tail -20
```

✅ **예상 결과:** aidl 컴파일 통과(에러 없이 진행). 💡 전체 통합 빌드는 6-4 후 14:40에 전원 함께 — 지금은 문법만.

# 🏁 Pass 판정 체크리스트

- [ ] aidl 파일이 서버 정확한 경로에 존재 (WinSCP 저장 반영 확인)
- [ ] Android.bp filegroup 포함 확인/수정
- [ ] 부분 빌드에서 aidl 관련 에러 0

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| `couldn't find import` | package와 경로 불일치 | `android/os/custom/` 폴더 ↔ `package android.os.custom;` 일치 |
| filegroup에 안 잡힘 | srcs 패턴 밖 | Step 2에서 패턴 추가 |
| 저장했는데 서버에 없음 | 로컬 사본 편집 | 반드시 WinSCP **서버 패널**에서 열기(D0-5) |

# 🚗 현업 활용 포인트

💡 플랫폼 AIDL은 **한 번 배포되면 계약**입니다(버전 관리 대상). "메서드 하나 추가"가 곧 API 리뷰 대상이 되는 이유를 6-5 검증 후 다시 이야기합니다.

---
*실습 D2-5 (16/36) · 다음: **D2-6 실습 6-2 Service 구현***
