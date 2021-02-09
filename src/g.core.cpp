#include "g.h"
#include <chrono>

GLFWwindow* g::gfx::GLFW_WIN;

void g::core::start(const core::opts& opts)
{
	if (opts.gfx.display)
	{
		if (!glfwInit()) { throw std::runtime_error("glfwInit() failed"); }
	
		g::gfx::GLFW_WIN = glfwCreateWindow(opts.gfx.width, opts.gfx.height, opts.name ? opts.name : "", NULL, NULL);
	
		if (!g::gfx::GLFW_WIN)
		{
			glfwTerminate();
			throw std::runtime_error("glfwCreateWindow() returned NULL");
		}

		glfwMakeContextCurrent(g::gfx::GLFW_WIN);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!initialize()) { throw std::runtime_error("User initialize() call failed"); }

	running = true;

	auto t_1 = std::chrono::system_clock::now();

	while (running)
	{
		auto t_0 = std::chrono::system_clock::now();
		std::chrono::duration<float> dt = t_0 - t_1;

		update(dt.count());
		t_1 = t_0;

		if (g::gfx::GLFW_WIN)
		{
			glfwSwapBuffers(g::gfx::GLFW_WIN);
			glfwPollEvents();
			running = !glfwWindowShouldClose(g::gfx::GLFW_WIN);
		}
	}
}


void g::utils::base64_encode(void *dst, const void *src, size_t len) // thread-safe, re-entrant
{
	static const unsigned char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	assert(dst != src);
	unsigned int *d = (unsigned int *)dst;
	const unsigned char *s = (const unsigned char*)src;
	const unsigned char *end = s + len;
	
	while(s < end)
	{
		uint32_t e = *s++ << 16;
		if (s < end) e |= *s++ << 8;
		if (s < end) e |= *s++;
		*d++ = b64[e >> 18] | (b64[(e >> 12) & 0x3F] << 8) | (b64[(e >> 6) & 0x3F] << 16) | (b64[e & 0x3F] << 24);
	}
	for (size_t i = 0; i < (3 - (len % 3)) % 3; i++) ((char *)d)[-1-i] = '=';
}