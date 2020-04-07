#include "mysql_config.h"
#include "scope_guard.h"
#include <fstream>
#include <glog/logging.h>
#include "../json/json.h"
#include "mysql_client.h"

bool MysqlConfigMgr::ConnectMysql(const std::string& conf_name, MysqlClient* const client)
{
    MysqlConfig config;
    if (!GetConfig(conf_name, config))
    {
        return false;
    }
    if (!client->mysql_connect(config))
    {
        LOG(ERROR) << "connect mysql failed. " << conf_name;
        return false;
    }
    return true;
}

bool MysqlConfigMgr::VLoadConfig()
{
    std::ifstream ifs;
    ifs.open(MYSQL_CONFIG_PATH, std::ios::binary);
    if (!ifs)
    {
		LOG(ERROR) << "reload config " << MYSQL_CONFIG_PATH << " failed"; 		
        return false;
    }

    ScopeGuard guard([]{}, [&ifs]{ ifs.close(); });

    Json::Reader    reader;
    Json::Value     root;
    if (!reader.parse(ifs, root))
    {
		LOG(ERROR) << " parse config " << MYSQL_CONFIG_PATH << " failed"; 		
		return false;
	}

    decltype(config_data_)  config;

	for (const auto & conf_name : root.getMemberNames())
	{
		const Json::Value & conf_data = root[conf_name];

        MysqlConfig   conf;
		if (!conf_data["host"].isNull())
		{
			conf.host = conf_data["host"].asString();
		}
        else
        {
			LOG(ERROR) << "config " << MYSQL_CONFIG_PATH << " name: " << conf_name << " host not found"; 		
			return false;
		}

		if (!conf_data["port"].isNull())
		{
			conf.port = conf_data["port"].asUInt();
		}
        else
        {
			LOG(ERROR) << "config " << MYSQL_CONFIG_PATH << " name: " << conf_name << " port not found"; 		
			return false;
		}

		if (!conf_data["username"].isNull())
		{
			conf.username = conf_data["username"].asString();
		}
        else
        {
			LOG(ERROR) << "config " << MYSQL_CONFIG_PATH << " name: " << conf_name << " username not found"; 		
			return false;
		}

		if (!conf_data["password"].isNull())
		{
			conf.password = conf_data["password"].asString();
		}
        else
        {
			LOG(ERROR) << "config " << MYSQL_CONFIG_PATH << " name: " << conf_name << " password not found"; 		
			return false;
		}

		if (!conf_data["db"].isNull())
		{
			conf.db = conf_data["db"].asString();
		}
        else
        {
			LOG(ERROR) << "config " << MYSQL_CONFIG_PATH << " name: " << conf_name << " db not found"; 		
			return false;
		}
        config.emplace(conf_name, conf);
	}

    if (config.empty())
    {
        LOG(ERROR) << "mysql config " << MYSQL_CONFIG_PATH << " connot empty";
        return false;
    }

    config_data_.swap(config);
	return true;
}

bool MysqlConfigMgr::GetConfig(const std::string & name, MysqlConfig& config)
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


