#ifndef __ROOMROUTER_ROOM_ROUTER_CONFIG_H__
#define __ROOMROUTER_ROOM_ROUTER_CONFIG_H__

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "../json/json.h"
#include "../pb/inner.pb.h"

enum class RoomServerFilterType : uint32_t
{
    FILTER_DEFAULT = 1,
};

using RouterFilterFunc = std::function<bool(const std::shared_ptr<pb::iCreateRoomREQ>&)>;

class RoomServerConfig final
{
    public:
        RoomServerConfig(const std::string& server_name, const std::string& room_name, std::vector<RoomServerFilterType> filter, uint32_t group);
        ~RoomServerConfig() = default;
        RoomServerConfig(const RoomServerConfig&);
        RoomServerConfig& operator= (const RoomServerConfig&) = delete;

    public:
        bool Filter(const std::shared_ptr<pb::iCreateRoomREQ>& req);
        std::string GetRoomName() const { return room_name_; }

        friend std::ostream& operator<<(std::ostream&, const RoomServerConfig&);

    private:
        std::string                                 server_name_;           // roomsvr的名称
        std::string                                 room_name_;          // room的名称，决定加载哪一个so去创建房间
        std::vector<RoomServerFilterType>           filter_type_; 
        // filter之间是或的关系, 只要一个通过即可
        std::vector<RouterFilterFunc>               filters_;
        uint32_t                                    group_;           
};

inline std::ostream& operator<<(std::ostream& out, const RoomServerConfig& obj)
{
    out << " server_name: " << obj.server_name_ << " room_name: " << obj.room_name_ << " group: " << obj.group_ << " filters_size: " << obj.filters_.size() << " filter_type: ";
    for (auto& elem : obj.filter_type_)
    {
        out << " " << static_cast<uint32_t>(elem);
    }
    return out;
}

class RoomRouterConfig final 
{
    public:
        RoomRouterConfig() = default;
        ~RoomRouterConfig() = default;
        RoomRouterConfig(const RoomRouterConfig&) = delete;
        RoomRouterConfig& operator= (const RoomRouterConfig&) = delete;

    public:
        static bool LoadConfig();
        static RouterFilterFunc FilterTypeToFunc(RoomServerFilterType type);
        static RoomServerFilterType StringToFilterType(std::string name);
        static RoomServerConfig* GetServerConfig(std::string server_name);

    private:
        static bool FilterDefault(const std::shared_ptr<pb::iCreateRoomREQ>&);

    private:
        static std::string  config_name_;
        static std::unordered_map<std::string, RoomServerConfig>    server_configs_;
        static uint32_t current_group_;        
        static std::unordered_map<uint32_t, RouterFilterFunc> filter_map_;
        static std::unordered_map<std::string, RoomServerFilterType> filter_type_map_;
};


#endif
