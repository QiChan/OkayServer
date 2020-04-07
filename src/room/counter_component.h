#ifndef __ROOM_COUNTER_COMPONENT_H__
#define __ROOM_COUNTER_COMPONENT_H__

#include <memory>
#include <unordered_map>

class Table;
class Seat;
class RoomUser;

class CounterComponent final
{
    using TablePtr = std::shared_ptr<Table>;
    using SeatPtr = std::shared_ptr<Seat>;

    struct UserInfo
    {
        uint64_t    uid_;
        TablePtr    table_ = nullptr;
        SeatPtr     seat_ = nullptr;
    };
    using UserInfoPtr = std::shared_ptr<UserInfo>;

public:
    // default is ok
    CounterComponent();
    ~CounterComponent () = default;
    CounterComponent (CounterComponent const&) = delete;
    CounterComponent& operator= (CounterComponent const&) = delete;

public:
    void        AddRoomUser(uint64_t uid);
    void        ResetRoomUserPtr(uint64_t uid);
    TablePtr    GetUserTable(uint64_t uid);
    SeatPtr     GetUserSeat(uint64_t uid);
    void        UpdateUserTable(uint64_t uid, TablePtr table);
    void        UpdateUserSeat(uint64_t uid, SeatPtr seat);

    void        GameOverEvent(uint64_t rake);           // 手牌结束事件

private:
    UserInfoPtr     GetUserInfo(uint64_t uid);

private:
    std::unordered_map<uint64_t, UserInfoPtr>   infos_;
    uint64_t    total_rake_;            // 房间总抽水
    uint32_t    total_hands_num_;       // 房间总手牌数
};


#endif 
