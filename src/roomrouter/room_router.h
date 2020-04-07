#ifndef __ROOMROUTER_ROOM_ROUTER_H__
#define __ROOMROUTER_ROOM_ROUTER_H__

#include "../common/msg_service.h"
#include "../pb/inner.pb.h"

class RoomServer final
{
public:
    RoomServer (uint32_t handle_, const std::string& name);
    ~RoomServer ();
    RoomServer (RoomServer const&);
    RoomServer& operator= (RoomServer const&) = delete;

public:
    uint32_t    handle() const { return handle_; }
    bool    IsAlive() const;
    void    Live();
    bool    Filter(const std::shared_ptr<pb::iCreateRoomREQ>&);
    const std::string& GetName() const;

private:
    uint32_t    handle_;
    time_t      alive_time_;
    std::string name_;
};

class RoomRouter final : public Service
{
public:
	RoomRouter();
	virtual ~RoomRouter();
    RoomRouter(const RoomRouter&) = delete;
    RoomRouter& operator= (const RoomRouter&) = delete;

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

protected:
    virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;
    virtual void VServiceStartEvent() override;
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;

private:
    void        RegisterCallBack();
    RoomServer* RouteServer(const std::shared_ptr<pb::iCreateRoomREQ>& msg);
    RoomServer* GetServer(const std::string& name);
    bool        NewServer(uint32_t handle, const std::string& name);

private:
    void    HandleServiceCreateRoomREQ(MessagePtr data, uint32_t handle);
    void    HandleServiceRegisterRoomServer(MessagePtr data, uint32_t source);
    void    HandleServiceRoomServerHeartBeat(MessagePtr data, uint32_t source);

private:
    std::unordered_map<std::string, RoomServer>     room_servers_;
    decltype(room_servers_)::iterator               room_servers_it_;
};

#endif
