#include "time_util.h"
#include <string>
#include <sys/time.h>

uint32_t TimeUtil::date_to_timestamp(int32_t date_no, int32_t timezone)
{
    std::string date_str = std::to_string(date_no);
    if (date_str.length() != 8)
    {
        return 0;
    }

    uint32_t year = std::stoi(date_str.substr(0, 4));
    uint32_t month = std::stoi(date_str.substr(4, 2));
    uint32_t day = std::stoi(date_str.substr(6, 2));

    struct tm timeinfo;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = -timezone;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;

    time_t gm_time = timegm(&timeinfo);

    return gm_time;
}

int32_t TimeUtil::timestamp_to_date(uint32_t timestamp, int32_t timezone)
{
    time_t tz_time = timestamp + timezone * 60 * 60;
    struct tm* gm = gmtime(&tz_time);
    char buf[16] = {0};
    snprintf(buf, 16 - 1, "%d%02d%02d", gm->tm_year + 1900, gm->tm_mon + 1, gm->tm_mday);
    return std::stoi(buf);
}

uint64_t TimeUtil::now_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t seconds = tv.tv_sec;
    uint64_t us = seconds * 1000000 + tv.tv_usec;
    return us;
}

uint64_t TimeUtil::now_tick_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t seconds = ts.tv_sec;
    return seconds * 1000000 + ts.tv_nsec / 1000;
}
