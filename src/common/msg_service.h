/*
 * 继承MsgService必须要做的工作
 *
 * 1. 重写VInitService，而且在子类的VInitService必须首先调用MsgServer的VInitService
 * 2. 在子类的VInitService里必须调用RegServiceName函数注册当前服务
 * 3. 在子类的VInitService里调用SetMsgServiceType设置服务的类型
 * 4. 加载完用户的数据后必须要调用HandleUserInitOK
 * 5. 重写VServiceOverEvent
 * 6. 重写VServerStopEvent
 * 7. 重写VReloadUserInfo，重新从数据库加载用户数据
 * 8. VServiceStartEvent只有MsgSlaveService和Room需要重写
 */

#ifndef __COMMON_MSG_SERVICE_H__
#define __COMMON_MSG_SERVICE_H__

#include "service.h"
#include "pending_processor.h"
#include "../pb/system.pb.h"
#include "../pb/agent.pb.h"
#include "../pb/inner.pb.h"
#include "../tools/text_parm.h"

extern "C"
{
#include "skynet_server.h"
}

enum class MsgServiceType : uint16_t
{
    SERVICE_NULL = 0,
    SERVICE_ROOM,
    SERVICE_ROOMLIST,
    SERVICE_VALUE,
    SERVICE_CLUB,
    SERVICE_DBHELPER,
    SERVICE_MAIL
};

struct MsgServiceTypeHash
{
    size_t operator() (const MsgServiceType& type) const 
    {
        return std::hash<size_t>{}(static_cast<size_t>(type));
    }
};

struct RSPCacheData
{
    uint64_t    timestamp_ = 0;         // us
    std::string data_;
};

struct RSPUidsCache
{
    std::unordered_map<uint64_t, std::shared_ptr<RSPCacheData>>  cache_;   // uid -> RSPCache 
};


class MsgUser;

template <typename User, typename = typename std::enable_if<std::is_base_of<MsgUser, User>::value>::type>
class MsgService : public Service
{
    public:
        using UserPtr = std::shared_ptr<User>;

    public:
        MsgService(MsgServiceType msg_service_type)
            : msg_service_type_(msg_service_type),
              client_msg_dsp_(&service_context_),
              pending_client_msg_(this)
        {
        }

        virtual ~MsgService() = default;
        MsgService(const MsgService<User>&) = delete;
        MsgService<User>& operator= (const MsgService<User>&) = delete;

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override
        {
            if (!Service::VInitService(ctx, parm, len))
            {
                return false;
            }

            // 定时清除消息缓存
            PeriodicallyClearRSPCache();

            RegisterCallBack();
            return true;
        }

        void HandleUserInitOK(UserPtr user)
        {
            if (user == nullptr)
            {
                LOG(ERROR) << "[" << service_name() << "] user is null";
                return;
            }
            user->SetInit(true);
            pending_client_msg_.process_pending(user->GetHandle());
        }

        const typename Dispatcher<UserPtr>::CallBackMap& GetClientMsgCallBacks() const
        {
            return client_msg_dsp_.CallBacks();
        }

    protected:
        void    MsgCallBack(const std::string& name, const typename Dispatcher<UserPtr>::CallBack& func)
        {
            client_msg_dsp_.RegisterCallBack(name, func);
        }

        // 将rsp cache，下次遇到相同的req，直接返回cache，不需要重新计算，uid等于0表示每个用户共享同一个rsp，默认cache 3秒
        void    CacheRSP(Message& msg, uint64_t uid, int second = 3)
        {
            if (!CanUseCache())
            {
                return ;
            }

            auto curr_process_uid = service_context_.current_process_uid();
            if (uid != 0 && uid != curr_process_uid)
            {
                LOG(ERROR) << "cache rsp error. cache_uid: " << uid << " process_uid: " << curr_process_uid;
                return;
            }

            auto cache = std::make_shared<RSPCacheData>();
            cache->timestamp_ = TimeUtil::now_us() + second * 1000000;

            char* data = nullptr;
            uint32_t size = 0;
            SerializeToInnerData(msg, data, size, current_pack_context());
            cache->data_.assign(data, size);
            skynet_free(data);

            auto& uids_cache = rsp_cache_[GetCacheKey()];
            uids_cache.cache_[uid] = cache;
        }

        void    BroadcastMsgToUser(const Message& msg, UserPtr except_user = nullptr)
        {
            for (auto& elem : handles_)
            {
                if (except_user == elem.second)
                {
                    continue;
                }
                SendToAgent(msg, elem.first);
            }
        }

        void ShutdownAgent(uint32_t agent)
        {
            pb::iShutdownAgentREQ   req;
            SendToService(req, agent);
        }

    protected:      // User相关
        UserPtr GetUser(uint64_t uid)
        {
            auto it = users_.find(uid);
            if (it != users_.end())
            {
                return it->second;
            }
            return nullptr;
        }

        UserPtr NewUser(uint32_t agent, uint64_t uid)
        {
            auto user = GetUser(uid);
            if (user)
            {
                handles_.erase(user->GetHandle());
                pending_client_msg_.erase(user->GetHandle());
            }
            else
            {
                user = std::make_shared<User>();
                users_[uid] = user;
            }
            handles_[agent] = user;
            user->InitUser(this, uid, agent);

            return user;
        }

        void DelUser(UserPtr user)
        {
            if (user == nullptr)
            {
                return;
            }
            // 这里需要ClearHandle, 防止其他地方有引用到UserPtr
            user->ClearHandle();
            handles_.erase(user->GetHandle());
            pending_client_msg_.erase(user->GetHandle());
            users_.erase(user->uid());
        }

        void RebindUser(UserPtr user, uint32_t handle)
        {
            if (user == nullptr)
            {
                return;
            }
            handles_.erase(user->GetHandle());
            pending_client_msg_.erase(user->GetHandle());

            handles_[user->GetHandle()] = user;
            user->InitUser(this, user->uid(), handle);
        }

        virtual void VUserDisconnectEvent(UserPtr user) {}
        virtual void VUserReleaseEvent(UserPtr user) {}
        virtual void VUserRebindEvent(UserPtr user) {}
        virtual void VReloadUserInfo(UserPtr user) = 0;

    protected:
        virtual void VBanMessage(const std::string& name) override { client_msg_dsp_.BanMessage(name); }
        virtual void VUnbanMessage(const std::string& name) override { client_msg_dsp_.UnbanMessage(name); }

        virtual void VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session) override
        {
            if (!IsServiceStart())
            {
                LOG(WARNING) << "get msg, but service not start. type: " << static_cast<uint16_t>(msg_service_type_);
                return;
            }

            // 防止雪崩，对过期的消息直接丢弃
            uint64_t nowus = TimeUtil::now_us();
            
            if (service_context_.msg_discard() && (current_pack_context().receive_time_ + service_context_.msg_discard_timeout() < nowus))
            {
                InPack pack;
                pack.ParseFromInnerData(data, size);
                LOG(INFO) << "client message timeout. pb_name: " << pack.type_name() << " uid: " << service_context_.current_process_uid() << " service_type: " << static_cast<uint16_t>(msg_service_type_) << " receive_time: " << current_pack_context().receive_time_ << " now: " << nowus;
                return;
            }

            auto it = handles_.find(handle);
            if (it == handles_.end())
            {
                LOG(WARNING) << "no agent. type: " << static_cast<uint16_t>(msg_service_type_) << " agent: "<< std::hex << handle;
                if (msg_service_type_ == MsgServiceType::SERVICE_ROOM)  // room不需要生成不存在的user
                {
                    return;
                }

                LOG(WARNING) << "message no find handle. reinit agent. type: " << static_cast<uint16_t>(msg_service_type_) << " uid: " << service_context_.current_process_uid() << " handle: " << std::hex << handle;
                pb::iInitAgentREQ   req;
                req.set_handle(skynet_context_handle(ctx()));
                SendToService(req, handle);

                pending_client_msg_.pend(handle);
                return;
            }

            auto user = it->second;
            if (!user->IsInit())
            {
				LOG(INFO) << "user not init ok. pending msg. uid:" << user->uid();
                pending_client_msg_.pend(handle);
                return;
            }

            if (Cache(nowus))
            {
                return;
            }
            
            DispatcherStatus status = client_msg_dsp_.DispatchMessage(data, size, user);
            if (status != DispatcherStatus::DISPATCHER_SUCCESS)
            {
                if (status == DispatcherStatus::DISPATCHER_CALLBACK_ERROR)
                {
                    InPack  pack;
                    pack.ParseFromInnerData(data, size);
                    LOG(WARNING) << "dispatch error. service_type: " << static_cast<uint16_t>(msg_service_type_) << " pb_name: " << pack.type_name(); 
                }
                ShutdownAgent(handle);
            }

            return;
        }

    protected:
        virtual void VServiceStartEvent() override
        {
            uint32_t handle = skynet_context_handle(ctx());

            SendClientMsgCallBack(handle);
            LOG(INFO) << "[" << service_name() << "] start.";
        }

        void SendClientMsgCallBack(uint32_t handle)
        {
            pb::iRegisterProto msg;
            msg.set_type(static_cast<uint32_t>(msg_service_type_));
            msg.set_handle(handle);
            for (auto& elem : client_msg_dsp_.CallBacks())
            {
                msg.add_proto(elem.first);
            }
            SendToSystem(msg, main_);
        }

    private:
        void RegisterCallBack()
        {
            ServiceCallBack(pb::iAgentInit::descriptor()->full_name(), std::bind(&MsgService::HandleServiceAgentInit, this, std::placeholders::_1, std::placeholders::_2));
            ServiceCallBack(pb::iAgentDisconnect::descriptor()->full_name(), std::bind(&MsgService::HandleServiceAgentDisconnect, this, std::placeholders::_1, std::placeholders::_2));
            ServiceCallBack(pb::iAgentRebind::descriptor()->full_name(), std::bind(&MsgService::HandleServiceAgentRebind, this, std::placeholders::_1, std::placeholders::_2));
            ServiceCallBack(pb::iAgentRelease::descriptor()->full_name(), std::bind(&MsgService::HandleServiceAgentRelease, this, std::placeholders::_1, std::placeholders::_2));

            ServiceCallBack(pb::iReloadUserInfo::descriptor()->full_name(), std::bind(&MsgService::HandleServiceReloadUserInfo, this, std::placeholders::_1, std::placeholders::_2));
        }

        // 定时清除消息缓存
        void    PeriodicallyClearRSPCache()
        {
            rsp_cache_.clear();
            StartTimer(kClearRSPCacheInterval, std::bind(&MsgService::PeriodicallyClearRSPCache, this));
        }

        bool    CanUseCache() const
        {
            if (GlobalContext::GetInstance()->rsp_cache_switch() && curr_msg_type_ == PTYPE_CLIENT)
            {
                return true;
            }
            return false;
        }

        std::string GetCacheKey() const
        {
            if (curr_msg_data_.size() <= sizeof(PackContext))
            {
                LOG(ERROR) << "curr_msg_data size: " << curr_msg_data_.size() << " < PackContext size: " << sizeof(PackContext);
                return std::string();
            }
            return std::string(curr_msg_data_.c_str() + sizeof(PackContext), curr_msg_data_.size() - sizeof(PackContext));
        }

        bool    Cache(uint64_t nowus)
        {
            if (!CanUseCache())
            {
                return false;
            }

            auto caches = rsp_cache_.find(GetCacheKey());
            if (caches == rsp_cache_.end())
            {
                return false;
            }

            auto& uids_cache = caches->second.cache_;
            auto curr_process_uid = service_context_.current_process_uid();
            auto cache_iter = std::find_if(uids_cache.begin(), uids_cache.end(), [curr_process_uid](const std::pair<uint64_t, std::shared_ptr<RSPCacheData>>& elem) { return elem.first == 0 || elem.first == curr_process_uid; });
            if (cache_iter == uids_cache.end())
            {
                return false;
            }
            if (cache_iter->second->timestamp_ < nowus) // 过期了
            {
                return false;
            }

            std::shared_ptr<RSPCacheData>   rsp;
            rsp = cache_iter->second;

            SendToAgent(rsp->data_, curr_msg_source_);

            InPack  pack;
            pack.ParseFromInnerData(rsp->data_.c_str(), rsp->data_.size());
            service_context_.MonitorDownstreamTraffic(pack.type_name(), pack.data_len(), TimeUtil::now_tick_us());

            LOG(INFO) << "send cache rsp. uid:" << curr_process_uid << " " << pack.type_name() << " rsp size:" << rsp->data_.size();
            return true;
        }

    private:
        void HandleServiceAgentInit(MessagePtr data, uint32_t handle)
        {
            LOG(INFO) << "[" << service_name() << "] agent init event. handle: " << std::hex << handle << " service_handle: " << skynet_context_handle(ctx()); 
            auto msg = std::dynamic_pointer_cast<pb::iAgentInit>(data);
            auto uid = msg->uid();

            auto curr_process_uid = service_context_.current_process_uid();
            if (uid != curr_process_uid)
            {
                LOG(ERROR) << __PRETTY_FUNCTION__ << " uid != processuid, uid: " << uid << ", process_uid: " << curr_process_uid;
            }

            auto user = NewUser(handle, uid);
            if (user == nullptr)
            {
                LOG(ERROR) << "NewUser failed";
                return;
            }
            user->SetIP(msg->ip());
        }

        void HandleServiceAgentDisconnect(MessagePtr data, uint32_t handle)
        {
            LOG(INFO) << "[" << service_name() << "] agent disconnect event. handle: " << std::hex << handle << " service_handle: " << skynet_context_handle(ctx()); 
            auto msg = std::dynamic_pointer_cast<pb::iAgentDisconnect>(data);
            uint64_t uid = msg->uid();

            auto curr_process_uid = service_context_.current_process_uid();
            if (uid != curr_process_uid)
            {
                LOG(ERROR) << __PRETTY_FUNCTION__ << " uid != processuid, uid: " << uid << ", process_uid: " << curr_process_uid;
            }

            auto user = GetUser(uid);
            if (user == nullptr)
            {
                return;
            }
            VUserDisconnectEvent(user);
        }

        void HandleServiceAgentRebind(MessagePtr data, uint32_t handle)
        {
            LOG(INFO) << "[" << service_name() << "] agent rebind event. handle: " << std::hex << handle << " service_handle: " << skynet_context_handle(ctx()); 
            auto msg = std::dynamic_pointer_cast<pb::iAgentRebind>(data);
            auto uid = msg->uid();

            auto curr_process_uid = service_context_.current_process_uid();
            if (uid != curr_process_uid)
            {
                LOG(ERROR) << __PRETTY_FUNCTION__ << " uid != processuid, uid: " << uid << ", process_uid: " << curr_process_uid;
            }

            auto user = GetUser(uid);
            if (user == nullptr)
            {
                return;
            }

            RebindUser(user, handle);
            user->SetIP(msg->ip());
            VUserRebindEvent(user);
        }

        void HandleServiceAgentRelease(MessagePtr data, uint32_t handle)
        {
            LOG(INFO) << "[" << service_name() << "] agent release event. handle: " << std::hex << handle << " service_handle: " << skynet_context_handle(ctx()); 
            auto it = handles_.find(handle);
            if (it == handles_.end())
            {
                LOG(INFO) << "agent release. but agent dont exist. type: " << static_cast<uint16_t>(msg_service_type_);
                return;
            }

            auto user = it->second;

            auto uid = user->uid();
            auto curr_process_uid = service_context_.current_process_uid();
            if (uid != curr_process_uid)
            {
                LOG(ERROR) << __PRETTY_FUNCTION__ << " uid != processuid, uid: " << uid << ", process_uid: " << curr_process_uid;
            }

            if (user->GetHandle() != handle)
            {
                LOG(ERROR) << "agent release. but not the correct agent. type:" << static_cast<uint16_t>(msg_service_type_);
                return;
            }

            VUserReleaseEvent(user);
            DelUser(user);
        }

        void HandleServiceReloadUserInfo(MessagePtr data, uint32_t)
        {
            auto msg = std::dynamic_pointer_cast<pb::iReloadUserInfo>(data);
            uint64_t uid = msg->uid();
            UserPtr user = GetUser(uid);
            if (user == nullptr)
            {
                return;
            }

            VReloadUserInfo(user);
        }

    protected:
        std::unordered_map<uint64_t, UserPtr>     users_;     // uid -> User

    private:
        MsgServiceType                      msg_service_type_;
        Dispatcher<UserPtr>                 client_msg_dsp_;        // 客户端消息分发
        PendingProcessor<uint32_t>          pending_client_msg_;    // Pend客户端消息

        std::unordered_map<uint32_t, UserPtr>     handles_;   // agent_handle -> User

        std::unordered_map<std::string, RSPUidsCache> rsp_cache_;

        static const uint32_t kClearRSPCacheInterval = 2 * 60 * 60 * 100;
};

#endif 
