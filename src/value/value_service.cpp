#include "value_service.h"
#include "../common/dbconfig.h"

ValueService::ValueService()
    : MsgService(MsgServiceType::SERVICE_VALUE)
{
}

ValueService::~ValueService()
{
}

bool ValueService::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!MsgService::VInitService(ctx, parm, len))
    {
        return false;
    }

    if (!ConnectMysql())
    {
        return false;
    }

    if (!InitValueMgr())
    {
        return false;
    }

    RegServiceName("value", true, true);

    RegisterCallBack();

    return true;
}

void ValueService::VReloadUserInfo(UserPtr user)
{
}

void ValueService::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void ValueService::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

bool ValueService::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    if (Service::VHandleTextMessage(from, data, source, session))
    {
        return true;
    }

    if (from == "php")
    {
        TextParm    parm(data.c_str());
        std::string response = "no cmd\n";
        const char* cmd = parm.get("cmd");
        if (strcmp(cmd, "changevalue") == 0)
        {
            auto value_type = static_cast<pb::ValueType>(parm.get_int("value_type"));
            pb::ValueKey    key;
            key.set_uid(parm.get_uint64("uid"));
            key.set_clubid(parm.get_uint("clubid"));
            PHPChangeValue(value_type, key, parm.get_int64("change"), parm.get_string("attach"), response);
            
        }

        ResponseTextMessage(from, response);
    }
    else
    {
        LOG(ERROR) << "from: " << from << " unknown text command: " << data;
    }
    return true;
}

void ValueService::PHPChangeValue(pb::ValueType value_type, const pb::ValueKey& key, int64_t change, const std::string& attach, std::string& strrsp)
{
    LOG(INFO) << "php change value. " << key.ShortDebugString() << " value_type: " << pb::ValueType_Name(value_type) << " change: " << change << " attach: " << attach;

    auto mgr = GetValueMgr(value_type);
    if (mgr == nullptr)
    {
        LOG(WARNING) << "no mgr. value_type: " << pb::ValueType_Name(value_type);
        strrsp = "change failed";
        return;
    }

    auto result = std::get<0>(mgr->ChangeValue(key, change, "php", attach));
    if (result == OperateValueRetCode::OPERATE_OK)
    {
        strrsp = "change value ok\n";
    }
    else
    {
        strrsp = "change value failed\n";
    }
}

void ValueService::RegisterCallBack()
{
    MsgCallBack(pb::ValueREQ::descriptor()->full_name(), [this](MessagePtr data, UserPtr user){
        auto msg = std::dynamic_pointer_cast<pb::ValueREQ>(data);
        auto ret = GetValue(msg->value_type(), msg->key());
        if (std::get<0>(ret) == OperateValueRetCode::NO_VALUE)
        {
            return;
        }

        pb::ValueRSP    rsp;
        rsp.set_value_type(msg->value_type());
        rsp.mutable_key()->CopyFrom(msg->key());
        rsp.set_value(std::get<1>(ret));
        user->SendToUser(rsp);
    });

    ServiceCallBack(pb::iChangeValueREQ::descriptor()->full_name(), [this](MessagePtr data, uint32_t){
        auto msg = std::dynamic_pointer_cast<pb::iChangeValueREQ>(data);
        auto ret = ChangeValue(msg);
        if (std::get<0>(ret) != OperateValueRetCode::OPERATE_OK)
        {
            LOG(ERROR) << "change value failed. " << msg->ShortDebugString() 
                << " ret_code: " << static_cast<int16_t>(std::get<0>(ret))
                << " value: " << std::get<1>(ret);
        }
    });

    RegisterRPC(pb::iChangeValueREQ::descriptor()->full_name(), [this](MessagePtr data) -> MessagePtr {
        auto msg = std::dynamic_pointer_cast<pb::iChangeValueREQ>(data);
        auto ret = ChangeValue(msg);
        auto rsp = std::make_shared<pb::iChangeValueRSP>();
        rsp->set_code(-1);
        rsp->set_value(std::get<1>(ret));
        if (std::get<0>(ret) == OperateValueRetCode::OPERATE_OK)
        {
            rsp->set_code(0);
        }
        return rsp;
    });
}

bool ValueService::ConnectMysql()
{
    MysqlConfigMgr  config_mgr;
    if (!config_mgr.VLoadConfig())
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER, &mysql_master_))
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER_FLOW, &mysql_master_flow_))
    {
        return false;
    }

    return true;
}

bool ValueService::InitValueMgr()
{
    value_mgrs_[pb::USER_MONEY] = std::make_shared<ValueMgr<pb::USER_MONEY>>(this);
    value_mgrs_[pb::CLUB_USER_CHIPS] = std::make_shared<ValueMgr<pb::CLUB_USER_CHIPS>>(this);

    return true;
}

std::shared_ptr<BaseValueMgr> ValueService::GetValueMgr(pb::ValueType value_type)
{
    auto it = value_mgrs_.find(value_type);
    if (it == value_mgrs_.end())
    {
        return nullptr;
    }
    return it->second;
}

std::tuple<OperateValueRetCode, int64_t> ValueService::GetValue(pb::ValueType value_type, const pb::ValueKey& key)
{
    auto value_mgr = GetValueMgr(value_type);
    if (value_mgr == nullptr)
    {
        LOG(ERROR) << "value_mgr not exists. value_type: " << pb::ValueType_Name(value_type);
        return std::make_tuple(OperateValueRetCode::NO_VALUE, 0);
    }
    return value_mgr->GetValue(key);
}

std::tuple<OperateValueRetCode, int64_t> ValueService::ChangeValue(BaseValueMgr::ChangeValueMsgPtr msg)
{
    auto value_mgr = GetValueMgr(msg->value_type());
    if (value_mgr == nullptr)
    {
        LOG(ERROR) << "value_mgr not exists. value_type: " << pb::ValueType_Name(msg->value_type());
        return std::make_tuple(OperateValueRetCode::NO_VALUE, 0);
    }
    return value_mgr->OperateValue(msg);
}

template<pb::ValueType VALUE_TYPE>
Value::ValuePtr ValueService::LoadValueFromDB(const pb::ValueKey& key)
{
    LOG(ERROR) << "unknown value_type: " << pb::ValueType_Name(VALUE_TYPE);
    return nullptr;
}

template<>
Value::ValuePtr ValueService::LoadValueFromDB<pb::USER_MONEY>(const pb::ValueKey& key)
{
    std::stringstream sql;
    sql << "select `money` from `money` where `uid` = " << key.uid();
    mysql_master_.mysql_exec(sql.str());
    MysqlResult result(mysql_master_.handle());
    MysqlRow row;
    if (!result.fetch_row(row))
    {
        return nullptr;
    }
    return std::make_shared<Value>(row.get_int64("money"));
}

template<>
Value::ValuePtr ValueService::LoadValueFromDB<pb::CLUB_USER_CHIPS>(const pb::ValueKey& key)
{
    std::stringstream sql;
    sql << "select `chips` from `club_user_chips` where `clubid` = "
        << " " << key.clubid() << " and "
        << " `uid` = " << key.uid();
    mysql_master_.mysql_exec(sql.str());
    MysqlResult result(mysql_master_.handle());
    MysqlRow    row;
    if (!result.fetch_row(row))
    {
        return nullptr;
    }
    return std::make_shared<Value>(row.get_int64("chips"));
}

template<pb::ValueType VALUE_TYPE>
bool ValueService::StoreValueToDB(const pb::ValueKey& key, int64_t value, int64_t change, const std::string& type, const std::string& attach)
{
    LOG(ERROR) << "unknown value_type: " << pb::ValueType_Name(VALUE_TYPE) << " " << key.ShortDebugString() << " value: " << value << " change: " << change << " type: " << type << " attach: " << attach;
    return true;
}

template<>
bool ValueService::StoreValueToDB<pb::USER_MONEY>(const pb::ValueKey& key, int64_t value, int64_t change, const std::string& type, const std::string& attach)
{
    {
        std::stringstream sql;
        sql << "update money set money = " << value << " where uid = " << key.uid();
        if (!mysql_master_.mysql_exec(sql.str()))
        {
            return false;
        }
    }

    {
        std::stringstream sql;
        uint64_t now = time(NULL);
        sql << "insert into flow_money(uid, `change`, `type`, `attach`, `left`, `time`) values("
            << key.uid() << ", "
            << change << ", "
            << "'" << mysql_string(type, mysql_master_flow_) << "', "
            << "'" << mysql_string(attach, mysql_master_flow_) << "', "
            << value << ", "
            << now << ")";
        if (!mysql_master_flow_.mysql_exec(sql.str()))
        {
            LOG(ERROR) << "insert into flow_money failed. key: " << key.ShortDebugString() << " change: " << change << " type: " << type << " attach: " << attach << " left: " << value << " time: " << now;
        }
    }
    return true;
}

template<>
bool ValueService::StoreValueToDB<pb::CLUB_USER_CHIPS>(const pb::ValueKey& key, int64_t value, int64_t change, const std::string& type, const std::string& attach)
{
    {
        std::stringstream sql;
        sql << "update `club_user_chips` set `chips` = " << value << " where `clubid` = "
            << " " << key.clubid() << " and "
            << " `uid` = " << key.uid();
        if (!mysql_master_.mysql_exec(sql.str()))
        {
            return false;
        }
    }

    {
        std::stringstream sql;
        uint64_t now = time(NULL);
        sql << "insert into `flow_club_user_chips`(`clubid`, `uid`, `change`, `type`, `attach`, `left`, `time`) values("
            << " " << key.clubid() << ", "
            << " " << key.uid() << ", "
            << " " << change << ", "
            << " '" << mysql_string(type, mysql_master_flow_) << "', "
            << " '" << mysql_string(attach, mysql_master_flow_) << "', "
            << " " << value << ", "
            << " " << now << ")";
        if (!mysql_master_flow_.mysql_exec(sql.str()))
        {
            LOG(ERROR) << "insert into flow_club_user_chips failed. key: " << key.ShortDebugString() << " change: " << change << " type: " << type << " attach: " << attach << " left: " << value << " time: " << now;
        }
    }

    return true;
}
