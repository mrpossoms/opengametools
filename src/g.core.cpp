#include "g.h"
#include <chrono>

void g::core::start(const core::opts& opts)
{
	GLFWwindow* glfw_win = nullptr;

	if (opts.gfx.display)
	{
		if (!glfwInit()) { throw std::runtime_error("glfwInit() failed"); }
	
		glfw_win = glfwCreateWindow(opts.gfx.width, opts.gfx.height, opts.name ? opts.name : "", NULL, NULL);
	
		if (!glfw_win)
		{
			glfwTerminate();
			throw std::runtime_error("glfwCreateWindow() returned NULL");
		}

		glfwMakeContextCurrent(glfw_win);
	}

	if (!initialize()) { throw std::runtime_error("User initialize() call failed"); }

	running = true;

	auto t_1 = std::chrono::system_clock::now();

	while (running)
	{
		auto t_0 = std::chrono::system_clock::now();
		std::chrono::duration<float> dt = t_0 - t_1;

		update(dt.count());
		t_1 = t_0;

		if (glfw_win)
		{
			glfwSwapBuffers(glfw_win);
			glfwPollEvents();
			running = !glfwWindowShouldClose(glfw_win);
		}
	}
}


g::split::it::it(std::string& str, std::string delim, size_t pos) : _str(str)
{
	_str = str;
	_delim = delim;
	_pos = pos;
	_next_pos = _str.find(_delim, _pos);
}


void g::split::it::operator++()
{
	_pos = _next_pos;

	if (_next_pos != std::string::npos)
	{
		_pos += _delim.size();
		_next_pos = _str.find(_delim, _pos);
	}
	else
	{
		_pos = _str.length() - 1;
	}
}


bool g::split::it::operator!=(g::split::it& i)
{
	return _pos != i._pos;
}


std::string g::split::it::operator*()
{
	size_t len = 0;

	if (_next_pos == std::string::npos)
	{
		len = _pos - _str.length() - 1;
	}
	else
	{
		len = _next_pos - _pos;
	}

	return _str.substr(_pos, len);
}


g::split::split(std::string& str, std::string delim) : _str(str)
{
	 _str = str;
	_delim = delim;
}


g::split::it g::split::begin()
{
	return g::split::it(_str, _delim, 0);
}


g::split::it g::split::end()
{
	return g::split::it(_str, _delim, _str.length() - 1);
}

std::string g::base64_encode(uint8_t const* buf, size_t len)
{
	static const std::string base64_chars =
	             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	             "abcdefghijklmnopqrstuvwxyz"
	             "0123456789+/";

	std::string ret;
	int i = 0;
	int j = 0;
	uint8_t char_array_3[3];
	uint8_t char_array_4[4];

	while (len--)
	{
		char_array_3[i++] = *(buf++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i <4) ; i++) ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++) char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];

		while((i++ < 3)) ret += '=';
	}

	return ret;
}