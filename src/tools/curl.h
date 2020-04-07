#ifndef __TOOLS_CURL_H__
#define __TOOLS_CURL_H__

#include <stdint.h>
#include <string>
#include <map>

int curl_get(const std::string& url, std::string* response);
int curl_post(const std::string& url, const std::string& postdata, std::string* response);

int curl_get(const std::string& url, const std::map<std::string, std::string>& parm, std::string* response);

std::string UrlEncode(const std::string& str);
#endif

