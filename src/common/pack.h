/*
 * Pack说明：
 * 服务之间通信都通过serialize_imsg接口序列化成内部包(包括通过agent转发给客户端的包)
 * 直接发送给客户端的包通过serialize_msg接口序列化网络包
 */
#ifndef __COMMON_PACK_H__
#define __COMMON_PACK_H__

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <cstdint>
#include <tools/time_util.h>

extern "C"
{
#include <skynet.h>
}

using namespace google::protobuf;

using MessagePtr = std::shared_ptr<Message>;

#define PTYPE_AGENT 15                  // 发给客户端的消息
#define PTYPE_SERVICE 16                // 服务内部消息
#define PTYPE_RPC_CLIENT 17             
#define PTYPE_RPC_SERVER 18
#define PTYPE_SYSTEM_SERVICE   19       // 自定义系统消息

#define PTYPE_MASK 0xFFFF

#define PACK_HEAD 4
#define MAX_PACK_SIZE 0x1000000
#define TYPE_HEAD 2
#define MAX_TYPE_SIZE 50


#pragma pack(push)
#pragma pack(1)
// 消息的上下文, 单字节对齐
struct PackContext
{
    // default copy-constructor and copy assignment operator is ok.
    PackContext()
        : version_(0), 
          receive_time_(0),
          receive_agent_tick_(0),
          process_uid_(0)
    {
    }

    void ResetContext(uint64_t uid, uint16_t harborid, uint64_t now_tick_us)
    {
        uint64_t now = TimeUtil::now_us();
        process_uid_ = uid;

        version_ = static_cast<uint64_t>(harborid & 0xffff) << 48;   // 高16位harborid
        version_ |= static_cast<uint64_t>((now / 1000000) & 0xffffffffffff); // 中间48位时间戳(秒)

        receive_time_ = now;
        receive_agent_tick_ = now_tick_us;
    }

    uint16_t harborid() const
    {
        return (version_ >> 48) & 0xffff;
    }

    void reset()
    {
        version_ = 0;
        receive_time_ = 0;
        receive_agent_tick_ = 0;
        process_uid_ = 0;
    }

    void ParseFromStream(const void* data)
    {
        if (data == nullptr)
            return;
        char* cur = const_cast<char*>(static_cast<const char*>(data));
        version_ = *reinterpret_cast<decltype(version_)*>(cur);
        cur += sizeof(version_);
        receive_time_ = *reinterpret_cast<decltype(receive_time_)*>(cur);
        cur += sizeof(receive_time_);
        receive_agent_tick_ = *reinterpret_cast<decltype(receive_agent_tick_)*>(cur);
        cur += sizeof(receive_agent_tick_);
        process_uid_ = *reinterpret_cast<decltype(process_uid_)*>(cur);
    }

    void SerializeToStream(char* data, size_t len) const
    {
        if (len < sizeof(PackContext))
        {
            return;
        }
        size_t idx = 0;
        memcpy(data + idx, &version_, sizeof(version_));
        idx += sizeof(version_);
        memcpy(data + idx, &receive_time_, sizeof(receive_time_));
        idx += sizeof(receive_time_);
        memcpy(data + idx, &receive_agent_tick_, sizeof(receive_agent_tick_));
        idx += sizeof(receive_agent_tick_);
        memcpy(data + idx, &process_uid_, sizeof(process_uid_));
    }

    uint64_t    version_;              // harborid(16) + 时间戳(48), 时间戳的溢出可不处理
    uint64_t    receive_time_;              // 接收到消息的时间戳(us)，第一次生成这个packcontext的时间,用来做一些包超时丢弃的判断
    uint64_t    receive_agent_tick_;        // 存放agent接收到消息的时钟tick, 用来算处理客户端消息的总负载(us)
    uint64_t    process_uid_;               // uid或者roomid
};

#pragma pack(pop)

inline bool operator== (const PackContext& left, const PackContext& right)
{
    return left.version_ == right.version_ && 
        left.receive_time_ == right.receive_time_ &&
        left.receive_agent_tick_ == right.receive_agent_tick_ &&
        left.process_uid_ == right.process_uid_;
}

inline std::ostream& operator<< (std::ostream& os, const PackContext& context)
{
    os << "process_uid: " << context.process_uid_ << " version: " << context.version_ << " timestamp: " << context.receive_time_ << " tick: " << context.receive_agent_tick_;
    return os;
}

//char* to message
class InPack final
{
public:
	InPack();
    virtual ~InPack() = default;
    InPack(const InPack&) = delete;
    InPack& operator= (const InPack&) = delete;

public:
    //反序列化网络包
	bool ParseFromNetData(const PackContext& context, const char* const data, uint32_t size = 0);
	//得到pb包
    MessagePtr CreateMessage() const;

    //反序列化带PackContext的流
	bool ParseFromInnerData(const char* const cdata, uint32_t size = 0);
    void set_pack_context(const PackContext& context) { pack_context_ = context; }
    const PackContext& pack_context() const { return pack_context_; }


public:
    uint32_t    data_len() const { return data_len_; }
    std::string type_name() const { return type_name_; }

protected:
	//解出m_type_name
	bool DecodeStream(const char* data, uint32_t size);

protected:
	const char*     data_;
	uint32_t        data_len_;
	const char*     pb_data_;
	uint32_t        pb_data_len_;
	std::string     type_name_;
    PackContext     pack_context_;
};

//message to char*
class OutPack final
{
public:
	OutPack() = default;
    ~OutPack() = default;
    OutPack(const OutPack&) = delete;
    OutPack& operator= (const OutPack&) = delete;
    OutPack(const Message& msg);

public:
	bool reset(const Message& msg);
	void SerializeToNetData(char* &result, uint32_t& size);
	void SerializeToInnerData(char* &result, uint32_t& size, const PackContext& context);

    std::string type_name() const { return type_name_; }

private:
	std::string pb_data_;
	std::string type_name_;
};


void SerializeToNetData(const Message& msg, char* &result, uint32_t &size);
void SerializeToInnerData(const Message& msg, char* &result, uint32_t& size, const PackContext& context);    

// 内部包生成网络包
void InnerDataToNetData(const char* in_data, uint32_t in_size, char* &o_data, uint32_t &o_size);
// 网络包生成内部包
void NetDataToInnerData(const char* in_data, uint32_t in_size, const PackContext& context, char* &o_data, uint32_t &o_size);

#endif
