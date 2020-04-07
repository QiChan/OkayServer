#ifndef __VALUE_VALUE_CLIENT_H__
#define __VALUE_VALUE_CLIENT_H__

#include "../common/service.h"
#include "../pb/inner.pb.h"

// 面向对象接口
class ValueClient
{
    public:
        ValueClient(pb::ValueType value_type, pb::ValueOP op)
        {
            req_.set_value_type(value_type);
            req_.set_op(op);
            key_ = req_.mutable_key();
        }
        ~ValueClient() = default;
        ValueClient(const ValueClient&) = delete;
        ValueClient& operator= (const ValueClient&) = delete;

    public:
        void    set_type(const std::string& type) { req_.set_type(type); }
        void    set_attach(const std::string& attach) { req_.set_attach(attach); }
        void    set_change(int64_t change) { req_.set_change(change); }
        void    set_uid(uint64_t uid) { key_->set_uid(uid); }
        void    set_clubid(uint32_t clubid) { key_->set_clubid(clubid); }

        void    exec(Service* service)
        {
            service->SendToService(req_, service->ServiceHandle("value"));
        }

        void    call(Service* service, const RPCCallBack& func)
        {
            service->CallRPC(service->ServiceHandle("value"), req_, func);
        }

    protected:
        pb::iChangeValueREQ     req_;
        pb::ValueKey*           key_;
};

// 面向过程接口
void OperateValue(Service* service, pb::ValueType value_type, pb::ValueOP op, const pb::ValueKey& key, int64_t change, const std::string& type, const std::string& attach, const RPCCallBack& func = nullptr)
{
    pb::iChangeValueREQ req;
    req.set_value_type(value_type);
    req.mutable_key()->CopyFrom(key);
    req.set_op(op);
    req.set_change(change);
    req.set_type(type);
    req.set_attach(attach);
    if (func == nullptr)
    {
        service->SendToService(req, service->ServiceHandle("value"));
    }
    else
    {
        service->CallRPC(service->ServiceHandle("value"), req, func);
    }
}

inline void ChangeUserMoney(Service* service, uint64_t uid, int64_t change, const std::string& type, const std::string& attach, const RPCCallBack& func = nullptr)
{
    pb::ValueKey    key;
    key.set_uid(uid);
    OperateValue(service, pb::USER_MONEY, pb::VALUEOP_CHANGE, key, change, type, attach, func);
}

inline void GetUserMoney(Service* service, uint64_t uid, const RPCCallBack& func)
{
    pb::ValueKey    key;
    key.set_uid(uid);
    OperateValue(service, pb::USER_MONEY, pb::VALUEOP_GET, key, 0, "", "", func);
}

inline void ChangeClubUserChips(Service* service, uint32_t clubid, uint64_t uid, int64_t change, const std::string& type, const std::string& attach, const RPCCallBack& func = nullptr)
{
    pb::ValueKey    key;
    key.set_clubid(clubid);
    key.set_uid(uid);
    OperateValue(service, pb::CLUB_USER_CHIPS, pb::VALUEOP_CHANGE, key, change, type, attach, func);
}

inline void GetClubUserChips(Service* service, uint32_t clubid, uint64_t uid, const RPCCallBack& func)
{
    pb::ValueKey key;
    key.set_clubid(clubid);
    key.set_uid(uid);
    OperateValue(service, pb::CLUB_USER_CHIPS, pb::VALUEOP_GET, key, 0, "", "", func);
}

#endif
