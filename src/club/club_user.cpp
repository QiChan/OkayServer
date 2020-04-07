#include "club_user.h"
#include "club.h"

ClubUser::ClubUser()
{
}

ClubUser::~ClubUser()
{
}

void ClubUser::VLoadUserInfo()
{
    auto club = static_cast<Club*>(service_);
    club->LoadUserInfo(uid());
}

void ClubUser::CompleteClubUser(const std::string& name, const std::string& icon, uint64_t last_login_time, const std::unordered_set<uint32_t>& clubids)
{
    name_ = name;
    icon_ = icon;
    last_login_time_ = last_login_time;
    clubids_ = clubids;

    auto club = static_cast<Club*>(service_);
    club->HandleUserInitOK(GetClubUserPtr());
}
