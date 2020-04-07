#include "club_robot.h"
#include "client.h"
    
CreateClubRobot::CreateClubRobot()
    : create_(true)
{
}

CreateClubRobot::~CreateClubRobot()
{
}

void CreateClubRobot::VInitRobot(Client* client, TextParm& parm)
{
    Robot::VInitRobot(client, parm);


    RegisterCallBack();
}

void CreateClubRobot::LoginSuccessEvent()
{
    GetClubList();
}

void CreateClubRobot::RegisterCallBack()
{
    client_->RegisterCallBack(pb::ClubListRSP::descriptor()->full_name(), std::bind(&CreateClubRobot::HandleMsgClubListRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::CreateClubRSP::descriptor()->full_name(), std::bind(&CreateClubRobot::HandleMsgCreateClubRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::DisbandClubRSP::descriptor()->full_name(), std::bind(&CreateClubRobot::HandleMsgDisbandClubRSP, this, std::placeholders::_1, std::placeholders::_2));
}

void CreateClubRobot::GetClubList()
{
    client_->StartTimer(400, [this]{
        pb::ClubListREQ req;
        client_->ClientSend(req);
    });
}

void CreateClubRobot::HandleMsgClubListRSP(MessagePtr data, uint64_t club)
{
    clubs_.clear();
    auto msg = std::dynamic_pointer_cast<pb::ClubListRSP>(data);
    for (int i = 0; i < msg->clubs_size(); i++)
    {
        auto club = msg->clubs(i);
        clubs_.insert(club.clubid());
    }

    // 检查创建成功的俱乐部是否都在列表
    for (uint32_t clubid : create_clubs_)
    {
        if (clubs_.find(clubid) == clubs_.end())
        {
            LOG(ERROR) << "create club not exist. clubid: " << clubid << " uid: " << uid_;
            return; 
        }
    }
    
    // 检查解散的俱乐部是否都不在列表
    for (uint32_t clubid : disband_clubs_)
    {
        if (clubs_.find(clubid) != clubs_.end())
        {
            LOG(ERROR) << "disband club exist. clubid: " << clubid << " uid: " << uid_;
            return;
        }
    }

    if (clubs_.empty() || create_)
    {
        CreateClub();
    }
    else
    {
        DisbandClub();
    }
}


void CreateClubRobot::CreateClub()
{
    create_ = true;

    uint64_t now = time(NULL);
    std::string club_name = "club_" + std::to_string(uid_) + "_" + std::to_string(now);
    std::string blackboard = club_name + ": hello world";
    pb::CreateClubREQ  req;
    req.set_club_name(club_name);
    req.set_blackboard(blackboard);
    client_->ClientSend(req);
}

void CreateClubRobot::DisbandClub()
{
    if (clubs_.empty())
    {
        LOG(ERROR) << "disband club error. clubs is empty. uid: " << uid_;
        return;
    }

    pb::DisbandClubREQ  req;
    auto begin = clubs_.begin();
    req.set_clubid(*begin);
    client_->ClientSend(req);
}

void CreateClubRobot::HandleMsgCreateClubRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::CreateClubRSP>(data);
    int32_t code = msg->code();
    auto club_info = msg->club_info();

    if (code == 0)
    {
        // 成功
        create_clubs_.insert(club_info.clubid());
        LOG(INFO) << "create club. clubid: " << club_info.clubid() << " uid: " << uid_;
    }
    else
    {
        LOG(INFO) << "create failed, start test disband club. uid: " << uid_;
        // 开始解散
        create_ = false;
    }

    GetClubList();
}

void CreateClubRobot::HandleMsgDisbandClubRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::DisbandClubRSP>(data);
    int32_t code = msg->code();
    uint32_t clubid = msg->clubid();
    if (code != 0)
    {
        LOG(ERROR) << "disband club error. clubid: " << clubid << " uid: " << uid_;
        return;
    }

    LOG(INFO) << "disband club. clubid: " << clubid << " uid: " << uid_;
    create_clubs_.erase(clubid);
    GetClubList();
}
