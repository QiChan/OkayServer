#ifndef __TOOLS_TIME_UTIL_H__
#define __TOOLS_TIME_UTIL_H__

#include <cstdint>

class TimeUtil final
{
public:
    TimeUtil () = default;
    ~TimeUtil () = default;
    TimeUtil (TimeUtil const&) = delete;
    TimeUtil& operator= (TimeUtil const&) = delete; 

public:
    static uint32_t date_to_timestamp(int32_t date_no, int32_t timezone);
    static int32_t  timestamp_to_date(uint32_t timestamp, int32_t timezone);
    static uint64_t now_us();
    static uint64_t now_tick_us();

private:
    /* data */
};

#endif
