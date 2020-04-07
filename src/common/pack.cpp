#include <arpa/inet.h>
#include <cstdlib>
#include <string>
#include <glog/logging.h>
#include "pack.h"

extern "C"
{
#include "skynet.h"
}

InPack::InPack()
    : data_(nullptr),
      data_len_(0),
      pb_data_(nullptr),
      pb_data_len_(0)
{
}

bool InPack::ParseFromNetData(const PackContext& context, const char* const data, uint32_t size)
{
	data_ = data;
	if (size == 0)
	{
		data_len_ = strlen(data);
		size = data_len_;
	}
	else
    {
		data_len_ = size;
    }

    pack_context_ = context;

	return DecodeStream(data, size);
}

//得到pb包
MessagePtr InPack::CreateMessage() const
{
	Message* message = nullptr;
	const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(type_name_);
	if (descriptor)
	{
		const Message* prototype = MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype)
		{
			message = prototype->New();
			if (!message->ParseFromArray(pb_data_, pb_data_len_))
			{
				delete message;
				message = nullptr;
                LOG(ERROR) << "pb parse error type: " << type_name_.c_str();
			}
		}
	}

	return MessagePtr(message);
}

bool InPack::DecodeStream(const char* data, uint32_t size)
{
	uint16_t type_name_len = *(uint16_t*)data;
	type_name_len = ntohs(type_name_len);
	if (type_name_len > size - TYPE_HEAD || type_name_len > MAX_TYPE_SIZE)
	{
        LOG(ERROR) << "pack type error len: " << type_name_len << " data_size: " << size - TYPE_HEAD;
		return false;
	}

	type_name_.assign(data + TYPE_HEAD, data + TYPE_HEAD + type_name_len);

	pb_data_ = data + TYPE_HEAD + type_name_len;
	pb_data_len_ = size - TYPE_HEAD - type_name_len;
	return true;
}

bool InPack::ParseFromInnerData(const char* const cdata, uint32_t size)
{
	const char* data = cdata;
	data_ = data;
	if (size == 0)
	{
		data_len_ = strlen(data);
		size = data_len_;
	}
	else
    {
		data_len_ = size;
    }

	//解出PackContext
    pack_context_.ParseFromStream(cdata);
	data += sizeof(pack_context_);
	size -= sizeof(pack_context_);

	return DecodeStream(data, size);
}

OutPack::OutPack(const Message& msg)
{
    reset(msg);
}

bool OutPack::reset(const Message& msg)
{
	if (!msg.SerializeToString(&pb_data_))
	{
		LOG(ERROR) << "serialize error. type: " << msg.GetTypeName();
		return false;
	}
	type_name_ = msg.GetTypeName();
	return true;
}

void OutPack::SerializeToNetData(char* &result, uint32_t& size) //【数据长度】【类型长度】【类型名】【数据】
{
	uint32_t data_len = pb_data_.size() + TYPE_HEAD + type_name_.size();

	size = data_len + PACK_HEAD;
	char* data = (char*)skynet_malloc(size);
	result = data;

	uint32_t pack_head = htonl(data_len);
	memcpy(data, &pack_head, PACK_HEAD);

	uint16_t type_head = htons(type_name_.size());
	memcpy(data + PACK_HEAD, &type_head, TYPE_HEAD);

	memcpy(data + PACK_HEAD + TYPE_HEAD, type_name_.c_str(), type_name_.size());

	memcpy(data + PACK_HEAD + TYPE_HEAD + type_name_.size(), pb_data_.c_str(), pb_data_.size());
}

void OutPack::SerializeToInnerData(char* &result, uint32_t& size, const PackContext& context)//【PackContext】【类型长度】【类型名】【数据】
{
	int data_len = sizeof(context) + TYPE_HEAD + type_name_.size() + pb_data_.size();

	size = data_len;
	char* data = (char*)skynet_malloc(size);
	result = data;

	size_t idx = 0;

    context.SerializeToStream(data + idx, size);
	idx += sizeof(context);

	uint16_t type_head = htons(type_name_.size());
	memcpy(data + idx, &type_head, TYPE_HEAD);
	idx += TYPE_HEAD;

	memcpy(data + idx, type_name_.c_str(), type_name_.size());
	idx += type_name_.size();

	memcpy(data + idx, pb_data_.c_str(), pb_data_.size());
}

void SerializeToNetData(const Message& msg, char* &result, uint32_t& size)
{
	OutPack opack;
	opack.reset(msg);
	opack.SerializeToNetData(result, size);
}

void SerializeToInnerData(const Message& msg, char* &result, uint32_t& size, const PackContext& context)
{
	OutPack opack;
	opack.reset(msg);
	opack.SerializeToInnerData(result, size, context);
}

void InnerDataToNetData(const char* in_data, uint32_t in_size, char* &o_data, uint32_t &o_size)
{
    uint32_t data_len = in_size - sizeof(PackContext);
    o_size = data_len + PACK_HEAD;
    o_data = (char*)skynet_malloc(o_size);

    uint32_t pack_head = htonl(data_len);
    memcpy(o_data, &pack_head, PACK_HEAD);

    memcpy(o_data + PACK_HEAD, in_data + sizeof(PackContext), data_len);
}

void NetDataToInnerData(const char* in_data, uint32_t in_size, const PackContext& context, char* &o_data, uint32_t &o_size)
{
    o_size = in_size + sizeof(context);
    o_data = (char*)skynet_malloc(o_size);
    context.SerializeToStream(o_data, o_size);
    memcpy(o_data + sizeof(context), in_data, in_size);
}
