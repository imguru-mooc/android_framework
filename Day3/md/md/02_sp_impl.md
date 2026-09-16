# 실습 2. `sp<>` 직접 구현 4단계 — raw delete → static 카운트 → 객체 내 카운트 → RefBase 분리

> **소요시간:** 25분 · **난이도:** ★★☆ · **챕터:** Ch 1 libutils
> **디렉토리:** `~/android/day3/sp_test/sp.cpp` (`#if` 로 단계 전환)

## 목표

- `sp<T>` 가 없는 상태에서 시작해 Android `RefBase` 구조에 스스로 도달한다.
- "참조 카운트를 **객체가** 가져야 한다(intrusive)" 는 것을 실패 사례로 체험한다.

---

## 단계 ① — sp 소멸자에서 delete

```cpp
#include <stdio.h>
class AAA {
public:
    AAA()  { printf("AAA::AAA()\n"); }
    void foo() { printf("AAA::foo()\n"); }
    ~AAA() { printf("AAA::~AAA()\n"); }
};

template <typename T>
class sp {
    T* mPtr;
public:
    sp(T* ptr) : mPtr(ptr) {}
    T* operator->() { return mPtr; }
    T& operator*()  { return *mPtr; }
    ~sp() { delete mPtr; }                 // ★ 소멸 시 delete
};

int main() {
    {
        sp<AAA> p = new AAA();
        p->foo();
    }
    printf("after\n");
    return 0;
}
```

동작한다. 그런데 **복사**하면?

```cpp
sp<AAA> p = new AAA();
sp<AAA> q = p;        // 두 sp 가 같은 객체 → 소멸자 2번 → double free (crash)
```

## 단계 ② — static 카운트

```cpp
template <typename T>
class sp {
    T* mPtr;
    static int mRefs;                      // ★ 클래스 전체 공유
public:
    sp(T* ptr) : mPtr(ptr)          { mRefs++; }
    sp(const sp<T>& r) : mPtr(r.mPtr) { mRefs++; }
    T* operator->() { return mPtr; }
    ~sp() { if (--mRefs == 0) delete mPtr; }
};
template <typename T> int sp<T>::mRefs = 0;
```

`sp<AAA> p = new AAA(); sp<AAA> q = p;` 는 통과. 그러나 **객체가 두 개**면?

```cpp
sp<AAA> a = new AAA();   // mRefs 1
sp<AAA> b = new AAA();   // mRefs 2  ← 다른 객체인데 같은 카운트
// a 소멸 → 1, b 소멸 → 0 → delete b 만. a 는 누수
```

카운트가 **객체마다** 있어야 한다.

## 단계 ③ — 객체 안에 카운트

```cpp
class AAA {
    int mRefs;
public:
    AAA() : mRefs(0) { printf("AAA::AAA()\n"); }
    void foo() { printf("AAA::foo()\n"); }
    void incStrong() { mRefs++; }
    void decStrong() { if (--mRefs == 0) delete this; }   // ★ 자기 자신을 delete
    ~AAA() { printf("AAA::~AAA()\n"); }
};

template <typename T>
class sp {
    T* mPtr;
public:
    sp(T* ptr) : mPtr(ptr)            { mPtr->incStrong(); }
    sp(const sp<T>& r) : mPtr(r.mPtr) { mPtr->incStrong(); }
    T* operator->() { return mPtr; }
    T& operator*()  { return *mPtr; }
    ~sp() { mPtr->decStrong(); }
};
```

정확히 동작한다. 하지만 `BBB`, `CCC` … 모든 클래스에 `mRefs / incStrong / decStrong` 을 반복해야 한다.

## 단계 ④ — RefBase 로 분리 (Android 구조)

```cpp
#include <stdio.h>

class RefBase {
    int mRefs;
public:
    RefBase() : mRefs(0) { printf("RefBase::RefBase()\n"); }
    void incStrong() { mRefs++; }
    void decStrong() { if (--mRefs == 0) delete this; }
    virtual ~RefBase() { printf("RefBase::~RefBase()\n"); }   // ★ virtual — delete this 가 파생 소멸자를 부르도록
};

class AAA : public RefBase {
public:
    AAA()  { printf("AAA::AAA()\n"); }
    void foo() { printf("AAA::foo()\n"); }
    ~AAA() { printf("AAA::~AAA()\n"); }
};

template <typename T>
class sp {
    T* mPtr;
public:
    sp(T* ptr) : mPtr(ptr)            { mPtr->incStrong(); }
    sp(const sp<T>& r) : mPtr(r.mPtr) { mPtr->incStrong(); }
    T* operator->() { return mPtr; }
    T& operator*()  { return *mPtr; }
    ~sp() { mPtr->decStrong(); }
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

```text
RefBase::RefBase()
AAA::AAA()
AAA::~AAA()
RefBase::~RefBase()
after
```

## 빌드 · 실행

각 단계를 `#if 1 … #endif` 로 감싸 한 번에 하나만 활성화한다.

```bash
m sp_test
adb push $OUT/system/bin/sp_test /data && adb shell chmod 755 /data/sp_test && adb shell /data/sp_test
```

(WinSCP 경유가 번거로우면 빌드 서버에 adb 가 있을 때 위처럼 직접 push 해도 된다.)

---

## 확인 포인트

- [ ] ① 복사 시 crash (double free) 재현
- [ ] ② 객체 2개일 때 누수 재현 (소멸자 1번만 출력)
- [ ] ③ ④ 소멸자 정확히 1번, `after` 출력
- [ ] ④ 에서 `~RefBase` 가 `virtual` 이 아니면 `AAA::~AAA()` 가 안 불림을 실험

## 핵심 정리

| 단계 | 문제 | 교훈 |
|---|---|---|
| ① | 복사 시 double free | 소유권을 세어야 한다 |
| ② | 객체 간 카운트 공유 | 카운트는 **객체마다** |
| ③ | 클래스마다 반복 | 공통 부모로 뽑는다 |
| ④ | — | `RefBase` + `sp` = Android 방식 (**intrusive** 참조 카운트) |

`std::shared_ptr` 은 카운트를 별도 control block 에 두는 non-intrusive 방식이다. Android 가 intrusive 를 택한 이유: Binder 객체처럼 `this` 를 여기저기 넘겨야 하는 상황에서 카운트가 객체 안에 있어야 안전하게 `sp` 로 다시 감쌀 수 있다.
