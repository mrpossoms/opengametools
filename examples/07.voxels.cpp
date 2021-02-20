#include "g.h"
#include <mutex>
#define OGT_VOX_IMPLEMENTATION
#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ogt_vox.h>
#include <ogt_voxel_meshify.h>

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
"attribute vec3 a_normal;"
"attribute vec4 a_color;"
"uniform mat4 u_model;"
"uniform mat4 u_view;"
"uniform mat4 u_proj;"
"varying vec4 v_color;"
"void main (void) {"
"v_color = a_color * vec4(1.0/254.0);"
"gl_Position = u_proj * u_view * u_model * vec4(a_position * 0.5, 1.0);"
"gl_PointSize = 2.0/(gl_Position.z);"
"}";

const std::string fs_tex_src =
"varying vec4 v_color;"
"void main (void) {"
"gl_FragColor = v_color;"
"}";

const std::string fs_white_src =
"void main (void) {"
"gl_FragColor = vec4(1.0);"
"}";
#endif


struct voxels : public g::core
{
	g::asset::store assets;

	g::gfx::shader basic_shader;
	g::gfx::mesh<g::gfx::vertex::pos_norm_color> temple;
	g::game::camera cam;
	float t;

	virtual bool initialize()
	{
		{ // graphics init
			basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_tex_src)
												   .add_src<GL_FRAGMENT_SHADER>(fs_tex_src)
												   .create();
		}

		temple = g::gfx::mesh_factory::from_voxels<g::gfx::vertex::pos_norm_color>(assets.vox("temple.vox"),
		[](ogt_mesh_vertex* v) {
			return g::gfx::vertex::pos_norm_color{
				{ v->pos.x, v->pos.y, v->pos.z },
				{ v->normal.x, v->normal.y, v->normal.z },
				{ v->color.r, v->color.g, v->color.b, v->color.a },
			};
		});

		return true;
	}

	virtual void update(float dt)
	{

		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * -dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * -dt;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) return;

		// cam.orientation = quat::from_axis_angle({0, 0, 1}, 0);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto model = mat4::translation({-10, -10, -20});

		temple.using_shader(basic_shader)
		.set_camera(cam)
		["u_model"].mat4(model)
		.draw<GL_TRIANGLES>();

	}
};


int main (int argc, const char* argv[])
{
	voxels game;

	game.start({
		"voxels",
		{ true, 1024, 768 }
	});

	return 0;
}
