#include <g.h>

struct player
{
	player() = default;

	int hp;
};


struct my_core : public g::core
{
	g::net::host<player> host;

	virtual bool initialize()
	{
		host.listen(1337);
	
		host.on_connection = [&](int sock, player& p) {
			std::cout << "player" << sock << " connected.\n";
			p.hp = 100;
		};

		host.on_disconnection = [&](int sock, player& p) {
			std::cout << "player" << sock << " disconnected\n";
		};

		host.on_packet = [&](int sock, player& p) -> int {
			char buf[128] = {};
			read(sock, buf, sizeof(buf));
			std::cout << "Player" << sock << " sent: " << std::string(buf) << std::endl;

			return 0;
		};

		return true;
	}

	virtual void update(float dt)
	{
		host.update();
	}
};


int main (int argc, const char* argv[])
{
	my_core server;

	server.start({});

	return 0;
}