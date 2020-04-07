#include "robot.h"
#include "client.h"

Robot::Robot()
    :client_(nullptr),
     uid_(0)
{
}

Robot::~Robot()
{
    client_ = nullptr;
}

void Robot::VInitRobot(Client* client, TextParm& parm)
{
    uid_ = parm.get_int64("uid");

    client_ = client;

    RegisterCallBack();
}

void Robot::RegisterCallBack()
{
}

void Robot::KillSelf()
{
    client_->ClientKillSelf();
}
