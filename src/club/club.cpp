#include "club.h"
#include "../pb/club.pb.h"
#include "../value/value_client.h"

Club::Club()
    : MsgService(MsgServiceType::SERVICE_CLUB)
{
}

Club::~Club()
{
}

bool Club::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!MsgService::VInitService(ctx, parm, len))
    {
        return false;
    }

    if (!ConnectMysql())
    {
        LOG(ERROR) << "connect mysql failed.";
        return false;
    }

    if (!club_config_mgr_.LoadClubConfig())
    {
        LOG(ERROR) << "load club config failed.";
        return false;
    }

    DependOnService("roomlist");
    DependOnService("dbhelper");
    DependOnService("value");

    RegServiceName("club", true, true);

    RegisterCallBack();

    return true;
}

void Club::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void Club::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

bool Club::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    if (Service::VHandleTextMessage(from, data, source, session))
    {
        return true;
    }

    if (from == "php")
    {
        std::string response = "no cmd\n";
        TextParm parm(data.c_str());
        const char* cmd = parm.get("cmd");
        if (strcmp(cmd, "reload_club_config") == 0)
        {
            if (club_config_mgr_.LoadClubConfig())
            {
                response = "reload club config ok\n";
            }
            else
            {
                response = "reload club config failed\n";
            }
        }

        ResponseTextMessage(from, response);
    }
    else
    {
        LOG(ERROR) << "from: " << from << " unknow text command: " << data; 
    }
    return true;
}

void Club::VReloadUserInfo(UserPtr user)
{
    LoadUserInfo(user->uid());
}

void Club::LoadUserInfo(uint64_t uid)
{
    pb::iLoadClubUserREQ    req;
    req.set_uid(uid);
    CallRPC(ServiceHandle("dbhelper"), req, [this](MessagePtr data) {
        auto msg = std::dynamic_pointer_cast<pb::iLoadClubUserRSP>(data);
        uint64_t uid = msg->info().uid();
        auto name = msg->info().name();
        auto icon = msg->info().icon();
        auto last_login_time = msg->info().last_login_time();

        auto user = GetUser(uid);
        if (user == nullptr)
        {
            return;
        }

        std::unordered_set<uint32_t>    clubids;
        for (int i = 0; i < msg->clubids_size(); ++i)
        {
            auto clubid = msg->clubids(i);
            auto club = GetClub(clubid);
            if (club == nullptr)
            {
                continue;
            }
            auto member = club->GetMember(uid);
            if (member == nullptr)
            {
                continue;
            }
            member->UpdateUserInfo(name, icon, last_login_time);
            clubids.insert(clubid);
        }

        user->CompleteClubUser(name, icon, last_login_time, clubids);
    });
}

void Club::RegisterCallBack()
{
    MsgCallBack(pb::CreateClubREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgCreateClubREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::ClubInfoREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgClubInfoREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::ClubListREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgClubListREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::DisbandClubREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgDisbandClubREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::JoinClubREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgJoinClubREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::JoinClubMsgListREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgJoinClubMsgListREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::HandleJoinClubMsgREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgHandleJoinClubMsgREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::QuitClubREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgQuitClubREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::KickClubUserREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgKickClubUserREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::SetClubRoleREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgSetClubRoleREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::SendClubMemberChipsREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgSendClubMemberChipsREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::SendClubPushMsgREQ::descriptor()->full_name(), std::bind(&Club::HandleClientMsgSendClubPushMsgREQ, this, std::placeholders::_1, std::placeholders::_2));
}

bool Club::ConnectMysql()
{
    MysqlConfigMgr  config_mgr;
    if (!config_mgr.VLoadConfig())
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER_CLUB, &mysql_master_club_))
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER_FLOW, &mysql_master_flow_))
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER, &mysql_master_))
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER_GAME_RECORD, &mysql_master_game_record_))
    {
        return false;
    }

    return true;
}

Club::ClubPtr Club::GetClub(uint32_t clubid)
{
    auto it = clubs_.find(clubid);
    if (it == clubs_.end())
    {
        return LoadClub(clubid);
    }
    return it->second;
}

Club::ClubPtr Club::LoadClub(uint32_t clubid)
{
    ClubPtr club = LoadClubBaseInfoFromDB(clubid);

    if (club == nullptr)
    {
        LOG(INFO) << "club not exist. clubid: " << clubid;
        return nullptr;
    }

    // load club member info
    if (!LoadClubMemberInfoFromDB(club))
    {
        return nullptr;
    }

    // load club join msg
    if (!LoadClubJoinMsgFromDB(club))
    {
        return nullptr;
    }

    // load club push msg
    if (!LoadClubPushMsgFromDB(club))
    {
        return nullptr;
    }

    clubs_[clubid] = club;

    LOG(INFO) << "load club done. clubid: " << clubid;
    return club;
}

Club::ClubPtr Club::LoadClubBaseInfoFromDB(uint32_t clubid)
{
    std::stringstream   sql;
    sql << "select `club_name`, `config_name`, `club_icon`, `ownerid`, `blackboard`, `create_time` from `club` "
        << " where `clubid` = " << clubid
        << " and `disband_time` = 0";

    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "load club error. clubid: " << clubid << " sql: " << sql.str();
        return nullptr;
    }
    MysqlResult result(mysql_master_club_.handle());
    MysqlRow    row;
    if (!result.fetch_row(row))
    {
        return nullptr;
    }

    auto club = std::make_shared<ClubInfo>(
        clubid, 
        row.get_string("club"),
        row.get_string("club_icon"),
        row.get_uint64("ownerid"),
        row.get_string("blackboard"),
        row.get_uint64("create_time"),
        row.get_string("config_name"));

    return club;
}

bool Club::LoadClubMemberInfoFromDB(ClubPtr club)
{
    if (club == nullptr)
    {
        return false;
    }

    std::stringstream   sql;
    sql << "select `club_member`.`memberid`, `club_member`.`uid`, `club_member`.`role`, `club_member`.`jointime`, `user`.`nickname`, `user`.`icon`, `user`.`last_login_time` "
        << " from okey_club.club_member left join okey_user.user on club_member.uid = user.uid "
        << " where `club_member`.`clubid` = " << club->clubid()
        << " and `club_member`.`leavetime` = 0";
    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "load club member error. clubid: " << club->clubid() << " sql: " << sql.str();
        return false;
    }

    MysqlResult result(mysql_master_club_.handle());
    MysqlRow    row;
    while (result.fetch_row(row))
    {
        auto member = std::make_shared<ClubMemberInfo>(
            row.get_uint64("memberid"),
            club->clubid(),
            row.get_uint64("uid"),
            static_cast<pb::ClubRole>(row.get_uint32("role")),
            row.get_uint64("jointime"),
            row.get_string("nickname"),
            row.get_string("icon"),
            row.get_uint64("last_login_time"));
        club->AddMember(member);
    }

    LOG(INFO) << "load club_member done. clubid: " << club->clubid();
    return true;
}

bool Club::LoadClubJoinMsgFromDB(ClubPtr club)
{
    if (club == nullptr)
    {
        return false;
    }

    std::stringstream sql;
    // 一个月有效期
    uint64_t expire_time = time(NULL) - 30 * 24 * 60 * 60;

    sql << "select `club_join_msg`.`msgid`, `club_join_msg`.`apply_uid`, `club_join_msg`.`apply_time`, `club_join_msg`.`content`, `user`.`nickname`, `user`.`icon` "
        << " from okey_club.club_join_msg left join okey_user.user on club_join_msg.apply_uid = user.uid "
        << " where `club_join_msg`.`clubid` = " << club->clubid()
        << " and `club_join_msg`.`deal_time` = 0"
        << " and `club_join_msg`.`apply_time` >= " << expire_time;

    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "load club join msg error. clubid: " << club->clubid() << " sql: " << sql.str();
        return false;
    }

    MysqlResult result(mysql_master_club_.handle());
    MysqlRow    row;
    while (result.fetch_row(row))
    {
        auto join_msg = std::make_shared<ClubJoinMsg>(
            row.get_uint64("msgid"),
            club->clubid(),
            row.get_uint64("apply_uid"),
            row.get_string("nickname"),
            row.get_string("icon"),
            row.get_uint64("apply_time"),
            row.get_string("content"));
        club->AddJoinMsg(join_msg);
    }

    LOG(INFO) << "load club join msg done. clubid: " << club->clubid();
    return true;
}

bool Club::LoadClubPushMsgFromDB(ClubPtr club)
{
    if (club == nullptr)
    {
        return false;
    }

    std::stringstream push_sql;
    // 一个月有效期
    uint64_t expire_time = time(NULL) - 30 * 24 * 60 * 60;
    push_sql << "select `push_id`, `title`, `content`, `push_time` from `club_push_msg` "
        << " where `clubid` = " << club-> clubid()
        << " and `push_time` >= " << expire_time;
    if (!mysql_master_club_.mysql_exec(push_sql.str()))
    {
        LOG(ERROR) << "load club push msg error. clubid: " << club->clubid() << " sql: " << push_sql.str();
        return false;
    }

    MysqlResult result(mysql_master_club_.handle());
    MysqlRow    row;
    while (result.fetch_row(row))
    {
        uint64_t push_id = row.get_uint64("push_id");
        uint64_t push_time = row.get_uint64("push_time");
        std::string title = row.get_string("title");
        std::string content = row.get_string("content");
        std::unordered_set<uint64_t>    uids;

        std::stringstream record_sql;
        record_sql << "select `uid` from `club_push_msg_record` "
            << " where `push_id` = " << push_id
            << " and `record_time` = 0";
        if (!mysql_master_club_.mysql_exec(record_sql.str()))
        {
            LOG(ERROR) << "load club push msg record error. clubid: " << club->clubid() << " push_id: " << push_id << " sql: " << record_sql.str();
            return false;
        }

        MysqlResult record_result(mysql_master_club_.handle());
        MysqlRow    record_row;
        while (record_result.fetch_row(record_row))
        {
            uint64_t uid = row.get_uint64("uid");
            uids.insert(uid);
        }

        auto push_msg = std::make_shared<ClubPushMsg>(push_id, push_time, title, content, uids);
        club->AddPushMsg(push_msg);
    }

    LOG(INFO) << "load club push msg done. clubid: " << club->clubid();
    return true;
}

void Club::HandleClientMsgCreateClubREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::CreateClubREQ>(data);
    pb::CreateClubRSP   rsp;

    uint32_t club_num = 0;
    auto& clubids = user->GetUserClubIDs();
    for (auto clubid : clubids)
    {
        auto club = GetClub(clubid);
        if (club == nullptr)
        {
            LOG(ERROR) << "club is nullptr. uid : " << user->uid() << " clubid: " << clubid;
            continue;
        }
        if (club->IsOwner(user->uid()))
        {
            club_num++;
        }
    }

    if (club_num >= MAX_CLUB_NUM)
    {
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    auto club_name = msg->club_name();
    auto blackboard = msg->blackboard();
    if (!CheckClubName(club_name) || blackboard.size() >= 4096)
    {
        rsp.set_code(-2);
        user->SendToUser(rsp);
        return;
    }

    auto club = CreateClub(user, club_name, blackboard);
    if (club == nullptr)
    {
        rsp.set_code(-3);
        user->SendToUser(rsp);
        return;
    }

    rsp.set_code(0);
    rsp.mutable_club_info()->CopyFrom(club->GetClubInfo(user->uid()));
    user->SendToUser(rsp);
    LOG(INFO) << "create club success. clubid: " << club->clubid() << " uid: " << user->uid();
}

bool Club::CheckClubName(const std::string& name)
{
    if (name.empty() || name.size() >= 64)
    {
        return false;
    }

    bool all_space = true;      // 是否全部是空格
    for (auto c : name)
    {
        if (c != ' ')
        {
            all_space = false;
            break;
        }
    }

    if (all_space)
    {
        return false;
    }

    // 判断名字是否重复
    std::stringstream sql;
    sql << "select count(1) as num from club "
        << " where `club_name` = '" << mysql_string(name, mysql_master_club_) << "'";
    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "sql error. " << sql.str();
        return false;
    }

    MysqlResult result(mysql_master_club_.handle());
    MysqlRow    row;
    if (!result.fetch_row(row))
    {
        LOG(ERROR) << "sql error. " << sql.str();
        return false;
    }

    uint32_t num = row.get_uint32("num");
    if (num > 0)
    {
        return false;
    }

    return true;
}

Club::ClubPtr Club::CreateClub(UserPtr user, const std::string& club_name, const std::string& blackboard)
{
    if (user == nullptr)
    {
        return nullptr;
    }

    Club::ClubPtr   club = nullptr;

    uint64_t now = time(NULL);

    {
        std::string default_club_icon = "default";
        std::string default_config_name = "default";
        // 插入club表
        std::stringstream sql;
        sql << "insert into `club`(`club_name`, `config_name`, `club_icon`, `ownerid`, `blackboard`, `create_time`) values("
            << " '" << mysql_string(club_name, mysql_master_club_) << "', "
            << " '" << mysql_string(default_config_name, mysql_master_club_) << "', "
            << " '" << mysql_string(default_club_icon, mysql_master_club_) << "', "
            << " " << user->uid() << ", "
            << " '" << mysql_string(blackboard, mysql_master_club_) << "', "
            << now << ")";

        if (!mysql_master_club_.mysql_exec(sql.str()))
        {
            LOG(ERROR) << "create club error. sql: " << sql.str();
            return nullptr;
        }

        uint32_t clubid = mysql_master_club_.get_insert_id_uint32();
        club = std::make_shared<ClubInfo>(clubid, club_name, default_club_icon, user->uid(), blackboard, now, default_config_name);
    }

    if (!NewClubMember(club, user->uid(), user->name(), user->icon(), user->last_login_time(), pb::CLUB_OWNER, now))
    {
        LOG(ERROR) << "insert club member error. clubid: " << club->clubid() << " uid: " << user->uid();
        return nullptr;
    }

    user->AddClub(club->clubid());
    clubs_[club->clubid()] = club;

    RecordClubAction(club->clubid(), user->uid(), "create_club", "");
    return club;
}

bool Club::DisbandClub(UserPtr user, uint32_t clubid)
{
    auto club = GetClub(clubid);
    if (club == nullptr || user == nullptr)
    {
        return false;
    }
    if (!club->IsOwner(user->uid()))
    {
        return false;
    }

    uint64_t now = time(NULL);
    std::stringstream sql;
    sql << "update `club` set `disband_time` = " << now 
        << " where `clubid` = " << club->clubid();
    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "disband club error. sql: " << sql.str();
        return false;
    }

    // 维护user表里面的clubid列表
    auto& members = club->GetAllMember();
    for (auto& elem : members)
    {
        auto member_user = GetUser(elem.first);
        if (member_user == nullptr)
        {
            continue;
        }
        member_user->DelClub(club->clubid());
    }

    clubs_.erase(clubid);

    RecordClubAction(clubid, user->uid(), "disband_club", "");

    LOG(INFO) << "disband club done. clubid: " << clubid << " uid: " << user->uid();
    return true;
}

void Club::HandleClientMsgClubInfoREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::ClubInfoREQ>(data);
    uint32_t clubid = msg->clubid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }

    pb::ClubInfoRSP rsp;
    rsp.mutable_club_info()->CopyFrom(club->GetClubInfo(user->uid()));

    auto now = time(NULL);
    auto push_msgs = club->ReadClubPushMsgList(user->uid());
    for (auto elem : push_msgs)
    {
        auto push_info = rsp.add_push_info();
        push_info->CopyFrom(elem->GetPushMsgInfo());
        
        std::stringstream sql;
        sql << "insert into `club_push_msg_record`(`push_id`, `uid`, `record_time`) values("
            << " " << elem->push_id() << ", "
            << " " << user->uid() << ", "
            << " " << now << ")";
        if (!mysql_master_club_.mysql_exec(sql.str()))
        {
            LOG(ERROR) << "insert error. sql: " << sql.str();
        }
    }

    user->SendToUser(rsp);
}

void Club::HandleClientMsgClubListREQ(MessagePtr data, UserPtr user)
{
    auto uid = user->uid();
    pb::iGetClubRoomNumREQ  req;
    auto& clubids = user->GetUserClubIDs();
    for (auto clubid : clubids)
    {
        req.add_clubids(clubid);
    }
    CallRPC(ServiceHandle("roomlist"), req, [this, uid](MessagePtr data){
        auto user = GetUser(uid);
        if (user == nullptr)
        {
            return;
        }

        auto msg = std::dynamic_pointer_cast<pb::iGetClubRoomNumRSP>(data);
        std::unordered_map<uint32_t, uint32_t>  club_nums;
        for (int i = 0; i < msg->items_size(); i++)
        {
            auto item = msg->items(i);
            club_nums[item.clubid()] = item.num();
        }

        pb::ClubListRSP rsp;
        auto& clubids = user->GetUserClubIDs();
        for (auto clubid : clubids)
        {
            auto club = GetClub(clubid);
            if (club == nullptr)
            {
                continue;
            }
            pb::ClubInfo info = club->GetClubInfo(user->uid());
            info.set_room_num(club_nums[clubid]); 
            rsp.add_clubs()->CopyFrom(info);
        }

        user->SendToUser(rsp);
    });
}

void Club::HandleClientMsgDisbandClubREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::DisbandClubREQ>(data);
    auto clubid = msg->clubid();
    uint64_t uid = user->uid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }
    if (!club->IsOwner(uid))
    {
        return;
    }

    // 检查当前俱乐部有没有桌子
    pb::iGetClubRoomNumREQ    req;
    req.add_clubids(clubid);
    CallRPC(ServiceHandle("roomlist"), req, [this, uid](MessagePtr data) {
        auto user = GetUser(uid);
        if (user == nullptr)
        {
            return;
        }

        auto msg = std::dynamic_pointer_cast<pb::iGetClubRoomNumRSP>(data);
        if (msg->items_size() < 1)
        {
            LOG(ERROR) << "get club_room_num failed";
            return;
        }
        // 这里只需要取第一个
        auto item =msg->items(0);
        auto clubid = item.clubid();
        auto room_num = item.num();

        pb::DisbandClubRSP  rsp;
        rsp.set_clubid(clubid);

        if (room_num > 0)
        {
            rsp.set_code(-1);
            user->SendToUser(rsp);
            return;
        }

        if (DisbandClub(user, clubid))
        {
            rsp.set_code(0);
        }
        else
        {
            rsp.set_code(-2);
        }
        user->SendToUser(rsp);
    });
}

void Club::HandleClientMsgJoinClubREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::JoinClubREQ>(data);
    auto content = msg->content();
    if (content.size() >= 128)
    {
        return;
    }

    auto clubid = msg->clubid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }
    
    pb::JoinClubRSP rsp;
    rsp.set_clubid(clubid);
    rsp.set_club_name(club->club_name());

    // 已经在俱乐部
    auto member = club->GetMember(user->uid());
    if (member != nullptr)
    {
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    // 已经在申请列表
    auto join_msg = club->GetJoinMsg(user->uid());
    if (join_msg != nullptr)
    {
        rsp.set_code(-2);
        user->SendToUser(rsp);
        return;
    }

    uint64_t now = time(NULL);
    std::stringstream sql;
    sql << "insert into `club_join_msg`(`clubid`, `apply_uid`, `apply_time`, `content`) values("
        << " " << clubid << ", "
        << " " << user->uid() << ", "
        << " " << now << ", "
        << " '" << mysql_string(content, mysql_master_club_) << "')";
    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "insert club_join_msg error. sql: " << sql.str();
        rsp.set_code(-3);
        user->SendToUser(rsp);
        return;
    }
    uint64_t msgid = mysql_master_club_.get_insert_id_uint64();
    join_msg = std::make_shared<ClubJoinMsg>(
        msgid,
        clubid,
        user->uid(),
        user->name(),
        user->icon(),
        now,
        content);
    club->AddJoinMsg(join_msg);

    rsp.set_code(0);
    user->SendToUser(rsp);

    // 广播给俱乐部管理员
    BroadcastToClubManager(club, join_msg->GetClubJoinMsg());
}

void Club::HandleClientMsgJoinClubMsgListREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::JoinClubMsgListREQ>(data);
    uint32_t clubid = msg->clubid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }

    if (!club->IsManager(user->uid()))
    {
        return;
    }

    pb::JoinClubMsgListRSP  rsp;
    auto& msgs = club->GetClubJoinMsgList();
    for (auto& elem : msgs)
    {
        rsp.add_infos()->CopyFrom(elem.second->GetClubJoinMsg());
    }
    user->SendToUser(rsp);
}

void Club::HandleClientMsgHandleJoinClubMsgREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::HandleJoinClubMsgREQ>(data);
    auto clubid = msg->clubid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }

    if (!club->IsManager(user->uid()))
    {
        return;
    }

    auto apply_uid = msg->uid();
    auto join_msg = club->GetJoinMsg(apply_uid);
    if (join_msg == nullptr)
    {
        return;
    }

    bool accept = msg->accept();

    pb::HandleJoinClubMsgRSP    rsp;
    rsp.set_clubid(clubid);
    rsp.set_uid(apply_uid);
 
    int status = 2;
    if (accept)
    {
        status = 1;
        if (club->GetMemberNum() >= MAX_CLUB_MEMBER_NUM)
        {
            rsp.set_code(-1);
            user->SendToUser(rsp);
            return;
        }
    }
    else
    {
        status = 2;
    }

    std::stringstream sql;
    uint64_t now = time(NULL);
    sql << "update `club_join_msg` set "
        << " `manager_uid` = " << user->uid() << ", "
        << " `deal_time` = " << now << ", "
        << " `status` = " << status << " "
        << " where `msgid` = " << join_msg->msgid();

    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "update club_join_msg error. sql: " << sql.str();
        return;
    }

    club->EraseJoinMsg(apply_uid);

    rsp.set_code(0);
    user->SendToUser(rsp);

    std::string attach = "manager_uid: " + std::to_string(user->uid());
    if (accept)            // 同意加入
    {
        /*
         * TODO: 
         * 1. send mail to uid
         * 2. send mail to manager
         */
        if (!NewClubMember(club, apply_uid, join_msg->name(), join_msg->icon(), now, pb::CLUB_MEMBER, now))
        {
            LOG(ERROR) << "new_club_member error. clubid: " << club->clubid() << " uid: " << apply_uid;
            return; 
        }

        auto member_user = GetUser(apply_uid);
        if (member_user != nullptr)
        {
            pb::JoinClubBRC brc;
            brc.mutable_club_info()->CopyFrom(club->GetClubInfo(apply_uid));
            member_user->SendToUser(brc);
        }

        RecordClubAction(club->clubid(), apply_uid, "join_club", attach);
    }
    else
    {
        // TODO: send reject mail to uid
        RecordClubAction(club->clubid(), apply_uid, "reject_join_club", attach);
    }
}

bool Club::NewClubMember(ClubPtr club, uint64_t uid, const std::string& name, const std::string& icon, uint64_t last_login_time, pb::ClubRole role, uint64_t now)
{
    if (club == nullptr)
    {
        return false;
    }

    auto member = club->GetMember(uid);
    if (member != nullptr)
    {
        LOG(ERROR) << "member already exists. clubid: " << club->clubid() << " uid: " << uid;
        return false;
    }

    std::stringstream select_sql;
    select_sql << "select `memberid` from `club_member` "
        << " where `clubid` = " << club->clubid()
        << " and `uid` = " << uid;
    if (!mysql_master_club_.mysql_exec(select_sql.str()))
    {
        return false;
    }

    uint64_t memberid = 0;
    MysqlResult     result(mysql_master_club_.handle());
    MysqlRow        row;
    if (result.fetch_row(row))      // 原来有加入过
    {
        memberid = row.get_uint64("memberid");

        std::stringstream update_sql;
        update_sql << "update `club_member` set `role` = "
            << static_cast<uint32_t>(role) << ", "
            << " `jointime` = " << now << ", "
            << " `leavetime` = 0 "
            << " where `memberid` = " << memberid;
        if (!mysql_master_club_.mysql_exec(update_sql.str()))
        {
            LOG(ERROR) << "update club_member error. sql: " << update_sql.str();
            return false;
        }
    }
    else
    {
        {
            // 插入okeygo.club_user_chips表
            std::stringstream insert_sql;
            insert_sql << "insert into `club_user_chips`(`clubid`, `uid`, `chips`) values("
                << " " << club->clubid() << ", "
                << " " << uid << ", "
                << "0)";
            if (!mysql_master_.mysql_exec(insert_sql.str()))
            {
                LOG(ERROR) << "insert okeygo.club_user_chips error. sql: " << insert_sql.str();
                return false;
            }
        }

        {
            // 插入okey_game_record.club_user_record表
            std::stringstream insert_sql;
            insert_sql << "insert into `club_user_record`(`clubid`, `uid`, `last_update_time`) values("
                << " " << club->clubid() << ", "
                << " " << uid << ", "
                << " " << now << ")";
            if (!mysql_master_game_record_.mysql_exec(insert_sql.str()))
            {
                LOG(ERROR) << "insert okey_game_record.club_user_record error. sql: " << insert_sql.str();
                return false;
            }
        }
        
        {
            // 插入okey_club.club_member表
            std::stringstream insert_sql;
            insert_sql << "insert into `club_member`(`clubid`, `uid`, `role`, `jointime`) values("
                << club->clubid() << ", "
                << " " << uid << ", "
                << " " << static_cast<uint32_t>(role) << ", "
                << " " << now << ")";

            if (!mysql_master_club_.mysql_exec(insert_sql.str()))
            {
                LOG(ERROR) << "insert club_member error. sql: " << insert_sql.str();
                return false;
            }

            memberid = mysql_master_club_.get_insert_id_uint64();
        }
    }

    member = std::make_shared<ClubMemberInfo>(
        memberid,
        club->clubid(),
        uid,
        role,
        now,
        name,
        icon,
        last_login_time);

    club->AddMember(member);

    auto member_user = GetUser(uid);
    if (member_user != nullptr)
    {
        member_user->AddClub(club->clubid());
    }

    return true; 
}

void Club::HandleClientMsgSetClubRoleREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::SetClubRoleREQ>(data);
    uint32_t clubid = msg->clubid();
    uint64_t role_uid = msg->uid();
    pb::ClubRole role = msg->role();

    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }

    if (!club->IsOwner(user->uid()))
    {
        return;
    }

    if (role != pb::CLUB_MEMBER || role != pb::CLUB_MANAGER)
    {
        return;
    }

    pb::SetClubRoleRSP  rsp;
    rsp.set_clubid(clubid);
    rsp.set_uid(role_uid);
    rsp.set_role(role);
    auto member = club->GetMember(role_uid);
    if (member == nullptr)
    {
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    std::stringstream sql;
    sql << "update `club_member` set `role` = " << static_cast<uint32_t>(role) << " where `memberid` = " << member->memberid();
    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "update club_member role error. sql: " << sql.str();
        return;
    }
    member->set_role(role);

    rsp.set_code(0);
    user->SendToUser(rsp);
    
    auto member_user = GetUser(role_uid);
    if (member_user != nullptr)
    {
        pb::SetClubRoleBRC  brc;
        brc.set_clubid(clubid);
        brc.set_role(role);
        member_user->SendToUser(brc);
    }

    std::stringstream attach;
    attach << "role: " << role << " manager_uid: " << user->uid();
    RecordClubAction(clubid, role_uid, "update_role", attach.str());
}

void Club::HandleClientMsgQuitClubREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::QuitClubREQ>(data);
    uint32_t clubid = msg->clubid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }

    pb::QuitClubRSP rsp;
    rsp.set_code(-1);
    rsp.set_clubid(clubid);
    if (!DelClubMember(club, user->uid()))
    {
        user->SendToUser(rsp);
        return;
    }

    rsp.set_code(0);
    user->SendToUser(rsp);
    RecordClubAction(clubid, user->uid(), "quit_club", "");
    // TODO: send mail to manager
}

void Club::HandleClientMsgKickClubUserREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::KickClubUserREQ>(data);
    uint64_t manager_uid = user->uid();
    uint32_t clubid = msg->clubid();
    uint64_t kicked_uid = msg->uid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }
    if (!club->IsManager(manager_uid))
    {
        return;
    }

    pb::KickClubUserRSP rsp;
    rsp.set_clubid(clubid);
    rsp.set_uid(kicked_uid);

    auto member = club->GetMember(kicked_uid);
    if (member == nullptr)
    {
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    if (club->IsManager(kicked_uid))
    {
        rsp.set_code(-2);
        user->SendToUser(rsp);
        return;
    }

    // 检查玩家身上的俱乐部币是否为0
    GetClubUserChips(this, clubid, kicked_uid, [this, clubid, manager_uid, kicked_uid](MessagePtr data)
    {
        auto club = GetClub(clubid);
        auto user = GetUser(manager_uid);
        if (club == nullptr || user == nullptr)
        {
            return;
        }

        // double check
        if (!club->IsManager(manager_uid))
        {
            return;
        }

        pb::KickClubUserRSP rsp;
        rsp.set_clubid(clubid);
        rsp.set_uid(kicked_uid);
        auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);
        if (msg->code() != 0)
        {
            LOG(WARNING) << "club_user_chips not exists. clubid: " << clubid << " uid: " << kicked_uid;

            rsp.set_code(-1);
            user->SendToUser(rsp);
            return;
        }

        if (club->IsManager(kicked_uid))
        {
            rsp.set_code(-2);
            user->SendToUser(rsp);
            return;
        }

        if (msg->value() > 0)
        {
            rsp.set_code(-3);
            user->SendToUser(rsp);
            return;
        }

        if (!DelClubMember(club, kicked_uid))
        {
            rsp.set_code(-4);
            user->SendToUser(rsp);
            return;
        }

        rsp.set_code(0);
        user->SendToUser(rsp);

        auto kicked_user = GetUser(kicked_uid);
        if (kicked_user != nullptr)
        {
            pb::KickedClubUserBRC   brc;
            brc.set_clubid(clubid);
            brc.set_club_name(club->club_name());
            kicked_user->SendToUser(brc);
            return;
        }

        RecordClubAction(clubid, kicked_uid, "kick_member", "manager_uid: " + std::to_string(user->uid()));
        // TODO: send mail to manager
    });
}

void Club::HandleClientMsgSendClubPushMsgREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::SendClubPushMsgREQ>(data);
    uint32_t clubid = msg->clubid();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }
    if (!club->IsManager(user->uid()))
    {
        return;
    }

    pb::SendClubPushMsgRSP  rsp;
    rsp.set_clubid(clubid);

    auto title = msg->title();
    auto content = msg->content();
    if (title.size() >= 256 || title.empty() || content.size() >= 2048 || content.empty())
    {
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    uint64_t now = time(NULL);
    std::stringstream sql;
    sql << "insert into `club_push_msg`(`clubid`, `push_time`, `title`, `content`) values("
        << " " << clubid << ", "
        << " " << now << ", "
        << " '" << mysql_string(title, mysql_master_club_) << "', "
        << " '" << mysql_string(content, mysql_master_club_) << "')";
    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "insert club_push_msg error. sql: " << sql.str();
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    uint64_t push_id = mysql_master_club_.get_insert_id_uint64();
    auto push_msg = std::make_shared<ClubPushMsg>(push_id, now, title, content);
    club->AddPushMsg(push_msg);

    rsp.set_code(0);
    user->SendToUser(rsp);

    RecordClubAction(clubid, user->uid(), "send_push_msg", std::to_string(push_id));
}

void Club::HandleClientMsgSendClubMemberChipsREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::SendClubMemberChipsREQ>(data);
    uint64_t manager_uid = user->uid();
    uint32_t clubid = msg->clubid();
    uint64_t uid = msg->uid();
    int64_t change = msg->change();
    auto club = GetClub(clubid);
    if (club == nullptr)
    {
        return;
    }
    if (!club->IsManager(manager_uid))
    {
        return;
    }

    auto member = club->GetMember(uid);
    if (member == nullptr)
    {
        return;
    }

    std::stringstream attach;
    attach << "manager_uid: " << manager_uid;

    ChangeClubUserChips(this, clubid, uid, change, "send_club_chips", attach.str(), [this, clubid, manager_uid, uid, change](MessagePtr data){
        auto club = GetClub(clubid);
        if (club == nullptr)
        {
            return;
        }

        auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);

        std::stringstream attach;
        attach << "manager_uid: " << manager_uid << " change: " << change;
        auto user = GetUser(manager_uid);
        if (user == nullptr)
        {
            if (msg->code() == 0)
            {
                RecordClubAction(clubid, uid, "send_club_chips", attach.str());
            }
            return;
        }

        pb::SendClubMemberChipsRSP  rsp;
        rsp.set_clubid(clubid);
        rsp.set_uid(uid);
        rsp.set_change(change);
        if (msg->code() != 0)
        {
            rsp.set_code(-1);
            user->SendToUser(rsp);
            return;
        }

        rsp.set_code(0);
        user->SendToUser(rsp);
        RecordClubAction(clubid, uid, "send_club_chips", attach.str());
        LOG(INFO) << "send_club_chips done. clubid: " << clubid << " uid: " << uid << " manager_uid: " << manager_uid << " change: " << change << " left: " << msg->value();
    });
}

bool Club::DelClubMember(ClubPtr club, uint64_t uid)
{
    if (club == nullptr)
    {
        return false;
    }

    auto member = club->GetMember(uid);
    if (member == nullptr)
    {
        return false;
    }

    if (club->IsManager(member))
    {
        return false;
    }

    uint64_t now = time(NULL);
    std::stringstream sql;
    sql << "update `club_member` set `leavetime` = "
        << " " << now << " where `memberid` = "
        << member->GetMemberID();
    if (!mysql_master_club_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "del_club_member error. sql: " << sql.str();
        return false;
    }

    club->DelMember(member);

    auto member_user = GetUser(uid);
    if (member_user != nullptr)
    {
        member_user->DelClub(club->clubid());
    }

    return true;
}

void Club::BroadcastToClubManager(ClubPtr club, const Message& msg)
{
    if (club == nullptr)
    {
        return;
    }

    auto& managers = club->GetClubManagers();
    for (auto& m : managers)
    {
        auto user = GetUser(m->uid());
        if (user == nullptr)
        {
            continue;
        }
        user->SendToUser(msg);
    }
}

void Club::RecordClubAction(uint32_t clubid, uint64_t uid, const std::string& type, const std::string& attach)
{
    uint64_t now = time(NULL);

    std::stringstream sql;
    sql << "insert into `flow_club_action`(`clubid`, `uid`, `type`, `attach`, `time`) values("
        << " " << clubid << ", "
        << " " << uid << ", "
        << " '" << mysql_string(type, mysql_master_flow_) << "', "
        << " '" << mysql_string(attach, mysql_master_flow_) << "', "
        << " " << now << ")";
    if (!mysql_master_flow_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "record club action error. sql: " << sql.str();
    }
}

std::shared_ptr<ClubConfig> Club::GetClubConfig(ClubPtr club)
{
    if (club == nullptr)
    {
        return nullptr;
    }

    return club_config_mgr_.GetClubConfig(club->config_name());
}
