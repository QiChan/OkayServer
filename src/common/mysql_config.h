#ifndef __COMMON_MYSQL_CONFIG_H__
#define __COMMON_MYSQL_CONFIG_H__

#include "dbconfig.h"

class MysqlClient;
class MysqlConfigMgr final : public DBConfigMgr
{
    public:
        MysqlConfigMgr() = default;
        ~MysqlConfigMgr() = default;
        MysqlConfigMgr(const MysqlConfigMgr&) = delete;
        MysqlConfigMgr& operator= (const MysqlConfigMgr&) = delete;

    public:
        bool    ConnectMysql(const std::string& conf_name, MysqlClient* const client);

    public:
        virtual bool VLoadConfig() override;
        bool GetConfig(const std::string& conf_name, MysqlConfig& config);

    private:
        std::unordered_map<std::string, MysqlConfig>  config_data_;
};


#endif
