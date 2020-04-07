#ifndef __TOOLS_REAL_RAND_H__
#define __TOOLS_REAL_RAND_H__

#include <cstdint>
#include "../common/singleton.h"

class RealRand final : public Singleton<RealRand>
{
    public:
        RealRand();
        ~RealRand();
        RealRand(const RealRand&) = delete;
        RealRand& operator= (const RealRand&) = delete;

    public:
        bool        Init();
        uint32_t    Rand(uint32_t min, uint32_t max);

    private:
        uint32_t    Rand();

    private:
        int                 fd_;
        std::mutex          mutex_;
};


#endif
