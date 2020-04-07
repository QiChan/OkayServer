#ifndef __ROOM_TABLE_H__
#define __ROOM_TABLE_H__

#include "../common/pack.h"
#include "seat_iterator.h"
#include "poker.h"
#include "seat.h"
#include "action_component.h"
#include <memory>
#include <array>
#include "../common/timer.h"
#include "../pb/inner.pb.h"

class RoomUser;
class Room;
class Table
{
protected:
    using SeatPtr = std::shared_ptr<Seat>;
    using RoomUserPtr = std::shared_ptr<RoomUser>;

public:
    Table (Room*, uint32_t tableid, uint32_t seat_num);
    virtual ~Table ();
    Table (Table const&) = delete;
    Table& operator= (Table const&) = delete;

public:
    void    DelayStartGame();
    void    ShutdownTable();
    bool    CheckShutdownTable() const;               // 判断能不能关闭房间

    bool    UserEnterTable(RoomUserPtr user);
    bool    UserLeaveTable(RoomUserPtr user);
    bool    UserStandUP(RoomUserPtr user);
    bool    UserHostSeat(RoomUserPtr user);     // 托管玩家的座位
    std::tuple<bool, SeatPtr> UserSitDown(RoomUserPtr user, int32_t seatid, int64_t chips, int64_t last_money);

    int         StartTimer(int time, const TimerCallBack& func, const std::string& timer_name = std::string());
    void        StopTimer(int& id);
    uint32_t    GetActionTime() const;

public:
    SeatPtr     GetSeat(uint32_t seatid);

    uint32_t    GetRoomID() const; 
    uint32_t    GetTableID() const;
    uint32_t    GetSitedNum();              // 当前坐下的人数
    uint32_t    GetReadyPlayNum();          // 当前准备好玩牌的人数
    uint32_t    GetNeedMinSeatNum() const;            // 最少玩牌人数
    bool    IsPlaying() const { return is_playing_; }
    bool    IsTableFull();              // 判断座位是否满人
    bool    IsTableEmpty();

protected:
    void    StartGame();
    bool    CheckStartGame();
    void    ReadyToPlayGame();
    void    StartPlayGame();
    void    GameOver(SeatPtr winner, RepeatedPtrField<pb::CardGroup>* cards);         // 游戏结束

    void    BroadcastToUser(const Message& msg, RoomUserPtr except_user = nullptr);
    void    ResetGameID();
    int64_t CalculateRake(int64_t chips) const;    
    pb::iGameUserRecord GetGameUserRecord(RoomUserPtr, int64_t rake, int64_t chips);

public:
    virtual pb::TableBrief  VGetTableBrief(RoomUserPtr user);
    virtual void VDealCards() = 0;              // 发牌
    virtual void VSystemAction() = 0;           // 超时, 系统行动 
    virtual void VHandleUserAction(std::shared_ptr<pb::ActionREQ> msg, RoomUserPtr user) = 0;
    virtual void VStatGameRecord(SeatPtr, RepeatedPtrField<pb::CardGroup>*, pb::iGameRecord& record, std::vector<SeatPtr>& playing_seats, int64_t& win_chips) = 0;

protected:
    Room*                   room_;
    bool                    is_playing_;                // 标识是否正在玩牌
    uint32_t                tableid_;
    std::string             game_id_;                   // 牌局唯一ID
    uint64_t                game_count_;                // 牌局数量
    int64_t                 base_score_;                // 底分
    int                     start_game_timer_;
    std::shared_ptr<Poker>  poker_;
    std::vector<SeatPtr>    seats_;
    std::unordered_set<RoomUserPtr>         bystanders_;    // 旁观

    std::shared_ptr<ActionComponent>        action_component_;          // 玩家行动组件
};

inline std::ostream& operator<< (std::ostream& os, PlayingSeatIterator&& iter)
{
    for (auto seat = *iter; seat != nullptr; seat = *(++iter))
    {
        os << seat;
    }

    return os;
}

inline std::ostream& operator<< (std::ostream& os, SitedSeatIterator&& iter)
{
    for (auto seat = *iter; seat != nullptr; seat = *(++iter))
    {
        os << seat;
    }

    return os;
}

#endif
