#ifndef __ROBOT_CLUB_ROBOT_H__
#define __ROBOT_CLUB_ROBOT_H__

#include "robot_factory.h"
#include "../pb/club.pb.h"

// curl "172.17.0.2:8005/?handler=robotwatchdog&cmd=createrobot&robot_type=create_club&from_uid=11001&num=1"
class CreateClubRobot : public Robot
{
    public:
        CreateClubRobot();
        ~CreateClubRobot();
        CreateClubRobot(const CreateClubRobot&) = delete;
        CreateClubRobot& operator= (const CreateClubRobot&) = delete;

    public:
        virtual void LoginSuccessEvent() override;
        virtual void VInitRobot(Client* client, TextParm& parm) override;

    private:
        void RegisterCallBack();
        void CreateClub();
        void GetClubList();
        void DisbandClub();

    private:
        void HandleMsgClubListRSP(MessagePtr data, uint64_t);
        void HandleMsgCreateClubRSP(MessagePtr data, uint64_t);
        void HandleMsgDisbandClubRSP(MessagePtr data, uint64_t);


    private:
        std::unordered_set<uint32_t>    clubs_;
        std::unordered_set<uint32_t>    create_clubs_;
        std::unordered_set<uint32_t>    disband_clubs_;
        bool    create_;
};


REGISTER_OBJECT_ROBOT(CreateClubRobot, "create_club");
#endif 
