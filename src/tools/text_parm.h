#ifndef __TOOLS_TEXT_PARM_H__
#define __TOOLS_TEXT_PARM_H__

#include <unordered_map>
#include <string>
#include <cstring>

//key=value key=value格式的字符串
class TextParm final
{
    public:
        TextParm() = default;
        ~TextParm() = default;
        TextParm(const TextParm&) = default;
        TextParm& operator= (const TextParm&) = delete;

        TextParm(const char* stream)
        {
            if (stream != nullptr)
                deserilize(stream, strlen(stream));
        }
        TextParm(const char* stream, int len)
        {
            if (stream != nullptr)
                deserilize(stream, len);
        }

    public:
        void deserilize(const char* stream, size_t len);
        const char* serilize();

        size_t serilize_size()
        {
            return stream_.size();
        }
        void erase(const std::string& key)
        {
            data_.erase(key);
        }
        void insert(const std::string& key, const std::string& value)
        {
            data_[key] = value;
        }
        void insert_int(const std::string& key, int value)
        {
            char v[10];
            sprintf(v, "%d", value);
            data_[key] = v;
        }
        void insert_uint(const std::string& key, uint32_t value)
        {
            char v[10];
            sprintf(v, "%u", value);
            data_[key] = v;
        }
        const char* get(const std::string& key) const
        {
            return get_string(key).c_str();
        }
        const std::string& get_string(const std::string& key) const
        {
            std::unordered_map<std::string, std::string>::const_iterator it = data_.find(key);
            if (it == data_.end())
                return nullstring_;
            return it->second;
        }
        int get_int(const std::string& key) const
        {
            return atoi(get(key));
        }

        int64_t get_int64(const std::string& key) const
        {
            return atoll(get(key));
        }

        uint32_t get_uint(const std::string& key) const
        {
            return strtoul(get(key), NULL, 10);
        }

        uint64_t get_uint64(const std::string& key) const
        {
            return strtoull(get(key), NULL, 10);
        }

        void print() const
        {
            std::unordered_map<std::string, std::string>::const_iterator it = data_.begin();
            while (it != data_.end())
            {
                printf("%s=%s\n", it->first.c_str(), it->second.c_str());
                ++it;
            }
        }

        bool has(const std::string& key)
        {
            std::unordered_map<std::string, std::string>::const_iterator it = data_.find(key);
            if (it == data_.end())
                return false;
            return true;
        }

    private:
        std::unordered_map<std::string, std::string>  data_;
        static const std::string            nullstring_;
        std::string                         stream_;
};

#endif
