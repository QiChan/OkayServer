#include "club_info.h"

ClubPushMsg::ClubPushMsg(uint64_t push_id, uint64_t push_time, const std::string& title, const std::string& content)
    : push_id_(push_id),
      push_time_(push_time)
{
    info_.set_title(title);
    info_.set_content(content);
}

ClubPushMsg::ClubPushMsg(uint64_t push_id, uint64_t push_time, const std::string& title, const std::string& content, const std::unordered_set<uint64_t>& uids)
    : ClubPushMsg(push_id, push_time, title, content)
{
    uid_records_ = uids;
}

bool ClubPushMsg::CheckRead(uint64_t uid) const
{
    return uid_records_.find(uid) == uid_records_.end();
}

void ClubPushMsg::ReadPushMsg(uint64_t uid)
{
    uid_records_.insert(uid);
}

ClubJoinMsg::ClubJoinMsg(uint64_t msgid, uint32_t clubid, uint64_t apply_uid, const std::string& apply_name, const std::string& apply_icon, uint64_t apply_time, const std::string& content)
    : msgid_(msgid),
      clubid_(clubid),
      apply_uid_(apply_uid),
      apply_name_(apply_name),
      apply_icon_(apply_icon),
      apply_time_(apply_time),
      content_(content)
{
}

ClubJoinMsg::~ClubJoinMsg()
{
}

pb::ClubJoinMsgInfo ClubJoinMsg::GetClubJoinMsg() const
{
    pb::ClubJoinMsgInfo msg;
    msg.set_clubid(clubid_);
    msg.set_apply_uid(apply_uid_);
    msg.set_apply_name(apply_name_);
    msg.set_apply_icon(apply_icon_);
    msg.set_apply_time(apply_time_);
    msg.set_content(content_);

    return msg;
}

ClubMemberInfo::ClubMemberInfo(uint64_t memberid, uint32_t clubid, uint64_t uid, pb::ClubRole role, uint64_t jointime, const std::string& name, const std::string& icon, uint64_t last_login_time)
    : memberid_(memberid),
      clubid_(clubid),
      uid_(uid),
      role_(role),
      jointime_(jointime),
      name_(name),
      icon_(icon),
      last_login_time_(last_login_time)
{
}

pb::ClubMemberInfo  ClubMemberInfo::GetClubMemberInfo() const
{
    pb::ClubMemberInfo  info;
    info.set_uid(uid_);
    info.set_clubid(clubid_);
    info.set_role(role_);
    info.set_jointime(jointime_);
    info.set_name(name_);
    info.set_icon(icon_);
    info.set_last_login_time(last_login_time_);

    return info;
}

ClubMemberInfo::~ClubMemberInfo()
{
}

void ClubMemberInfo::UpdateUserInfo(const std::string& name, const std::string& icon, uint64_t login_time)
{
    name_ = name;
    icon_ = icon;
    last_login_time_ = login_time;
}


ClubInfo::ClubInfo(uint32_t clubid, const std::string& club_name, const std::string& club_icon, uint64_t ownerid, const std::string& blackboard, uint64_t create_time, const std::string& config_name)
    : clubid_(clubid),
      club_name_(club_name),
      club_icon_(club_icon),
      ownerid_(ownerid),
      blackboard_(blackboard),
      create_time_(create_time),
      config_name_(config_name)
{
}

ClubInfo::~ClubInfo()
{
}

void ClubInfo::AddMember(ClubMemberPtr member)
{
    if (member == nullptr)
    {
        return;
    }
    members_[member->uid()] = member;

    if (IsManager(member))
    {
        managers_.insert(member);
    }
}

void ClubInfo::DelMember(ClubMemberPtr member)
{
    if (member == nullptr)
    {
        return;
    }

    if (IsManager(member))
    {
        managers_.erase(member);
    }

    members_.erase(member->uid());
}

bool ClubInfo::IsManager(uint64_t uid)
{
    auto member = GetMember(uid);
    return IsManager(member);
}


bool ClubInfo::IsManager(ClubMemberPtr member) const
{
    if (member == nullptr)
    {
        return false;
    }

    if (member->role() == pb::CLUB_MANAGER || member->role() == pb::CLUB_OWNER)
    {
        return true;
    }
    return false;
}

void ClubInfo::AddJoinMsg(ClubJoinMsgPtr msg)
{
    if (msg == nullptr)
    {
        return;
    }
    join_msgs_[msg->apply_uid()] = msg;
}

void ClubInfo::AddPushMsg(ClubPushMsgPtr msg)
{
    if (msg == nullptr)
    {
        return;
    }
    push_msgs_.push_back(msg);
}

ClubInfo::ClubMemberPtr ClubInfo::GetMember(uint64_t uid)
{
    auto it = members_.find(uid);
    if (it == members_.end())
    {
        return nullptr;
    }
    return it->second;
}

pb::ClubInfo ClubInfo::GetClubInfo(uint64_t uid)
{
    pb::ClubInfo    info;
    info.set_clubid(clubid_);
    info.set_club_name(club_name_);
    info.set_club_icon(club_icon_);
    info.set_blackboard(blackboard_);
    info.set_member_num(GetMemberNum());

    auto sp_member = GetMember(uid);
    if (sp_member != nullptr)
    {
        info.set_role(sp_member->role());
    }

    return info;
}

ClubInfo::ClubJoinMsgPtr ClubInfo::GetJoinMsg(uint64_t uid)
{
    auto it = join_msgs_.find(uid);
    if (it == join_msgs_.end())
    {
        return nullptr;
    }
    return it->second;
}

void ClubInfo::EraseJoinMsg(uint64_t uid)
{
    join_msgs_.erase(uid);
}

std::vector<ClubInfo::ClubPushMsgPtr> ClubInfo::ReadClubPushMsgList(uint64_t uid)
{
    std::vector<ClubInfo::ClubPushMsgPtr>   result;
    result.reserve(push_msgs_.size());

    uint64_t expire_time = time(NULL) - 30 * 24 * 60 * 60;
    for (auto it = push_msgs_.begin(); it != push_msgs_.end();)
    {
        auto msg = *it;
        if (msg->push_time() <= expire_time)
        {
            // 删除过期的消息
            push_msgs_.erase(it++);
            continue;
        }

        // 记录该用户已读取push消息的状态
        if (msg->CheckRead(uid))
        {
            msg->ReadPushMsg(uid);
            result.push_back(msg);
        }

        ++it;
    }

    return result;
}
