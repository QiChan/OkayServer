#ifndef __MONITOR_MONITOR_H__
#define __MONITOR_MONITOR_H__

#include "../common/service.h"
#include "../pb/monitor.pb.h"

struct MonitorReport
{
    MonitorReport()
    {
    }

    pb::MonitorInfo     monitor_info_;
    timeval             last_report_time_;
};

class Monitor final : public Service
{
    public:
        Monitor();
        ~Monitor() = default;
        Monitor(const Monitor&) = delete;
        Monitor& operator= (const Monitor&) = delete;

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

    protected:
        virtual void VServiceStartEvent() override;
        virtual void VServiceOverEvent() override;
        virtual void VServerStopEvent() override;

    private:
        void    RegisterCallBack();
        void    MonitorService();

        void    HandleSystemServiceREG(MessagePtr data, uint32_t handle);
        void    HandleSystemServiceHB(MessagePtr data, uint32_t handle);

    private:
        uint32_t    service_interval_;
        int32_t     monitor_timer_;

        int32_t     monitor_checktime_;
        std::unordered_map<uint32_t, MonitorReport>     service_report_;
};
#endif 
