#include <stdio.h>

class AAA
{
	int data;
	public:
		void foo(void)  // void foo( AAA *this=0 )
		{
			data=10;  // this->data = 10; => (*this).data = 10
			printf("AAA::foo()\n");
		}
};

int main()
{
	AAA *p = 0;
	p->foo(); // AAA::foo(p)
	return 0;
}
