#ifndef __CLUB_CLUB_H__
#define __CLUB_CLUB_H__

#include "../common/msg_service.h"
#include "../common/mysql_client.h"
#include "club_user.h"
#include "club_info.h"
#include "club_config_mgr.h"

class Club final : public MsgService<ClubUser>
{
    using ClubPtr = std::shared_ptr<ClubInfo>;
public:
    Club ();
    virtual ~Club ();
    Club (Club const&) = delete;
    Club& operator= (Club const&) = delete;

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;
    virtual void VReloadUserInfo(UserPtr user) override;

protected:
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;

protected:
    virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;

public:
    void    LoadUserInfo(uint64_t uid);

private:
    void        RegisterCallBack();
    bool        ConnectMysql();
    ClubPtr     GetClub(uint32_t clubid);
    ClubPtr     LoadClub(uint32_t clubid);
    bool        NewClubMember(ClubPtr club, uint64_t uid, const std::string& name, const std::string& icon, uint64_t last_login_time, pb::ClubRole role, uint64_t jointime);
    bool        DelClubMember(ClubPtr club, uint64_t uid);
    ClubPtr     CreateClub(UserPtr user, const std::string& club_name, const std::string& blackboard);
    bool        DisbandClub(UserPtr user, uint32_t clubid);

    void        RecordClubAction(uint32_t clubid, uint64_t uid, const std::string& type, const std::string& attach);
    ClubPtr     LoadClubBaseInfoFromDB(uint32_t clubid);
    bool        LoadClubMemberInfoFromDB(ClubPtr club);
    bool        LoadClubJoinMsgFromDB(ClubPtr club);
    bool        LoadClubPushMsgFromDB(ClubPtr club);

    bool        CheckClubName(const std::string& name);
    void        BroadcastToClubManager(ClubPtr club, const Message& msg);

    std::shared_ptr<ClubConfig>     GetClubConfig(ClubPtr club);

private:
    void        HandleClientMsgCreateClubREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgClubInfoREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgClubListREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgDisbandClubREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgJoinClubREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgJoinClubMsgListREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgHandleJoinClubMsgREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgQuitClubREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgKickClubUserREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgSetClubRoleREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgSendClubMemberChipsREQ(MessagePtr data, UserPtr user);
    void        HandleClientMsgSendClubPushMsgREQ(MessagePtr data, UserPtr user);

private:
    MysqlClient     mysql_master_club_;
    MysqlClient     mysql_master_flow_;
    MysqlClient     mysql_master_;
    MysqlClient     mysql_master_game_record_; 

    ClubConfigMgr   club_config_mgr_;
    std::unordered_map<uint32_t, ClubPtr>   clubs_;     // clubid => ClubPtr

    // TODO: 配置
    static const uint32_t MAX_CLUB_NUM = 5;             // 每人最多创建的俱乐部数量
    static const uint32_t MAX_CLUB_MEMBER_NUM = 2000;   // 俱乐部最大人数
    static const uint32_t MAX_CLUB_MANAGER_NUM = 11;    // 俱乐部最大管理员数量, 包含创始人
};

#endif
