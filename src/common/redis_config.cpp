#include "redis_config.h"
#include "scope_guard.h"
#include <fstream>
#include <glog/logging.h>
#include "../json/json.h"

bool RedisConfigMgr::VLoadConfig()
{
    std::ifstream ifs;
    ifs.open(REDIS_CONFIG_PATH, std::ios::binary);
    if (!ifs)
    {
		LOG(ERROR) << " reload config " << REDIS_CONFIG_PATH << " failed"; 		
        return false;
    }

    ScopeGuard guard([]{}, [&ifs]{ ifs.close(); });

    Json::Reader    reader;
    Json::Value     root;
    if (!reader.parse(ifs, root))
    {
		LOG(ERROR) << " parse config " << REDIS_CONFIG_PATH << " failed"; 		
		return false;
	}

    decltype(config_data_)  config;

	for (const auto & conf_name : root.getMemberNames())
	{
		const Json::Value & conf_data = root[conf_name];

        RedisConfig   conf;
		if (!conf_data["host"].isNull())
		{
			conf.host = conf_data["host"].asString();
		}
        else
        {
			LOG(ERROR) << "config " << REDIS_CONFIG_PATH << " name: " << conf_name << " host not found"; 		
			return false;
		}

		if (!conf_data["port"].isNull())
		{
			conf.port = conf_data["port"].asUInt();
		}
        else
        {
			LOG(ERROR) << "config " << REDIS_CONFIG_PATH << " name: " << conf_name << " port not found"; 		
			return false;
		}

		if (!conf_data["password"].isNull())
		{
			conf.password = conf_data["password"].asString();
		}
        else
        {
			LOG(ERROR) << "config " << REDIS_CONFIG_PATH << " name: " << conf_name << " password not found"; 		
			return false;
		}

        config.emplace(conf_name, conf);
	}

    if (config.empty())
    {
        LOG(ERROR) << "Redis config " << REDIS_CONFIG_PATH << " connot empty";
        return false;
    }

    config_data_.swap(config);
	return true;
}

bool RedisConfigMgr::GetConfig(const std::string& name, RedisConfig& config)
{
	auto it = config_data_.find(name);

	if (it == config_data_.end())
	{
		LOG(ERROR) << "config name: " << name << " not found"; 		
		return false;
	}

    config = it->second;

	return true;
}
