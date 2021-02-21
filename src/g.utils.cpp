#include "g.utils.h"


g::utils::split::it::it(const std::string& str, std::string delim, size_t pos) : _str(str)
{
	_delim = delim;
	_pos = pos;
	_next_pos = _str.find(_delim, _pos);
}


void g::utils::split::it::operator++()
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


bool g::utils::split::it::operator!=(g::utils::split::it& i)
{
	return _pos != i._pos;
}


std::string g::utils::split::it::operator*()
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


g::utils::split::split(const std::string& str, std::string delim) : _str(str)
{
	_delim = delim;
}


g::utils::split::it g::utils::split::begin()
{
	return g::utils::split::it(_str, _delim, 0);
}


g::utils::split::it g::utils::split::end()
{
	return g::utils::split::it(_str, _delim, _str.length() - 1);
}