# [D3-4] Lab 3-3 — SRO(정적 리소스 오버레이) + RRO 비교표

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 3 Ch 9 · 필수 | RROTarget 프로젝트 |

> 🎯 **실습 목표** — 빌드 시점에 리소스를 갈아끼우는 **SRO**를 Gradle sourceSets로 구현하고, RRO와의 차이를 표로 못 박는다.

## Step 1. 오버레이 소스 디렉토리 생성

🖱 프로젝트 루트에 `sro_overlay/res/values/strings.xml`:

```xml
<resources>
    <string name="hello_text">Hello from SRO (build-time)!</string>
</resources>
```

## Step 2. Gradle에 소스셋 등록

🖱 `app/build.gradle`:

```groovy
android {
    sourceSets {
        main {
            res.srcDirs += ['../sro_overlay/res']   // 뒤에 오는 디렉토리가 우선
        }
    }
}
```

## Step 3. 빌드 → 확인

Sync → Run.

✅ **예상 결과:** 앱 문구가 SRO 버전으로 — **설치 파일 자체가 바뀐 값**을 품고 있음(런타임 스위치 없음)

## Step 4. 우선순위 교차 실험

동적 RRO(3-1)를 다시 enable:

```bat
adb shell cmd overlay enable com.example.rrooverlay
adb shell am force-stop com.example.rrotarget
```

✅ **예상 결과:** 화면은 **RRO 값** — 동적 RRO > (빌드 결과물) 우선순위 실증(🎬 ㉒ 스택)

## Step 5. 비교표 완성

| 항목 | SRO | RRO(동적) |
|---|---|---|
| 적용 시점 | 빌드 타임 | 런타임 (cmd overlay) |
| 재빌드 필요 | 예 | 아니오 |
| 타겟 수정 | APK 내용 자체가 변함 | 무수정(idmap) |
| 스위치 on/off | 불가 | 가능 |
| 대표 용도 | 제품 변형(트림) 고정값 | 지역/브랜드/실험 토글 |

# 🏁 Pass 판정 체크리스트

- [ ] SRO 문구 반영 화면 확인
- [ ] RRO enable 시 SRO를 덮는 것 확인
- [ ] 비교표 5행 완성

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| Duplicate resources 에러 | 같은 소스셋 내 중복 | srcDirs 순서로 해결되는지 확인, 파일 정리 |
| SRO가 반영 안 됨 | Sync 누락/경로 오타 | Gradle Sync 후 clean build |

# 🚗 현업 활용 포인트

💡 AOSP의 `PRODUCT/DEVICE_PACKAGE_OVERLAYS`가 SRO의 플랫폼판입니다. "고정은 SRO, 가변은 RRO" — 리소스 전략 회의에서 이 표 한 장이 결론을 냅니다.

---
*실습 D3-4 (28/36) · 다음: **D3-5 관찰 3-B 렌더링***
