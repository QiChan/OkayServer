#ifndef __COMMON_MYSQL_CLIENT_H__
#define __COMMON_MYSQL_CLIENT_H__

#include <string>
#include <unordered_map>
#include <vector>
#include <mysql/mysql.h>
#include "mysql_config.h"


class MysqlClient final
{
public:
	MysqlClient();
	~MysqlClient();
    MysqlClient(const MysqlClient&) = delete;
    MysqlClient& operator= (const MysqlClient&) = delete;

public:
	bool mysql_connect(const MysqlConfig& config);
	MYSQL* mysql_reconnect();
	bool mysql_exec(const std::string& sql);
	uint32_t mysql_last_affected_rows();

    MYSQL* handle() { return mysql_ctx_; }

	uint32_t get_insert_id_uint32();
    uint64_t get_insert_id_uint64();

	std::string get_info();

private:
	void init_mysql_ctx();

private:
    MysqlConfig     config_;

	MYSQL*          mysql_ctx_;
};

class MysqlResult;
class MysqlRow  final ///MysqlResult::fetch_row返回true才能用
{
	friend class MysqlResult;
public:
	MysqlRow();
    ~MysqlRow() = default;
    MysqlRow(const MysqlRow&) = delete;
    MysqlRow& operator= (const MysqlRow&) = delete;

public:
	int64_t     get_int64(const std::string& key);
    uint64_t    get_uint64(const std::string& key);
	const std::string& get_string(const std::string& key);
	int get_int(const std::string& key);
    uint32_t    get_uint32(const std::string& key);

	double get_double(const std::string& key);

	void clear()
	{
		rows_.clear();
	}
private:
	std::vector<std::string>    rows_;
	MysqlResult*                result_;
};

class MysqlResult final
{
public:
	MysqlResult(MYSQL* mysql);
	~MysqlResult();
    MysqlResult(const MysqlResult&) = delete;
    MysqlResult& operator= (const MysqlResult&) = delete;

public:
	bool fetch_row(MysqlRow& row);
	int64_t name2idx(const std::string& name);

private:
	MYSQL_RES*                                  result_;
	std::unordered_map<std::string, int64_t>    fields_;
};

std::string mysql_string(const std::string& from, MysqlClient& mysql);

#endif
