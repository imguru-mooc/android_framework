# 실습 3. Android `RefBase` · `wp<>` · `promote()` · 순환 참조

> **소요시간:** 20분 · **난이도:** ★★☆ · **챕터:** Ch 1 libutils
> **디렉토리:** `~/android/day3/sp_test/my_sp.cpp` (`#include <utils/RefBase.h>`)

## 목표

- 실습 2 의 자작 `sp` 를 Android 실제 `utils/RefBase.h` 로 바꿔 같은 동작을 확인한다.
- `wp<>` 는 카운트를 올리지 않는다 · `promote()` 는 살아 있을 때만 `sp` 를 준다.
- 순환 참조를 만들고 `wp` 로 푼다.

---

## ① Android sp — 실습 2 ④ 와 동일

```cpp
#include <stdio.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
using namespace android;

class AAA : public RefBase {
public:
    AAA()  { printf("AAA::AAA()\n"); }
    void foo() { printf("AAA::foo()\n"); }
    ~AAA() { printf("AAA::~AAA()\n"); }
};

int main() {
    {
        sp<AAA> p = new AAA();
        sp<AAA> q = p;
    }
    printf("after\n");
    return 0;
}
```

## ② wp 만 있으면 즉시 소멸

```cpp
int main() {
    {
        wp<AAA> p = new AAA();     // ★ 강한 참조 0 → 생성 직후 소멸
        // p->foo();               // wp 는 -> 가 없다 (컴파일 오류)
        sp<AAA> q = p.promote();   // 이미 죽었으므로 nullptr
        if (q == nullptr) printf("promote failed — 객체 없음\n");
    }
    return 0;
}
```

## ③ sp 가 살아 있는 동안만 promote 성공

```cpp
int main() {
    {
        sp<AAA> owner = new AAA();     // 강한 참조 1
        wp<AAA> p = owner;             // 약한 참조 — 카운트 무관
        {
            sp<AAA> q = p.promote();   // 성공 (강한 2)
            if (q != nullptr) q->foo();
        }                              // 강한 1
        printf("step 1\n");
        owner.clear();                 // 강한 0 → 소멸
        sp<AAA> r = p.promote();       // ★ nullptr
        if (r != nullptr) r->foo(); else printf("대상 객체가 이미 소멸됨\n");
    }
    printf("step 2\n");
    return 0;
}
```

```text
AAA::AAA()
AAA::foo()
step 1
AAA::~AAA()
대상 객체가 이미 소멸됨
step 2
```

## ④ 순환 참조 — sp ↔ sp

```cpp
class AAA; class BBB;
class AAA : public RefBase {
public:
    sp<BBB> pb;                     // ★ 강한
    AAA()  { printf("AAA::AAA()\n"); }
    ~AAA() { printf("AAA::~AAA()\n"); }
};
class BBB : public RefBase {
public:
    sp<AAA> pa;                     // ★ 강한
    BBB()  { printf("BBB::BBB()\n"); }
    ~BBB() { printf("BBB::~BBB()\n"); }
};

int main() {
    {
        sp<AAA> p = new AAA();
        sp<BBB> q = new BBB();
        p->pb = q;                  // A → B
        q->pa = p;                  // B → A
    }                               // p, q 소멸 — 하지만 서로가 잡고 있어 카운트 1 유지
    printf("after\n");              // ★ 소멸자가 안 찍힌다 = 누수
    return 0;
}
```

## ⑤ 한쪽을 wp 로

```cpp
class BBB : public RefBase {
public:
    wp<AAA> pa;                     // ★ 약한 — 순환 끊김
    ...
    void use() { sp<AAA> a = pa.promote(); if (a != nullptr) a->foo(); }
};
```

```text
AAA::AAA()
BBB::BBB()
AAA::~AAA()
BBB::~BBB()
after
```

## 빌드 · 실행

```bash
m my_sp_test
adb push $OUT/system/bin/my_sp_test /data && adb shell chmod 755 /data/my_sp_test && adb shell /data/my_sp_test
```

---

## 확인 포인트

- [ ] ② `promote failed`, ③ `대상 객체가 이미 소멸됨`
- [ ] ④ `after` 만 찍히고 소멸자 없음 → ⑤ 에서 둘 다 소멸

## 핵심 정리

| API | 의미 |
|---|---|
| `sp<T>` | 강한 참조. 생성/복사 `incStrong`, 소멸 `decStrong` |
| `wp<T>` | 약한 참조. 카운트 무관, `->` 없음, `promote()` 로만 접근 |
| `promote()` | 객체가 살아 있으면 `sp`, 아니면 `nullptr` — 반드시 검사 |
| `onFirstRef()` | 첫 `sp` 가 붙을 때 1회 호출 — 실습 4 의 `Thread::run` 트리거 |
| 순환 참조 | Listener · Callback · 부모↔자식 은 한쪽을 `wp` |

Day 2 의 `sp<IBinder>`, Day 3 의 `sp<SurfaceControl>` · `sp<Looper>` · `sp<Thread>` 가 전부 이 `RefBase` 다. `delete` 를 쓰지 않는 이유가 여기 있다.
