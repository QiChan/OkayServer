#include "club_config_mgr.h"
#include <sstream>
#include <glog/logging.h>

ClubConfigMgr::ClubConfigMgr()
{
}

ClubConfigMgr::~ClubConfigMgr()
{
}

bool ClubConfigMgr::ConnectMysql()
{
    MysqlConfigMgr  config_mgr;
    if (!config_mgr.VLoadConfig())
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_S1_OKEYPOKER_CONFIG, &mysql_slave_config_))
    {
        return false;
    }

    return true;
}

bool ClubConfigMgr::LoadClubConfig()
{
    if (!ConnectMysql())
    {
        return false;
    }

    std::stringstream sql;
    sql << "select `config_name`, `rate` from `club_config`";
    if (!mysql_slave_config_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "load club config sql error. sql: " << sql.str();
        return false;
    }

    decltype(club_configs_)     config; 

    MysqlResult result(mysql_slave_config_.handle());
    MysqlRow    row;
    while (result.fetch_row(row))
    {
        std::string config_name = row.get_string("config_name");
        uint32_t    rate = row.get_uint32("rate");
        config[config_name] = std::make_shared<ClubConfig>(config_name, rate);
    }

    if (config.empty())
    {
        LOG(ERROR) << "club config cannot empty";
        return false;
    }

    club_configs_.swap(config);
    LOG(INFO) << "load club config done. size: " << club_configs_.size();
    return true;
}

std::shared_ptr<ClubConfig> ClubConfigMgr::GetClubConfig(const std::string& config_name)
{
    auto it = club_configs_.find(config_name);
    if (it == club_configs_.end())
    {
        return nullptr;
    }
    return it->second;
}
