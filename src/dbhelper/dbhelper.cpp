#include "dbhelper.h"
#include "../pb/club.pb.h"

extern "C"
{
#include "skynet_server.h"
}

DBHelper::DBHelper()
    : MsgSlaveService(MsgServiceType::SERVICE_DBHELPER)
{
}

DBHelper::~DBHelper()
{
}

bool DBHelper::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!MsgSlaveService::VInitService(ctx, parm, len))
    {
        return false;
    }

    if (!ConnectMysql())
    {
        return false;
    }

    RegServiceName("dbhelper", true);
    RegisterCallBack();

    return true;
}

void DBHelper::VReloadUserInfo(UserPtr user)
{
    LOG(INFO) << "[" << service_name() << "] uid: " << user->uid() << " handle: " << std::hex << skynet_context_handle(ctx());
}

void DBHelper::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void DBHelper::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

void DBHelper::RegisterCallBack()
{
    RegisterRPC(pb::iLoadRoomListUserREQ::descriptor()->full_name(), std::bind(&DBHelper::HandleRPCLoadRoomListUserREQ, this, std::placeholders::_1));
    RegisterRPC(pb::iLoadClubUserREQ::descriptor()->full_name(), std::bind(&DBHelper::HandleRPCLoadClubUserREQ, this, std::placeholders::_1));
    
    MsgCallBack(pb::ClubMemberListREQ::descriptor()->full_name(), std::bind(&DBHelper::HandleClientMsgClubMemberListREQ, this, std::placeholders::_1, std::placeholders::_2));
}

bool DBHelper::ConnectMysql()
{
    MysqlConfigMgr  config_mgr;
    if (!config_mgr.VLoadConfig())
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_S1_OKEYPOKER_USER, &mysql_slave_user_))
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_S1_OKEYPOKER_CLUB, &mysql_slave_club_))
    {
        return false;
    }
    return true;
}

MessagePtr DBHelper::HandleRPCLoadRoomListUserREQ(MessagePtr data)
{
    auto msg = std::dynamic_pointer_cast<pb::iLoadRoomListUserREQ>(data);
    uint64_t uid = msg->uid();
    auto info = GetBaseUserInfo(uid);
    auto rsp = std::make_shared<pb::iLoadRoomListUserRSP>();
    rsp->mutable_info()->CopyFrom(info);

    return rsp;
}

void DBHelper::HandleClientMsgClubMemberListREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::ClubMemberListREQ>(data);
    uint32_t clubid = msg->clubid();

    std::stringstream sql;
    sql << "select `club_member`.`uid`, `club_member`.`role`, `club_member`.`jointime`, `user`.`nickname`, `user`.`icon`, `user`.`last_login_time`, `club_user_chips`.`chips`, `club_user_record`.`hands_num`, `club_user_record`.`rake`, `club_user_record`.`last_update_time` "
    << " from `okey_club`.`club_member` left join `okeygo`.`club_user_chips` on `club_member`.`clubid` = `club_user_chips`.`clubid` and `club_member`.`uid` = `club_user_chips`.`uid` "
    << " left join `okey_game_record`.`club_user_record` on `club_member`.`clubid` = `club_user_record`.`clubid` and `club_member`.`uid` = `club_user_record`.`uid` "
    << " left join `okey_user`.`user` on `club_member`.`uid` = `user`.`uid` "
    << " where `club_member`.`clubid` = "
    << " " << clubid << " and `leavetime` = 0";
    if (!mysql_slave_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "get club_member info error. sql: " << sql.str();
        return;
    }
    
    pb::ClubMemberListRSP   rsp;
    rsp.set_clubid(clubid);
    MysqlResult result(mysql_slave_club_.handle());
    MysqlRow    row;
    while (result.fetch_row(row))
    {
        auto member = rsp.add_members();
        member->set_uid(row.get_uint64("uid"));
        member->set_clubid(clubid);
        member->set_role(static_cast<pb::ClubRole>(row.get_uint32("role")));
        member->set_jointime(row.get_uint64("jointime"));
        member->set_name(row.get_string("nickname"));
        member->set_icon(row.get_string("icon"));
        member->set_last_login_time(row.get_uint64("last_login_time"));
        member->set_hands_num(row.get_uint32("hands_num"));
        member->set_rake(row.get_uint64("rake"));
        member->set_chips(row.get_int64("chips"));
    }

    user->SendToUser(rsp);
    CacheRSP(rsp, 0, 10);
}

MessagePtr DBHelper::HandleRPCLoadClubUserREQ(MessagePtr data)
{
    auto msg = std::dynamic_pointer_cast<pb::iLoadClubUserREQ>(data);
    uint64_t uid = msg->uid();
    auto info = GetBaseUserInfo(uid);

    auto rsp = std::make_shared<pb::iLoadClubUserRSP>();
    rsp->mutable_info()->CopyFrom(info);

    {
        std::stringstream sql;
        sql << "select `club_member`.`clubid` "
            << " from `club_member` STRAIGHT_JOIN `club` on `club_member`.`clubid` = `club`.`clubid` "
            << " where `club_member`.`uid` = " << uid
            << " and `club_member`.`leavetime` = 0 "
            << " and `club`.`disband_time` = 0";
        if (!mysql_slave_club_.mysql_exec(sql.str()))
        {
            LOG(ERROR) << "load club_user error. sql: " << sql.str();
            return rsp;
        }

        MysqlResult result(mysql_slave_club_.handle());
        MysqlRow    row;
        while (result.fetch_row(row))
        {
            rsp->add_clubids(row.get_uint32("clubid"));
        }
    }

    return rsp;
}

pb::iBaseUserInfo DBHelper::GetBaseUserInfo(uint64_t uid)
{
    pb::iBaseUserInfo   info;
    info.set_uid(uid);

    std::stringstream sql;
    sql << "select nickname, icon, last_login_time from user where uid = " << uid;
    mysql_slave_user_.mysql_exec(sql.str());
    MysqlResult result(mysql_slave_user_.handle());
    MysqlRow    row;
    if (result.fetch_row(row))
    {
        info.set_icon(row.get_string("icon"));
        info.set_name(row.get_string("nickname"));
        info.set_last_login_time(row.get_uint64("last_login_time"));
    }

    return info;
}
