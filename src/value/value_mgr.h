#ifndef __VALUE_VALUE_MGR_H__
#define __VALUE_VALUE_MGR_H__

#include "../common/mysql_client.h"
#include "../pb/inner.pb.h"
#include <tuple>

/*
 * 添加一个新的pb::ValueType必须要做的工作
 * 1. 在value_mgr.cpp文件末尾显示实例化此pb::ValueType类型的ValueMgr
 * 2. 在ValueService::InitValueMgr函数中实例化此ValueMgr
 * 3. 在value_service.cpp文件中特化此类型的ValueService::LoadValueFromDB成员函数
 * 4. 在value_service.cpp文件中特化此类型的ValueService::StoreToDB成员函数
 * 5. 在value_mgr.cpp文件中特化此类型的ValueKeyHash和ValueKeyCmp, 或者显示实例化该类型, 看具体需要哪些key决定用哪种方式
 */

enum class OperateValueRetCode : int16_t
{
	OPERATE_FAILED = -1,  // 操作失败
	OPERATE_OK = 0,       // 操作成功
	NO_VALUE = 1,         // 没有数据
};

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template<pb::ValueType VALUE_TYPE>
class ValueKeyHash final
{
public:
    size_t operator() (const pb::ValueKey& key) const;
};

template<pb::ValueType VALUE_TYPE>
class ValueKeyCmp final
{
public:
    bool operator() (const pb::ValueKey& lhs, const pb::ValueKey& rhs) const;
};


//interface class
class BaseValueMgr
{
public:
    using ChangeValueMsgPtr = std::shared_ptr<pb::iChangeValueREQ>;
    BaseValueMgr() = default;
	virtual ~BaseValueMgr() = default;
    BaseValueMgr(const BaseValueMgr&) = delete;
    BaseValueMgr& operator= (const BaseValueMgr&) = delete;

public:
    virtual std::tuple<OperateValueRetCode, int64_t> OperateValue(const ChangeValueMsgPtr msg) = 0;
    virtual std::tuple<OperateValueRetCode, int64_t> TestOperateValue(const ChangeValueMsgPtr msg) = 0;
    virtual std::tuple<OperateValueRetCode, int64_t> GetValue(const pb::ValueKey& key) = 0;
    virtual std::tuple<OperateValueRetCode, int64_t> DeleteValue(const pb::ValueKey& key, const std::string& type, const std::string& attach) = 0;
    virtual std::tuple<OperateValueRetCode, int64_t> ChangeValue(const pb::ValueKey& key, int64_t change, const std::string& type, const std::string& attach) = 0;
};

class ValueService;

class Value final
{
    public:
        using ValuePtr = std::shared_ptr<Value>;

    public:
        Value(int64_t v): value_(v) {};
        ~Value() = default;
        Value(const Value&) = delete;
        Value& operator= (const Value&) = delete;

    public:
        int64_t    get_value() const { return value_; }
        void        set_value(int64_t v) { value_ = v; }
    private:
        int64_t    value_ = 0;
};

template<pb::ValueType VALUE_TYPE>
class ValueMgr final : public BaseValueMgr
{
public:
	ValueMgr(ValueService*);
    virtual ~ValueMgr();
    ValueMgr(const ValueMgr<VALUE_TYPE>&) = delete;
    ValueMgr<VALUE_TYPE>& operator= (const ValueMgr<VALUE_TYPE>&) = delete;
    
public:
    virtual std::tuple<OperateValueRetCode, int64_t> OperateValue(const ChangeValueMsgPtr msg) override;
    virtual std::tuple<OperateValueRetCode, int64_t> TestOperateValue(const ChangeValueMsgPtr msg) override;
    virtual std::tuple<OperateValueRetCode, int64_t> GetValue(const pb::ValueKey& key) override;
    virtual std::tuple<OperateValueRetCode, int64_t> DeleteValue(const pb::ValueKey& key, const std::string& type, const std::string& attach) override;
    virtual std::tuple<OperateValueRetCode, int64_t> ChangeValue(const pb::ValueKey& key, int64_t change, const std::string& type, const std::string& attach) override;

private:
    void               AddNewValue(const pb::ValueKey& key, const Value::ValuePtr value);
    Value::ValuePtr    LoadValueFromDB(const pb::ValueKey& key);

private:
    Value::ValuePtr  _get_value(const pb::ValueKey& k);
    std::tuple<OperateValueRetCode, int64_t> _delete_value(const pb::ValueKey& k, const std::string& type, const std::string& attach);
    std::tuple<OperateValueRetCode, int64_t> _change_value(const pb::ValueKey& k, int64_t change, const std::string& type, const std::string& attach);

public:
	ValueService*                           service_;
    std::unordered_map<pb::ValueKey, Value::ValuePtr, ValueKeyHash<VALUE_TYPE>, ValueKeyCmp<VALUE_TYPE>> values_;
};

#endif
