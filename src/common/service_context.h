#ifndef __COMMON_SERVICE_CONTEXT_H__
#define __COMMON_SERVICE_CONTEXT_H__

#include "ali_yun_log.h"
#include "pack.h"
#include <unordered_map>
#include "socket_type.h"

extern "C"
{
#include <skynet_socket.h>
}

class skynet_context;
// 存储每个服务的上下文信息
class ServiceContext final
{
public:
    ServiceContext ();
    ~ServiceContext () = default;
    ServiceContext (ServiceContext const&) = delete;
    ServiceContext& operator= (ServiceContext const&) = delete; 

public:
    void        InitServiceContext(skynet_context* ctx);
    void        ServiceOverEvent();

public:
    uint16_t    harborid() const { return harborid_; }
    void        set_service_name(const std::string& name) { service_name_ = name; }
    const std::string&  service_name() const { return service_name_; }
    skynet_context*     get_skynet_context() const { return ctx_; }

public:
    bool        msg_discard() const { return GlobalContext::GetInstance()->msg_discard(); }
    uint64_t    msg_discard_timeout() const { return GlobalContext::GetInstance()->msg_discard_timeout(); }
    void        set_discard_msg(bool discard, uint64_t timeout) { GlobalContext::GetInstance()->set_discard_msg(discard, timeout); }
    bool        rsp_cache_switch() const { return GlobalContext::GetInstance()->rsp_cache_switch(); }
    void        set_rsp_cache_switch(bool cache_switch) { GlobalContext::GetInstance()->set_rsp_cache_switch(cache_switch); }

public:
    uint64_t    current_process_uid() const { return pack_context_.process_uid_; }
    void        set_current_process_uid(uint64_t uid) { pack_context_.process_uid_ = uid; }
    void        set_pack_context(const PackContext& packctx) { pack_context_ = packctx; }
    void        clear_pack_context() { pack_context_.reset(); }
    void        reset_pack_context(uint64_t process_uid, uint64_t now_tick_us = 0) { pack_context_.ResetContext(process_uid, harborid(), now_tick_us); }
    void        parse_pack_context(const void* msg) { pack_context_.ParseFromStream(msg); }
    const PackContext& pack_context() const { return pack_context_; }

public:
    void    MonitorDownstreamTraffic(const std::string& type_name, uint32_t data_len, uint64_t now_tick_us);
    void    MonitorUpstreamTraffic(const std::string& type_name, uint32_t data_len, uint64_t now_tick_us);
    void    MonitorWorkload(const std::string& type_name, int64_t spend_us, uint64_t now_tick_us);

    bool    proto_profile() const { return GlobalContext::GetInstance()->proto_profile(); }
    void    set_proto_profile(bool profile) { GlobalContext::GetInstance()->set_proto_profile(profile); }
    void    set_profile_log_interval(uint64_t interval) { GlobalContext::GetInstance()->set_profile_log_interval(interval); }

public:
    int     WrapSkynetSend(struct skynet_context* ctx, uint32_t source, uint32_t destination, int type, int session, void* msg, size_t sz);
    int     WrapSkynetSocketSend(struct skynet_context* ctx, int id, const Message& msg, SocketType socket_type, uint32_t gate);
    int     WrapSkynetSocketSend(struct skynet_context* ctx, int id, const void* inner_data, uint32_t sz, SocketType socket_type, uint32_t gate);

private:
    int     SkynetSocketSendPlus(struct skynet_context* ctx, int id, void* data, uint32_t size, SocketType socket_type, uint32_t gate);

// service自身数据
private:
    skynet_context*         ctx_;
    std::string             service_name_;          
    uint16_t                harborid_;

// 当前上下文信息
private:
    PackContext         pack_context_;          // 当前正在处理的协议上下文

// 日志统计
private:
    std::unordered_map<std::string, ContextTrafficLog>   upstream_traffic_log_;     // 上行流量日志
    std::unordered_map<std::string, ContextTrafficLog>   downstream_traffic_log_;   // 下行流量日志
    std::unordered_map<std::string, ContextWorkloadLog>  workload_log_;             // 负载日志
};

#endif 
