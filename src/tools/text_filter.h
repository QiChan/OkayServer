#ifndef __TOOLS_TEXT_FILTER_H__
#define __TOOLS_TEXT_FILTER_H__

#include <string>
#include <vector>
#include <mutex>
#include "../common/singleton.h"

bool is_str_utf8(const char* str);
std::string mb_strtolower(const std::string& m);

class TextFilter final : public Singleton<TextFilter>
{

public:
	TextFilter() = default;
    ~TextFilter() = default;
	TextFilter(const TextFilter&) = delete;
	TextFilter& operator=(const TextFilter&) = delete;

public:
    std::string Filter(const std::string& text);
	void LoadSensitiveWordsDict();

private:
    std::vector<std::string> sensitive_chs_;
    std::vector<std::string> sensitive_ens_;
    std::vector<std::string> sensitive_rus_;

	std::mutex mutex_;      // TODO: 优化去掉锁
};

#endif
