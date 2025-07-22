class JniFuncMain
{
	static
	{
		System.loadLibrary( "jnifunc" );  // libjnifunc.so
	}

	public static native JniTest createJniObject();

	public static void main( String args[] )
	{
		System.out.println("Hello java");
		JniTest jniObj = createJniObject();
	}
};

class JniTest
{
	public JniTest( int num )
	{
		System.out.println("JniTest.JniTest() , num=" + num );
	}
};
