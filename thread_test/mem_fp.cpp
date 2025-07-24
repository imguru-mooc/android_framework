#if 1
#include <stdio.h>
class AAA
{
	int mData;
	public:
		static void* foo(void* data)   // void* foo(void*)
		{
			AAA *self = (AAA*)data;
			self->mData=10;  // self->data = 10;
			printf("AAA::foo()\n");
		}
};

int main()
{
	void* (*fp)(void*) =  &AAA::foo;
	AAA a;
	(*fp)(&a);  
	return 0;
}
#endif

#if 0
#include <stdio.h>
class AAA
{
	int data;
	public:
		static void* foo(void*)   // void* foo(void*)
		{
			data=10;  // this->data = 10;
			printf("AAA::foo()\n");
		}
};

int main()
{
	void* (*fp)(void*) =  &AAA::foo;
	(*fp)(0);  
	return 0;
}
#endif
#if 0
#include <stdio.h>
class AAA
{
	public:
		void foo(void)   // void foo(AAA *this)
		{
			printf("AAA::foo()\n");
		}
};

int main()
{
	void (AAA::*fp)(void) =  &AAA::foo;
	AAA a;
	(a.*fp)();  // a.foo() => AAA::foo(&a)
	return 0;
}
#endif
