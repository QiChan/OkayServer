#ifndef __COMMON_PENDINGPROCESSOR_H__
#define __COMMON_PENDINGPROCESSOR_H__

#include <functional>
#include <unordered_map>
#include <vector>
#include "service.h"

using PendingFunc = std::function<void()>;

template<typename K, typename HASH = std::hash<K>, typename PRED = std::equal_to<K>>
class PendingProcessor final
{
    public:
        PendingProcessor(Service* service)
            : service_(service),
              pending_size_(kMaxPendingSize)
        {
            pending_funcs_.reserve(pending_size_);
        }
        ~PendingProcessor() = default;
        PendingProcessor(const PendingProcessor&) = delete;
        PendingProcessor& operator=(const PendingProcessor&) = delete;

    public:
        void set_pending_size(uint32_t pend_size)
        {
            pending_size_ = pend_size;
        }

        void pend(K k)
        {
            if (pending_size_ == 0)
            {
                return;
            }

            auto& funcs = pending_funcs_[k];
            if (funcs.size() >= pending_size_)
            {
                LOG(WARNING) << "Pend_Key: " << k << " pending func size > " << pending_size_;
                return;
            }

            funcs.emplace_back(std::bind(&Service::ServicePollString, service_, service_->curr_msg_data_, service_->curr_msg_source_, service_->curr_msg_session_, service_->curr_msg_type_));
        }

        void clear()
        {
            pending_funcs_.clear();
        }

        void erase(K k)
        {
            pending_funcs_.erase(k);
        }

        void process_pending(K k)
        {
            auto it = pending_funcs_.find(k);
            if (it == pending_funcs_.end())
            {
                return;
            }

            auto funcs = it->second;
            erase(k);

            LOG(INFO) << "process pending message, pend_key:" << k << " size:" << funcs.size();
            for (auto& f : funcs)
            {
                f();
            }
        }

        void process_pending_all()
        {
            for (auto& elem : pending_funcs_)
            {
                auto funcs = elem.second;
                LOG(INFO) << "process pending message, pend_key:" << elem.first << " size:" << funcs.size();
                for (auto& f : funcs)
                {
                    f();
                }
            }
            pending_funcs_.clear();
        }

    private:
        std::unordered_map<K, std::vector<PendingFunc>, HASH, PRED>     pending_funcs_;
        Service*    service_;
        uint32_t    pending_size_;

        static const uint32_t kMaxPendingSize = 20;
};

#endif 
