#ifndef __MASTER_MASTER_H__
#define __MASTER_MASTER_H__

#include "../common/service.h"

class Master final : public Service
{
    public:
        Master();
        ~Master() = default;
        Master(const Master&) = delete;
        Master& operator= (const Master&) = delete;

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

    protected:
        virtual void VServiceStartEvent() override;
        virtual void VServiceOverEvent() override;
        virtual void VServerStopEvent() override;

    protected:
        virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;
        virtual void VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session) override;
        virtual void VHandleServiceMsg(const char* data, uint32_t size, uint32_t source, int session) override;
        virtual void VHandleSystemMsg(const char* data, uint32_t size, uint32_t source, int session) override;
        virtual void VHandleRPCServerMsg(const char* data, uint32_t size, uint32_t source, int session) override;
        virtual void VHandleRPCClientMsg(const char* data, uint32_t size, uint32_t source, int session) override;

    private:
        void    ServiceBroadcastFilterInit();
        void    RegisterCallBack();
        void    RouteToSlave(const char* data, uint32_t size, uint32_t source, int session, int type, bool broadcast);

    private:
        int                     slave_num_;
        std::string             slave_name_;
        std::string             slave_parm_;
        std::vector<uint32_t>   slaves_;
        std::unordered_set<std::string>     dsp_broadcast_filter_;  // 存放要广播给slave的协议名
};

#endif 
