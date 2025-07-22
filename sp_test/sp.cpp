#if 1
#include <stdio.h>
class RefBase
{
	int mRefs;
	public : 
		RefBase():mRefs(0) {printf("RefBase::RefBase()\n");}
		void incStrong() { mRefs++; }
		void decStrong() { if(--mRefs == 0 ) delete this; }
		virtual ~RefBase() {printf("RefBase::~RefBase()\n");}
};
//---------------------------------------------------------------
class AAA : public RefBase
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

template < typename T >
class sp
{
	T *mPtr;
	public:
		sp( T *ptr ) : mPtr(ptr) 
	    {
			mPtr->incStrong();
		}
		sp( const sp<T> &r ) : mPtr(r.mPtr) 
	    {
			mPtr->incStrong();
		}
		T *operator->() { return mPtr; }
		T &operator*() { return *mPtr; }
		~sp(){ 
			mPtr->decStrong();
		}
};

int main()
{
	{
		sp<AAA> p = new AAA();
		sp<AAA> q = p;
	}
	printf("after\n");
	return 0;
}
#endif
#if 0
#include <stdio.h>
class AAA
{
	int mRefs;
	public : 
		AAA():mRefs(0) {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		void incStrong() { mRefs++; }
		void decStrong() { if(--mRefs == 0 ) delete this; }
		~AAA() {printf("AAA::~AAA()\n");}
};

template < typename T >
class sp
{
	T *mPtr;
	public:
		sp( T *ptr ) : mPtr(ptr) 
	    {
			mPtr->incStrong();
		}
		sp( const sp<T> &r ) : mPtr(r.mPtr) 
	    {
			mPtr->incStrong();
		}
		T *operator->() { return mPtr; }
		T &operator*() { return *mPtr; }
		~sp(){ 
			mPtr->decStrong();
		}
};

int main()
{
	{
		sp<AAA> p = new AAA();
		sp<AAA> q = p;
	}
	printf("after\n");
	return 0;
}
#endif
#if 0
#include <stdio.h>
class AAA
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

template < typename T >
class sp
{
	T *mPtr;
	static int mRefs;
	public:
		sp( T *ptr ) : mPtr(ptr) 
	    {
			mRefs++;
		}
		sp( const sp<T> &r ) : mPtr(r.mPtr) 
	    {
			mRefs++;
		}
		T *operator->() { return mPtr; }
		T &operator*() { return *mPtr; }
		~sp(){ 
			if( --mRefs == 0 )
				delete mPtr;
		}
};

template < typename T >
int sp<T>::mRefs=0;

int main()
{
	{
		sp<AAA> p = new AAA();
		sp<AAA> q = p;
	}
	printf("after\n");
	return 0;
}
#endif
#if 0
#include <stdio.h>
class AAA
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

template < typename T >
class sp
{
	T *mPtr;
	public:
		sp( T *ptr ) : mPtr(ptr) {}
		T *operator->() { return mPtr; }
		T &operator*() { return *mPtr; }
		~sp(){ delete mPtr;}
};

int main()
{
	{
		sp<AAA> p = new AAA();
		sp<AAA> q = p;
	}
	printf("after\n");
	return 0;
}
#endif
#if 0
#include <stdio.h>
class AAA
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

template < typename T >
class sp
{
	T *mPtr;
	public:
		sp( T *ptr ) : mPtr(ptr) {}
		T *operator->() { return mPtr; }
		T &operator*() { return *mPtr; }
		~sp(){ delete mPtr;}
};

int main()
{
	{
		sp<AAA> p = new AAA();
		p->foo(); // p.operator->()->foo();
		(*p).foo();
	}
	printf("after\n");
	return 0;
}
#endif
#if 0
#include <stdio.h>
class AAA
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

class sp
{
	AAA *mPtr;
	public:
		sp( AAA *ptr ) : mPtr(ptr) {}
		AAA *operator->() { return mPtr; }
		AAA &operator*() { return *mPtr; }
		~sp(){ delete mPtr;}
};

int main()
{
	{
		sp p = new AAA();
		p->foo(); // p.operator->()->foo();
		(*p).foo();
	}
	printf("after\n");
	return 0;
}
#endif



#if 0
#include <stdio.h>
class AAA
{
	public : 
		void foo() { printf("AAA::foo()\n"); }
		AAA() {printf("AAA::AAA()\n");}
		~AAA() {printf("AAA::~AAA()\n");}
};

int main()
{
	AAA *p = new AAA();
	p->foo();
	delete p;
	return 0;
}
#endif
