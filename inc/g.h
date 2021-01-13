#pragma once

#define MTYPE float
#include <xmath.h>

namespace g {

struct core
{
	struct opts
	{

	};


	virtual bool initalize () { return true; }

	virtual void update (float dt) { }

	start(const core::opts& opts);
	{
		if (!initialize()) { return; }

		while (running)
		{
			auto dt = 0.f;

			update(dt);
		}
	}

	bool running = false;
};


} // namespace g
