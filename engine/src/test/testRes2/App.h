#ifndef _App_h_
#define _App_h_
#include "defaultApp/gosDefaultApp.h"


/******************************************
* App
*
*/
class App : public gos::engine::DefaultApp
{
public:
			App();

protected:
	void	on__setup () final;
	void 	on__unsetup() final;
	void	on__render() final;

private:
	gos::engine::Renderer_PIPE3			*renderer_PIPE3;

	gos::VtxBufferH		vtxbuffer_handle;
	gos::IdxBufferH		idxbuffer_handle;
	gos::TextureH		texture1_handle;
	
};

#endif //_App_h_
