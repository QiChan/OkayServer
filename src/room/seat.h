#ifndef __ROOM_SEAT_H__
#define __ROOM_SEAT_H__

#include "poker.h"
#include <unordered_set>
#include <list>
#include "../pb/table.pb.h"

class RoomUser;

enum class SeatState : uint16_t
{
    IDLE_STATE = 0,             // 空闲状态，可以吃牌和摸牌
    TAKE_STATE = 1,             // 摸牌状态，可以出牌和胡牌
};

struct SeatMutableData final
{
    void    Reset();

    SeatState   state_ = SeatState::IDLE_STATE;
    bool        is_empty_ = true;           // 是否有人坐
    bool        is_playing_ = false;        // 是否在玩牌
    bool        is_host_ = false;           // 是否在托管
    bool        is_ready_ = false;          // 是否准备好玩牌
    int64_t     hand_chips_ = 0;            // 手上的筹码
    int64_t     last_money_ = 0;            // 剩下的钱
    std::unordered_multiset<PokerCard>  hand_cards_;    // 手牌
    std::list<PokerCard>                throw_cards_;   // 丢掉的牌

    std::shared_ptr<RoomUser>   GetUser()
    {
        if (is_empty_)
        {
            return nullptr;
        }
        return user_;
    }
    void SetUser(std::shared_ptr<RoomUser> user)
    {
        user_ = user;
    }
private:
    std::shared_ptr<RoomUser>     user_;
};

class Seat final
{
public:
    Seat (uint32_t seatid);
    virtual ~Seat ();
    Seat ( Seat const&) = delete;
    Seat& operator= ( Seat const&) = delete;

public:
    pb::SeatBrief      GetSeatBrief(std::shared_ptr<RoomUser> user = nullptr);
    std::shared_ptr<RoomUser>   GetSeatUser();
    bool    SitDown(std::shared_ptr<RoomUser> user, int64_t chips, int64_t last_money);
    bool    StandUP();
    void    EnterTable(std::shared_ptr<RoomUser> user);
    void    HostSeat();     // 托管
    void    ReadyToPlayGame(int64_t cose_chips);      // 开始游戏前的准备工作
    void        StartPlayGame(int64_t cost_chips);
    int64_t     GameOver(int64_t chips);
    bool    AddHandChips(int64_t chips, int64_t last_money);
    void    AddCard(PokerCard card);                                // 发牌
    bool    EatCard(std::shared_ptr<Seat> seat, PokerCard& card);   // 吃牌
    bool    TakeCard(PokerCard card);               // 摸牌
    bool    ThrowCard(PokerCard& card);             // 打牌
    std::tuple<PokerCard, PokerCard>   GetLastTwoThrowCards() const;
    void    PopLastThrowCard();

public:
    std::unordered_multiset<PokerCard>  GetHandCards() const { return seat_data_.hand_cards_; }
    int64_t     GetHandChips() const { return seat_data_.hand_chips_; }
    uint32_t    GetSeatID() const { return seatid_; }
    uint64_t    GetSeatUID();
    bool        IsHost() const { return seat_data_.is_host_; }       // 是否托管中
    bool        IsSited() const { return !seat_data_.is_empty_; }    // 判断是否有人坐下了
    bool        IsPlaying() const { return seat_data_.is_playing_; }    // 判断是否在玩牌
    bool        IsReady() const { return seat_data_.is_ready_; }    // 判断是否准备好玩牌
    bool        IsIdleState() const { return IsPlaying() && seat_data_.state_ == SeatState::IDLE_STATE; }
    bool        IsTakeState() const { return IsPlaying() && seat_data_.state_ == SeatState::TAKE_STATE; }

private:
    bool    CheckSitDown() const;
    bool    CheckStandUP() const;

private:
    uint32_t           seatid_;
    SeatMutableData    seat_data_;
};

inline std::ostream& operator<< (std::ostream& os, std::shared_ptr<Seat> seat)
{
    if (seat == nullptr)
    {
        return os;
    }
    os << " " << seat->GetSeatBrief(seat->GetSeatUser()).ShortDebugString();
    return os;
}

#endif
