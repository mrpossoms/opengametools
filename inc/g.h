#pragma once

#define MTYPE float
#include <xmath.h>

namespace g {

struct core
{
	struct opts
	{

	};


	virtual bool initialize () { return true; }

	virtual void update (float dt) { }

	void start(const core::opts& opts);

	bool running = false;
};


} // namespace g
