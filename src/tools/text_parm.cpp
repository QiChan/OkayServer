#include "text_parm.h"

const std::string TextParm::nullstring_;

void TextParm::deserilize(const char* stream, size_t len)
{
	char key[10000];
	char value[10000];
	int vlen;
	if (len > 10000)
		return;
    std::unordered_map<std::string, std::string> temp_map;
	size_t i = 0;
	while (i < len)
	{
		int klen = 0;
		if (stream[i] == ' ')
		{
			++i;
			continue;
		}
		while (i < len && stream[i] != '=')
			key[klen++] = stream[i++];
		key[klen] = 0;
		++i;
		vlen = 0;
		while (i < len && stream[i] != ' ')
			value[vlen++] = stream[i++];
		value[vlen] = 0;
		++i;
		temp_map[key] = value;
	}
	data_.swap(temp_map);
	stream_.assign(stream);
}

const char* TextParm::serilize()
{
    std::unordered_map<std::string, std::string>::const_iterator it = data_.begin();
	stream_.clear();
	while (it != data_.end())
	{
		stream_ += it->first;
		stream_.push_back('=');
		stream_ += it->second;
		stream_.push_back(' ');
		++it;
	}
	return stream_.c_str();
}

