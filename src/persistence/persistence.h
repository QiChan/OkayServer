#ifndef __PERSISTENCE_PERSISTENCE_H__
#define __PERSISTENCE_PERSISTENCE_H__

#include "../common/service.h"
#include "../common/mysql_client.h"

/*
 * 持久化服务
 * 数据落地服务
 */

class Persistence final : public Service
{
    public:
        Persistence();
        ~Persistence();
        Persistence(const Persistence&) = delete;
        Persistence& operator= (const Persistence&) = delete;

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

    protected:
        virtual void VServiceStartEvent() override;
        virtual void VServiceOverEvent() override;  
        virtual void VServerStopEvent() override;    

    private:
        void RegisterCallBack();
        bool ConnectMysql();

    private:
        void HandleServiceRoomRecord(MessagePtr data, uint32_t handle);
        void HandleServiceGameRecord(MessagePtr data, uint32_t handle);

    private:
        MysqlClient     mysql_master_user_;
        MysqlClient     mysql_master_game_record_;
};

#endif
