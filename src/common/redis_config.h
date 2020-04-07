#ifndef __COMMON_REDIS_CONFIG_H__
#define __COMMON_REDIS_CONFIG_H__

#include "dbconfig.h"

class RedisConfigMgr final : public DBConfigMgr
{
    public:
        RedisConfigMgr() = default;
        ~RedisConfigMgr() = default;
        RedisConfigMgr(const RedisConfigMgr&) = delete;
        RedisConfigMgr& operator= (const RedisConfigMgr&) = delete;

    public:
        virtual bool VLoadConfig() override;
        bool GetConfig(const std::string& name, RedisConfig& config);

    private:
        std::unordered_map<std::string, RedisConfig>  config_data_;
};


#endif
