#ifndef _Land1_app_h_
#define _Land1_app_h_
#include "../DefaultApp/DefaultApp.h"
#include "Land1.h"


/******************************************
* Land1_app 
*
*/
class Land1_app : public DefaultApp
{
public:
			Land1_app();
			~Land1_app();

protected:
	void	on__load_assets () final;
	void	on__handle_input () final;
	void	on__render() final;
	void 	on__cleanup() final;

private:
	Land1	land;
};

#endif //_Land1_app_h_
