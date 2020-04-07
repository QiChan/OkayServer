#ifndef __CLUB_CLUB_USER_H__
#define __CLUB_CLUB_USER_H__

#include "../common/msg_user.h"

class ClubUser final : public MsgUser
{
public:
    ClubUser ();
    virtual ~ClubUser ();
    ClubUser (ClubUser const&) = delete;
    ClubUser& operator= (ClubUser const&) = delete;

public:
    virtual void VLoadUserInfo() override;

    void CompleteClubUser(const std::string& name, const std::string& icon, uint64_t last_login_time, const std::unordered_set<uint32_t>& clubids);
    const std::string&  name() const { return name_; }
    const std::string&  icon() const { return icon_; }
    uint64_t last_login_time() const { return last_login_time_; }
    const std::unordered_set<uint32_t>& GetUserClubIDs() const { return clubids_; }

    void    AddClub(uint32_t clubid) { clubids_.insert(clubid); }
    void    DelClub(uint32_t clubid) { clubids_.erase(clubid); }

private:
    std::shared_ptr<ClubUser>   GetClubUserPtr() { return std::dynamic_pointer_cast<ClubUser>(shared_from_this()); }

private:
    std::string     name_;
    std::string     icon_;
    uint64_t        last_login_time_;
    std::unordered_set<uint32_t>        clubids_;
};

#endif 
