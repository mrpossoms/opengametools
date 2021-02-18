#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

#ifdef __EMSCRIPTEN__
const std::string vs_tex_src =
"attribute vec3 a_position;"
"attribute vec2 a_uv;"
"attribute vec3 a_normal;"
"uniform mat4 u_model;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"varying lowp vec2 v_uv;"
"void main (void) {"
"v_uv = a_uv;"
"gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
"}";

const std::string fs_tex_src =
"varying lowp vec2 v_uv;"
"uniform sampler2D u_tex;"
"void main (void) {"
"gl_FragColor = texture2D(u_tex, v_uv);"
"}";

const std::string fs_white_src =
"void main (void) {"
"gl_FragColor = vec4(1.0);"
"}";
#else
const std::string vs_tex_src =
"attribute vec3 a_position;"
"attribute vec2 a_uv;"
"attribute vec3 a_normal;"
"uniform mat4 u_model;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"varying vec2 v_uv;"
"void main (void) {"
"v_uv = a_uv;"
"gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
"gl_PointSize = 2.0/(gl_Position.z);"
"}";

const std::string fs_tex_src =
"varying vec2 v_uv;"
"uniform sampler2D u_tex;"
"void main (void) {"
"gl_FragColor = texture2D(u_tex, v_uv);"
"}";

const std::string fs_white_src =
"void main (void) {"
"gl_FragColor = vec4(1.0);"
"}";
#endif


struct voxels : public g::core
{
	g::asset::store assets;

	g::gfx::shader basic_shader, star_shader;
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::gfx::mesh<g::gfx::vertex::pos> stars;
	g::game::camera cam;

	virtual bool initialize()
	{
		{ // graphics init
			basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_tex_src)
												   .add_src<GL_FRAGMENT_SHADER>(fs_tex_src)
												   .create();
			star_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_tex_src)
												   .add_src<GL_FRAGMENT_SHADER>(fs_white_src)
												   .create();
			plane = g::gfx::mesh_factory::plane();


			std::vector<g::gfx::vertex::pos> star_verts;
			for (int i = 0; i < 10000; i++)
			{
				vec<3> pos = { rand() % 128 - 64.f, rand() % 128 - 64.f, rand() % 128 - 64.f};
				star_verts.push_back({pos.unit() * 100});
			}
			stars = g::gfx::mesh_factory::empty_mesh<g::gfx::vertex::pos>().set_vertices(star_verts);

			glEnable(GL_PROGRAM_POINT_SIZE);
		}

		return true;
	}

	virtual void update(float dt)
	{

		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) return;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) return;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) return;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) return;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) return;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) return;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) return;

		cam.position = {0, 0, 0};
		// cam.orientation = quat::from_axis_angle({0, 0, 1}, 0);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// auto model = mat4::scale({1000, 1000, 1000}) * mat4::translation({0, 0, -10.1});

		// plane.using_shader(basic_shader)
		// .set_camera(cam)
		// ["u_model"].mat4(model)
		// ["u_tex"].texture(assets.tex("nebula.png"))
		// .draw<GL_TRIANGLE_FAN>();

	}
};


int main (int argc, const char* argv[])
{
	voxels game;

	game.start({
		"voxels",
		{ true, 512, 512 }
	});

	return 0;
}
