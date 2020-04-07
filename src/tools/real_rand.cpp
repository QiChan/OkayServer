#include "real_rand.h"
#include <unistd.h>
#include <fcntl.h>

RealRand::RealRand()
    : fd_(-1)
{

}

RealRand::~RealRand()
{
    if (fd_ >= 0)
    {
        close(fd_);
    } 
}

bool RealRand::Init()
{
    std::lock_guard<std::mutex>     lck(mutex_);

    fd_ = open("/dev/urandom", O_RDONLY, 0);
    if (fd_ < 0)
    {
        return false;
    }

    return true;
}

uint32_t RealRand::Rand()
{
    uint32_t rand = 0;
    std::lock_guard<std::mutex>     lck(mutex_);
    read(fd_, &rand, sizeof(rand));
    return rand;
}

uint32_t RealRand::Rand(uint32_t min, uint32_t max)
{
    uint32_t div = max - min + 1;
    uint32_t rand_num = Rand();
    return (rand_num % div) + min;
}
