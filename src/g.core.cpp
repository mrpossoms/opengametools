#include "g.h"
#include <chrono>

void g::core::start(const core::opts& opts)
{
	if (!initialize()) { return; }

	running = true;

	auto t_1 = std::chrono::system_clock::now();

	while (running)
	{
		auto t_0 = std::chrono::system_clock::now();
		std::chrono::duration<float> dt = t_0 - t_1;

		update(dt.count());
		t_1 = t_0;
	}
}
