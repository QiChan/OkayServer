#include "main.h"
#include <tools/string_util.h>
#include "../pb/system.pb.h"
#include "../pb/agent.pb.h"
#include "../pb/inner.pb.h"
#include "../common/scope_guard.h"
#include "../json/json.h"
#include <fstream>

extern "C"
{
#include "skynet.h"
}

Main::Main()
    : check_rdkey_(true)
{
}

bool Main::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    if (!ConnectUserRedis())
    {
        return false;
    }

    if (!LoadClientVersion())
    {
        return false;
    }

    if (atoi(skynet_getenv("check_rdkey")) == 0)
    {
        check_rdkey_ = false;
        LOG(WARNING) << "check_rdkey is false";
    }

    RegServiceName("main", true, true);

    RegisterCallBack();
    return true;
}


bool Main::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    if (Service::VHandleTextMessage(from, data, source, session))
    {
        return true;
    }

    TextParm parm(data.c_str());
    if (from == "lua")
    {
        const char* cmd = parm.get("cmd");
        uint32_t handle = 0;
        if (strcmp(cmd, "service_handle") == 0)
        {
            handle = GetServiceHandle(parm.get_string("name"));
        }
        char rsp[100];
        sprintf(rsp, "%u", handle);
        ResponseTextMessage(from, rsp);
    }
    else if (from == "php")
    {
        std::string response = "no cmd\n";
        const char* cmd = parm.get("cmd");
        if (strcmp(cmd, "listen") == 0)
        {
            TextParm sendparm;
            sendparm.insert("cmd", "listen");
            TextMessageBroadcastToWatchdog(sendparm);
            response = "listen ok\n";
        }
        else if (strcmp(cmd, "set_rate") == 0)
        {
            TextParm sendparm;
            sendparm.insert("cmd", "set_rate");
            sendparm.insert_uint("rate_switch", parm.get_uint("rate_switch"));
            sendparm.insert_uint("rate", parm.get_uint("rate"));
            sendparm.insert_uint("bucket_cap", parm.get_uint("bucket_cap"));
            TextMessageBroadcastToWatchdog(sendparm);
            response = "set rate ok\n";
        }
        else if (strcmp(cmd, "stopserver") == 0)
        {
            PhpStopServer();
            response = "stop server ok\n";
        }
        else if (strcmp(cmd, "kick_user") == 0)
        {
            PhpKickUser(parm.get_uint64("uid"));
            response = "kick user ok\n";
        }
        else if (strcmp(cmd, "disable_profile_all") == 0)
        {
            TextParm sendparm;
            sendparm.insert("cmd", "disable_profile");
            sendparm.insert_int("broadcast", 1);
            TextMessageBroadcast(sendparm);

            response = "disable_profile_all ok\n";
        }
        else if (strcmp(cmd, "enable_profile_all") == 0)
        {
            TextParm sendparm;
            sendparm.insert("cmd", "enable_profile");
            sendparm.insert_uint("profile_interval", parm.get_uint("profile_interval"));
            sendparm.insert_int("broadcast", 1);
            TextMessageBroadcast(sendparm);

            response = "enable_profile_all ok\n";
        }
        else if (strcmp(cmd, "set_discard_msg_all") == 0)
        {
            TextParm sendparm;
            sendparm.insert("cmd", "set_discard_msg");
            sendparm.insert_uint("discard", parm.get_uint("discard"));
            sendparm.insert_uint("timeout", parm.get_uint("timeout"));
            sendparm.insert_int("broadcast", 1);
            TextMessageBroadcast(sendparm);

            response = "set_discard_msg_all ok\n";
        }
        else if (strcmp(cmd, "reconn_redis") == 0)
        {
            if (!ConnectUserRedis())
            {
                response = "reconn redis failed.\n";
            }
            else
            {
                response = "reconn redis ok.\n";
            }
        }
        else if (strcmp(cmd, "reload_version") == 0)
        {
            if (!LoadClientVersion())
            {
                response = "reload version failed.\n";
            }
            else
            {
                response = "reload version ok.\n";
            }
        }
        else if (strcmp(cmd, "reload_user_info") == 0)
        {
            pb::iReloadUserInfo brc;
            uint64_t uid = parm.get_uint64("uid");
            brc.set_uid(uid);
            ServiceBroadcastToMsgService(brc, uid);
            response = "reload user ok.\n";
        }

        ResponseTextMessage(from, response);
    }
    else
    {
        LOG(ERROR) << "from: " << from << " unknow text command: " << data;
    }

    return true;
}

void Main::HandleSystemRegisterName(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iRegisterName>(data);
    if (!msg->name().empty())
    {
        auto name = msg->name();
        auto expose = msg->expose();
        auto handle = msg->handle();
        LOG(INFO) << "get service name register. name:[" << name << "] handle:" << std::hex << msg->handle() << " expose:" << expose;
        if (expose)
        {
            pb::iNameList   brc;
            pb::ServiceInfo* item = brc.add_service_info();
            item->set_handle(handle);
            item->set_name(name);
            SystemBroadcastToNamedService(brc);

            named_services_[name] = handle;
        }
    }

    pb::iNameList   rsp;
    for (auto& elem : named_services_)
    {
        pb::ServiceInfo* item = rsp.add_service_info();
        item->set_handle(elem.second);
        item->set_name(elem.first);
    }
    SendToSystem(rsp, handle);
}

void Main::HandleSystemRegisterWatchdog(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iRegisterWatchdog>(data);
    uint32_t dog = msg->handle();
    watchdogs_.push_back(dog);
    LOG(INFO) << "main get watchdog. handle:" << std::hex << dog;

    pb::iRegisterMsgService req;
    for (auto& elem : msg_services_)
    {
        auto& service = elem.second;
        pb::iRegisterProto* item = req.add_item();
        item->set_type(static_cast<uint32_t>(service.msg_service_type_));
        item->set_handle(service.handle_);
        for (auto& e : service.proto_list_)
        {
            item->add_proto(e);
        }
    }
    SendToSystem(req, dog);
}

void Main::HandleSystemRegisterProto(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iRegisterProto>(data);
    MsgServiceType type = static_cast<MsgServiceType>(msg->type());

    MsgServiceInfo& service = msg_services_[type];
    service.msg_service_type_ = type;

    service.handle_ = msg->handle();

    for (auto& elem : msg->proto())
    {
        service.proto_list_.insert(elem);
    }
    LOG(INFO) << "register msg service:" << static_cast<int>(type) << " size:" << msg_services_.size();

    pb::iRegisterMsgService req;
    pb::iRegisterProto* item = req.add_item();
    item->set_type(static_cast<uint32_t>(service.msg_service_type_));
    item->set_handle(service.handle_);
    for (auto& elem : service.proto_list_)
    {
        item->add_proto(elem);
    }
    for (auto& wd : watchdogs_)
    {
        SendToSystem(req, wd);
    }
}

void Main::ServiceBroadcastToNamedService(const Message& msg)
{
    for (auto& elem : named_services_)
    {
        SendToService(msg, elem.second);
    }
}

void Main::SystemBroadcastToNamedService(const Message& msg)
{
    for (auto& elem : named_services_)
    {
        SendToSystem(msg, elem.second);
    }
}

void Main::RegisterCallBack()
{
    SystemCallBack(pb::iRegisterName::descriptor()->full_name(), std::bind(&Main::HandleSystemRegisterName, this, std::placeholders::_1, std::placeholders::_2));

    SystemCallBack(pb::iRegisterWatchdog::descriptor()->full_name(), std::bind(&Main::HandleSystemRegisterWatchdog, this, std::placeholders::_1, std::placeholders::_2));

    SystemCallBack(pb::iRegisterProto::descriptor()->full_name(), std::bind(&Main::HandleSystemRegisterProto, this, std::placeholders::_1, std::placeholders::_2));

    RegisterRPC(pb::iCheckLoginREQ::descriptor()->full_name(), std::bind(&Main::HandleRPCCheckLoginREQ, this, std::placeholders::_1));
    
}

uint32_t Main::GetServiceHandle(const std::string& name)
{
    auto it = named_services_.find(name);
    if (it != named_services_.end())
    {
        return it->second;
    }
    return 0;
}

void Main::ServiceBroadcastToMsgService(const Message& msg, uint64_t uid)
{
    for (auto& elem : msg_services_)
    {
        if (elem.second.handle_ == 0)
        {
            LOG(INFO) << "[" << service_name() << "] handle is zero. type: " << static_cast<uint16_t>(elem.first);
            continue;
        }
        SendToService(msg, elem.second.handle_, uid);
    }
}

void Main::TextMessageBroadcastToWatchdog(TextParm& parm)
{
    char sendline[256];
    int len = snprintf(sendline, sizeof(sendline) - 1, "main %lu %s", service_context_.current_process_uid(), parm.serilize());
    if (len >= 256)
    {
        LOG(ERROR) << "parm too long. parm: " << parm.serilize();
        return;
    }
    for (auto& wd : watchdogs_)
    {
        WrapSkynetSend(ctx(), 0, wd, PTYPE_TEXT, 0, sendline, strlen(sendline));
    }
}

void Main::TextMessageBroadcast(TextParm& parm)
{
    char sendline[256];
    int len = snprintf(sendline, sizeof(sendline) - 1, "main %lu %s", service_context_.current_process_uid(), parm.serilize());
    if (len >= 256)
    {
        LOG(ERROR) << "parm too long. parm: " << parm.serilize();
        return;
    }
    for (auto& ns : named_services_)
    {
        WrapSkynetSend(ctx(), 0, ns.second, PTYPE_TEXT, 0, sendline, strlen(sendline));
    }
}

void Main::PhpStopServer()
{
    pb::iServerStop brc;
    SystemBroadcastToNamedService(brc);
}

void Main::PhpKickUser(uint64_t uid)
{
    pb::iKickUser   req;
    req.set_uid(uid);

    for (auto& wd : watchdogs_)
    {
        SendToService(req, wd);
    }
}

void Main::VServiceStartEvent()
{
    LOG(INFO) << "[" << service_name() << "] start.";
}

void Main::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void Main::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

bool Main::ConnectUserRedis()
{
    RedisConfigMgr  config_mgr;
    if (!config_mgr.VLoadConfig())
    {
        return false; 
    }

    RedisConfig config;
    if (!config_mgr.GetConfig(USER_REDIS, config))
    {
            return false;
    }

    if (!user_redis_.redis_connect(config))
    {
        return false;
    }
    return true;
}

MessagePtr Main::HandleRPCCheckLoginREQ(MessagePtr data)
{
    auto msg = std::dynamic_pointer_cast<pb::iCheckLoginREQ>(data); 
    uint64_t uid = msg->uid();
    auto rdkey = msg->rdkey();
    auto version = msg->version();

    std::shared_ptr<pb::iCheckLoginRSP> rsp = std::make_shared<pb::iCheckLoginRSP>();
    do
    {
        if (!CheckVersion(version))
        {
            rsp->set_code(-2);
            break;
        }

        if (!check_rdkey_)
        {
            break;
        }

        std::string verify_rdkey;
        user_redis_.redis_hget(uid, "rdkey", verify_rdkey);
        if (verify_rdkey.empty() || verify_rdkey != rdkey)
        {
            rsp->set_code(-1);
            break;
        }

   } while(false);

    std::string client_ip;
    user_redis_.redis_hget(uid, "ip", client_ip);
    rsp->set_client_ip(client_ip);
 
    rsp->set_code(0);
    return rsp;
}

bool Main::LoadClientVersion()
{
    std::ifstream ifs;
    ifs.open("./client_publish/version.json");
    if (!ifs)
    {
        LOG(ERROR) << "open version.conf failed.";
        return false;
    }

    ScopeGuard guard([]{}, [&ifs]{ ifs.close(); });

    Json::Reader    reader;
    Json::Value     root;
    if (!reader.parse(ifs, root))
    {
        LOG(ERROR) << "parse version.conf error";
        return false;
    }
    client_version_ = root["client_ver"].asString();
    LOG(INFO) << "load client version success. client_version: " << client_version_;
    return true;
}

bool Main::CheckVersion(const std::string& version) const
{
    std::string curr_head, curr_mid;
    std::tie(curr_head, curr_mid, std::ignore) = ParseClientVersion(client_version_);
    std::string head, mid;
    std::tie(head, mid, std::ignore) = ParseClientVersion(version);
    return curr_head == head && curr_mid == mid;
}

std::tuple<std::string, std::string, std::string> Main::ParseClientVersion(const std::string& version) const
{
    std::string  head, mid, tail;
    std::tie(head, mid) = StringUtil::DivideString(version, '.');
    std::tie(mid, tail) = StringUtil::DivideString(mid, '.');
    return std::make_tuple(head, mid, tail);
}
