#include "msg_user.h"

MsgUser::MsgUser()
    : service_(nullptr),
      uid_(0),
      handle_(0),
      is_init_ok_(false),
      loading_userinfo_interval_(0)
{
}

/*
 * User Init流程
 * MsgService收到iAgentInit消息后，调用NewUser创建新用户
 * 创建出来的新用户会调用此处的InitUser初始化用户的信息
 * InitUser会调用子类实现的VLoadUserInfo去加载各个服务自己的用户数据
 * 加载完成之后，调用MsgService的HandleUserInitOK接口完成初始化工作
 */
void MsgUser::InitUser(Service* service, uint64_t uid, uint32_t handle)
{
    service_ = service;
    uid_ = uid;
    handle_ = handle;

    // 已经初始化完成，不重新load数据
    if (IsInit())           
    {
        return;
    }

    // 正在loading，不能连续load
    if (!CheckLoadUserInfo())
    {
        return;
    }

    VLoadUserInfo();
}

bool MsgUser::CheckLoadUserInfo()
{
    if (loading_userinfo_interval_ > 0)         
    {
        --loading_userinfo_interval_;
        return false;
    }

    loading_userinfo_interval_ = kLoadingUserInfoInterval;
    return true;
}

void MsgUser::SendToUser(const Message& msg)
{
    if (service_ == nullptr || handle_ == 0)
    {
        return;
    }

    service_->SendToAgent(msg, handle_);
}
