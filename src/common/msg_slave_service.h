/*
 * 继承MsgSlaveService必须要做的工作
 *
 * 1. 重写VInitService，而且在子类的VInitService必须首先调用MsgSlaveServer的VInitService
 * 2. 在子类的VInitService里必须调用RegServiceName函数注册当前服务
 * 3. 在子类的VInitService里调用SetMsgServiceType设置服务的类型
 * 4. 加载完用户的数据后必须要调用HandleUserInitOK
 * 5. 重写VServiceOverEvent
 * 6. 重写VServerStopEvent
 * 7. 重写VReloadUserInfo，重新从数据库加载用户数据
 */

#ifndef __COMMON_MSG_SLAVE_SERVICE_H__
#define __COMMON_MSG_SLAVE_SERVICE_H__

#include "msg_service.h"
#include <tools/string_util.h>

#include "../pb/system.pb.h"
#include "../pb/monitor.pb.h"


template <typename User, typename = typename std::enable_if<std::is_base_of<MsgUser, User>::value>::type>
class MsgSlaveService : public MsgService<User>
{
public:
	MsgSlaveService(MsgServiceType msg_service_type)
        : MsgService<User>(msg_service_type),
          is_leader_(false)
	{
	}

	virtual ~MsgSlaveService() = default;
    MsgSlaveService(const MsgSlaveService<User>&) = delete;
    MsgSlaveService<User>& operator= (const MsgSlaveService<User>&) = delete;

public:
	virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override
	{
        if (!MsgService<User>::VInitService(ctx, parm, len))
        {
            return false;
        }

        std::string parm_data(static_cast<const char*>(parm), len);

        std::string temp;
		std::tie(temp, std::ignore) = StringUtil::DivideString(parm_data, ' ');
		if (temp == "leader")
		{
			is_leader_ = true;
		}
		LOG(INFO) << "slave parm: " << parm;

        return true;
	}

    virtual void VServiceStartEvent() override
    {
        // 只有leader节点才把客户端协议注册到main
		if (is_leader_)
		{
            this->SendClientMsgCallBack(this->GetParentHandle());
		}

        LOG(INFO) << "[" << Service::service_name() << "] start. curr_handle: " << std::hex << skynet_context_handle(this->ctx()) << " parent_handle: " << this->GetParentHandle();
	}

    // 不expose自己的名字，消息全由master节点转发过来
	void RegServiceName(const std::string& name, bool monitor)
	{
        Service::RegServiceName(name, false, monitor);
	}

private:
	bool    is_leader_;
};

#endif

