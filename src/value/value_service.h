#ifndef __VALUE_VALUE_SERVICE_H__
#define __VALUE_VALUE_SERVICE_H__
#include "../common/msg_service.h"
#include "../common/mysql_client.h"
#include "value_user.h"
#include "value_mgr.h"

struct ValueTypeHash
{
    int operator() (const pb::ValueType& type) const
    {
        return std::hash<int>{}(static_cast<int>(type));
    }
};

class ValueService final : public MsgService<ValueUser>
{
    public:
        ValueService();
        virtual ~ValueService();
        ValueService(const ValueService&) = delete;
        ValueService& operator= (const ValueService&) = delete;

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;
        virtual void VReloadUserInfo(UserPtr user) override;

    public:
        template<pb::ValueType  VALUE_TYPE>
        Value::ValuePtr LoadValueFromDB(const pb::ValueKey& key);

        template<pb::ValueType VALUE_TYPE>
        bool StoreValueToDB(const pb::ValueKey& key, int64_t value, int64_t change, const std::string& type, const std::string& attach);

    protected:
        virtual void VServiceOverEvent() override;
        virtual void VServerStopEvent() override;
        virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;

    private:
        void    RegisterCallBack();
        bool    ConnectMysql();
        bool    InitValueMgr();
        
        void    PHPChangeValue(pb::ValueType value_type, const pb::ValueKey& key, int64_t change, const std::string& attach, std::string& strrsp);
        std::shared_ptr<BaseValueMgr>   GetValueMgr(pb::ValueType value_type);
        std::tuple<OperateValueRetCode, int64_t>   GetValue(pb::ValueType value_type, const pb::ValueKey& key);
        std::tuple<OperateValueRetCode, int64_t>   ChangeValue(BaseValueMgr::ChangeValueMsgPtr msg);

    private:
        MysqlClient     mysql_master_;
        MysqlClient     mysql_master_flow_;
        std::unordered_map<pb::ValueType, std::shared_ptr<BaseValueMgr>, ValueTypeHash>    value_mgrs_;
};

#endif
