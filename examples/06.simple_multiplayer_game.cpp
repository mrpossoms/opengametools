#include "g.h"
#include <mutex>

using namespace xmath;
using mat4 = xmath::mat<4,4>;

const std::string vs_src =
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
"}";

const std::string fs_src =
"varying vec2 v_uv;"
"uniform sampler2D u_tex;"
"void main (void) {"
"gl_FragColor = texture2D(u_tex, v_uv);"
"}";

struct player_commands : g::net::msg
{
	float ang_vel;
	vec<2> thrust;
	uint8_t shooting;

	void to_network() {  }
	void to_machine() {  }
};

struct mover
{
	vec<2> position, velocity;

	void update(float dt)
	{
		position += velocity * dt;
	}

	void to_network() {  }
	void to_machine() {  }
};

struct player_info
{
	uint8_t index;
};

struct player : public mover
{
	float angle;
	uint8_t hp;
	uint8_t shooting;

	void to_network() {  }
	void to_machine() {  }
};

struct game_state_hdr
{
	uint8_t your_idx;
	uint8_t player_count;
	uint8_t bullet_count;

	void to_network() {  }
	void to_machine() {  }
};

struct game_state
{
	g::bounded_list<player, 10> players;
	g::bounded_list<mover, 100> bullets;
};

struct zappers : public g::core
{
	std::mutex state_lock;
	bool is_host = false;
	int my_index = 0;
	std::unordered_map<int, player_commands> commands;
	game_state state;

	g::net::client client;
	g::net::host<player_info> host;

	g::gfx::shader basic_shader;
	g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
	g::gfx::texture player_tex, bg_tex;
	g::game::camera cam;

	virtual bool initialize()
	{
		{ // graphics init
			basic_shader = g::gfx::shader_factory{}.add_src<GL_VERTEX_SHADER>(vs_src)
												   .add_src<GL_FRAGMENT_SHADER>(fs_src)
												   .create();
			plane = g::gfx::mesh_factory::plane();
			player_tex = g::gfx::texture_factory{}.from_png("data/tex/ship.png").pixelated().create();
			bg_tex = g::gfx::texture_factory{}.from_png("data/tex/nebula.png").pixelated().create();
		}

		{ // server behaviors
			host.on_connection = [&](int sock, player_info& p) {
				std::cout << "player" << sock << " connected.\n";
				p.index = state.players.size();
				state.players.push_back({});
			};

			host.on_disconnection = [&](int sock, player_info& p) {
				std::cout << "player" << sock << " disconnected\n";
			};

			host.on_packet = [&](int sock, player_info& p) -> int {
				player_commands msg;
				auto bytes = read(sock, &msg, sizeof(msg));
				msg.to_machine();

				commands[p.index] = msg;

				return 0;
			};
		}

		{ // client behaviors
			client.on_disconnection = [&](int sock) {
				std::cout << "you have been disconnected\n";
			};

			client.on_packet = [&](int sock) -> int {
				state_lock.lock();
				game_state_hdr msg;
				read(sock, &msg, sizeof(msg));
				msg.to_machine();
				my_index = msg.your_idx;

				// read all players
				state.players.clear();
				for (auto i = 0; i < msg.player_count; i++)
				{
					player p;
					read(sock, &p, sizeof(p));
					p.to_machine();
					state.players.push_back(p);
				}

				// read all bullets
				state.bullets.clear();
				for (auto i = 0; i < msg.bullet_count; i++)
				{
					mover b;
					read(sock, &b, sizeof(b));
					b.to_machine();
					state.bullets.push_back(b);
				}

				state_lock.unlock();

				return 0;
			};
		}

		return true;
	}

	virtual void update(float dt)
	{
		if (is_host)
		{
			for (int i = 0; i < state.players.size(); i++)
			{
				auto& player = state.players[i];
				auto q = quat::from_axis_angle({0, 0, 1}, player.angle);
				auto thrust = q.rotate({commands[i].thrust[0], commands[i].thrust[1], 0}) * dt;
				player.velocity += thrust.slice<2>(0);
				player.angle += commands[i].ang_vel * dt;
				player.update(dt);
			}

			for (auto& player : host.sockets)
			{
				int sock = player.first;
				game_state_hdr hdr = {
					player.second.index,
					(uint8_t)state.players.size(),
					(uint8_t)state.bullets.size(),
				};

				write(sock, &hdr, sizeof(hdr));

				for (auto i = 0; i < state.players.size(); i++)
				{
					auto p = state.players[i];
					write(sock, &p, sizeof(p));
					p.to_machine();
				}

				for (auto i = 0; i < state.bullets.size(); i++)
				{
					auto p = state.bullets[i];
					write(sock, &p, sizeof(p));
					p.to_machine();
				}
			}
		}
		else
		{
			if (!client.is_connected)
			{
				std::cerr << "Connecting...\n";

				if (client.connect("127.0.0.1", 1337))
				{
					std::cerr << "connected!\n";
					client.listen();				
				}
				else
				{
					std::cerr << "connection failure, switching to host\n";
					is_host = true;
					host.listen(1337);
					state.players.push_back({});
				}
			}
		}

		player_commands cmd = {};
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cmd.thrust += { 0,  1 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cmd.thrust += { 0, -1 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cmd.thrust += {-1,  0 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cmd.thrust += { 1,  0 };
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cmd.ang_vel = 1;
		if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cmd.ang_vel = -1;

		if (!is_host)
		{
			write(client.socket, &cmd, sizeof(cmd));
		}
		else
		{
			commands[0] = cmd;
		}


		state_lock.lock();

		cam.position = {-state.players[my_index].position[0], -state.players[my_index].position[1], 0};
		cam.orientation = quat::from_axis_angle({0, 0, 1}, state.players[my_index].angle);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto model = mat4::scale({100, 100, 100}) * mat4::translation({0, 0, -10.1});
		plane.using_shader(basic_shader)
		.set_camera(cam)
		["u_model"].mat4(model)
		["u_tex"].texture(bg_tex)
		.draw<GL_TRIANGLE_FAN>();

		for (auto player : state.players)
		{
			auto model = (mat4::rotation({0, 0, 1}, player.angle) * mat4::scale({0.25, 0.25, 0.25})) * mat4::translation({player.position[0], player.position[1], -1});
			plane.using_shader(basic_shader)
			.set_camera(cam)
			["u_model"].mat4(model)
			["u_tex"].texture(player_tex)
			.draw<GL_TRIANGLE_FAN>();			
		}

		state_lock.unlock();
	}
};


int main (int argc, const char* argv[])
{
	zappers game;

	game.start({});

	return 0;
}