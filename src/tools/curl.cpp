#include <curl/curl.h>
#include "curl.h"
using namespace std;

static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
	std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);
	if (NULL == str || NULL == buffer)
	{
		return -1;
	}

	char* pData = (char*)buffer;
	str->append(pData, size * nmemb);
	return nmemb;
}

int curl_get(const string& url, string* response)
{
	CURL* curl = curl_easy_init();
	if (curl == NULL)
		return -1;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res;
}

int curl_post(const string& url, const string& postdata, string* response)
{
	CURL* curl = curl_easy_init();
	if (curl == NULL)
		return -1;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res;
}

int curl_get(const std::string& url, const std::map<string, string>& parm, std::string* response)
{
	CURL* curl = curl_easy_init();
	if (curl == NULL)
		return -1;
	if (url.empty())
		return -1;
	std::string data = url;
	data += "?";
	auto it = parm.begin();
	for (; it != parm.end(); ++it)
	{
		if (it != parm.begin())
		{
			data += "&";
		}

		char* output = curl_easy_escape(curl, it->first.c_str(), it->first.size());
		data += output;
		curl_free(output);

		data += "=";

		output = curl_easy_escape(curl, it->second.c_str(), it->second.size());
		data += output;
		curl_free(output);
	}
	curl_easy_setopt(curl, CURLOPT_URL, data.c_str());
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res;
}

unsigned char ToHex(unsigned char x) 
{ 
    return  x > 9 ? x + 55 : x + 48; 
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) || 
                (str[i] == '-') ||
                (str[i] == '_') || 
                (str[i] == '.') || 
                (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}
