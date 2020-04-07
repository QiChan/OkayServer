#include "text_filter.h"
#include <fstream>
#include <streambuf>
#include <sstream>
#include <algorithm>
#include <regex>
#include "../json/json.h"

bool is_str_utf8(const char* str) 
{
	unsigned int nBytes = 0;//UFT8可用1-6个字节编码,ASCII用一个字节  
	unsigned char chr = *str;
	bool bAllAscii = true;

	for (unsigned int i = 0; str[i] != '\0'; ++i) {
		chr = *(str + i);
		//判断是否ASCII编码,如果不是,说明有可能是UTF8,ASCII用7位编码,最高位标记为0,0xxxxxxx 
		if (nBytes == 0 && (chr & 0x80) != 0) {
			bAllAscii = false;
		}

		if (nBytes == 0) {
			//如果不是ASCII码,应该是多字节符,计算字节数  
			if (chr >= 0x80) {
				if (chr >= 0xFC && chr <= 0xFD) {
					nBytes = 6;
				}
				else if (chr >= 0xF8) {
					nBytes = 5;
				}
				else if (chr >= 0xF0) {
					nBytes = 4;
				}
				else if (chr >= 0xE0) {
					nBytes = 3;
				}
				else if (chr >= 0xC0) {
					nBytes = 2;
				}
				else {
					return false;
				}
				nBytes--;
			}
		}
		else {
			//多字节符的非首字节,应为 10xxxxxx 
			if ((chr & 0xC0) != 0x80) {
				return false;
			}
			//减到为零为止
			nBytes--;
		}
	}

	//违返UTF8编码规则 
	if (nBytes != 0) {
		return false;
	}

	if (bAllAscii) { //如果全部都是ASCII, 也是UTF8
		return true;
	}

	return true;
}

std::string mb_strtolower(const std::string& m) 
{
    std::string out;
	int j = 0;
	for (int i = 0, ix = m.length(); i<ix; i += 1, j++)
	{
		unsigned char c0 = m[i];
		if (c0 >= 0 && c0 <= 127)
		{
			if ('A' <= c0 && c0 <= 'Z') { c0 += 0x20; }
			out += c0;
		}
		else if (c0 >= 192 && c0 <= 223)
		{
			unsigned char c1 = m[i + 1];

			if (c0 == 0xc4 && c1 == 0xb0) { out += 0x69; }
			else if (c0 == 0xd2 && c1 == 0x80) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc4 && c1 == 0xbf) { out += (c0 + 1); out += 0x80; }
			else if (c0 == 0xc5 && c1 == 0xb8) { out += (c0 - 2); out += 0xbf; }
			else if (c0 == 0xc6 && c1 == 0x81) { out += (c0 + 3); out += 0x93; }
			else if (c0 == 0xc6 && c1 == 0x86) { out += (c0 + 3); out += 0x94; }
			else if (c0 == 0xc6 && c1 == 0x89) { out += (c0 + 3); out += 0x96; }
			else if (c0 == 0xc6 && c1 == 0x8a) { out += (c0 + 3); out += 0x97; }
			else if (c0 == 0xc6 && c1 == 0x8e) { out += (c0 + 1); out += 0x9d; }
			else if (c0 == 0xc6 && c1 == 0x8f) { out += (c0 + 3); out += 0x99; }
			else if (c0 == 0xc6 && c1 == 0x90) { out += (c0 + 3); out += 0x9b; }
			else if (c0 == 0xc6 && c1 == 0x93) { out += (c0 + 3); out += 0xa0; }
			else if (c0 == 0xc6 && c1 == 0x94) { out += (c0 + 3); out += 0xa3; }
			else if (c0 == 0xc6 && c1 == 0x96) { out += (c0 + 3); out += 0xa9; }
			else if (c0 == 0xc6 && c1 == 0x97) { out += (c0 + 3); out += 0xa8; }
			else if (c0 == 0xc6 && c1 == 0x9c) { out += (c0 + 3); out += 0xaf; }
			else if (c0 == 0xc6 && c1 == 0x9d) { out += (c0 + 3); out += 0xb2; }
			else if (c0 == 0xc6 && c1 == 0x9f) { out += (c0 + 3); out += 0xb5; }
			else if (c0 == 0xc7 && c1 == 0x84) { out += c0; out += (c1 + 2); }
			else if (c0 == 0xc7 && c1 == 0x87) { out += c0; out += (c1 + 2); }
			else if (c0 == 0xc7 && c1 == 0x8a) { out += c0; out += (c1 + 2); }
			else if (c0 == 0xc7 && c1 == 0xb1) { out += c0; out += (c1 + 2); }
			else if (c0 == 0xc7 && c1 == 0xb6) { out += (c0 - 1); out += 0x95; }
			else if (c0 == 0xc7 && c1 == 0xb7) { out += (c0 - 1); out += 0xbf; }
			else if (c0 == 0xc8 && c1 == 0xa0) { out += (c0 - 2); out += 0x9e; }
			else if (c0 == 0xc6 && c1 == 0xa6) { out += 0xca; out += 0x80; }
			else if (c0 == 0xc6 && c1 == 0xa9) { out += 0xca; out += 0x83; }
			else if (c0 == 0xc6 && c1 == 0xae) { out += 0xca; out += 0x88; }
			else if (c0 == 0xc6 && c1 == 0xb1) { out += 0xca; out += 0x8a; }
			else if (c0 == 0xc6 && c1 == 0xb2) { out += 0xca; out += 0x8b; }
			else if (c0 == 0xc6 && c1 == 0xb7) { out += 0xca; out += 0x92; }
			else if (c0 == 0xce && c1 == 0x86) { out += 0xce; out += 0xac; }
			else if (c0 == 0xce && c1 == 0x88) { out += 0xce; out += 0xad; }
			else if (c0 == 0xce && c1 == 0x89) { out += 0xce; out += 0xae; }
			else if (c0 == 0xce && c1 == 0x8a) { out += 0xce; out += 0xaf; }
			else if (c0 == 0xce && c1 == 0x8c) { out += 0xcf; out += 0x8c; }
			else if (c0 == 0xce && c1 == 0x8e) { out += 0xcf; out += 0x8d; }
			else if (c0 == 0xce && c1 == 0x8f) { out += 0xcf; out += 0x8e; }
			else if (c0 == 0xcf && c1 == 0xb4) { out += 0xce; out += 0xb8; }
			else if (c0 == 0xce && 0x91 <= c1 && c1 <= 0x9f) { out += c0; out += (c1 + 0x20); }
			else if (c0 == 0xd0 && 0x90 <= c1 && c1 <= 0x9f) { out += c0; out += (c1 + 0x20); }
			else if (c0 == 0xd5 && 0x80 <= c1 && c1 <= 0x8f) { out += c0; out += (c1 + 0x30); }
			else if (c0 == 0xd0 && 0xa0 <= c1 && c1 <= 0xaf) { out += (c0 + 1); out += (c1 - 0x20); }
			else if (c0 == 0xd0 && 0x80 <= c1 && c1 <= 0x8f) { out += (c0 + 1); out += (c1 + 0x10); }
			else if (c0 == 0xd4 && 0xb1 <= c1 && c1 <= 0xbf) { out += (c0 + 1); out += (c1 - 0x10); }
			else if (c0 == 0xd5 && 0x90 <= c1 && c1 <= 0x96) { out += (c0 + 1); out += (c1 - 0x10); }
			else if (c0 == 0xc4 && 0x80 <= c1 && c1 <= 0xae && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc4 && 0xb2 <= c1 && c1 <= 0xb6 && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc4 && 0xb9 <= c1 && c1 <= 0xbd && c1 % 2 == 1) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc5 && 0x81 <= c1 && c1 <= 0x87 && c1 % 2 == 1) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc5 && 0x8a <= c1 && c1 <= 0x8e && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc5 && 0x90 <= c1 && c1 <= 0xae && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc5 && 0xb0 <= c1 && c1 <= 0xb6 && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc5 && 0xb9 <= c1 && c1 <= 0xbd && c1 % 2 == 1) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc7 && 0x91 <= c1 && c1 <= 0x9b && c1 % 2 == 1) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc7 && 0x9e <= c1 && c1 <= 0xae && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc8 && 0xb0 <= c1 && c1 <= 0xb2 && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xcf && 0x98 <= c1 && c1 <= 0xae && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xd1 && 0xa0 <= c1 && c1 <= 0xbe && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xd2 && 0x8a <= c1 && c1 <= 0xbe && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xd3 && 0x81 <= c1 && c1 <= 0x8d && c1 % 2 == 1) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xd4 && 0x80 <= c1 && c1 <= 0x8e && c1 % 2 == 0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc3 && 0x80 <= c1 && c1 <= 0x9e && c1 != 0x97) { out += c0; out += (c1 + 0x20); }
			else if (c0 == 0xce && 0xa0 <= c1 && c1 <= 0xab && c1 != 0xa2) { out += (c0 + 1); out += (c1 - 0x20); }
			else if (c0 == 0xc8 && 0x80 <= c1 && c1 <= 0xae && c1 % 2 == 0 && c1 != 0xa0) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xd3 && 0x90 <= c1 && c1 <= 0xb8 && c1 % 2 == 0 && c1 != 0xb6) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc7 && (c1 == 0xba || c1 == 0xbc || c1 == 0xbe)) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc7 && (c1 == 0x8d || c1 == 0x8f || c1 == 0xb4 || c1 == 0xb8)) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc6 && (c1 == 0x82 || c1 == 0x84 || c1 == 0x87 || c1 == 0x8b)) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc6 && (c1 == 0x91 || c1 == 0x98 || c1 == 0xa0 || c1 == 0xa2)) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc6 && (c1 == 0xa4 || c1 == 0xa7 || c1 == 0xac || c1 == 0xaf)) { out += c0; out += (c1 + 1); }
			else if (c0 == 0xc6 && (c1 == 0xb3 || c1 == 0xb5 || c1 == 0xb8 || c1 == 0xbc)) { out += c0; out += (c1 + 1); }
			else { out += c0; out += c1; }
		}
		else if (c0 >= 224 && c0 <= 239)
		{
			unsigned char c1 = m[i + 1];
			unsigned char c2 = m[i + 2];
			if (c0 == 0xe2 && c1 == 0x84 && c2 == 0xa6) { out += 0xcf; out += 0x89; }
			else if (c0 == 0xe2 && c1 == 0x84 && c2 == 0xab) { out += 0xc3; out += 0xa5; }
			else if (c0 == 0xe2 && c1 == 0x84 && c2 == 0xaa) { out += 0x6b; }
			else if (c0 == 0xe1 && c1 == 0xbe && c2 == 0xba) { out += c0; out += (c1 - 1); out += 0xb0; }
			else if (c0 == 0xe1 && c1 == 0xbe && c2 == 0xbb) { out += c0; out += (c1 - 1); out += 0xb1; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x88) { out += c0; out += (c1 - 2); out += 0xb2; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x89) { out += c0; out += (c1 - 2); out += 0xb3; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x8a) { out += c0; out += (c1 - 2); out += 0xb4; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x8b) { out += c0; out += (c1 - 2); out += 0xb5; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x98) { out += c0; out += c1;     out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x99) { out += c0; out += c1;     out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x9a) { out += c0; out += (c1 - 2); out += 0xb6; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0x9b) { out += c0; out += (c1 - 2); out += 0xb7; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xa8) { out += c0; out += c1;     out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xa9) { out += c0; out += c1;     out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xaa) { out += c0; out += (c1 - 2); out += (c2 + 0x10); }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xab) { out += c0; out += (c1 - 2); out += (c2 + 0x10); }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xac) { out += c0; out += c1;     out += 0xa5; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xb8) { out += c0; out += (c1 - 2); out += c2; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xb9) { out += c0; out += (c1 - 2); out += c2; }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xba) { out += c0; out += (c1 - 2); out += (c2 + 2); }
			else if (c0 == 0xe1 && c1 == 0xbf && c2 == 0xbb) { out += c0; out += (c1 - 2); out += (c2 + 2); }
			else if (c0 == 0xe1 && c1 == 0xbc && 0x88 <= c2 && c2 <= 0x8f) { out += c0; out += c1; out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbc && 0x98 <= c2 && c2 <= 0x9d) { out += c0; out += c1; out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbc && 0xa8 <= c2 && c2 <= 0xaf) { out += c0; out += c1; out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbc && 0xb8 <= c2 && c2 <= 0xbf) { out += c0; out += c1; out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbd && 0x88 <= c2 && c2 <= 0x8d) { out += c0; out += c1; out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbd && 0xa8 <= c2 && c2 <= 0xaf) { out += c0; out += c1; out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xbe && 0xb8 <= c2 && c2 <= 0xb9) { out += c0; out += c1; out += (c2 - 8); }
			else if (c0 == 0xe1 && c1 == 0xb8 && 0x80 <= c2 && c2 <= 0xbe && c2 % 2 == 0) { out += c0; out += c1; out += (c2 + 1); }
			else if (c0 == 0xe1 && c1 == 0xb9 && 0x80 <= c2 && c2 <= 0xbe && c2 % 2 == 0) { out += c0; out += c1; out += (c2 + 1); }
			else if (c0 == 0xe1 && c1 == 0xba && 0x80 <= c2 && c2 <= 0x94 && c2 % 2 == 0) { out += c0; out += c1; out += (c2 + 1); }
			else if (c0 == 0xe1 && c1 == 0xba && 0xa0 <= c2 && c2 <= 0xbe && c2 % 2 == 0) { out += c0; out += c1; out += (c2 + 1); }
			else if (c0 == 0xe1 && c1 == 0xbb && 0x80 <= c2 && c2 <= 0xb8 && c2 % 2 == 0) { out += c0; out += c1; out += (c2 + 1); }
			else if (c0 == 0xe1 && c1 == 0xbd && 0x99 <= c2 && c2 <= 0x9f && c2 % 2 == 1) { out += c0; out += c1; out += (c2 - 8); }
			else { out += c0; out += c1; out += c2; }
		}

		if (c0 >= 0 && c0 <= 127) { i += 0; }
		else if (c0 >= 192 && c0 <= 223) { i += 1; }
		else if (c0 >= 224 && c0 <= 239) { i += 2; }
		else if (c0 >= 240 && c0 <= 247) { i += 3; }
	}
	return out;
}

std::string TextFilter::Filter(const std::string& text)
{
	std::lock_guard<std::mutex> guard(mutex_);

    std::string out_text = text;
    std::string lower_text = out_text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
	for (std::string sensitive_word : sensitive_chs_) 
    {
        std::transform(sensitive_word.begin(), sensitive_word.end(), sensitive_word.begin(), ::tolower);
		while (lower_text.find(sensitive_word) != std::string::npos) 
        {
			out_text.replace(lower_text.find(sensitive_word), sensitive_word.length(), "**");
			lower_text = out_text;
            std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
		}
	}

	out_text = " " + out_text + " ";
	std::string fmt(" ** ");
	for (std::string sensitive_word : sensitive_ens_) 
    {
		std::regex reg(sensitive_word, std::regex::extended | std::regex::icase);
		while (std::regex_replace(out_text, reg, fmt) != out_text) 
        {
			out_text = std::regex_replace(out_text, reg, fmt);
		}
	}

	std::smatch what;
	lower_text = mb_strtolower(out_text);
	std::string::const_iterator start = lower_text.begin();
	std::string::const_iterator end = lower_text.end();
	for (std::string sensitive_word : sensitive_rus_) 
    {
		sensitive_word = mb_strtolower(sensitive_word);
		std::regex reg(sensitive_word, std::regex::extended | std::regex::icase);
		while (std::regex_search(start, end, what, reg)) 
        {
			out_text.replace(what[0].first - lower_text.begin(), what[0].second - what[0].first, fmt);
			lower_text.replace(what[0].first - lower_text.begin(), what[0].second - what[0].first, fmt);
			start = lower_text.begin();
			end = lower_text.end();
		}
	}
	out_text = out_text.substr(1, out_text.length() - 2);
	
	return out_text;
}

void TextFilter::LoadSensitiveWordsDict() 
{
	std::lock_guard<std::mutex> guard(mutex_);

    std::ifstream ifs("./config/sensitive_words.json");
    std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	Json::Value root;
	Json::Reader reader;
	if (reader.parse(str, root)) 
    {
		sensitive_chs_.clear();
		for (const auto& ch : root["chs"]) 
        {
			sensitive_chs_.push_back(ch.asString());
		}

		sensitive_ens_.clear();
		for (const auto& en : root["ens"]) 
        {
			sensitive_ens_.push_back(en.asString());
		}

		sensitive_rus_.clear();
		for (const auto& ru : root["rus"]) 
        {
			sensitive_rus_.push_back(ru.asString());
		}
	}
}
