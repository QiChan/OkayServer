#ifndef __CLUB_CLUB_CONFIG_MGR_H__
#define __CLUB_CLUB_CONFIG_MGR_H__

#include "../common/mysql_client.h"
#include <memory>

class ClubConfig final
{
public:
    ClubConfig(const std::string& config_name, uint32_t rate)
        : config_name_(config_name),
          rate_(rate)
    {
    }
    ~ClubConfig() = default;
    ClubConfig(const ClubConfig&) = delete;
    ClubConfig& operator= (const ClubConfig&) = delete;

public:
    uint32_t    rate() const { return rate_; }

private:
    std::string config_name_;
    uint32_t    rate_ = 0;           // 开桌消耗
};

class ClubConfigMgr final
{
public:
    ClubConfigMgr ();
    virtual ~ClubConfigMgr ();
    ClubConfigMgr (ClubConfigMgr const&) = delete;
    ClubConfigMgr& operator= (ClubConfigMgr const&) = delete; 

public:
    bool    LoadClubConfig();
    bool    ConnectMysql();
    
    std::shared_ptr<ClubConfig>     GetClubConfig(const std::string& config_name);

private:
    MysqlClient     mysql_slave_config_;

    std::unordered_map<std::string, std::shared_ptr<ClubConfig>>     club_configs_;
};


#endif 
