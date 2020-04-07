#include "timer.h"
#include "scope_guard.h"

extern "C"
{
#include "skynet_timer.h"
#include "skynet_server.h"
}

Timer::Timer()
    : idx_(1),
      service_context_(nullptr)
{
}

void Timer::InitTimer(ServiceContext* sctx)
{
    service_context_ = sctx;
}

int Timer::StartTimer(int time, const TimerCallBack& func, const std::string& timer_name)
{
    if (service_context_ == nullptr)
    {
        LOG(ERROR) << "start_timer error";
        return -1;    
    }

    auto it = timers_.find(idx_);
    if (it != timers_.end())
    {
        LOG(ERROR) << "timer overlap";
        return -1;
    }

    int id = idx_++;
    if (idx_ >= kMaxTimerIdx || idx_ < 0)
    {
        idx_ = 1;
    }

    skynet_timeout(skynet_context_handle(service_context_->get_skynet_context()), time, id);
    auto& info = timers_[id];
    info.func_ = func;
    info.pack_context_ = service_context_->pack_context();
    info.timer_name_ = timer_name;
    return id;
}

void Timer::StopTimer(int& id)
{
    if (id <= 0)
    {
        return;
    }

    timers_.erase(id);
    id = 0;
}

void Timer::TimerTimeout(int id)
{
    if (service_context_ == nullptr)
    {
        LOG(ERROR) << "timer_timeout error";
        return;
    }

	auto timer_it = timers_.find(id);
	if (timer_it == timers_.end())
    {
		return;
    }


    service_context_->reset_pack_context(timer_it->second.pack_context_.process_uid_);

    if (!timer_it->second.timer_name_.empty() && service_context_->proto_profile())
    {
        auto timer_name = timer_it->second.timer_name_;
        uint64_t start = TimeUtil::now_tick_us();
        timer_it->second.func_();
        uint64_t end = TimeUtil::now_tick_us();
        service_context_->MonitorWorkload(timer_name, end - start, end);
    }
    else
    {
        timer_it->second.func_();
    }
	timers_.erase(id);
}
