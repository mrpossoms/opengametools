#include <g.h>

const std::string vs_src =
"attribute vec3 a_position;"
"attribute vec2 a_uv;"
"attribute vec3 a_normal;"
"varying vec2 v_uv;"
"void main (void) {"
"v_uv = a_uv;"
"gl_Position = vec4(a_position * 0.5, 1.0);"
"}";

const std::string fs_src =
"varying vec2 v_uv;"
"void main (void) {"
"gl_FragColor = vec4(v_uv.xy, 0.0, 1.0);"
"}";


struct my_core : public g::core
{
	g::gfx::shader basic_shader;
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;

	virtual bool initialize()
	{
		std::cout << "initialize your game state here.\n";

		basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
											   .add_src<GL_FRAGMENT_SHADER>(fs_src)
											   .create();

		plane = g::gfx::mesh_factory::plane();

		return true;
	}

	virtual void update(float dt)
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		plane.using_shader(basic_shader).draw_tri_fan();
	}
};


int main (int argc, const char* argv[])
{
	my_core core;

	core.start({});

	return 0;
}
