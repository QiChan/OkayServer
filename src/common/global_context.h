/*
 * 进程全局上下文，进程内的单例
 * 需要保证线程安全，最好不要用锁，用CAS操作
 */

#ifndef __COMMON_GLOBAL_CONTEXT_H__
#define __COMMON_GLOBAL_CONTEXT_H__

#include <atomic>
#include "singleton.h"
#include "../aliyunlog/log_producer_client.h"
#include "../aliyunlog/log_producer_config.h"

extern "C"
{
#include "skynet_env.h"
}

class GlobalContext final : public Singleton<GlobalContext>
{
public:
    GlobalContext ();
    virtual ~GlobalContext ();
    GlobalContext (const GlobalContext&) = delete;
    GlobalContext& operator= (const GlobalContext&) = delete;

public:
    void    set_proto_profile(bool profile);
    bool    proto_profile() const;

    void        set_profile_log_interval(uint64_t interval);
    uint64_t    profile_log_interval() const;

    void    set_upstream_monitor(log_producer* producer, log_producer_client* client);
    void    set_downstream_monitor(log_producer* producer, log_producer_client* client);
    void    set_workload_monitor(log_producer* producer, log_producer_client* client);
    log_producer_client*    get_upstream_monitor_client() const;
    log_producer_client*    get_downstream_monitor_client() const;
    log_producer_client*    get_workload_monitor_client() const;

public:
    bool        msg_discard() const;
    uint64_t    msg_discard_timeout() const;
    void        set_discard_msg(bool discard, uint64_t timeout);
    bool        rsp_cache_switch() const;
    void        set_rsp_cache_switch(bool cache_switch);

private:
    static const uint64_t   kDefaultProfileLogInterval = 60000000;    // us
    std::atomic<bool>       proto_profile_switch_;      // 是否开启接口监控
    std::atomic<uint64_t>   profile_log_interval_;      // 监控日志上传时间间隔
    // 上行日志
    std::atomic<log_producer*>              upstream_monitor_producer_;
    std::atomic<log_producer_client*>       upstream_monitor_client_;
    // 下行日志
    std::atomic<log_producer*>              downstream_monitor_producer_;
    std::atomic<log_producer_client*>       downstream_monitor_client_;
    // 负载日志
    std::atomic<log_producer*>              workload_monitor_producer_;
    std::atomic<log_producer_client*>       workdload_monitor_client_;

private:
    static const uint64_t   kDefaultMsgDiscardTimeout = 15000000;   // us
    std::atomic<bool>       msg_discard_switch_;
    std::atomic<uint64_t>   msg_discard_timeout_;       // 客户端消息超时丢弃时间 us
    std::atomic<bool>       rsp_cache_switch_;
};

#endif
