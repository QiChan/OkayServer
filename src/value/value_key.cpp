#include "value_key.h"
#include "../pb/value.pb.h"

ValueKey::ValueKey()
    : uid_(0),
      clubid_(0)
{
}

ValueKey::ValueKey(const ValueKey& obj)
    : uid_(obj.uid_),
      clubid_(obj.clubid_)
{
}

void ValueKey::ParseFromPB(const pb::ValueKey& pb)
{
    uid_ = pb.uid();
    clubid_ = pb.clubid();
}

void ValueKey::SerializeToPB(pb::ValueKey* pb) const
{
    pb->set_uid(uid_);
    pb->set_clubid(clubid_);
}
