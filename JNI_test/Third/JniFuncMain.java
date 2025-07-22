class JniFuncMain
{
	static
	{
		System.loadLibrary( "jnimap" );  // libjnimap.so
	}

	public native void  foo();

	public static void main( String args[] )
	{
		System.out.println("Hello java");
		JniFuncMain obj = new JniFuncMain();
		obj.foo();
	}
};

