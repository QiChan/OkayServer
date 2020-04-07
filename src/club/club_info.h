#ifndef __CLUB_CLUB_INFO_H__
#define __CLUB_CLUB_INFO_H__

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <list>
#include "../pb/club.pb.h"

class ClubPushMsg final
{
public:
    ClubPushMsg (uint64_t push_id, uint64_t push_time, const std::string& title, const std::string& content);
    ClubPushMsg (uint64_t push_id, uint64_t push_time, const std::string& title, const std::string& content, const std::unordered_set<uint64_t>& uids);
    virtual ~ClubPushMsg () = default;
    ClubPushMsg (ClubPushMsg const&) = delete;
    ClubPushMsg& operator= (ClubPushMsg const&) = delete; 

    uint64_t    push_id() const { return push_id_; }
    uint64_t    push_time() const { return push_time_; }
    bool        CheckRead(uint64_t uid) const;
    void        ReadPushMsg(uint64_t uid);

    pb::ClubPushMsgInfo GetPushMsgInfo() const { return info_; }

private:
    uint64_t                            push_id_;
    uint64_t                            push_time_;
    std::unordered_set<uint64_t>        uid_records_;
    pb::ClubPushMsgInfo                 info_;
};

class ClubJoinMsg final
{
public:
    ClubJoinMsg(uint64_t msgid, uint32_t clubid, uint64_t apply_uid, const std::string& apply_name, const std::string& apply_icon, uint64_t apply_time, const std::string& content);
    ~ClubJoinMsg();
    ClubJoinMsg(ClubJoinMsg const&) = delete;
    ClubJoinMsg& operator= (ClubJoinMsg const&) = delete;

public:
    pb::ClubJoinMsgInfo GetClubJoinMsg() const;
    uint64_t    msgid() const { return msgid_; }
    uint64_t    apply_uid() const { return apply_uid_; }
    const std::string&  name() const { return apply_name_; }
    const std::string&  icon() const { return apply_icon_; }

private:
    uint64_t    msgid_;
    uint32_t    clubid_;
    uint64_t    apply_uid_;         // 申请人uid
    std::string apply_name_;        // 申请人名称
    std::string apply_icon_;        // 申请人头像
    uint64_t    apply_time_;        // 申请时间
    std::string content_;           // 申请附加消息
};

class ClubMemberInfo final
{
public:
    ClubMemberInfo (uint64_t memberid, uint32_t clubid, uint64_t uid, pb::ClubRole role, uint64_t jointime, const std::string& name, const std::string& icon, uint64_t last_login_time);
    ~ClubMemberInfo ();
    ClubMemberInfo (ClubMemberInfo const&) = delete;
    ClubMemberInfo& operator= (ClubMemberInfo const&) = delete;

public:
    uint32_t    uid() const { return uid_; }
    uint32_t    clubid() const { return clubid_; }
    uint64_t    memberid() const { return memberid_; }
    void        set_role(pb::ClubRole role) { role_ = role; }
    pb::ClubRole    role() const { return role_; }

    pb::ClubMemberInfo  GetClubMemberInfo() const;

public:
    void        UpdateUserInfo(const std::string& name, const std::string& icon, uint64_t login_time);
    uint64_t    GetMemberID() const { return memberid_; }

private:
    uint64_t        memberid_;      // 数据库里面的主键
    uint32_t        clubid_;
    uint64_t        uid_;
    pb::ClubRole    role_;
    uint64_t        jointime_;
    std::string     name_;              // 昵称
    std::string     icon_;              // 头像
    uint64_t        last_login_time_;   // 上次登录时间
};

class ClubInfo final
{
    using ClubMemberPtr = std::shared_ptr<ClubMemberInfo>;
    using ClubJoinMsgPtr = std::shared_ptr<ClubJoinMsg>;
    using ClubPushMsgPtr = std::shared_ptr<ClubPushMsg>;

public:
    ClubInfo (uint32_t clubid, const std::string& club_name, const std::string& club_icon, uint64_t ownerid, const std::string& blackboard, uint64_t create_time, const std::string& config_name);
    ~ClubInfo ();
    ClubInfo (ClubInfo const&) = delete;
    ClubInfo& operator= (ClubInfo const&) = delete;

public:
    void    AddMember(ClubMemberPtr member);
    void    DelMember(ClubMemberPtr member);
    void    AddJoinMsg(ClubJoinMsgPtr msg);
    void    EraseJoinMsg(uint64_t uid);
    void    AddPushMsg(ClubPushMsgPtr msg);
    ClubMemberPtr   GetMember(uint64_t uid);
    ClubJoinMsgPtr  GetJoinMsg(uint64_t uid);

    const std::unordered_map<uint64_t, ClubMemberPtr>& GetAllMember() const { return members_; }
    
public:
    uint32_t    clubid() const { return clubid_; }
    bool        IsManager(ClubMemberPtr member) const;
    bool        IsManager(uint64_t uid);
    bool        IsOwner(uint64_t uid) { return ownerid_ == uid; }
    uint32_t    GetMemberNum() const { return members_.size(); }
    uint32_t    GetManagerNum() const { return managers_.size(); }

    pb::ClubInfo    GetClubInfo(uint64_t uid);
    const std::string& club_name() const { return club_name_; }
    const std::string& config_name() const { return config_name_; }
    
    const std::unordered_set<ClubMemberPtr>& GetClubManagers() const { return managers_; }
    const std::unordered_map<uint64_t, ClubJoinMsgPtr>& GetClubJoinMsgList() const { return join_msgs_; }
    std::vector<ClubPushMsgPtr>     ReadClubPushMsgList(uint64_t uid);

public:
    uint32_t    clubid_;
    std::string club_name_;
    std::string club_icon_;
    uint64_t    ownerid_;
    std::string blackboard_;
    uint64_t    create_time_;
    std::string config_name_;

    std::unordered_set<ClubMemberPtr>               managers_;  // 管理员列表，包含创始人
    std::unordered_map<uint64_t, ClubMemberPtr>     members_;   // uid => ClubMemberPtr
    std::unordered_map<uint64_t, ClubJoinMsgPtr>    join_msgs_; // apply_uid => ClubJoinMsgPtr

    std::list<ClubPushMsgPtr>                       push_msgs_;      // 俱乐部推送消息
};

#endif 
