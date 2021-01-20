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