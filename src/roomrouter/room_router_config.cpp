#include "room_router_config.h"
#include <algorithm>
#include <fstream>
#include <glog/logging.h>
#include "../common/scope_guard.h"

// 两张映射表
std::unordered_map<std::string, RoomServerFilterType> RoomRouterConfig::filter_type_map_ = {
    {"default", RoomServerFilterType::FILTER_DEFAULT}
};

std::unordered_map<uint32_t, RouterFilterFunc> RoomRouterConfig::filter_map_ = {
    {static_cast<uint32_t>(RoomServerFilterType::FILTER_DEFAULT), RoomRouterConfig::FilterDefault}
};


std::string RoomRouterConfig::config_name_ = "./config/roomsvr_router.json";
std::unordered_map<std::string, RoomServerConfig> RoomRouterConfig::server_configs_;
uint32_t RoomRouterConfig::current_group_ = 1;


RoomServerConfig::RoomServerConfig(const std::string& server_name, const std::string& room_name, std::vector<RoomServerFilterType> filter_type, uint32_t group)
    :server_name_(server_name), 
     room_name_(room_name),
     filter_type_(filter_type), 
     group_(group)
{
    if (filter_type_.empty())
    {
        filter_type_.push_back(RoomServerFilterType::FILTER_DEFAULT);
    }

    for (auto type : filter_type_)
    {
        filters_.emplace_back(RoomRouterConfig::FilterTypeToFunc(type));
    }
}

RoomServerConfig::RoomServerConfig(const RoomServerConfig& obj)
{
    server_name_ = obj.server_name_;
    room_name_ = obj.room_name_;
    group_ = obj.group_;
    filter_type_.assign(obj.filter_type_.begin(), obj.filter_type_.end());
    filters_.assign(obj.filters_.begin(), obj.filters_.end());
}

bool RoomServerConfig::Filter(const std::shared_ptr<pb::iCreateRoomREQ>& req)
{
    for (auto filter : filters_)
    {
        if (filter(req))
        {
            return true;
        }
    }
    return false;
}


bool RoomRouterConfig::LoadConfig()
{
    std::ifstream ifs;
    ifs.open(config_name_, std::ios::binary);
    if (!ifs)
    {
        LOG(ERROR) << "open RoomRouter_config error";
        return false;
    }

    ScopeGuard guard([]{}, [&ifs]{ ifs.close(); });
    
    Json::Reader    reader;
    Json::Value     root;
    if (!reader.parse(ifs, root))
    {
        LOG(ERROR) << "parse RoomRouter_config error";
        return false;
    }

    decltype(server_configs_)   config;
    decltype(current_group_)    current_group = root["current_group"].asUInt();

    // 当前组

    // 读取服务器配置信息
    Json::Value server_config = root["roomsvr_config"];
    for (unsigned int i = 0; i < server_config.size(); i++)
    {
        uint32_t group = server_config[i]["group"].asUInt();
        if (group != current_group)
        {
            continue;
        }
        std::string server_name = server_config[i]["server"].asString();
        if (server_name.empty())
        {
            LOG(ERROR) << "roomsvr router config server_name cannot empty.";
            return false;
        }

        server_name.erase(0, server_name.find_first_not_of(" "));
        server_name.erase(server_name.find_last_not_of(" ") + 1);

        std::string room_name = server_config[i]["room"].asString();
        if (room_name.empty())
        {
            LOG(ERROR) << "roomsvr router config room_name cannot empty.";
            return false;
        }
        room_name.erase(0, room_name.find_first_not_of(" "));
        room_name.erase(room_name.find_last_not_of(" ") + 1);

        Json::Value filterValue = server_config[i]["filter"];
        std::vector<RoomServerFilterType>     filter_type;
        for (unsigned int j = 0; j < filterValue.size(); j++)
        {
            filter_type.push_back(StringToFilterType(filterValue[j].asString()));
        }
        config.emplace(server_name, RoomServerConfig(server_name, room_name, filter_type, group));
    }

    if (config.empty())
    {
        LOG(ERROR) << "roomsvr router config cannot empty";
        return false;
    }

    current_group_ = current_group;
    server_configs_.swap(config);
    for (auto& elem : server_configs_)
    {
        LOG(INFO) << elem.second;
    }

    LOG(INFO) << "load RoomRouterConfig done";
    return true;
}

RouterFilterFunc RoomRouterConfig::FilterTypeToFunc(RoomServerFilterType type)
{
    auto it = filter_map_.find(static_cast<uint32_t>(type));
    if (it == filter_map_.end())
    {
        LOG(ERROR) << "filter_type: " << static_cast<uint32_t>(type) << " don't have filter function";
        return RoomRouterConfig::FilterDefault;
    }
    return it->second;
}

RoomServerFilterType RoomRouterConfig::StringToFilterType(std::string name)
{
    if (name.empty())
    {
        return RoomServerFilterType::FILTER_DEFAULT;
    } 

    // 去掉首尾空格
    name.erase(0, name.find_first_not_of(" "));
    name.erase(name.find_last_not_of(" ") + 1);
    // 转换成小写
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    auto it = filter_type_map_.find(name);
    if (it == filter_type_map_.end())
    {
        return RoomServerFilterType::FILTER_DEFAULT;
    }
    else
    {
        return it->second;
    }
}

RoomServerConfig* RoomRouterConfig::GetServerConfig(std::string server_name)
{
    auto it = server_configs_.find(server_name);
    if (it == server_configs_.end())
    {
        return nullptr;
    }
    else
    {
        return &it->second;
    }
}

// default过滤器需要其它过滤器全部返回false
bool RoomRouterConfig::FilterDefault(const std::shared_ptr<pb::iCreateRoomREQ>& req)
{
    for (auto& elem : filter_map_)
    {
        if (elem.first != static_cast<uint32_t>(RoomServerFilterType::FILTER_DEFAULT))
        {
            if (elem.second(req))
            {
                return false;
            }
        }
    }
    return true;
}
