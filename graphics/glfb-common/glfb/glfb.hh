/*
	glfb.hh
	-------
*/

#ifndef GLFB_GLFB_HH
#define GLFB_GLFB_HH


namespace glfb
{
	
	extern bool overlay_enabled;
	
	void initialize();
	
	void set_dimensions( int width, int height );
	
	void blit( const void* src_addr );
	
	void terminate();
	
}

#endif
