#include "seat.h"
#include "room_user.h"

void SeatMutableData::Reset()
{
    state_ = SeatState::IDLE_STATE;
    is_empty_ = true;
    is_playing_ = false;
    is_ready_ = false;
    hand_chips_ = 0;
    last_money_ = 0;
    is_host_ = false;
    user_.reset();
    hand_cards_.clear();
    throw_cards_.clear();
}

Seat::Seat(uint32_t seatid)
    : seatid_(seatid)
{
}

Seat::~Seat()
{
}

pb::SeatBrief Seat::GetSeatBrief(std::shared_ptr<RoomUser> user)
{
    pb::SeatBrief   brief;
    brief.set_seatid(seatid_);
    brief.set_chips(seat_data_.hand_chips_);
    brief.set_last_money(seat_data_.last_money_);
    brief.set_empty(seat_data_.is_empty_);
    brief.set_playing(seat_data_.is_playing_);
    brief.set_host(seat_data_.is_host_);

    auto seat_user = GetSeatUser();
    if (seat_user != nullptr)
    {
        brief.mutable_user_brief()->CopyFrom(seat_user->GetSeatUserBrief(user));
    }

    if (IsPlaying())
    {
        PokerCard last_card, penultimate_card;
        std::tie(penultimate_card, last_card) = GetLastTwoThrowCards();
        brief.add_throw_cards(last_card);
        brief.add_throw_cards(penultimate_card);

        if (seat_user == user)
        {
            for (auto card : seat_data_.hand_cards_)
            {
                brief.add_hand_cards(card);
            }
        }
    }

    return brief;
}

std::shared_ptr<RoomUser> Seat::GetSeatUser()
{
    return seat_data_.GetUser();
}

void Seat::ReadyToPlayGame(int64_t cost_chips)
{
    if (!IsSited())
    {
        return;
    }

    auto user = GetSeatUser();
    if (user == nullptr)
    {
        return;
    }

    // 没筹码或者筹码不够踢出去
    // TODO: 比赛需要特殊处理
    if (seat_data_.hand_chips_ <= 0 || seat_data_.hand_chips_ < cost_chips)
    {
        user->StandUP();
        LOG(INFO) << "user standup beacuse no more money. hand_chips: " << seat_data_.hand_chips_ << " cost_chips: " << cost_chips;
        return;
    }

    seat_data_.is_ready_ = true;
    seat_data_.state_ = SeatState::IDLE_STATE;
}

void Seat::StartPlayGame(int64_t cost_chips)
{
    if (IsReady())
    {
        seat_data_.is_playing_ = true;
        seat_data_.is_ready_ = false;

        // TODO: 比赛需要特殊处理
        if (seat_data_.hand_chips_ < cost_chips)
        {
            LOG(ERROR) << GetSeatUser() << " hand_chips: " << seat_data_.hand_chips_ << " cost_chips: " << cost_chips;
        }
    }
}

int64_t Seat::GameOver(int64_t lost_chips)
{
    // TODO: 比赛需要特殊处理
    int64_t lose = 0;
    int64_t hand_chips = seat_data_.hand_chips_;
    hand_chips += lost_chips;
    if (hand_chips < 0)
    {
        LOG(ERROR) << GetSeatUser() << " hand_chips: " << seat_data_.hand_chips_ << " lost: " << lost_chips;
        lose = seat_data_.hand_chips_;
        seat_data_.hand_chips_ = 0;
    }
    else
    {
        lose = -lost_chips;
        seat_data_.hand_chips_ = hand_chips;
    }

    seat_data_.is_playing_ = false;
    seat_data_.state_ = SeatState::IDLE_STATE;

    seat_data_.hand_cards_.clear();
    seat_data_.throw_cards_.clear();

    return lose;
}

bool Seat::StandUP()
{
    if (!CheckStandUP())
    {
        return false;
    }

    seat_data_.Reset();
    return true;
}

void Seat::HostSeat()
{
    seat_data_.is_host_ = true;
}

void Seat::EnterTable(std::shared_ptr<RoomUser> user)
{
    seat_data_.is_host_ = false;
    seat_data_.SetUser(user);
}

bool Seat::SitDown(std::shared_ptr<RoomUser> user, int64_t chips, int64_t last_money)
{
    if (!CheckSitDown())
    {
        return false;
    }

    seat_data_.Reset();

    seat_data_.is_empty_ = false;
    seat_data_.hand_chips_ = chips;
    seat_data_.last_money_ = last_money;
    seat_data_.SetUser(user);
    return true;
}

bool Seat::CheckSitDown() const
{
    return !IsSited();
}

bool Seat::CheckStandUP() const
{
    return !seat_data_.is_playing_;     // 正在玩牌不允许离开
}

uint64_t Seat::GetSeatUID()
{
    auto user = GetSeatUser();
    if (user == nullptr)
    {
        return 0;
    }
    return user->uid();
}

bool Seat::AddHandChips(int64_t chips, int64_t last_money)
{
    if (!IsSited())
    {
        return false;
    }
    seat_data_.hand_chips_ += chips;
    seat_data_.last_money_ = last_money;
    return true;
}

std::tuple<PokerCard, PokerCard> Seat::GetLastTwoThrowCards() const
{
    auto it = seat_data_.throw_cards_.rbegin();
    if (it == seat_data_.throw_cards_.rend())
    {
        return std::make_tuple(POKER_CARD_NULL, POKER_CARD_NULL);
    }

    PokerCard last_card = *it;
    ++it;
    PokerCard penultimate_card = POKER_CARD_NULL;
    if (it != seat_data_.throw_cards_.rend())
    {
        penultimate_card = *it;
    }
    return std::make_tuple(penultimate_card, last_card);
}

void Seat::PopLastThrowCard()
{
    if (seat_data_.throw_cards_.empty())
    {
        return;
    }

    auto it = seat_data_.throw_cards_.end();
    --it;
    seat_data_.throw_cards_.erase(it);
}

void Seat::AddCard(PokerCard card)
{
    seat_data_.hand_cards_.insert(card);
}

bool Seat::TakeCard(PokerCard card)
{
    if (!IsIdleState())
    {
        return false;
    }

    seat_data_.hand_cards_.insert(card);
    seat_data_.state_ = SeatState::TAKE_STATE;
    return true;
}

bool Seat::EatCard(std::shared_ptr<Seat> seat, PokerCard& card)
{
    std::tie(std::ignore, card) = seat->GetLastTwoThrowCards(); 
    if (card == POKER_CARD_NULL)
    {
        return false;
    }

    if (!TakeCard(card))
    {
        return false;
    }
    
    seat->PopLastThrowCard();
    return true;
}

bool Seat::ThrowCard(PokerCard& card)
{
    if (!IsTakeState())
    {
        return false;
    }

    if (card == POKER_CARD_NULL)
    {
        // 取第一张牌
        auto it = seat_data_.hand_cards_.begin();
        if (it != seat_data_.hand_cards_.end())
        {
            card = *it;
        }
    }

    auto it = seat_data_.hand_cards_.find(card);
    if (it == seat_data_.hand_cards_.end())
    {
        return false;
    }

    seat_data_.hand_cards_.erase(it);
    seat_data_.throw_cards_.push_back(card);
    seat_data_.state_ = SeatState::IDLE_STATE;
    return true;
}
