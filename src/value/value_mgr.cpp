#include "value_mgr.h"
#include "value_service.h"


template<pb::ValueType VALUE_TYPE>
ValueMgr<VALUE_TYPE>::ValueMgr(ValueService* service)
    : service_(service)
{
}

template<pb::ValueType VALUE_TYPE>
ValueMgr<VALUE_TYPE>::~ValueMgr()
{
}

template<pb::ValueType VALUE_TYPE>
std::tuple<OperateValueRetCode, int64_t> ValueMgr<VALUE_TYPE>::GetValue(const pb::ValueKey& key)
{
    Value::ValuePtr sp_value = _get_value(key);
    if (sp_value == nullptr)
    {
        return std::make_tuple(OperateValueRetCode::NO_VALUE, 0);
    }
    return std::make_tuple(OperateValueRetCode::OPERATE_OK, sp_value->get_value());
}

template<pb::ValueType VALUE_TYPE>
std::tuple<OperateValueRetCode, int64_t> ValueMgr<VALUE_TYPE>::DeleteValue(const pb::ValueKey& key, const std::string& type, const std::string& attach)
{
    return _delete_value(key, type, attach);
}

template<pb::ValueType VALUE_TYPE>
std::tuple<OperateValueRetCode, int64_t> ValueMgr<VALUE_TYPE>::ChangeValue(const pb::ValueKey& key, int64_t change, const std::string& type, const std::string& attach)
{
    return _change_value(key, change, type, attach);
}

template<pb::ValueType VALUE_TYPE>
std::tuple<OperateValueRetCode, int64_t> ValueMgr<VALUE_TYPE>::TestOperateValue(const ChangeValueMsgPtr msg)
{
    pb::ValueKey key = msg->key();

    Value::ValuePtr sp_value = _get_value(key);
    if (sp_value == nullptr)
    {
        return std::make_tuple(OperateValueRetCode::NO_VALUE, 0);
    }

    pb::ValueOP op = msg->op();
    switch(op)
    {
        case pb::VALUEOP_GET:
        {
            return std::make_tuple(OperateValueRetCode::OPERATE_OK, sp_value->get_value());
        }
        case pb::VALUEOP_DELETE:
        {
            return std::make_tuple(OperateValueRetCode::OPERATE_OK, sp_value->get_value());
        }
        case pb::VALUEOP_CHANGE:
        {
            auto change = msg->change();
            auto left = sp_value->get_value();
            if ((left + change) < 0)
            {
                return std::make_tuple(OperateValueRetCode::OPERATE_FAILED, sp_value->get_value());
            }
            return std::make_tuple(OperateValueRetCode::OPERATE_OK, sp_value->get_value());
        }
        default:
        {
            LOG(ERROR) << "op not exists. op: " << op << " " << key.ShortDebugString() << " type: " << msg->type() << " attach: " << msg->attach() << " value_type: " << pb::ValueType_Name(VALUE_TYPE);
            return std::make_tuple(OperateValueRetCode::OPERATE_FAILED, 0);
        }
    }
}

template<pb::ValueType VALUE_TYPE>
std::tuple<OperateValueRetCode, int64_t> ValueMgr<VALUE_TYPE>::OperateValue(const ChangeValueMsgPtr msg)
{
    pb::ValueKey key = msg->key();

    pb::ValueOP op = msg->op();
    switch (op)
    {
        case pb::VALUEOP_GET:
        {
            Value::ValuePtr sp_value = _get_value(key);
            if (sp_value == nullptr)
            {
                return std::make_tuple(OperateValueRetCode::NO_VALUE, 0);
            }
            return std::make_tuple(OperateValueRetCode::OPERATE_OK, sp_value->get_value());
        }
        case pb::VALUEOP_DELETE:
        {
            return _delete_value(key, msg->type(), msg->attach());
        }
        case pb::VALUEOP_CHANGE:
        {
            return _change_value(key, msg->change(), msg->type(), msg->attach());
        }
        default:
        {
            LOG(ERROR) << "op not exists. op: " << op << " " << key.ShortDebugString() << " type: " << msg->type() << " attach: " << msg->attach() << " value_type: " << pb::ValueType_Name(VALUE_TYPE);
            return std::make_tuple(OperateValueRetCode::OPERATE_FAILED, 0);
        }
    }
}

template<pb::ValueType VALUE_TYPE>
std::tuple<OperateValueRetCode, int64_t> ValueMgr<VALUE_TYPE>::_change_value(const pb::ValueKey& key, int64_t change, const string& type, const string& attach)
{
    Value::ValuePtr sp_value = _get_value(key);

    if (sp_value == nullptr)
    {
        return std::make_tuple(OperateValueRetCode::NO_VALUE, 0);
    }

    auto value = sp_value->get_value();
    int64_t left = value + change; 
    if (left < 0)
    {
        return std::make_tuple(OperateValueRetCode::OPERATE_FAILED, value);
    }

    if (!service_->StoreValueToDB<VALUE_TYPE>(key, left, change, type, attach))
    {
        return std::make_tuple(OperateValueRetCode::OPERATE_FAILED, value);
    }

    sp_value->set_value(left);


    LOG(INFO) << "change value.  value_type:" << pb::ValueType_Name(VALUE_TYPE) 
        << " " << key.ShortDebugString() << " change: " << change << " left: " << sp_value->get_value()
        << " type: " << type << " attach: " << attach;

    return std::make_tuple(OperateValueRetCode::OPERATE_OK, sp_value->get_value());
}

template<pb::ValueType VALUE_TYPE>
std::tuple<OperateValueRetCode, int64_t> ValueMgr<VALUE_TYPE>::_delete_value(const pb::ValueKey& key, const string& type, const string& attach)
{
    Value::ValuePtr sp_value = _get_value(key);

    if (sp_value == nullptr)
    {
        return std::make_tuple(OperateValueRetCode::NO_VALUE, 0);
    }

    LOG(INFO) << "delete value. value_type: " << pb::ValueType_Name(VALUE_TYPE) 
        << " " << key.ShortDebugString() << " left: " << sp_value->get_value() 
        << " type: " << type << " attach:" << attach;

    auto temp = sp_value->get_value();

    values_.erase(key);

    return std::make_tuple(OperateValueRetCode::OPERATE_OK, temp);
}

template<pb::ValueType VALUE_TYPE>
Value::ValuePtr ValueMgr<VALUE_TYPE>::_get_value(const pb::ValueKey& key)
{
    auto it = values_.find(key);
    if (it == values_.end())
    {
        return LoadValueFromDB(key);
    }

    return it->second;
}

template<pb::ValueType VALUE_TYPE>
Value::ValuePtr ValueMgr<VALUE_TYPE>::LoadValueFromDB(const pb::ValueKey& key)
{
    Value::ValuePtr sp_value = service_->LoadValueFromDB<VALUE_TYPE>(key);
    if (sp_value == nullptr)
    {
        LOG(ERROR) << "load value error. db dont have value. key: " << key.ShortDebugString() << " type: " << pb::ValueType_Name(VALUE_TYPE);
        return nullptr;
    }

    AddNewValue(key, sp_value);
    return sp_value;
}

template<pb::ValueType VALUE_TYPE>
void ValueMgr<VALUE_TYPE>::AddNewValue(const pb::ValueKey& key, const Value::ValuePtr sp_value)
{
    auto it = values_.find(key);
    if (it == values_.end())
    {
        values_.emplace(key, sp_value);
    }
    else
    {
        LOG(ERROR) << "key is exists. " << key.ShortDebugString() << " type: " << pb::ValueType_Name(VALUE_TYPE) << " value: " << sp_value->get_value() << " exist_value: " << it->second->get_value();
    }
}

template<pb::ValueType VALUE_TYPE>
size_t ValueKeyHash<VALUE_TYPE>::operator() (const pb::ValueKey& key) const
{
    size_t seed = 0;
    hash_combine(seed, key.uid());
    hash_combine(seed, key.clubid());
    return seed;
}

template<>
size_t ValueKeyHash<pb::USER_MONEY>::operator() (const pb::ValueKey& key) const
{
    return std::hash<size_t>{}(static_cast<size_t>(key.uid()));
}

template<pb::ValueType VALUE_TYPE>
bool ValueKeyCmp<VALUE_TYPE>::operator() (const pb::ValueKey& lhs, const pb::ValueKey& rhs) const
{
    return lhs.uid() == rhs.uid() && lhs.clubid() == rhs.clubid();
}

template<>
bool ValueKeyCmp<pb::USER_MONEY>::operator() (const pb::ValueKey& lhs, const pb::ValueKey& rhs) const
{
    return lhs.uid() == rhs.uid();
}

// Explicit instantiations
template class ValueMgr<pb::USER_MONEY>;
template class ValueMgr<pb::CLUB_USER_CHIPS>;

template class ValueKeyHash<pb::CLUB_USER_CHIPS>;
template class ValueKeyCmp<pb::CLUB_USER_CHIPS>;
