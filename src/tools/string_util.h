#ifndef __TOOLS_STRING_UTIL_H__
#define __TOOLS_STRING_UTIL_H__

#include <string>
#include <tuple>

class StringUtil final
{
public:
    StringUtil () = default;
    ~StringUtil () = default;
    StringUtil(StringUtil const&) = delete;
    StringUtil& operator= (StringUtil const&) = delete;

public:
    static std::tuple<std::string, std::string> DivideString(const std::string& source, char gap);

    static size_t StringLen(const char* str); 
};

#endif
