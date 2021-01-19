#include "g.h"
#include <chrono>

void g::core::start(const core::opts& opts)
{
	if (!initialize()) { return; }

	running = true;

    auto start = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();

	while (running)
	{
		auto dt = (now - start).count();

		update(dt);
	}
}
