#include "mysql_client.h"
#include <glog/logging.h>

static std::string null_string;

MysqlClient::MysqlClient()
    : mysql_ctx_(nullptr)
{
}

MysqlClient::~MysqlClient()
{
	if (mysql_ctx_)
    {
		mysql_close(mysql_ctx_);
        mysql_ctx_ = nullptr;
    }
}

void MysqlClient::init_mysql_ctx()
{
	mysql_close(mysql_ctx_);	

	mysql_ctx_ = mysql_init(NULL);
	if (mysql_ctx_ == nullptr)
	{
		LOG(ERROR) << "mysql init failed";
		return;
	}

	unsigned int timeout = 20;
	mysql_options(mysql_ctx_, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(mysql_ctx_, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

	mysql_ctx_ = mysql_real_connect(mysql_ctx_, config_.host.c_str(), config_.username.c_str(), config_.password.c_str(), config_.db.c_str(), config_.port, NULL, CLIENT_MULTI_RESULTS | CLIENT_MULTI_STATEMENTS);

	if (mysql_ctx_ == nullptr)
	{
		return;
	}

	mysql_set_character_set(mysql_ctx_, "utf8mb4");
}

bool MysqlClient::mysql_connect(const MysqlConfig& config)
{
    config_ = config;

	init_mysql_ctx();

	if (mysql_ctx_ == nullptr)
	{
		LOG(ERROR) << "mysql connect failed " << get_info().c_str();
		return false;
	}	
	LOG(INFO) << "connect mysql ok. " << get_info().c_str();
	return true;
}


std::string MysqlClient::get_info()
{
    std::stringstream info;
	info 
		<< "host:" << config_.host << " "
		<< "port:" << config_.port << " "
		<< "db:" << config_.db << " "
		<< "user:" << config_.username;


	return info.str();
}

MYSQL* MysqlClient::mysql_reconnect()
{
	if (mysql_ctx_ == nullptr || mysql_ping(mysql_ctx_) != 0)
	{
		init_mysql_ctx();

		if (mysql_ctx_ != nullptr)
		{
			LOG(INFO) << "mysql reconnect " << get_info() << " OK";
		}
		else
		{
			LOG(ERROR) << "mysql reconnect " << get_info() << " error";
		}
	}
	return mysql_ctx_;
}

bool MysqlClient::mysql_exec(const std::string& sql)
{
	if (mysql_reconnect() == nullptr)
	{
		LOG(ERROR) << "mysqlexec error, sql:" << sql.c_str();
		return false;
	}
		
	if (mysql_real_query(mysql_ctx_, sql.c_str(), sql.size()) != 0)
	{
		LOG(ERROR) << "mysql error:" << mysql_error(mysql_ctx_) << " sql :" << sql;
		return false;
	}
	return true;
}

uint32_t MysqlClient::mysql_last_affected_rows()
{
	if (mysql_reconnect() == nullptr)
	{
		LOG(ERROR) << __func__ << "() mysql reconnect error";
		return 0;
	}

	return mysql_affected_rows(mysql_ctx_);
}

uint32_t MysqlClient::get_insert_id_uint32()
{
	mysql_exec("select  @@IDENTITY as id;");

	MysqlResult result(handle());
	MysqlRow row;
	if (!result.fetch_row(row))
	{
		return -1;
	}
	return row.get_uint32("id");
}

uint64_t MysqlClient::get_insert_id_uint64()
{
	mysql_exec("select  @@IDENTITY as id;");

	MysqlResult result(handle());
	MysqlRow row;
	if (!result.fetch_row(row))
	{
		return -1;
	}
	return row.get_uint64("id");
}



MysqlRow::MysqlRow()
    : result_(nullptr)
{
}

int64_t MysqlRow::get_int64(const std::string& key)
{
	return atoll(get_string(key).c_str());
}

uint64_t MysqlRow::get_uint64(const std::string& key)
{
    return strtoull(get_string(key).c_str(), NULL, 10);
}

uint32_t MysqlRow::get_uint32(const std::string& key)
{
    return strtoul(get_string(key).c_str(), NULL, 10);
}

int MysqlRow::get_int(const std::string& key)
{
	return atoi(get_string(key).c_str());
}

double MysqlRow::get_double(const std::string& key)
{
	return stod(get_string(key));
}

const std::string& MysqlRow::get_string(const std::string& key)
{
	int64_t idx = result_->name2idx(key);
	if (idx < 0 || idx >= (int64_t)rows_.size())
	{
		return null_string;
	}
	return rows_[idx];
}


MysqlResult::MysqlResult(MYSQL* mysql)
{
	if (mysql == nullptr)
	{
		result_ = nullptr;
		return;
	}

	result_ = mysql_store_result(mysql);
	MYSQL_FIELD* field = nullptr;
	if (result_ == nullptr)
    {
		return;
    }

	int64_t i = 0;
	while ((field = mysql_fetch_field(result_)))
	{
		fields_[field->name] = i++;
	}
}

MysqlResult::~MysqlResult()
{
	if (result_)
    {
		mysql_free_result(result_);
        result_ = nullptr;
    }
}

int64_t MysqlResult::name2idx(const std::string& name)
{
    auto it = fields_.find(name);
    if (it == fields_.end())
    {
        return -1;
    }
    return it->second;
}

bool MysqlResult::fetch_row(MysqlRow& row)
{
	row.clear();
	row.result_ = this;
	if (result_ == nullptr)
    {
		return false;
    }

	MYSQL_ROW r = mysql_fetch_row(result_);
	if (r == nullptr)
	{
		return false;
	}

	unsigned long* l = mysql_fetch_lengths(result_);
	for (size_t i = 0; i < fields_.size(); ++i)
	{
		if (r[i] == NULL)
        {
			row.rows_.push_back("");
        }
		else
		{
			if (l == nullptr)
            {
				row.rows_.push_back(r[i]);
            }
			else
            {
				row.rows_.push_back(std::string(r[i], l[i]));   // binarry
            }
		}
	}

	if (row.rows_.empty())
    {
		return false;
    }
	return true;
}

std::string mysql_string(const std::string& from,  MysqlClient& mysql)
{
    std::vector<char> temp;
	temp.resize(2 * from.size() + 1);
	if (mysql.handle() == nullptr)
    {
		return std::string();
    }

	mysql_real_escape_string(mysql.handle(), &temp[0], from.c_str(), from.size());
	return &temp[0];
}
