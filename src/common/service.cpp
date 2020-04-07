#include "service.h"
#include "scope_guard.h"
#include <tools/string_util.h>
#include <tools/text_parm.h>
#include "../pb/system.pb.h"

extern "C"
{
#include "skynet_server.h"
#include "skynet_handle.h"
}

Service::Service()
    : service_context_(),
      system_msg_dsp_(&service_context_),
      service_msg_dsp_(&service_context_),
      service_started_(false),
      server_stop_(false),
      monitor_(false)
{
    parent_ = skynet_current_handle();

    curr_msg_source_ = 0;
    curr_msg_session_ = 0;
    curr_msg_type_ = 0;
}

bool Service::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    service_context_.InitServiceContext(ctx);
    
    InitTimer(&service_context_);
    InitRPCClient(&service_context_);
    InitRPCServer(&service_context_);

    main_ = skynet_queryname(ctx, ".main");

    DependOnService("main");
    RegisterCallBack();

    return true;
}

void Service::ServiceCallBack(const std::string& name, const typename Dispatcher<uint32_t>::CallBack& func)
{
    service_msg_dsp_.RegisterCallBack(name, func);
}

void Service::SystemCallBack(const std::string& name, const typename Dispatcher<uint32_t>::CallBack& func)
{
    system_msg_dsp_.RegisterCallBack(name, func);
}

void Service::DependOnService(const std::string& name)
{
    depend_services_[name] = 0;
}

void Service::RegisterCallBack()
{
    auto default_parse_pack_ctx_cb = [this](const char* data)
    {
        service_context_.parse_pack_context(data);
    };
    // PTYPE_TEXT和PTYPE_RESPONSE在处理消息的函数里面自己解析pack_context
    RegisterPTypeParsePackCTXCallBack(PTYPE_CLIENT, default_parse_pack_ctx_cb);
    RegisterPTypeParsePackCTXCallBack(PTYPE_AGENT, default_parse_pack_ctx_cb);
    RegisterPTypeParsePackCTXCallBack(PTYPE_SERVICE, default_parse_pack_ctx_cb);
    RegisterPTypeParsePackCTXCallBack(PTYPE_SYSTEM_SERVICE, default_parse_pack_ctx_cb);
    RegisterPTypeParsePackCTXCallBack(PTYPE_RPC_SERVER, default_parse_pack_ctx_cb);
    RegisterPTypeParsePackCTXCallBack(PTYPE_RPC_CLIENT, default_parse_pack_ctx_cb);

    SystemCallBack(pb::iServiceOver::descriptor()->full_name(), [this](MessagePtr data, uint32_t handle)
    {
        auto msg = std::dynamic_pointer_cast<pb::iServiceOver>(data);
        DelChild(msg->handle());
    });

    SystemCallBack(pb::iNameList::descriptor()->full_name(), std::bind(&Service::HandleSystemNameList, this, std::placeholders::_1, std::placeholders::_2));

    // 服务器关闭的消息
    SystemCallBack(pb::iServerStop::descriptor()->full_name(), std::bind(&Service::HandleSystemServerStop, this, std::placeholders::_1, std::placeholders::_2));
}

void Service::VHandleServiceMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    DispatcherStatus status = service_msg_dsp_.DispatchMessage(data, size, source);

    if (status != DispatcherStatus::DISPATCHER_SUCCESS)
    {
        LOG(ERROR) << "dispatch error. service_name: " << service_name() << " status: " << static_cast<uint16_t>(status);
    }
}

void Service::VHandleSystemMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    DispatcherStatus status = system_msg_dsp_.DispatchMessage(data, size, source);

    if (status != DispatcherStatus::DISPATCHER_SUCCESS)
    {
        LOG(ERROR) << "dispatch error. service_name:" << service_name() << " status: " << static_cast<uint16_t>(status);
    }
}

void Service::ServicePoll(const char* data, uint32_t size, uint32_t source, int session, int type)
{
    ScopeGuard guard(
            [this, data, size, source, session, type] 
            {
                curr_msg_source_ = source;
                curr_msg_session_ = session,
                curr_msg_type_ = type;
                curr_msg_data_.assign(data, size);
            },
            [this]
            {
                curr_msg_source_ = 0;
                curr_msg_session_ = 0;
                curr_msg_type_ = 0;
                curr_msg_data_.clear();

                service_context_.clear_pack_context();
            });

    auto parse_cb_it = ptype_parse_pack_ctx_cbs_.find(type);
    if (parse_cb_it != ptype_parse_pack_ctx_cbs_.end() && parse_cb_it->second != nullptr)
    {
        parse_cb_it->second(data);
    }
    else
    {
        if (type != PTYPE_TEXT && type != PTYPE_RESPONSE)
        {
            LOG(ERROR) << "[" << service_name() << "] parse pack context failed, dont have parse callback, type: " << type;
            return;
        }
    }

    switch (type)
    {
        case PTYPE_TEXT:
            {
                HandleTextMessage();
                break;
            }
        case PTYPE_RESPONSE:
            {
                VHandleResponseMessage(data, size, source, session);
                break;
            }
        case PTYPE_CLIENT:
            {
                VHandleClientMsg(data, size, source, session);
                break;
            }
        case PTYPE_AGENT:
            {
                VHandleForwardMsg(data, size, source, session);
                break;
            }
        case PTYPE_SERVICE:
            {
                VHandleServiceMsg(data, size, source, session);
                break;
            }
        case PTYPE_SYSTEM_SERVICE:
            {
                VHandleSystemMsg(data, size, source, session);
                break;
            }
        case PTYPE_RPC_SERVER:
            {
                VHandleRPCServerMsg(data, size, source, session);
                break;
            }
        case PTYPE_RPC_CLIENT:
            {
                VHandleRPCClientMsg(data, size, source, session);
                break;
            }
        default:
            {
                LOG(ERROR) << "[" << service_name() << "] unknown message type: " << type;
                break;
            }
    }
}

void Service::VHandleRPCServerMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    RPCServerEvent(data, size, source, session);
}

void Service::VHandleRPCClientMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    RPCClientEvent(data, size, source, session);
}

void Service::VHandleResponseMessage(const char* data, uint32_t size, uint32_t handle, int session)
{
    TimerTimeout(session);
}

void Service::HandleTextMessage()
{
    if (curr_msg_data_.empty())
    {
        return;
    }

    if (!IsServiceStart())
    {
        LOG(WARNING) << "[" << service_name() << "] service not start. received text message: " << curr_msg_data_;
    }

    uint64_t process_uid = 0;
    std::string text_msg;
    std::string from;
    if (!ParseTextMessage(curr_msg_data_, from, process_uid, text_msg))
    {
        LOG(ERROR) << "parse text message error. msg: " << curr_msg_data_;
        return;
    }


    service_context_.reset_pack_context(process_uid); 

    VHandleTextMessage(from, text_msg, curr_msg_source_, curr_msg_session_);
}

bool Service::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    bool    processed = false;
    if (from != "php" && from != "main")
    {
        return processed;
    }

    std::string response;
    TextParm parm(data.c_str());
    const char* cmd = parm.get("cmd");
    if (strcmp(cmd, "set_vlog_level") == 0)
    {
        FLAGS_v = parm.get_int("vlog_level");
        LOG(INFO) << "[" << service_name() << "] set log flags:" << FLAGS_v;
        response = "set_vlog_level ok\n";
        processed = true;
    }
    else if (strcmp(cmd, "rsp_cache_switch") == 0)
    {
        bool cache_switch = parm.get_int("value");
        service_context_.set_rsp_cache_switch(cache_switch);
        LOG(INFO) << "[" << service_name() << "] web set rsp cache switch:" << cache_switch;
        response = "rsp_cache_switch ok\n";
        processed = true;
    }
    else if (strcmp(cmd, "ban_message") == 0)
    {
        const std::string pb_name = parm.get_string("name");
        VBanMessage(pb_name);
        LOG(INFO) << "[" << service_name() << "] ban message " << pb_name;
        response = "ban_message ok\n";
        processed = true;
    }
    else if (strcmp(cmd, "unban_message") == 0)
    {
        const std::string pb_name = parm.get_string("name");
        VUnbanMessage(pb_name);
        LOG(INFO) << "[" << service_name() << "] unban message " << pb_name;
        response = "unban_message ok\n";
        processed = true;
    }
    else if (strcmp(cmd, "disable_profile") == 0)
    {
        service_context_.set_proto_profile(false);
        LOG(INFO) << "[" << service_name() << "] disable profile.";
        response = "disable_profile ok\n";
        processed = true;
    }
    else if (strcmp(cmd, "enable_profile") == 0)
    {
        service_context_.set_proto_profile(true);
        uint64_t profile_interval = parm.get_uint("profile_interval") * 1000000;
        service_context_.set_profile_log_interval(profile_interval);
        LOG(INFO) << "[" << service_name() << "] enable profile. profile_interval: " << profile_interval;
        response = "enable_profile ok\n";
        processed = true;
    }
    else if (strcmp(cmd, "set_discard_msg") == 0)
    {
        bool discard = parm.get_uint("discard");
        uint64_t timeout = parm.get_uint("timeout") * 1000000;
        service_context_.set_discard_msg(discard, timeout);
        LOG(INFO) << "[" << service_name() << "] set discard msg. discard: " << discard << " timeout: " << timeout;
        response = "set_discard_msg ok\n";
        processed = true;
    }

    if (processed)
    {
        ResponseTextMessage(from, response);
    }
    return processed;
}

uint32_t Service::ServiceHandle(const std::string& name) const
{
    auto it = depend_services_.find(name);
    if (it == depend_services_.end())
    {
        LOG(ERROR) << "service name cannot find. name: " << name << " service: " << service_name();
        return 0;
    }

    if (it->second == 0)
    {
        LOG(ERROR) << "handle is zero. name: " << name << " service:" << service_name();
    }
    return it->second;
}

uint32_t Service::NewChildService(const std::string& name, const std::string& param)
{
    if (IsServerStop())
    {
        LOG(ERROR) << "server stoped, cannot create child service. service_name: " << service_name() << " child_name: " << name << " param: " << param;
        return 0;
    }

    skynet_context* ctx = skynet_context_new(name.c_str(), param.c_str());
    uint32_t handle = 0;
    if (ctx)
    {
        handle = skynet_context_handle(ctx);
        AddChild(handle);
    }
    else
    {
        LOG(ERROR) << "new skynet service error. name: " << name;
    }
    return handle;
}

void Service::SendToService(const Message& msg, uint32_t handle)
{
    SendToService(msg, handle, current_pack_context());
}

void Service::SendToService(const Message& msg, uint32_t handle, uint64_t uid)
{
    PackContext pctx = current_pack_context();
    pctx.process_uid_ = uid;
    SendToService(msg, handle, pctx);
}

void Service::SendToService(const Message& msg, uint32_t handle, const PackContext& pctx, uint32_t source)
{
    if (handle == 0)
    {
        LOG(ERROR) << "[" << service_name() << "] handle is zero. " << msg.ShortDebugString();
        return;
    }

    char* data = nullptr;
    uint32_t size = 0;
    SerializeToInnerData(msg, data, size, pctx);

    WrapSkynetSend(ctx(), source, handle, PTYPE_SERVICE | PTYPE_TAG_DONTCOPY, 0, data, size);
    VLOG(50) << "SEND PTYPE_PROTO PACK. " << pctx << " type: " << msg.GetTypeName() << " content: [" << msg.ShortDebugString() << "]";
}

void Service::SendToSystem(const Message& msg, uint32_t handle)
{
    SendToSystem(msg, handle, current_pack_context());
}

void Service::SendToSystem(const Message& msg, uint32_t handle, const PackContext& pctx, uint32_t source)
{
    if (handle == 0)
    {
        return;
    }

    char* data = nullptr;
    uint32_t size = 0;
    SerializeToInnerData(msg, data, size, pctx);

    WrapSkynetSend(ctx(), source, handle, PTYPE_SYSTEM_SERVICE | PTYPE_TAG_DONTCOPY, 0, data, size);
    VLOG(50) << "SEND PTYPE_SYSTEM_SERVICE PACK. " << pctx << " type: " << msg.GetTypeName() << " content: [" << msg.ShortDebugString() << "]" << " size: " << size;
}

void Service::SendToAgent(const Message& msg, uint32_t agent)
{
    if (agent == 0)
    {
        return;
    }

    char* data = nullptr;
    uint32_t size = 0;
    SerializeToInnerData(msg, data, size, current_pack_context());

	WrapSkynetSend(ctx(), 0, agent, PTYPE_AGENT | PTYPE_TAG_DONTCOPY, 0, data, size);
	VLOG(50) << "SEND PTYPE_AGENT PACK. " << current_pack_context() << " type: " << msg.GetTypeName() << " content: [" << msg.ShortDebugString() << "]" << " size: " << size;
}

void Service::SendToAgent(const std::string& msg, uint32_t agent)
{
    if (agent == 0)
    {
        return;
    }
    WrapSkynetSend(ctx(), 0, agent, PTYPE_AGENT, 0, (void*) msg.c_str(), msg.size());
	VLOG(50) << "SEND PTYPE_AGENT PACK. " << current_pack_context() << " content: [" << msg << "]" << " size: " << msg.size();
}

void Service::ShutdownService()
{
    VServiceOverEvent();

	auto handle = skynet_context_handle(ctx());
	if (parent_ != 0)
	{
		pb::iServiceOver req;
		req.set_handle(handle);
		SendToSystem(req, parent_);
	}

    service_context_.ServiceOverEvent();

	LOG(INFO) << "[" << service_name() << "] retire handle:" << std::hex << handle;
	if (skynet_handle_retire(handle) != 1)
    {
		LOG(ERROR) << "[" << service_name() << "] retire handle error. handle:" << std::hex << handle;
    }
}

bool Service::CheckServiceDepend() const
{
    for (auto& elem : depend_services_)
    {
        if (elem.second == 0)
        {
            return false;
        }
    }

    return true;
}

void Service::TryStartServer()
{
    if (IsServiceStart())
    {
        return;
    }

    if (!CheckServiceDepend())
    {
        return;
    }

    service_started_ = true;

    RegMonitor();

	//定时器执行start，防止start执行在其他流程里。
	StartTimer(1, std::bind(&Service::VServiceStartEvent, this));
}

// TODO: 这里优化，获取其他服务的名称和expose自己可以分开，避免相互依赖的情况
// TODO: 这里会造成服务还没启动，但是依赖它的服务已经依赖完成
void Service::RegServiceName(const std::string& name, bool expose, bool monitor)
{
    if (name == "main")
    {
        main_ = skynet_context_handle(ctx());
    }

    if (monitor)
    {
        DependOnService("monitor");
        monitor_ = true;
    }

    service_context_.set_service_name(name);

    pb::iRegisterName   req;
    req.set_handle(skynet_context_handle(ctx()));
    req.set_name(name);
    req.set_expose(expose);
    SendToSystem(req, main_);
}

void Service::HandleSystemNameList(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iNameList>(data);

    for (int i = 0; i < msg->service_info_size(); ++i)
    {
        const std::string& name = msg->service_info(i).name();
        auto it = depend_services_.find(name);
        if (it != depend_services_.end())
        {
            it->second = msg->service_info(i).handle();
        }
    }

    TryStartServer();

    SystemBroadcastToChildren(*msg);
}

void Service::SystemBroadcastToChildren(const Message& msg)
{
    for (auto& child : children_)
    {
        SendToSystem(msg, child);
    }
}

void Service::ResponseTextMessage(const std::string& from, const std::string& rsp)
{
    // 只能响应从php和lua发过来的消息
    if (!rsp.empty() && (from == "php" || from == "lua"))
    {
        WrapSkynetSend(ctx(), 0, curr_msg_source_, PTYPE_RESPONSE, curr_msg_session_, (void*)rsp.c_str(), rsp.size());
    }
}

void Service::HandleSystemServerStop(MessagePtr data, uint32_t handle)
{
    if (IsServerStop())
    {
        return;
    }

    server_stop_ = true;
    auto msg = std::dynamic_pointer_cast<pb::iServerStop>(data);
    SystemBroadcastToChildren(*msg);

    VServerStopEvent();
}

void Service::RegMonitor()
{
    if (!monitor_)
    {
        return;
    }

    pb::mServiceREG reg;
    pb::MonitorInfo* info = reg.mutable_base();
    FillMonitorInfo(info);
    SendToSystem(reg, ServiceHandle("monitor"));

    StartTimer(MONITOR_SERVICE_INTERVAL, std::bind(&Service::MonitorHeartBeat, this));
}

void Service::MonitorHeartBeat()
{
    pb::mServiceHB  hb;
    hb.set_report_time(time(NULL));
    pb::MonitorInfo* info = hb.mutable_base();
    FillMonitorInfo(info);
    SendToSystem(hb, ServiceHandle("monitor"));

    StartTimer(MONITOR_SERVICE_INTERVAL, std::bind(&Service::MonitorHeartBeat, this));
}

void Service::FillMonitorInfo(pb::MonitorInfo* info)
{
    info->set_name(service_name());
    info->set_harbor(service_context_.harborid());
    info->set_handle(skynet_context_handle(ctx()));
}

bool Service::ParseTextMessage(const std::string& msg, std::string& from, uint64_t& process_uid, std::string& data)
{
    std::tie(from, data) = StringUtil::DivideString(msg, ' ');
    if (from.empty() || data.empty())
    {
        return false;
    }

    if (from == "php" || from == "main" || from == "dog")
    {
        std::string uid;
        std::tie(uid, data) = StringUtil::DivideString(data, ' ');
        if (uid.empty() || data.empty())
        {
            return false;
        }
        process_uid = std::stoull(uid);
    }
    return true;
}

void Service::RegisterPTypeParsePackCTXCallBack(int type, const PTypeParsePackContextCallBack& func)
{
    ptype_parse_pack_ctx_cbs_[type] = func;
}
