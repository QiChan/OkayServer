#ifndef __COMMON_ALIYUNLOG_H__
#define __COMMON_ALIYUNLOG_H__

#include <glog/logging.h>
#include "global_context.h"

#define UPSTREAM_MONITOR(pb_name, total_sz, pack_num, service_name, harborid) do { \
    log_producer_client* client = GlobalContext::GetInstance()->get_upstream_monitor_client(); \
    if (client == nullptr) { \
        printf("upstream client nullptr"); \
    } else { \
        log_producer_client_add_log(client, 10, \
            "pb_name", pb_name.c_str(), \
            "total_sz", std::to_string(total_sz).c_str(), \
            "pack_num", std::to_string(pack_num).c_str(), \
            "service_name", service_name.c_str(), \
            "harborid", std::to_string(harborid).c_str()); \
    } \
} while(0)

#define DOWNSTREAM_MONITOR(pb_name, total_sz, pack_num, service_name, harborid) do { \
    log_producer_client* client = GlobalContext::GetInstance()->get_downstream_monitor_client(); \
    if (client == nullptr) { \
        printf("downstream client nullptr"); \
    } else { \
        log_producer_client_add_log(client, 10, \
            "pb_name", pb_name.c_str(), \
            "total_sz", std::to_string(total_sz).c_str(), \
            "pack_num", std::to_string(pack_num).c_str(), \
            "service_name", service_name.c_str(), \
            "harborid", std::to_string(harborid).c_str()); \
    } \
} while(0)

#define WORKLOAD_MONITOR(pb_name, service_name, harborid, avg_spend_us, total_spend_us, max_spend_us, pack_num) do { \
    log_producer_client* client = GlobalContext::GetInstance()->get_workload_monitor_client(); \
    if (client == nullptr) { \
        printf("workload client nullptr"); \
    } else { \
        log_producer_client_add_log(client, 14, \
            "pb_name", pb_name.c_str(), \
            "service_name", service_name.c_str(), \
            "harborid", std::to_string(harborid).c_str(), \
            "avg_spend_us", std::to_string(avg_spend_us).c_str(), \
            "total_spend_us", std::to_string(total_spend_us).c_str(), \
            "max_spend_us", std::to_string(max_spend_us).c_str(), \
            "pack_num", std::to_string(pack_num).c_str());    \
    } \
} while(0)


void log_producer_post_logs();

// 流量日志
class ContextAliyunLog
{
public:
    ContextAliyunLog (uint64_t now_us);
    virtual ~ContextAliyunLog () = default;
    ContextAliyunLog (ContextAliyunLog const&) = default;
    ContextAliyunLog& operator= (ContextAliyunLog const&) = default; 

public:
    bool    CheckUpdateLog(uint64_t now_us);
    size_t  pack_num() const;
    void    reset(uint64_t now_us);

protected:
    uint64_t        last_update_time_;  // 上次更新时间 us
    size_t          pack_num_;          // 包数量 
};

class ContextTrafficLog final : public ContextAliyunLog
{
public:
    ContextTrafficLog ();
    virtual ~ContextTrafficLog () = default;
    ContextTrafficLog (ContextTrafficLog const&) = default;
    ContextTrafficLog& operator= (ContextTrafficLog const&) = default; 

public:
    void    reset(uint64_t now_us);
    size_t  total_sz() const;
    void    StatTraffic(size_t sz);

private:
    size_t      total_traffic_sz_;      // 总流量大小
};

// 工作负载日志
class ContextWorkloadLog final : public ContextAliyunLog
{
public:
    ContextWorkloadLog ();
    virtual ~ContextWorkloadLog () = default;
    ContextWorkloadLog (ContextWorkloadLog const&) = default;
    ContextWorkloadLog& operator= (ContextWorkloadLog const&) = default; 

public:
    void        reset(uint64_t now_us);
    void        StatSpend(uint64_t spend_us);
    uint64_t    GetAvgSpendUs() const;
    uint64_t    GetTotalSpendUs() const;
    uint64_t    GetMaxSpendUs() const;

private:
    uint64_t    total_spend_us_;        // 总耗时
    uint64_t    max_spend_us_;          // 最大耗时
};

#endif
