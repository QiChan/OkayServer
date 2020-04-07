#include "value_robot.h"
#include "client.h"
#include "../value/value_client.h"

ValueRobot::ValueRobot()
    : money_(-1),
      route_(false),
      change_(-1)
{
}

ValueRobot::~ValueRobot()
{
}

void ValueRobot::VInitRobot(Client* client, TextParm& parm)
{
    Robot::VInitRobot(client, parm);
    RegisterCallBack();
}

void ValueRobot::LoginSuccessEvent()
{
    PeriodicallyGetUserMoney();
}

void ValueRobot::PeriodicallyGetUserMoney()
{
    pb::ValueREQ    req;
    req.set_value_type(pb::USER_MONEY);
    pb::ValueKey    key;
    key.set_uid(uid_);
    req.mutable_key()->CopyFrom(key);
    client_->ClientSend(req);

    client_->StartTimer(200, std::bind(&ValueRobot::PeriodicallyGetUserMoney, this));
}

void ValueRobot::RegisterCallBack()
{
    client_->RegisterCallBack(pb::ValueRSP::descriptor()->full_name(), std::bind(&ValueRobot::HandleMsgValueRSP, this, std::placeholders::_1, std::placeholders::_2));
}

void ValueRobot::HandleMsgValueRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::ValueRSP>(data);
    if (msg->value_type() != pb::USER_MONEY || msg->key().uid() != uid_ || msg->key().clubid() != 0)
    {
        LOG(ERROR) << msg->ShortDebugString();
        client_->ClientKillSelf();
        return;
    } 
    if (money_ == -1)
    {
        money_ = msg->value();
    }

    if (money_ != static_cast<int64_t>(msg->value()))
    {
        LOG(ERROR) << "value robot test failed. uid: " << uid_ << " money: " << money_ << " value: " << msg->value();
        client_->ClientKillSelf();
       return; 
    }

    if (money_ > 100)
    {
        route_ = true;
        change_ = -1;
    }
    else if(money_ == 0)
    {
        route_ = false;
        change_ = 1;
    }

    if (route_)
    {
        TestPO();
    }
    else
    {
        TestOOP();
    }
}

void ValueRobot::TestPO()
{
    // 测试面向过程的接口
    int64_t change = change_;
    ChangeUserMoney(client_, uid_, change, "value_robot", "value_robot_po_test");
    money_ = money_ + change >= 0 ? money_ + change : money_;

    ChangeUserMoney(client_, uid_, change, "value_robot", "value_robot_po_test_rpc", [this, change](MessagePtr data){
        auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);
        int32_t code = -1;
        if (money_ + change >= 0)
        {
            code = 0;
            money_ += change;
        }

        if (msg->code() != code || msg->value() != money_)
        {
            LOG(ERROR) << "value robot TestOOP ChangeUserMoney failed. uid: " << uid_ << " code: " << msg->code() << " money: " << money_ << " value: " << msg->value();
            client_->ClientKillSelf();
        }
    });

    GetUserMoney(client_, uid_, [this](MessagePtr data){
        auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);
        if (msg->code() != 0 || msg->value() != money_)
        {
            LOG(ERROR) << "value robot TestOOP GetUserMoney failed. uid: " << uid_ << " code: " << msg->code() << " money: " << money_ << " value: " << msg->value();
            client_->ClientKillSelf();
        }
    });
}

void ValueRobot::TestOOP()
{
    int64_t change = change_;

    {
        ValueClient client(pb::USER_MONEY, pb::VALUEOP_CHANGE);
        client.set_uid(uid_);
        client.set_type("value_robot");
        client.set_attach("value_robot_oop_test");
        client.set_change(change);
        client.exec(client_);

        money_ = money_ + change >= 0 ? money_ + change : money_;
    }

    {
        ValueClient client(pb::USER_MONEY, pb::VALUEOP_CHANGE);
        client.set_uid(uid_);
        client.set_type("value_robot");
        client.set_attach("value_robot_oop_test_rpc");
        client.set_change(change);
        client.call(client_, [this, change](MessagePtr data){
            auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);
            int32_t code = -1;
            if (money_ + change >= 0)
            {
                code = 0;
                money_ += change;
            }

            if (msg->code() != code || msg->value() != money_)
            {
                LOG(ERROR) << "value robot TestOOP ChangeUserMoney failed. uid: " << uid_ << " code: " << msg->code() << " money: " << money_ << " value: " << msg->value();
                client_->ClientKillSelf();
            }
        });
    }

    {
        ValueClient client(pb::USER_MONEY, pb::VALUEOP_GET);
        client.set_uid(uid_);
        client.call(client_, [this](MessagePtr data){
            auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data); 
            if (msg->code() != 0 || msg->value() != money_)
            {
                LOG(ERROR) << "value robot TestOOP GetUserMoney failed. uid: " << uid_ << " code: " << msg->code();
                client_->ClientKillSelf();
            }
        });
    }
}

void ValueRobot::HandleChangeValueRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);
    if (msg->code() == 0)
    {
        if (static_cast<int64_t>(msg->value()) != money_)
        {
            LOG(ERROR) << "value robot test failed. uid: " << uid_ << " money: " << money_ << " value: " << msg->value();
            client_->ClientKillSelf();
            return;
        }
    }
}
