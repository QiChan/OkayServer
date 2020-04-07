#include "global_context.h"

GlobalContext::GlobalContext()
    : proto_profile_switch_(false),
      profile_log_interval_(kDefaultProfileLogInterval),
      upstream_monitor_producer_(nullptr),
      upstream_monitor_client_(nullptr),
      downstream_monitor_producer_(nullptr),
      downstream_monitor_client_(nullptr),
      workload_monitor_producer_(nullptr),
      workdload_monitor_client_(nullptr),
      msg_discard_switch_(true),
      msg_discard_timeout_(kDefaultMsgDiscardTimeout),
      rsp_cache_switch_(true)
{
    std::string profile = "true";
    if (skynet_getenv("proto_profile") == profile)
    {
        set_proto_profile(true);
    }
}

GlobalContext::~GlobalContext()
{
    if (upstream_monitor_producer_ != nullptr)
    {
        destroy_log_producer(upstream_monitor_producer_);
    }
    if (downstream_monitor_producer_ != nullptr)
    {
        destroy_log_producer(downstream_monitor_producer_);
    }
    if (workload_monitor_producer_ != nullptr)
    {
        destroy_log_producer(workload_monitor_producer_);
    }
    log_producer_env_destroy();
}

void GlobalContext::set_proto_profile(bool profile)
{
    proto_profile_switch_.store(profile);
}

bool GlobalContext::proto_profile() const
{
    return proto_profile_switch_.load();
}

void GlobalContext::set_profile_log_interval(uint64_t interval)
{
    profile_log_interval_.store(interval);
}

uint64_t GlobalContext::profile_log_interval() const
{
    return profile_log_interval_.load();
}

void GlobalContext::set_upstream_monitor(log_producer* producer, log_producer_client* client)
{
    upstream_monitor_producer_.store(producer);
    upstream_monitor_client_.store(client);
}

log_producer_client* GlobalContext::get_upstream_monitor_client() const
{
    return upstream_monitor_client_.load();
}

void GlobalContext::set_downstream_monitor(log_producer* producer, log_producer_client* client)
{
    downstream_monitor_producer_.store(producer);
    downstream_monitor_client_.store(client);
}

log_producer_client* GlobalContext::get_downstream_monitor_client() const
{
    return downstream_monitor_client_.load();
}

void GlobalContext::set_workload_monitor(log_producer* producer, log_producer_client* client)
{
    workload_monitor_producer_.store(producer);
    workdload_monitor_client_.store(client);
}

log_producer_client* GlobalContext::get_workload_monitor_client() const
{
    return workdload_monitor_client_.load();
}

bool GlobalContext::msg_discard() const
{
    return msg_discard_switch_.load();
}

uint64_t GlobalContext::msg_discard_timeout() const
{
    return msg_discard_timeout_.load();
}

void GlobalContext::set_discard_msg(bool discard, uint64_t timeout)
{
    msg_discard_switch_.store(discard);
    msg_discard_timeout_.store(timeout);
}

bool GlobalContext::rsp_cache_switch() const
{
    return rsp_cache_switch_.load();
}

void GlobalContext::set_rsp_cache_switch(bool cache_switch)
{
    rsp_cache_switch_.store(cache_switch);
}
