#ifndef __ROBOT_CLIENT_H__
#define __ROBOT_CLIENT_H__

#include "../common/service.h"
#include "robot.h"
#include "robot_factory.h"

extern "C"
{
#include "skynet.h"
#include "skynet_socket.h"
#include "skynet_server.h"
#include "skynet_handle.h"
#include "skynet_env.h"
#include "skynet_timer.h"
}

class Client : public Service
{
public:
	Client();
    ~Client() = default;
    Client(const Client&) = delete;
    Client& operator= (const Client&) = delete;

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

public:
	void Connect(const char* ip, int port);
    void RegisterCallBack(const std::string& name, const std::function<void(MessagePtr, uint64_t)>& func);
    void ClientKillSelf();
    void ClientSend(const Message&);

protected:
    virtual void VServiceStartEvent() override;
    virtual void VServiceOverEvent() override {}
    virtual void VServerStopEvent() override {}

    virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;
    virtual void VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session) override;

private:
    void    RegisterCallBack();
    void    ClientHeartBeat();

	void    HandleConnect(int fd);
    void    LoginSuccessEvent();

    void    HandleMsgLogin(MessagePtr msg, uint64_t uid);
    void    HandleMsgHeartBeat(MessagePtr msg, uint64_t uid);
    void    HandleMsgUserLogout(MessagePtr msg, uint64_t uid);

    bool    LoadClientVersion();

private:
    std::string version_;
    uint32_t    gate_;
    uint32_t    watchdog_;
    uint64_t    uid_;
    int         client_fd_;
    std::shared_ptr<Robot>  robot_;
    Dispatcher<uint64_t>    client_msg_dsp_;
};

#endif
