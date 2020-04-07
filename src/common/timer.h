#ifndef __COMMON_TIMER_H__
#define __COMMON_TIMER_H__

#include <unordered_map>
#include "service_context.h"

using TimerCallBack = std::function<void()>;

struct TimerInfo
{
    TimerCallBack   func_;
    PackContext     pack_context_;
    std::string     timer_name_;
};

class Timer
{
public:
    Timer ();
    virtual ~Timer () = default;
    Timer (Timer const&) = delete;
    Timer& operator= (Timer const&) = delete; 

protected:
    void    InitTimer(ServiceContext*);

public:
    int     StartTimer(int time, const TimerCallBack& func, const std::string& timer_name = std::string());
    void    StopTimer(int& id);
    void    TimerTimeout(int id);

private:
    int                 idx_;
    ServiceContext*     service_context_;
    std::unordered_map<int, TimerInfo>  timers_;

    static const int kMaxTimerIdx = 0x7fffffff;
};

#endif
