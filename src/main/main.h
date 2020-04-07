#ifndef __MAIN_MAIN_H__
#define __MAIN_MAIN_H__

#include <tools/text_parm.h>
#include "../common/msg_service.h"
#include "../common/redis_client.h"

struct MsgServiceInfo
{
    MsgServiceType          msg_service_type_;
    uint32_t                handle_;
    std::unordered_set<std::string> proto_list_;
};

class Main final : public Service
{
    public:
        Main();
        ~Main() = default;
        Main(const Main&) = delete;
        Main& operator= (const Main&) = delete;

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

    protected:
        virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;
        virtual void VServiceStartEvent() override;
        virtual void VServiceOverEvent() override;
        virtual void VServerStopEvent() override;

    private:
        void    RegisterCallBack();
        void    HandleSystemRegisterName(MessagePtr data, uint32_t handle);
        void    HandleSystemRegisterWatchdog(MessagePtr data, uint32_t handle);
        void    HandleSystemRegisterProto(MessagePtr data, uint32_t handle);

        MessagePtr  HandleRPCCheckLoginREQ(MessagePtr data);

        void    ServiceBroadcastToNamedService(const Message& msg);
        void    SystemBroadcastToNamedService(const Message& msg);
        void    ServiceBroadcastToMsgService(const Message& msg, uint64_t uid);
        void    TextMessageBroadcast(TextParm& parm);
        void    TextMessageBroadcastToWatchdog(TextParm& parm);

        uint32_t    GetServiceHandle(const std::string& name);

        void    PhpStopServer();
        void    PhpKickUser(uint64_t uid);

        bool    ConnectUserRedis();

        bool    CheckVersion(const std::string& version) const;
        bool    LoadClientVersion();
        std::tuple<std::string, std::string, std::string>    ParseClientVersion(const std::string& version) const;


    private:
        std::unordered_map<std::string, uint32_t>   named_services_;
        std::vector<uint32_t>                       watchdogs_;
        std::unordered_map<MsgServiceType, MsgServiceInfo, MsgServiceTypeHash>    msg_services_;
        RedisClient                                 user_redis_;
        std::string                                 client_version_;
        bool                                        check_rdkey_;
};

#endif
