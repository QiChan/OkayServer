#include "monitor.h"

Monitor::Monitor()
    : service_interval_(0),
      monitor_timer_(0),
      monitor_checktime_(1 * 100)
{
}

bool Monitor::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    RegServiceName("monitor", true, false);

    RegisterCallBack();

    service_interval_ = (MONITOR_SERVICE_INTERVAL / 100) * 2;

    monitor_timer_ = StartTimer(monitor_checktime_, std::bind(&Monitor::MonitorService, this));

    return true;
}

void Monitor::RegisterCallBack()
{
    SystemCallBack(pb::mServiceREG::descriptor()->full_name(), std::bind(&Monitor::HandleSystemServiceREG, this, std::placeholders::_1, std::placeholders::_2));
    SystemCallBack(pb::mServiceHB::descriptor()->full_name(), std::bind(&Monitor::HandleSystemServiceHB, this, std::placeholders::_1, std::placeholders::_2));
}

void Monitor::MonitorService()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    for (auto& service : service_report_)
    {
        auto s = now.tv_sec - service.second.last_report_time_.tv_sec;
        // 超过一分钟不打日志了
        if (s > 60)
        {
            continue;
        }

        if (s > service_interval_)
        {
            LOG(ERROR) << "service heartbeat timeout. service_name: " << service.second.monitor_info_.name() << " harbor: " << service.second.monitor_info_.harbor() << " handle: " << std::hex << service.second.monitor_info_.handle() << " last report time: " << std::dec << service.second.last_report_time_.tv_sec;
        }
    }

    monitor_timer_ = StartTimer(monitor_checktime_, std::bind(&Monitor::MonitorService, this));
}

void Monitor::HandleSystemServiceREG(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::mServiceREG>(data);
    LOG(INFO) << "Service reg:" << msg->ShortDebugString();

    MonitorReport& reg = service_report_[msg->base().handle()];
    reg.monitor_info_.CopyFrom(msg->base());
    gettimeofday(&reg.last_report_time_, NULL);
}

void Monitor::HandleSystemServiceHB(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::mServiceHB>(data);

    auto it = service_report_.find(msg->base().handle());
    if (it == service_report_.end())
    {
        LOG(ERROR) << "heartbeat handle:" << msg->base().handle() << " not found";
        return;
    }

    auto last_report_time = it->second.last_report_time_;
    gettimeofday(&it->second.last_report_time_, NULL);

    auto s = it->second.last_report_time_.tv_sec - last_report_time.tv_sec;
    if (s > service_interval_)
    {
        LOG(INFO) << "service heartbeat resume. service_name: " << it->second.monitor_info_.name() << " harbor: " << it->second.monitor_info_.harbor() << " handle: " << std::hex << it->second.monitor_info_.handle() << " time: " << std::dec << it->second.last_report_time_.tv_sec;
    }
}

void Monitor::VServiceStartEvent()
{
    LOG(INFO) << "[" << service_name() << "] start.";
}

void Monitor::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void Monitor::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}
