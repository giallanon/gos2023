#ifndef _gosEngine_line3d_h_
#define _gosEngine_line3d_h_
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
	class Engine; //fwd


	/**************
	* @brief	
	* 
	* 
	*/
	class Line3DCtx
	{
	public:
		void	reset();
		u16		point_add (const vec3f &p);
		u16		point_add (f32 x, f32 y, f32 z)											{ return point_add (vec3f(x, y, z)); }

		void	line_begin();
		void	line_add_point (f32 x, f32 y, f32 z)									{ line_add_point (vec3f(x, y, z)); }
		void	line_add_point (const vec3f &p)											{ const u16 point_index = point_add(p); line_add_point(point_index); }
		void	line_add_point (u16 point_index);
		void	line_end();

		//utils
		void	line (f32 p1x, f32 p1y, f32 p1z, f32 p2x, f32 p2y, f32 p2z)				{ line_begin(); line_add_point(p1x, p1y, p1z); line_add_point(p2x, p2y, p2z); line_end(); }
		void	line (const vec3f &p1, const vec3f &p2)									{ line (p1.x, p1.y, p1.z, p2.x, p2.y, p2.z); }


		//render
		void	fillCommandBuffer (gos::gpu::pipe2::CmdBufferWriter2 &cw);

	protected:
				Line3DCtx();
				~Line3DCtx()													{ unsetup(); }
		void	setup (gos::Allocator *allocator, u16 estimated_num_point);
		void	unsetup ();

	private:
		enum class eCMD : u16
		{
			line_def	= 0x0001,
		};

	private:
		gos::Allocator		*allocator;
		FastArray<vec3f>	pointList;
		FastArray<u16>		program;
		u32					line_started_at;


	friend Engine;
	};

} //namespace gos


#endif //_gosEngine_line3d_h_


