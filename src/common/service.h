#ifndef __COMMON_SERVICE_H__
#define __COMMON_SERVICE_H__

#include "timer.h"
#include "rpc.h"
#include "../pb/monitor.pb.h"

#define MONITOR_SERVICE_INTERVAL (5 * 100)

template<typename, typename, typename> class PendingProcessor;

/* 
 * 继承Service必须要做的工作
 *
 * 1. 重写VInitService，而且在子类的VInitService必须首先调用父类的VInitService
 * 2. 在子类的VInitService里必须调用RegServiceName函数注册当前服务
 * 3. 重写VServiceStartEvent
 * 4. 重写VServiceOverEvent
 * 5. 重写VServerStopEvent
 */

class Service : public Timer, public RPCClient, public RPCServer
{
    template<typename, typename, typename> friend class PendingProcessor;

    using PTypeParsePackContextCallBack = std::function<void(const char* data)>;

public:
    Service ();
    virtual ~Service () = default;
    Service (Service const&) = delete;
    Service& operator= (Service const&) = delete; 

public:
    void        ServicePoll(const char* data, uint32_t size, uint32_t source, int session, int type);
    void        ServicePollString(const std::string& data, uint32_t source, int session, int type)  { ServicePoll(data.c_str(), data.size(), source, session, type); }

    uint32_t    ServiceHandle(const std::string& name) const;

    uint32_t    NewChildService(const std::string& name, const std::string& param);

    // 带上uid的话，对方会收到一个uid，master-slave模式的service会根据这个uid hash转发
    void        SendToService(const Message& msg, uint32_t handle);
    void        SendToService(const Message& msg, uint32_t handle, uint64_t uid);
    void        SendToService(const Message& msg, uint32_t handle, const PackContext& context, uint32_t source = 0);

    void        SendToSystem(const Message& msg, uint32_t handle);
    void        SendToSystem(const Message& msg, uint32_t handle, const PackContext& pctx, uint32_t source = 0);

    void        SendToAgent(const Message& msg, uint32_t agent);
    void        SendToAgent(const std::string& msg, uint32_t agent);

    void        WrapSkynetSend(skynet_context* ctx, uint32_t source, uint32_t dest, int type, int session, void* msg, size_t sz) { service_context_.WrapSkynetSend(ctx, source, dest, type, session, msg, sz); }
    void        WrapSkynetSocketSend(skynet_context* ctx, int id, const Message& msg, SocketType socket_type, uint32_t gate) { service_context_.WrapSkynetSocketSend(ctx, id, msg, socket_type, gate); }
    void        WrapSkynetSocketSend(skynet_context* ctx, int id, void* inner_data, uint32_t size, SocketType socket_type, uint32_t gate) { service_context_.WrapSkynetSocketSend(ctx, id, inner_data, size, socket_type, gate); }

public:
    /*
     * 被override后，需要在override的函数最开始主动调用
     */
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len);       


protected:
    void    RegisterPTypeParsePackCTXCallBack(int type, const PTypeParsePackContextCallBack& func);
    void    ServiceCallBack(const std::string& name, const typename Dispatcher<uint32_t>::CallBack& func);
    void    SystemCallBack(const std::string& name, const typename Dispatcher<uint32_t>::CallBack& func);

    void    DependOnService(const std::string& name);
    void    RegServiceName(const std::string& name, bool expose, bool monitor);
    const std::string&  service_name() const { return service_context_.service_name(); }
    const PackContext&  current_pack_context() const { return service_context_.pack_context(); }
    skynet_context*     ctx() const { return service_context_.get_skynet_context(); }

    void    AddChild(uint32_t handle) { children_.insert(handle); }
    bool    DelChild(uint32_t handle) { return children_.erase(handle) > 0; }
    void    SystemBroadcastToChildren(const Message& msg);

    void    ResponseTextMessage(const std::string& from, const std::string& rsp);

    bool    IsServiceStart() const { return service_started_; }
    bool    IsServerStop() const { return server_stop_; }

     // 主动关闭服务，这个函数调用后不能再执行其他语句
    void    ShutdownService();

    uint32_t    GetParentHandle() const { return parent_; }
    
    bool     ParseTextMessage(const std::string& msg, std::string& from, uint64_t& uid, std::string& data);

protected:
    virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session);
    virtual void VHandleResponseMessage(const char* data, uint32_t size, uint32_t handle, int session);
    virtual void VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session) {}
    virtual void VHandleForwardMsg(const char* data, uint32_t size, uint32_t handle, int session) {}
    virtual void VHandleServiceMsg(const char* data, uint32_t size, uint32_t source, int session);  // 内部消息回调
    virtual void VHandleSystemMsg(const char* data, uint32_t size, uint32_t source, int session);  // 系统消息
    virtual void VHandleRPCServerMsg(const char* data, uint32_t size, uint32_t source, int session);
    virtual void VHandleRPCClientMsg(const char* data, uint32_t size, uint32_t source, int session);


    virtual void VBanMessage(const std::string& name) {}
    virtual void VUnbanMessage(const std::string& name) {}

    virtual void VServiceStartEvent() = 0;   // 服务启动完成
    virtual void VServiceOverEvent() = 0;    // 服务关闭事件
    virtual void VServerStopEvent() = 0;    // 关闭服务器事件

private:
    bool    CheckServiceDepend() const;
    void    TryStartServer();
    void    RegisterCallBack();

    void    RegMonitor();
    void    MonitorHeartBeat();
    void    FillMonitorInfo(pb::MonitorInfo* info);

private:
    void    HandleSystemNameList(MessagePtr data, uint32_t handle);
    void    HandleSystemServerStop(MessagePtr data, uint32_t handle);

    void    HandleTextMessage();

protected:
    ServiceContext          service_context_;
    Dispatcher<uint32_t>    system_msg_dsp_;        // 系统消息分发
    Dispatcher<uint32_t>    service_msg_dsp_;       // 内部服务的消息分发

protected:
    bool    service_started_;       // 服务是否已启动
    bool    server_stop_;           // 服务是否关闭

protected:
    std::unordered_map<std::string, uint32_t>   depend_services_;
    uint32_t                        main_;
    uint32_t                        parent_;
    std::unordered_set<uint32_t>    children_;

private:
    bool    monitor_;               // 是否启动集群监控

    std::unordered_map<int, PTypeParsePackContextCallBack>  ptype_parse_pack_ctx_cbs_;

protected:
    // 当前消息信息
    uint32_t    curr_msg_source_;
    int         curr_msg_session_;
    int         curr_msg_type_;
    std::string curr_msg_data_;
};
#endif
