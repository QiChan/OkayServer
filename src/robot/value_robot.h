#ifndef __ROBOT_VALUE_ROBOT_H__
#define __ROBOT_VALUE_ROBOT_H__

#include "robot_factory.h"
#include "../pb/value.pb.h"

// curl "172.17.0.2:8005/?handler=robotwatchdog&cmd=createrobot&robot_type=value&from_uid=11001&num=1"
class ValueRobot : public Robot
{
    public:
        ValueRobot();
        ~ValueRobot();
        ValueRobot(const ValueRobot&) = delete;
        ValueRobot& operator= (const ValueRobot&) = delete;

    public:
        virtual void LoginSuccessEvent() override;
        virtual void VInitRobot(Client* client, TextParm& parm) override;

    private:
        void RegisterCallBack();
        void PeriodicallyGetUserMoney();

        void HandleMsgValueRSP(MessagePtr data, uint64_t);
        void HandleChangeValueRSP(MessagePtr data, uint64_t);

        void TestOOP();
        void TestPO();

    private:
        int64_t     money_;
        bool        route_;
        int64_t     change_;
};

REGISTER_OBJECT_ROBOT(ValueRobot, "value");
#endif
