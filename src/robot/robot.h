#ifndef __ROBOT_ROBOT_H__
#define __ROBOT_ROBOT_H__

#include "../common/service.h"
#include <tools/text_parm.h>

class Client;

class Robot
{
    public:
        Robot();
        virtual ~Robot();
        Robot(const Robot&) = delete;
        Robot& operator= (const Robot&) = delete;

        void RegisterCallBack();


    public:
        virtual void LoginSuccessEvent() = 0;
        virtual void VInitRobot(Client* client, TextParm& parm);

    protected:
        void KillSelf();

    protected:
        Client*     client_;
        uint64_t    uid_;
};


#endif 
