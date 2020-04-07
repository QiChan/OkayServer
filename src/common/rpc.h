#ifndef __COMMON_RPC_H__
#define __COMMON_RPC_H__

#include "service_context.h"
#include "dispatcher.h"

using RPCCallBack = std::function<void(MessagePtr msg)>;

class RPCClient
{
public:
    RPCClient ();
    virtual ~RPCClient () = default;
    RPCClient (RPCClient const&) = delete;
    RPCClient& operator= (RPCClient const&) = delete; 

protected:
    void    InitRPCClient(ServiceContext*);

public:
    int     CallRPC(uint32_t dest, const Message& msg, const RPCCallBack& func); 
    void    CancelRPC(int& id);

public:
    void    RPCClientEvent(const char* data, uint32_t size, uint32_t source, int session);
    void    RPCTimeoutEvent(int session);

private:
    ServiceContext*     service_context_;
    int                 idx_;       // session id
    std::unordered_map<int, RPCCallBack>    rpc_;
};

class RPCServer
{
    using CallBack = std::function<MessagePtr(MessagePtr)>;
    using CallBackMap = std::unordered_map<std::string, CallBack>;

public:
    RPCServer ();
    virtual ~RPCServer () = default;
    RPCServer (RPCServer const&) = delete;
    RPCServer& operator= (RPCServer const&) = delete; 

public:
    void    InitRPCServer(ServiceContext*);
    void    RegisterRPC(const std::string& name, const CallBack& func);

public:
    DispatcherStatus    RPCServerEvent(const char* data, uint32_t size, uint32_t source, int session);

private:
    void    response(MessagePtr msg, uint32_t dest, int session);

private:
    ServiceContext* service_context_;
    CallBackMap     callbacks_;

private:
    uint32_t        source_;
    int             session_;
    PackContext     rpc_pack_context_;
};

#endif
