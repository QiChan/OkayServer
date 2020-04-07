#include "string_util.h"
#include <cstring>

std::tuple<std::string, std::string> StringUtil::DivideString(const std::string& source, char gap)
{
	auto idx = source.find(gap);
	std::string fir;
	std::string sec;
	if (idx != std::string::npos)
	{
		fir = source.substr(0, idx);
		sec = source.substr(idx + 1);
	}
	else
	{
		fir = source;
	}
	return make_tuple(fir, sec);
}

size_t StringUtil::StringLen(const char* str)
{
    if (str == nullptr)
    {
        return 0;
    }

    return strlen(str);
}
