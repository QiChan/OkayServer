#include "persistence.h"
#include "../pb/inner.pb.h"

extern "C"
{
#include "skynet_server.h"
}

Persistence::Persistence()
{
}

Persistence::~Persistence()
{
}

bool Persistence::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    if (!ConnectMysql())
    {
        return false;
    }

    RegServiceName("persistence", false, true);
    RegisterCallBack();

    return true;
}

void Persistence::VServiceStartEvent()
{
    LOG(INFO) << "[" << service_name() << "] start. curr_handle: " << std::hex << skynet_context_handle(ctx()) << " parent_handle: " << parent_;
}

void Persistence::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void Persistence::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

void Persistence::RegisterCallBack()
{
    ServiceCallBack(pb::iRoomRecord::descriptor()->full_name(), std::bind(&Persistence::HandleServiceRoomRecord, this, std::placeholders::_1, std::placeholders::_2));
    ServiceCallBack(pb::iGameRecord::descriptor()->full_name(), std::bind(&Persistence::HandleServiceGameRecord, this, std::placeholders::_1, std::placeholders::_2));
}

bool Persistence::ConnectMysql()
{
    MysqlConfigMgr  config_mgr;
    if (!config_mgr.VLoadConfig())
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER_USER, &mysql_master_user_))
    {
        return false;
    }

    if (!config_mgr.ConnectMysql(MY_M1_OKEYPOKER_GAME_RECORD, &mysql_master_game_record_))
    {
        return false;
    }
    return true;
}

void Persistence::HandleServiceRoomRecord(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iRoomRecord>(data);
    std::string room_set_id = msg->room_set_id();
    auto room_brief = msg->brief();
    uint64_t ownerid = msg->ownerid();

    std::stringstream sql;
    sql << "insert into `room_record`(`room_set_id`, `clubid`, `ownerid`, `roomid`, `room_type`, `room_mode`, `seat_num`, `create_time`, `base_score`) values( "
        << "'" << mysql_string(room_set_id, mysql_master_game_record_) << "', "
        << " " << room_brief.clubid() << ", "
        << " " << ownerid << ", "
        << " " << room_brief.roomid() << ", "
        << " " << static_cast<int32_t>(room_brief.room_type()) << ", "
        << " " << static_cast<int32_t>(room_brief.room_mode()) << ", "
        << " " << room_brief.seat_num() << ", "
        << " " << room_brief.create_time() << ", "
        << " " << room_brief.base_score() << ")";
    if (!mysql_master_game_record_.mysql_exec(sql.str()))
    {
        LOG(ERROR) << "insert into room_record error. sql: " << sql.str();
        return;
    }
}

void Persistence::HandleServiceGameRecord(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iGameRecord>(data);
    std::string room_set_id = msg->room_set_id();
    std::string game_id = msg->game_id();
    uint64_t    game_time = msg->game_time();

    int64_t    total_rake = 0;
    int64_t    total_chips = 0;
    // 更新room_record的hands_num
    {
        std::stringstream sql;
        sql << "update `room_record` set `hands_num` = `hands_num` + 1 where "
            << " `room_set_id` = '" << mysql_string(room_set_id, mysql_master_game_record_) << "'";
        if (!mysql_master_game_record_.mysql_exec(sql.str()))
        {
            LOG(ERROR) << "update room_record hands_num error. sql: " << sql.str();
        }
    }

    for (int i = 0; i < msg->user_records_size(); i++)
    {
        auto user_record = msg->user_records(i);

        {
            // 插入game_record表
            std::stringstream sql;
            sql << "insert into `game_record`(`room_set_id`, `game_id`, `clubid`, `uid`, `rake`, `chips`, `game_time`) values( "
                << " '" << mysql_string(room_set_id, mysql_master_game_record_) << "', "
                << " '" << mysql_string(game_id, mysql_master_game_record_) << "', "
                << " " << user_record.clubid() << ", "
                << " " << user_record.uid() << ", "
                << " " << user_record.rake() << ", "
                << " " << user_record.chips() << ", "
                << " " << game_time << ")";
            if (!mysql_master_game_record_.mysql_exec(sql.str()))
            {
                LOG(ERROR) << "insert into game_record error. sql: " << sql.str();
            }

            total_chips += user_record.chips();
            total_rake += user_record.rake();
        }

        {
            // 更新club_user_record表的hands_num和rake和last_update_time
            std::stringstream sql;
            sql << "update `club_user_record` set "
                << " `hands_num` = `hands_num` + 1 " << ", "
                << " `rake` = `rake` + " << user_record.rake() << ", "
                << " `last_update_time` = " << game_time << " "
                << " where `clubid` = " << user_record.clubid() << " and "
                << " `uid` = " << user_record.uid();
            if (!mysql_master_game_record_.mysql_exec(sql.str()))
            {
                LOG(ERROR) << "update club_user_record error. sql: " << sql.str();
            }
        }
    }

    if ((total_rake + total_chips) != 0)
    {
        LOG(ERROR) << "total_rake != total_chips, total_rake: " << total_rake << " total_chips: " << total_chips;
    }
}
