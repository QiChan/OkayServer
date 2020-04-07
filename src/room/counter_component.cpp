#include "counter_component.h"
#include "room_user.h"

CounterComponent::CounterComponent()
    : total_rake_(0),
      total_hands_num_(0)
{
}

void CounterComponent::AddRoomUser(uint64_t uid)
{
    auto sp_info = GetUserInfo(uid);
    if (sp_info == nullptr)
    {
        sp_info = std::make_shared<UserInfo>();
    }

    sp_info->uid_ = uid;
    infos_[uid] = sp_info;
}

CounterComponent::TablePtr CounterComponent::GetUserTable(uint64_t uid)
{
    auto sp_info = GetUserInfo(uid);
    if (sp_info == nullptr)
    {
        return nullptr;
    }
    return sp_info->table_;
}

CounterComponent::SeatPtr CounterComponent::GetUserSeat(uint64_t uid)
{
    auto sp_info = GetUserInfo(uid);
    if (sp_info == nullptr)
    {
        return nullptr;
    }
    return sp_info->seat_;
}

void CounterComponent::UpdateUserTable(uint64_t uid, TablePtr table)
{
    auto sp_info = GetUserInfo(uid);
    if (sp_info == nullptr)
    {
        return;
    }
    sp_info->table_ = table;
}

void CounterComponent::UpdateUserSeat(uint64_t uid, SeatPtr seat)
{
    auto sp_info = GetUserInfo(uid);
    if (sp_info == nullptr)
    {
        return;
    }
    sp_info->seat_ = seat;
}

CounterComponent::UserInfoPtr CounterComponent::GetUserInfo(uint64_t uid)
{
    auto it = infos_.find(uid);
    if (it == infos_.end())
    {
        return nullptr;
    }
    return it->second;
}

void CounterComponent::GameOverEvent(uint64_t rake)
{
    total_rake_ += rake;
    total_hands_num_ += 1;
}
