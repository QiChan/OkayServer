#ifndef __AGENT_AGENT_H__
#define __AGENT_AGENT_H__

#include "../common/service.h"
#include "leaky_bucket.h"
#include "network_system.h"

class Agent final : public Service
{
    private:
        friend class SubSystem;

        struct RoomInfo
        {
            RoomInfo() = delete;
            RoomInfo(const RoomInfo&) = delete;
            RoomInfo& operator= (const RoomInfo&) = delete;
            ~RoomInfo() = default;
            RoomInfo(uint32_t roomid, uint32_t room_handle, uint32_t clubid)
                : roomid_(roomid), room_handle_(room_handle), clubid_(clubid)
            {
            }
            uint32_t        roomid_ = 0;
            uint32_t        room_handle_ = 0;
            uint32_t        clubid_ = 0;
        };

    public:
        Agent();
        ~Agent();
        Agent(const Agent&) = delete;
        Agent& operator= (const Agent&) = delete;

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

    public:
        void MsgCallBack(const std::string& name, const typename Dispatcher<uint64_t>::CallBack& func);

        void BroadcastToService(const Message& msg);
        void SendToUser(const Message& msg);
        void SendToUser(void* inner_data, uint32_t size);

        uint64_t uid() const { return uid_; }

    protected:
        virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;
        virtual void VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session) override;
        virtual void VHandleForwardMsg(const char* data, uint32_t size, uint32_t handle, int session) override;

        virtual void VServiceStartEvent() override;
        virtual void VServiceOverEvent() override;
        virtual void VServerStopEvent() override;

    private:
        void    RegisterCallBack();
        void    ClientForward(const char* data, uint32_t size, uint32_t dest);
        std::shared_ptr<RoomInfo>   GetRoom(uint32_t roomid);

        void    HandleServiceAgentRoomStatus(MessagePtr data, uint32_t handle);

        bool    FilterEnterRoomREQ(MessagePtr data);
        bool    FilterClientMsg(const InPack& pack);

    private:
        Dispatcher<uint64_t>        client_msg_dsp_;
        uint64_t                    uid_;
        uint64_t                    create_timestamp_;      // 创建时间戳
        LeakyBucket                 rate_limiter_;          
        NetworkSystem               network_subsystem_;

        std::unordered_map<uint32_t, std::shared_ptr<RoomInfo>>  rooms_;     // 玩家所在的房间 roomid -> RoomInfo
        uint32_t    curr_roomid_;

    private:
        using FilterFunc = std::function<bool(MessagePtr)>;
        std::unordered_map<std::string, FilterFunc>     filters_;
};

#endif 
