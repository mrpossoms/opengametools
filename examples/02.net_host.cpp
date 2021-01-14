#include <g.h>

struct player
{
	int hp;
};


struct my_core : public g::core
{
	virtual bool initialize()
	{
		g::net::host<player> host(1337);
	
		host.on_connection = [&](int sock, player& p) {
			std::cout << "player connected.\n";
			p.hp = 100;
		};

		host.on_packet = [&](int sock, player& p) -> int {
			return 0;
		};

		return true;
	}

	virtual void update(float dt)
	{

	}
};


int main (int argc, const char* argv[])
{
	my_core server;

	server.start({});

	return 0;
}