#include "room.h"
#include "../value/value_client.h"
#include "okey_table.h"

Room::Room()
    : MsgService(MsgServiceType::SERVICE_ROOM),
      is_shutdown_(false),
      create_time_(time(NULL))
{
}

Room::~Room()
{
}

bool Room::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!MsgService::VInitService(ctx, parm, len))
    {
        return false;
    }

    DependOnService("roomlist");
    DependOnService("value");
    DependOnService("persistence");

    RegServiceName("room", false, false);

    ResetRoomSetID();
    return true;
}

void Room::VServiceStartEvent()
{
    PeriodicallyReportSelf();

    pb::iRoomRecord record = GetRoomRecord();
    SendToConcurrentService(record, ServiceHandle("persistence"));

    ROOM_XLOG(INFO) << "service start. room_set_id: " << room_set_id_ << " ownerid: " << GetRoomOwnerID() << " clubid: " << GetRoomClubID();
}

void Room::VServiceOverEvent()
{
    pb::iRoomOver   report;
    report.set_clubid(GetRoomClubID());
    report.set_roomid(GetRoomID());
    SendToService(report, ServiceHandle("roomlist"));

    // 踢出房间的所有玩家, 注意迭代器失效的情况
    for (auto it = users_.begin(); it != users_.end();)
    {
        auto user = it->second;
        it++;
        KickRoomUser(user);
    }

    // 销毁所有的房间
    for (auto& elem : tables_)
    {
        elem.second->ShutdownTable();
    }

    ROOM_XLOG(INFO) << "shutdown. room_set_id: " << room_set_id_;
}

void Room::VServerStopEvent()
{
    ROOM_XLOG(INFO) << "server stop event.";
}

// 此函数被调用之前，必须要保证所有牌桌都能被shutdown
void Room::DelayShutdownRoom(uint32_t seconds)
{
    if (is_shutdown_)
    {
        ROOM_XLOG(ERROR) << "already shutdown";
        return;
    }
    is_shutdown_ = true;

    if (seconds == 0)
    {
        seconds = 1;
        return;
    }
    StartTimer(seconds * 100, std::bind(&Room::ShutdownService, this));
}

void Room::RegisterCallBack()
{
    ServiceCallBack(pb::iEnterRoom::descriptor()->full_name(), std::bind(&Room::HandleServiceEnterRoom, this, std::placeholders::_1, std::placeholders::_2));

    MsgCallBack(pb::LeaveRoomREQ::descriptor()->full_name(), std::bind(&Room::HandleClientMsgLeaveRoomREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::ActionREQ::descriptor()->full_name(), std::bind(&Room::HandleClientMsgActionREQ, this, std::placeholders::_1, std::placeholders::_2));
}

void Room::HandleServiceEnterRoom(MessagePtr data, uint32_t handle)
{
    if (IsServerStop())
    {
        return;
    }

    auto msg = std::dynamic_pointer_cast<pb::iEnterRoom>(data);

    if (msg->clubid() != GetRoomClubID())
    {
        return;
    }

    UserPtr user = NewRoomUser(msg); 
    ROOM_XLOG(INFO) << "user enter room." << user;
}

void Room::HandleClientMsgLeaveRoomREQ(MessagePtr data, UserPtr user)
{
    if (!user->LeaveTable())
    {
        // 离开房间失败就开启托管
        if (!user->HostSeat())
        {
            ROOM_XLOG(ERROR) << "user leave table failed. but seat is nullptr";
        }
    }

    DelRoomUser(user->uid());
    ROOM_XLOG(INFO) << "user leave room." << user;
}

void Room::HandleClientMsgActionREQ(MessagePtr data, UserPtr user)
{
    auto table = GetUserTable(user->uid());
    if (table == nullptr)
    {
        return;
    }
    auto msg = std::dynamic_pointer_cast<pb::ActionREQ>(data);
    table->VHandleUserAction(msg, user);
}

void Room::VUserEnterRoomEvent(UserPtr user)
{
    // 同步room_handle到agent
    pb::iAgentRoomStatus    status = FillAgentRoomStatus(true, user->GetClubID());
    SendToService(status, user->GetHandle());

    pb::EnterRoomRSP    rsp;
    rsp.set_clubid(user->GetClubID());
    rsp.set_roomid(GetRoomID());
    rsp.set_code(0);
    user->SendToUser(rsp);

    counter_.AddRoomUser(user->uid());
}

void Room::VUserLeaveRoomEvent(UserPtr user)
{
    // 同步room_handle到agent
    pb::iAgentRoomStatus    status = FillAgentRoomStatus(false, user->GetClubID());
    SendToService(status, user->GetHandle());

    // 发送离开房间的消息到客户端
    pb::LeaveRoomRSP    rsp;
    rsp.set_roomid(GetRoomID());
    rsp.set_clubid(user->GetClubID());
    user->SendToUser(rsp);
}

void Room::VUserEnterTableEvent(UserPtr user, TablePtr table)
{
    counter_.UpdateUserTable(user->uid(), table);

    pb::EnterTableRSP   rsp;
    rsp.set_roomid(GetRoomID());
    rsp.set_tableid(table->GetTableID());
    rsp.mutable_table_brief()->CopyFrom(table->VGetTableBrief(user));
    user->SendToUser(rsp);

    ROOM_XLOG(INFO) << " enter table success. tableid: " << table->GetTableID() << " uid: " << user->uid();
}

void Room::VUserSitDownEvent(UserPtr user, RoomUser::SeatPtr seat)
{
    counter_.UpdateUserSeat(user->uid(), seat);
}

// 事件发生时，用户可能不在房间
void Room::VUserLeaveTableEvent(uint64_t uid)
{
    counter_.UpdateUserTable(uid, nullptr);

    ROOM_XLOG(INFO) << "uid: " << uid;
}

// 事件发生时，用户可能已经不在房间
void Room::VUserStandUPEvent(uint64_t uid, pb::SeatBrief brief)
{
    counter_.UpdateUserSeat(uid, nullptr);
}

void Room::VGameStartEvent(uint32_t tableid, const std::string& game_id)
{
}

void Room::VGameOverEvent(uint32_t tableid, const std::string& game_id, std::vector<SeatPtr>& playing_seats, SeatPtr winner, int64_t win_chips)
{
}

bool Room::VCheckStartGame(uint32_t tableid)
{
    return true;
}

void Room::VGameStartFailEvent(uint32_t tableid)
{
}

Room::TablePtr Room::NewRoomTable()
{
    if (GetRoomType() == pb::OKEY_ROOM)
    {
        TablePtr table = std::make_shared<OkeyTable>(this, tables_.size(), GetSeatNum());
        tables_[tables_.size()] = table;
        return table;
    }
    else if (GetRoomType() == pb::OKEY_101_ROOM)
    {
        TablePtr table = std::make_shared<OkeyTable101>(this, tables_.size(), GetSeatNum());
        tables_[tables_.size()] = table;
        return table;
    }

    ROOM_XLOG(ERROR) << "unknown room_type: " << pb::RoomType_Name(GetRoomType());
    return nullptr;
}

MsgService<RoomUser>::UserPtr Room::NewRoomUser(const std::shared_ptr<pb::iEnterRoom>& msg)
{
    uint32_t clubid = msg->clubid();
    uint64_t uid = msg->uid();
    uint32_t agent_handle = msg->handle();
    std::string ip = msg->ip();

    UserPtr user = NewUser(agent_handle, uid);
    user->CompleteRoomUser(ip, clubid);

    // TODO: gps ip limit
    VUserEnterRoomEvent(user);
    return user;
}

/*
 * 注意：
 * 循环KickRoomUser注意迭代器失效的情况
 */
bool Room::KickRoomUser(UserPtr user)
{
    if (user == nullptr)
    {
        return true;
    }

    if(!user->LeaveTable())
    {
        return false;
    }
    DelRoomUser(user->uid());
    return true;
}

void Room::DelRoomUser(uint64_t uid)
{
    auto user = GetUser(uid);
    if (user != nullptr)
    {
        VUserLeaveRoomEvent(user);
        DelUser(user);
    }
}

void Room::PeriodicallyReportSelf()
{
    pb::iRoomBriefReport    report;
    report.set_handle(GetRoomHandle());
    report.mutable_room()->CopyFrom(VGetRoomBrief());
    SendToService(report, ServiceHandle("roomlist"));

    // 每隔10秒向roomlist上传自己的简要报告
    StartTimer(10 * 100, std::bind(&Room::PeriodicallyReportSelf, this));
}

void Room::VUserDisconnectEvent(UserPtr user)
{
    // 尝试离开房间，失败就托管
    if (!user->LeaveTable())
    {
        user->HostSeat();
    }
    ROOM_XLOG(INFO) << "disconnect: " << user->uid();
}

void Room::VUserReleaseEvent(UserPtr user)
{
    ROOM_XLOG(INFO) << "release: " << user->uid();
}

void Room::VUserRebindEvent(UserPtr user)
{
    /*
     * rebind的处理
     * 不进行其他处理，让客户端重新发EnterRoomREQ请求
     */
    ROOM_XLOG(INFO) << "rebind: " << user->uid();
}

pb::iAgentRoomStatus Room::FillAgentRoomStatus(bool enter, uint32_t clubid) const
{
    pb::iAgentRoomStatus    status;
    status.set_enter(enter);
    status.set_roomid(GetRoomID());
    status.set_handle(GetRoomHandle());
    status.set_clubid(clubid);
    return status;
}

void Room::ResetRoomSetID()
{
    time_t ct = time(NULL);
    char str[128];
    struct tm* t = localtime(&ct);
    t->tm_year -= 100;
    t->tm_mon += 1;
    sprintf(str, "%02d%02d%02d%02d%02d%02d-%d", t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, GetRoomID());

    room_set_id_ = str;
}

void Room::ChangeRoomUserMoney(int64_t money, uint64_t uid, const std::string& type, const std::string& attach, const RPCCallBack& func)
{
    if (money == 0)
    {
        return;
    }

    pb::RoomMode    mode = GetRoomMode();
    pb::ValueType   value_type;
    switch (mode)
    {
        case pb::CLUB_CASH_ROOM:
            {
                value_type = pb::CLUB_USER_CHIPS;
                break;
            }
        default:
            {
                ROOM_XLOG(ERROR) << "unknow room_mode. " << pb::RoomMode_Name(mode);
                value_type = pb::USER_MONEY;
                break;
            }
    }

    ValueClient client(value_type, pb::VALUEOP_CHANGE);
    client.set_uid(uid);
    client.set_clubid(GetRoomClubID());
    client.set_change(money);
    client.set_type(type);
    client.set_attach(attach);

    if (func == nullptr)
    {
        client.exec(this);
    }
    else
    {
        client.call(this, func);
    }
}

pb::RoomBrief Room::VGetRoomBrief() const
{
    pb::RoomBrief brief;
    brief.set_roomid(GetRoomID());
    brief.set_room_name(GetRoomName());
    brief.set_clubid(GetRoomClubID());
    brief.set_room_type(GetRoomType());
    brief.set_room_mode(GetRoomMode());
    brief.set_seat_num(GetSeatNum());
    brief.set_sited_num(GetSitedNum());
    brief.set_create_time(create_time_);
    return brief;
}

pb::iRoomRecord Room::GetRoomRecord() const
{
    pb::iRoomRecord record;
    record.set_room_set_id(GetRoomSetID());
    record.mutable_brief()->CopyFrom(VGetRoomBrief());
    record.set_ownerid(GetRoomOwnerID());

    return record;
}

uint32_t Room::GetSitedNum() const
{
    uint32_t num = 0;
    for (auto elem : tables_)
    {
        uint32_t temp = elem.second->GetSitedNum();
        num += temp;
    }

    return num;
}

void Room::SendToConcurrentService(const Message& msg, uint32_t handle)
{
    SendToService(msg, handle, GetRoomID());
}

Room::TablePtr Room::GetTable(uint32_t tableid)
{
    auto it = tables_.find(tableid);
    if (it == tables_.end())
    {
        return nullptr;
    }
    return it->second;
}
