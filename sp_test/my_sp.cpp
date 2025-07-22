#if 1
#include <stdio.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//---------------------------------------------------------------

using namespace android;

class AAA : public RefBase
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

int main()
{
	{
		wp<AAA> p = new AAA();
		{
			sp<AAA> q = p.promote();
		}
		printf("step 1\n");
		sp<AAA> p1 = p.promote();

		if( p1 != 0 )
			    p1->foo();
		else
			    printf("대상 객체가 이미 소멸됨\n");
	}
	printf("step 2\n");
	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//---------------------------------------------------------------

using namespace android;

class AAA : public RefBase
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

int main()
{
	{
		wp<AAA> p = new AAA();
		{
			sp<AAA> q = p.promote();
		}
		printf("step 1\n");
		p.promote()->foo();
	}
	printf("step 2\n");
	return 0;
}
#endif

#if 0
#include <stdio.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//---------------------------------------------------------------

using namespace android;

class AAA : public RefBase
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

int main()
{
	{
		wp<AAA> p = new AAA();
		//p->foo();
		p.promote()->foo();
	}
	return 0;
}
#endif
#if 0
#include <stdio.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//---------------------------------------------------------------

using namespace android;

class AAA : public RefBase
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};

int main()
{
	{
		wp<AAA> p = new AAA();
		{
			sp<AAA> q = p.promote();
		}
		printf("step 1\n");
	}
	printf("step 2\n");
	return 0;
}
#endif

#if 0
#include <stdio.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//---------------------------------------------------------------

using namespace android;

class AAA;
class BBB;

class AAA : public RefBase
{
	public : 
		wp<BBB> pb;
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
};
class BBB : public RefBase
{
	public : 
		wp<AAA> pa;
		BBB() {printf("BBB::BBB()\n");}
		void foo() { printf("BBB::foo()\n"); }
		~BBB() {printf("BBB::~BBB()\n");}
};

int main()
{
	{
		sp<AAA> p = new AAA();
		sp<BBB> q = new BBB();
		p->pb = q;
		q->pa = p;
	}
	printf("after\n");
	return 0;
}
#endif

#if 0
#include <stdio.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
//---------------------------------------------------------------

using namespace android;

class AAA : public RefBase
{
	public : 
		AAA() {printf("AAA::AAA()\n");}
		void foo() { printf("AAA::foo()\n"); }
		~AAA() {printf("AAA::~AAA()\n");}
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
