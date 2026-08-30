# [D3-3] Lab 3-2 — RRO 2차: Drawable(이미지·배경) 오버레이

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 3 Ch 8 · 필수 | Lab 3-1 완료 상태 |

> 🎯 **실습 목표** — 문자열을 넘어 **drawable(gradient 배경·아이콘)** 까지 오버레이해, "리소스면 무엇이든 바꿀 수 있다"를 확인한다.

## Step 1. 타겟에 drawable 공개 추가

🖱 `RROTarget` overlayable.xml에 항목 추가 → 재빌드·재설치:

```xml
<item type="drawable" name="background_shape"/>
<item type="drawable" name="ic_target_icon"/>
```

## Step 2. 오버레이에 동일명 drawable 제작

🖱 `RROOverlay/res/drawable/background_shape.xml`:

```xml
<shape xmlns:android="http://schemas.android.com/apk/res/android">
    <gradient android:startColor="#1B5E20" android:endColor="#66BB6A"
              android:angle="45"/>
    <corners android:radius="24dp"/>
</shape>
```

🖱 `ic_target_icon.xml` 도 다른 색/모양의 벡터로 교체(내용 자유) → 재빌드·재설치

## Step 3. 재활성화 → 확인

```bat
adb shell cmd overlay enable com.example.rrooverlay
adb shell am force-stop com.example.rrotarget
```

✅ **예상 결과:** 배경이 초록 그라데이션 + 아이콘 교체 — 문자열 오버레이(3-1)와 **같은 메커니즘, 다른 타입**

💡 확인 퀴즈: 오버레이 APK를 업데이트했는데 반영이 안 되면? → idmap 재생성을 위해 **disable→enable 재왕복** + 타겟 재시작.

# 🏁 Pass 판정 체크리스트

- [ ] drawable 2종(배경/아이콘) 교체 화면 확인
- [ ] overlayable에 drawable 항목 추가 없이 시도 → 미적용 확인(대조 실험)
- [ ] "오버레이 갱신 시 재왕복" 절차 숙지

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 문자열은 되는데 그림만 안 바뀜 | drawable 미공개 | Step 1 항목 + 타겟 재설치 확인 |
| Resources$NotFoundException | 타입/이름 불일치 | 타겟과 동일 type·name 유지 |

# 🚗 현업 활용 포인트

💡 시즌 테마·다크 전용 에셋·지역 아이콘 교체가 전부 이 패턴입니다. 디자이너 산출물(drawable)만 갈아끼우는 **무코드 배포 파이프라인**의 기초.

---
*실습 D3-3 (27/36) · 다음: **D3-4 Lab 3-3 SRO***
