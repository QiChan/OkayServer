#include "ali_yun_log.h"
#include "../aliyunlog/log_api.h"
#include <tools/time_util.h>

extern "C"
{
#include "skynet_env.h"
}

void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message, const unsigned char * raw_buffer)
{
	if (result == LOG_PRODUCER_OK)
	{
/*		printf("send success, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s \n",
			config_name, (result),
			(int)log_bytes, (int)compressed_bytes, req_id);
*/

        /*
        LOG(INFO) << "aliyunlog send success, config: " << config_name
            << " result: " << result
            << " log bytes: " << log_bytes
            << " compressed bytes: " << compressed_bytes 
            << " request id: " << req_id;
            */
	}
	else
	{
/*		printf("send fail, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s\n",
			config_name, (result),
			(int)log_bytes, (int)compressed_bytes, req_id, message);
*/
        LOG(ERROR) << "aliyunlog send fail, config: " << config_name
            << " result: " << result
            << " log bytes: " << log_bytes
            << " compressed bytes: " << compressed_bytes
            << " request id: " << req_id
            << " error message: " << message;
	}

}

log_producer * create_log_producer_wrapper(const char* project_name, const char* logstore, const char* topic, on_log_producer_send_done_function on_send_done)
{
	log_producer_config* config = create_log_producer_config();
	// endpoint list:  https://help.aliyun.com/document_detail/29008.html
	log_producer_config_set_endpoint(config, skynet_getenv("aliyunlog"));
	log_producer_config_set_project(config, project_name);
	log_producer_config_set_logstore(config, logstore);
	log_producer_config_set_access_id(config, skynet_getenv("aliyunlog_access_id"));
	log_producer_config_set_access_key(config, skynet_getenv("aliyunlog_access_key"));

	// @note, if using security token to send log, remember reset token before expired. this function is thread safe
	//log_producer_config_reset_security_token(config, "${your_access_key_id}", "${your_access_key_secret}", "${your_access_security_token}");

	// if you do not need topic or tag, comment it
	log_producer_config_set_source(config, skynet_getenv("service_name"));
	log_producer_config_set_topic(config, topic);
	//log_producer_config_add_tag(config, "tag_1", "val_1");
	//log_producer_config_add_tag(config, "tag_2", "val_2");
	//log_producer_config_add_tag(config, "tag_3", "val_3");
	//log_producer_config_add_tag(config, "tag_4", "val_4");
	//log_producer_config_add_tag(config, "tag_5", "val_5");

	// set resource params
	log_producer_config_set_packet_log_bytes(config, 4 * 1024 * 1024);
	log_producer_config_set_packet_log_count(config, 4096);
	log_producer_config_set_packet_timeout(config, 3000);
	log_producer_config_set_max_buffer_limit(config, 64 * 1024 * 1024);

	// set send thread count
	log_producer_config_set_send_thread_count(config, 2);

	// set compress type : lz4
	log_producer_config_set_compress_type(config, 1);

	// set timeout
	log_producer_config_set_connect_timeout_sec(config, 10);
	log_producer_config_set_send_timeout_sec(config, 15);
	log_producer_config_set_destroy_flusher_wait_sec(config, 1);
	log_producer_config_set_destroy_sender_wait_sec(config, 1);

	// set interface
	log_producer_config_set_net_interface(config, NULL);

	return create_log_producer(config, on_send_done);
}


void log_producer_post_logs()
{
	if (log_producer_env_init(LOG_GLOBAL_ALL) != LOG_PRODUCER_OK) {
		exit(1);
	}

    // 初始化云日志库连接
    const char* logstore = skynet_getenv("aliyunlog_logstore");
    const char* project_name = skynet_getenv("aliyunlog_project");
    {
        log_producer* producer = create_log_producer_wrapper(project_name, logstore, "upstream", on_log_send_done);
        if (producer == nullptr)
        {
            printf("create log producer by config fail \n");
            exit(1);
        }

        log_producer_client* client = get_log_producer_client(producer, NULL);
        if (client == NULL)
        {
            printf("create log producer client by config fail \n");
            exit(1);
        }

        GlobalContext::GetInstance()->set_upstream_monitor(producer, client);
    }

    {
        log_producer* producer = create_log_producer_wrapper(project_name, logstore, "downstream", on_log_send_done);
        if (producer == nullptr)
        {
            printf("create log producer by config fail \n");
            exit(1);
        }

        log_producer_client* client = get_log_producer_client(producer, NULL);
        if (client == NULL)
        {
            printf("create log producer client by config fail \n");
            exit(1);
        }

        GlobalContext::GetInstance()->set_downstream_monitor(producer, client);
    }

    {
        log_producer* producer = create_log_producer_wrapper(project_name, logstore, "workload", on_log_send_done);
        if (producer == nullptr)
        {
            printf("create log producer by config fail \n");
            exit(1);
        }

        log_producer_client* client = get_log_producer_client(producer, NULL);
        if (client == NULL)
        {
            printf("create log producer client by config fail \n");
            exit(1);
        }

        GlobalContext::GetInstance()->set_workload_monitor(producer, client);
    }
}

ContextAliyunLog::ContextAliyunLog(uint64_t now_us)
    : last_update_time_(now_us),
      pack_num_(0)
{
}

bool ContextAliyunLog::CheckUpdateLog(uint64_t now_us)
{
    return (now_us - last_update_time_) >= GlobalContext::GetInstance()->profile_log_interval() ? true : false;
}

size_t ContextAliyunLog::pack_num() const
{
    return pack_num_;
}

void ContextAliyunLog::reset(uint64_t now_us)
{
    last_update_time_ = now_us;
    pack_num_ = 0;
}

ContextTrafficLog::ContextTrafficLog()
    : ContextAliyunLog(TimeUtil::now_tick_us()),
      total_traffic_sz_(0)
{
}

void ContextTrafficLog::reset(uint64_t now_us)
{
    ContextAliyunLog::reset(now_us);
    total_traffic_sz_ = 0;
}

size_t ContextTrafficLog::total_sz() const
{
    return total_traffic_sz_;
}

void ContextTrafficLog::StatTraffic(size_t sz)
{
    total_traffic_sz_ += sz;
    pack_num_++;
}

ContextWorkloadLog::ContextWorkloadLog()
    : ContextAliyunLog(TimeUtil::now_tick_us()),
      total_spend_us_(0),
      max_spend_us_(0)
{
}

void ContextWorkloadLog::reset(uint64_t now_us)
{
    ContextAliyunLog::reset(now_us);
    total_spend_us_ = 0;
    max_spend_us_ = 0;
}

void ContextWorkloadLog::StatSpend(uint64_t spend_us)
{
    total_spend_us_ += spend_us;
    if (spend_us > max_spend_us_)
    {
        max_spend_us_ = spend_us;
    }
    pack_num_++;
}

uint64_t ContextWorkloadLog::GetAvgSpendUs() const
{
    if (pack_num_ == 0)
    {
        return total_spend_us_;
    }
    return total_spend_us_ / pack_num_;
}

uint64_t ContextWorkloadLog::GetTotalSpendUs() const
{
    return total_spend_us_;
}

uint64_t ContextWorkloadLog::GetMaxSpendUs() const
{
    return max_spend_us_;
}
