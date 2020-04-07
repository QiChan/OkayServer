#ifndef __DBHELPER_DBHELPER_H__
#define __DBHELPER_DBHELPER_H__

#include "../common/msg_slave_service.h"
#include "../common/mysql_client.h"
#include "../pb/inner.pb.h"
#include "dbhelper_user.h"

/*
 * dbhelper
 * 提供数据库读操作
 */

class DBHelper final : public MsgSlaveService<DBHelperUser>
{
public:
    DBHelper ();
    virtual ~DBHelper ();
    DBHelper (DBHelper const&) = delete;
    DBHelper& operator= (DBHelper const&) = delete; 

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;
    virtual void VReloadUserInfo(UserPtr user) override;

protected:
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;

private:
    void    RegisterCallBack();
    bool    ConnectMysql();

private:
    MessagePtr  HandleRPCLoadRoomListUserREQ(MessagePtr);
    MessagePtr  HandleRPCLoadClubUserREQ(MessagePtr);
    void        HandleClientMsgClubMemberListREQ(MessagePtr data, UserPtr user);

private:
    pb::iBaseUserInfo   GetBaseUserInfo(uint64_t uid);

private:
    MysqlClient     mysql_slave_user_;
    MysqlClient     mysql_slave_club_;
};


#endif
