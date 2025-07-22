class Hello
{
	native void foo();
	public static void main( String args[] )
	{
		System.out.println("Hello java");
		System.loadLibrary( "Hello" );  // libHello.so
		Hello hello = new Hello();
		hello.foo();
	}
};
