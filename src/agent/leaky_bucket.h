#ifndef __AGENT_LEAKY_BUCKET_H__
#define __AGENT_LEAKY_BUCKET_H__

#include <cstdint>
#include <unordered_map>
#include <string>
#include <tuple>

class LeakyBucket final
{
    public:
        LeakyBucket()
        {}
        ~LeakyBucket() = default;
        LeakyBucket(const LeakyBucket&) = delete;
        LeakyBucket& operator= (const LeakyBucket&) = delete;

        std::tuple<bool, uint32_t> take(const std::string& name, uint64_t now_tick_us, uint32_t bucket_cap, uint32_t rate)
        {
            auto result = std::make_tuple(false, 0);
            auto& bucket = buckets_[name];
            uint64_t last_tick = 0;
            uint32_t water = 0;
            std::tie(last_tick, water) = bucket;

            // 剩下的水位 = 当前水位 - 当前可流出的水位
            int32_t outflow_water = (now_tick_us - last_tick) * rate / 1000000;
            water = std::max<int32_t>(static_cast<int32_t>(water) - outflow_water, 0);

            // 剩下的水位比桶的容量小表示可流出
            if (water < bucket_cap)
            {
                water++;
                std::get<0>(result) = true;
            }

            if (outflow_water > 0)
            {
                std::get<0>(bucket) = now_tick_us;
            }

            std::get<1>(bucket) = water;
            std::get<1>(result) = water;
            return result;
        }

    private:
        std::unordered_map<std::string, std::tuple<uint64_t, uint32_t>>     buckets_;        // pb_name, last_time, water
};



#endif
