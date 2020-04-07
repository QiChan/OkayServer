#ifndef __COMMON_REDIS_CLIENT_H__
#define __COMMON_REDIS_CLIENT_H__

#include "redis_config.h"

class redisContext;

class RedisClient final
{
public:
	RedisClient();
    ~RedisClient();
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator= (const RedisClient&) = delete;

public:
	bool redis_connect(const RedisConfig& config);
	void redis_set(const std::string& key, const std::string& value);
	void redis_get(const std::string& key, std::string&value);
	void redis_get(uint64_t key, std::string& value);
	void redis_hget(uint64_t key, const std::string& key1, std::string& value);
	void redis_hget(const std::string& key, int key1, std::string& value);

	void redis_hset(uint64_t key, const std::string& key1, const std::string& value);
	void redis_hset(uint64_t key, const std::string& key1, int64_t value);
	void redis_hset(const std::string& key, uint64_t key1, int64_t value);
	void redis_hset(const std::string& key, uint64_t key1, const std::string& value);
	void redis_hset(const std::string& key, const std::string& key1, const std::string& value);

	void redis_del(uint64_t key);
	void redis_del(const std::string& key);
	void redis_hdel(uint64_t key, const std::string& key1);
	void redis_hdel(const std::string& key, uint64_t key1);
	void redis_hgetall(const std::string& key, std::unordered_map<int64_t, int64_t>& result);
	void redis_hgetall(const std::string& key, std::unordered_map<std::string, std::string>& result);
private:

	redisContext*   redis_ctx_;
    RedisConfig     config_;
};

#endif
