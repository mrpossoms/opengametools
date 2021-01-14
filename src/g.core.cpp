#include "g.h"

void g::core::start(const core::opts& opts)
{
	if (!initialize()) { return; }

	running = true;

	while (running)
	{
		auto dt = 0.f;

		update(dt);
	}
}