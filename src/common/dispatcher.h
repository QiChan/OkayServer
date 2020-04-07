#ifndef __COMMON_DISPATCHER_H__
#define __COMMON_DISPATCHER_H__

#include "service_context.h"
#include <unordered_set>

enum class DispatcherStatus : uint16_t
{
    DISPATCHER_SUCCESS = 0,
    DISPATCHER_PACK_ERROR = 1,      // 包格式错误
    DISPATCHER_PB_ERROR = 2,        // 反序列化失败
    DISPATCHER_CALLBACK_ERROR = 3,  // 协议没有注册回调函数
    DISPATCHER_FORBIDDEN = 4,       // 协议被禁
    DISPATCHER_ERROR = 5,           // 其它错误
};


template<typename T>
class Dispatcher final
{
    public:
        using CallBack      = std::function<void(MessagePtr, T)>;
        using CallBackMap   = std::unordered_map<std::string, CallBack>;

    public:
        Dispatcher() = delete;
        ~Dispatcher() = default;
        Dispatcher(const Dispatcher<T>&) = delete;
        Dispatcher<T>& operator= (const Dispatcher<T>&) = delete;

        Dispatcher(ServiceContext* sctx)
            : service_context_(sctx)
        {
        }

    public:
        void    BanMessage(const std::string& name) 
        {
            LOG(INFO) << "Ban message: " << name;
            filters_.insert(name);
        }

        void    UnbanMessage(const std::string& name)
        {
            LOG(INFO) << "Unban Message: " << name;
            filters_.erase(name);
        }

    public:
        const CallBackMap& CallBacks() const
        {
            return callbacks_;
        }

        void    RegisterCallBack(const std::string& name, const CallBack& func) { callbacks_[name] = func; }

        DispatcherStatus    DispatchMessage(const char* data, uint32_t size, T user)
        {
            InPack pack;
            if (!pack.ParseFromInnerData(data, size))
            {
                LOG(ERROR) << "dispatch message pack error."; 
                return DispatcherStatus::DISPATCHER_PACK_ERROR;
            }
            return dispatch(pack, user);
        }

        DispatcherStatus    DispatchClientMessage(const char* data, uint32_t size, T user)
        {
            InPack  pack;
            if (!pack.ParseFromNetData(service_context_->pack_context(), data, size))
            {
                return DispatcherStatus::DISPATCHER_PACK_ERROR;
            }
            return dispatch(pack, user);
        }

    private:
        DispatcherStatus    dispatch(const InPack& pack, T user)
        {
            // 过滤被禁止的协议
            if (filters_.find(pack.type_name()) != filters_.end())
            {
                return DispatcherStatus::DISPATCHER_FORBIDDEN;
            }

            typename CallBackMap::iterator it = callbacks_.find(pack.type_name());
            if (it == callbacks_.end())
            {
                return DispatcherStatus::DISPATCHER_CALLBACK_ERROR;
            }

            MessagePtr msg = pack.CreateMessage();
            if (msg == nullptr)
            {
                LOG(ERROR) << "dispatch message pb error. type: " << pack.type_name();
                return DispatcherStatus::DISPATCHER_PB_ERROR;
            }

            VLOG(50) << "RECV PACK. proto type: " << pack.type_name() << " content: [" << msg->ShortDebugString() << "]";

            if (service_context_->proto_profile())
            {
                uint64_t start = TimeUtil::now_tick_us();
                service_context_->MonitorDownstreamTraffic(pack.type_name(), pack.data_len(), start);

                it->second(msg, user);

                uint64_t end = TimeUtil::now_tick_us();
                service_context_->MonitorWorkload(pack.type_name(), end - start, end);
            }
            else
            {
                it->second(msg, user);
            }

            return DispatcherStatus::DISPATCHER_SUCCESS;
        }

    private:
        CallBackMap     callbacks_;
        ServiceContext* service_context_;
        std::unordered_set<std::string>     filters_;
};

#endif
