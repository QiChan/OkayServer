#include "redis_client.h"
#include <hiredis/hiredis.h>
#include <glog/logging.h>
#include "scope_guard.h"

using namespace std;
class redisReply;

RedisClient::RedisClient()
    : redis_ctx_(nullptr)
{
}

RedisClient::~RedisClient()
{
    if (redis_ctx_ != nullptr)
    {
        redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
    }
}

bool RedisClient::redis_connect(const RedisConfig& config)
{
    if (redis_ctx_ != nullptr)
    {
        redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
    }

    config_ = config;
	redis_ctx_ = redisConnect(config_.host.c_str(), config_.port);
	if (redis_ctx_->err != 0)
	{
        LOG(ERROR) << "redis connect error. " << config_.host << ":" << config_.port;
		redisFree(redis_ctx_);
        redis_ctx_ = nullptr;
		return false;
	}
    LOG(INFO) << "redis connect ok. " << config_.host << ":" << config_.port;

    redisReply* reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "AUTH %s", config.password.c_str()));

    ScopeGuard guard(
            []{}, 
            [&reply]
            { 
                if (reply != nullptr)
                {
                    freeReplyObject(reply);
                    reply = nullptr;
                }
            });

    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) 
    {
        LOG(ERROR) << "redis auth failed. " << config_.host << ":" << config_.port << " pass: " << config.password.c_str();
        return false;
    }
    LOG(INFO) << "redis auth ok. " << config_.host << ":" << config_.port;
    return true;
}


void RedisClient::redis_get(const string& key, string& value)
{
    value.clear();
    char cmd[5000];
    sprintf(cmd, "get %s", key.c_str());
    auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, cmd));
    if (reply == NULL)
	{
		LOG(INFO) << "redis get false. key:" << key;
		return;
	}
	if (reply->str)
		value.assign(reply->str, reply->len);
	freeReplyObject(reply);
}

void RedisClient::redis_get(uint64_t key, std::string& value)
{
	value.clear();
	char cmd[5000];
	sprintf(cmd, "get %lu", key);
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, cmd));
	if (reply == NULL)
	{
		LOG(INFO) << "redis get false. key:" << key;
		return;
	}
	if (reply->str)
		value.assign(reply->str, reply->len);
	freeReplyObject(reply);
}

void RedisClient::redis_hget(uint64_t key, const std::string& key1, std::string& value)
{
	value.clear();
	char cmd[5000];
	sprintf(cmd, "hget %lu %s", key, key1.c_str());
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, cmd));
	if (reply == NULL)
	{
		LOG(INFO) << "redis get false. key:" << key;
		return;
	}
	if (reply->str)
		value.assign(reply->str, reply->len);
	freeReplyObject(reply);
}

void RedisClient::redis_hget(const std::string& key, int key1, std::string& value)
{
	value.clear();
	char cmd[5000];
	sprintf(cmd, "hget %s %d", key.c_str(), key1);
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, cmd));
	if (reply == NULL)
	{
		LOG(INFO) << "redis get false. key:" << key;
		return;
	}
	if (reply->str)
		value.assign(reply->str, reply->len);
	freeReplyObject(reply);
}

void RedisClient::redis_hset(uint64_t key, const std::string& key1, const std::string& value)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hset %lu %s %b", key, key1.c_str(), value.c_str(), value.size()));
	if (reply == NULL)
	{
        LOG(INFO) << "redis set false. key:" << key << " key1:" << key1 << " value:" << value;
		return;
	}
	if (reply->type == REDIS_REPLY_ERROR)
	{
		LOG(WARNING) << "redis set error. " << key << " " << key1 << " " << value;
		if (reply->str)
			LOG(WARNING) << reply->str;
	}

	freeReplyObject(reply);
}

void RedisClient::redis_hset(const std::string& key, uint64_t key1, const std::string& value)
{
	char key1_s[100];
	sprintf(key1_s, "%lu", key1);
	redis_hset(key, key1_s, value);
}

void  RedisClient::redis_hset(const std::string& key, const std::string& key1, const std::string& value)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hset %s %s %b", key.c_str(), key1.c_str(), value.c_str(), value.size()));
	if (reply == NULL)
	{
        LOG(INFO) << "redis set false. key:" << key << " key1:" << key1 << " value:" << value;
		return;
	}
	if (reply->type == REDIS_REPLY_ERROR)
	{
		LOG(WARNING) << "redis set error. " << key << " " << key1 << " " << value;
		if (reply->str)
			LOG(WARNING) << reply->str;
	}

	freeReplyObject(reply);
}

void RedisClient::redis_hset(uint64_t key, const std::string& key1, int64_t value)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hset %lu %s %ld", key, key1.c_str(), value));
	if (reply == NULL)
	{
        LOG(INFO) << "redis set false. key:" << key << " key1:" << key1 << " value:" << value;
		return;
	}

	if (reply->type == REDIS_REPLY_ERROR)
	{
		LOG(WARNING) << "redis set error. " << key << " " << key1 << " " << value;
		if (reply->str)
			LOG(WARNING) << reply->str;
	}

	freeReplyObject(reply);
}

void RedisClient::redis_hset(const std::string& key, uint64_t key1, int64_t value)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hset %s %lu %ld", key.c_str(), key1, value));
	if (reply == NULL)
	{
		return;
	}
	freeReplyObject(reply);
}


void RedisClient::redis_set(const std::string& key, const std::string& value)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "set %s %b", key.c_str(), value.c_str(), value.size()));
	if (reply == NULL)
	{
        LOG(INFO) << "redis set false. key:" << key << " value:" << value;
		return;
	}
	freeReplyObject(reply);
}

void RedisClient::redis_del(uint64_t key)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "del %lu", key));
	if (reply == NULL)
	{
        LOG(INFO) << "redis del false. key:" << key;
		return;
	}
	freeReplyObject(reply);
}

void RedisClient::redis_del(const std::string& key)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "del %s", key.c_str()));
	if (reply == NULL)
	{
        LOG(INFO) << "redis del false. key:" << key;
		return;
	}
	freeReplyObject(reply);
}

void RedisClient::redis_hdel(uint64_t key, const std::string& key1)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hdel %lu %s", key, key1.c_str()));
	if (reply == NULL)
	{
        LOG(INFO) << "redis hdel false. key:" << key;
		return;
	}
	freeReplyObject(reply);
}

void RedisClient::redis_hdel(const std::string& key, uint64_t key1)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hdel %s %lu", key.c_str(), key1));
	if (reply == NULL)
	{
        LOG(INFO) << "redis hdel false. key:" << key;
		return;
	}
	freeReplyObject(reply);
}

void RedisClient::redis_hgetall(const std::string& key, std::unordered_map<int64_t, int64_t>& result)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hgetall %s", key.c_str()));
	if (reply == NULL)
	{
        LOG(INFO) << "redis hgetall false. key:" << key;
		return;
	}
	size_t n = reply->elements / 2;
	for (size_t i = 0; i < n; ++i)
	{
		redisReply* k = reply->element[i * 2];
		redisReply* v = reply->element[i * 2 + 1];
		if (!k || !v)
			continue;
		result[atoll(k->str)] = atoll(v->str);
	}
	freeReplyObject(reply);
}

void RedisClient::redis_hgetall(const std::string& key, std::unordered_map<std::string, std::string>& result)
{
	auto reply = static_cast<redisReply*>(redisCommand(redis_ctx_, "hgetall %s", key.c_str()));
	if (reply == NULL)
	{
        LOG(INFO) << "redis hgetall false. key:" << key;
		return;
	}
	size_t n = reply->elements / 2;
	for (size_t i = 0; i < n; ++i)
	{
		redisReply* k = reply->element[i * 2];
		redisReply* v = reply->element[i * 2 + 1];
		if (!k || !v)
			continue;
		result[k->str] = v->str;
	}
	freeReplyObject(reply);
}
