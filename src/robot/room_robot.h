#ifndef __ROBOT_ROOM_ROBOT_H__
#define __ROBOT_ROOM_ROBOT_H__

#include "robot_factory.h"
#include "../pb/room.pb.h"

// curl "172.17.0.2:8005/?handler=robotwatchdog&cmd=createrobot&robot_type=room&from_uid=11001&num=2"
class RoomRobot : public Robot
{
    public:
        RoomRobot();
        ~RoomRobot();
        RoomRobot(const RoomRobot&) = delete;
        RoomRobot& operator= (const RoomRobot&) = delete;

    public:
        virtual void LoginSuccessEvent() override;
        virtual void VInitRobot(Client* client, TextParm& parm) override;

    private:
        void RegisterCallBack();
        void CreateClubRoom();
        void PeriodicallyGetClubRoom();

        void EnterRoom(uint32_t roomid);
        void LeaveRoom();

    private:
        void HandleMsgCreateRoomRSP(MessagePtr data, uint64_t);
        void HandleMsgClubRoomRSP(MessagePtr data, uint64_t);
        void HandleMsgEnterRoomRSP(MessagePtr data, uint64_t);
        void HandleMsgLeaveRoomRSP(MessagePtr data, uint64_t);

    private:
        uint32_t                            clubid_;
        uint32_t                            roomid_;
        std::shared_ptr<pb::ClubRoomRSP>    rooms_;
};


REGISTER_OBJECT_ROBOT(RoomRobot, "room");
#endif 
