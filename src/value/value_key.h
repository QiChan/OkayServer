#ifndef __VALUE_VALUE_KEY_H__
#define __VALUE_VALUE_KEY_H__

#include <unordered_map>
#include <google/protobuf/message.h>
#include "../pb/inner.pb.h"

using namespace google::protobuf;


class ValueKey final
{
    public:
        ValueKey();
        ~ValueKey() = default;
        ValueKey(const ValueKey&);
        ValueKey& operator= (const ValueKey&) = delete;

    public:
        void ParseFromPB(const pb::ValueKey& pb);
        void SerializeToPB(pb::ValueKey* pb) const;

        uint64_t    GetUID() const { return uid_; }
        uint32_t    GetClubID() const { return clubid_; }

    private:
        uint64_t    uid_;
        uint32_t    clubid_;
};

struct ValueKeyCmp
{
    bool operator() (const ValueKey& lhs, const ValueKey& rhs) const
    {
        return lhs.GetUID() == rhs.GetUID() && lhs.GetClubID() == rhs.GetClubID();
    }
};

inline std::ostream& operator<<(std::ostream& out, const ValueKey& o)
{
    out << " uid: " << o.GetUID() << " clubid: " << o.GetClubID();
	return out;
}

#endif
