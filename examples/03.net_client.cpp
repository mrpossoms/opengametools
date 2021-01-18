#include <g.h>


struct my_core : public g::core
{
	g::net::client client;

	virtual bool initialize()
	{
		client.on_disconnection = [&](int sock) {
			std::cout << "you have been disconnected\n";
		};

		client.on_packet = [&](int sock) -> int {
			char buf[128] = {};
			read(sock, buf, sizeof(buf));
			std::cout << " sent: " << std::string(buf) << std::endl;

			return 0;
		};

		client.connect("127.0.0.1", 1337);

		return true;
	}

	virtual void update(float dt)
	{
		// client.update();
	}
};


int main (int argc, const char* argv[])
{
	my_core client;

	client.start({});

	return 0;
}
