#ifndef __ROBOT_GAME_ROBOT_H__
#define __ROBOT_GAME_ROBOT_H__

#include "robot_factory.h"
#include "../pb/room.pb.h"

// curl "172.17.0.2:8005/?handler=robotwatchdog&cmd=createrobot&robot_type=game&from_uid=11001&num=2&clubid=1&roomid=100"
class GameRobot : public Robot
{
public:
    GameRobot ();
    virtual ~GameRobot ();
    GameRobot (GameRobot const&) = delete;
    GameRobot& operator= (GameRobot const&) = delete; 

public:
    virtual void LoginSuccessEvent() override;
    virtual void VInitRobot(Client* client, TextParm& parm) override;

private:
    void HandleMsgEnterRoomRSP(MessagePtr data, uint64_t);
    void HandleMsgEnterTableRSP(MessagePtr data, uint64_t);
    void HandleMsgSitDownRSP(MessagePtr data, uint64_t);
    void HandleMsgGameStartBRC(MessagePtr data, uint64_t);
    void HandleMsgActionBRC(MessagePtr data, uint64_t);

private:
    void RegisterCallBack();
    void EnterRoom();
    void SitDown();
    void TakeOrEatCard();
    void ThrowCard();
    int32_t    GetNextActionSeatID(uint32_t curr_seatid);

private:
    uint32_t    clubid_;
    uint32_t    roomid_;
    uint32_t    tableid_;
    int32_t     seatid_;

    std::set<uint32_t>    playing_seats_;
    uint32_t    card_;

    bool        can_eat_;
};


REGISTER_OBJECT_ROBOT(GameRobot, "game");
#endif
