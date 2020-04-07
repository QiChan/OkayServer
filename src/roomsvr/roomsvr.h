#ifndef __ROOMSVR_ROOMSVR_H__
#define __ROOMSVR_ROOMSVR_H__

#include "../common/service.h"
#include "../pb/inner.pb.h"

class RoomSvr final : public Service
{
public:
    RoomSvr ();
    virtual ~RoomSvr ();
    RoomSvr (RoomSvr const&) = delete;
    RoomSvr& operator= (RoomSvr const&) = delete; 

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

protected:
    virtual void VServiceStartEvent() override;
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;

private:
    void    RegisterCallBack();
    void    PeriodicallyHeartBeat();

private:
    void    HandleServiceCreateRoomREQ(MessagePtr data, uint32_t handle);
};

#endif
