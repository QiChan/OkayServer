#ifndef __COMMON_DBCONFIG_H__
#define __COMMON_DBCONFIG_H__

#include <unordered_map>

#define MY_M1_OKEYPOKER                 "m1_okeygo"
#define MY_M1_OKEYPOKER_USER            "m1_okey_user"
#define MY_M1_OKEYPOKER_FLOW            "m1_okey_flow"
#define MY_M1_OKEYPOKER_CLUB            "m1_okey_club"
#define MY_M1_OKEYPOKER_GAME_RECORD     "m1_okey_game_record"

#define MY_S1_OKEYPOKER                 "s1_okeygo";
#define MY_S1_OKEYPOKER_USER            "s1_okey_user"
#define MY_S1_OKEYPOKER_CLUB            "s1_okey_club"
#define MY_S1_OKEYPOKER_CONFIG          "s1_okey_config"
#define MY_S1_OKEYPOKER_GAME_RECORD     "s1_okey_game_record"

#define USER_REDIS                      "user_redis"

#define MYSQL_CONFIG_PATH 				"./db_config/mysql.json"
#define REDIS_CONFIG_PATH               "./db_config/redis.json"

struct MysqlConfig
{
    MysqlConfig() : port(0) {}
    MysqlConfig(const MysqlConfig&) = default;      // default is ok
	std::string host;
	uint32_t    port;
	std::string db;
	std::string username;
	std::string password;
};

struct RedisConfig
{
    RedisConfig() : port(0) {}
    RedisConfig(const RedisConfig&) = default;
    std::string host;
    uint32_t    port;
    std::string password;
};

class DBConfigMgr
{
	public:
		DBConfigMgr() = default;
		virtual ~DBConfigMgr() = default;
        DBConfigMgr(const DBConfigMgr&) = delete;
        DBConfigMgr& operator= (const DBConfigMgr&) = delete;

	public:
		virtual bool VLoadConfig()= 0;
};

#endif
