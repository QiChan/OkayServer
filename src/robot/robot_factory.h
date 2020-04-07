#ifndef __ROBOT_ROBOT_FACTORY_H__
#define __ROBOT_ROBOT_FACTORY_H__

#include "../common/singleton.h"
#include "robot.h"
#include <memory>
#include <unordered_map>
#include <functional>
#include <glog/logging.h>

class RobotFactory final : public Singleton<RobotFactory>
{
    friend class Singleton<RobotFactory>;
    private:
        RobotFactory() = default;    
        virtual ~RobotFactory() = default;
        RobotFactory(const RobotFactory&) = delete;
        RobotFactory& operator= (const RobotFactory&) = delete; 

    public:
        std::shared_ptr<Robot>  CreateRobot(const std::string& key)
        {
            auto it = m_map.find(key);
            if (it == m_map.end())
            {
                LOG(ERROR) << "key: " << key << " not register yet.";
                return nullptr;
            }
            return it->second();
        }

    public:
        template
        <
            typename T,
            typename = typename std::enable_if
                <
                    std::is_base_of<Robot, typename std::decay<T>::type>::value 
                >::type 
        >
        class Register_F
        {
            public:
                template
                <
                    typename... Args,
                    typename = typename std::enable_if
                    <
                        std::is_constructible<T, Args...>::value 
                    >::type 
                >
                Register_F(std::string key, Args&&... args)
                {
                    RobotFactory::GetInstance()->m_map[key] = [=]()->std::shared_ptr<Robot>
                    {
                        return std::static_pointer_cast<Robot>(std::make_shared<T>(std::forward<Args>(args)...));
                    };
                }
        };
        
    private:
        std::unordered_map<std::string, std::function<std::shared_ptr<Robot>(void)>>    m_map;
};

#define REGISTER_OBJECT_ROBOT(CLASS, KEY, ...) \
    RobotFactory::Register_F<CLASS>  __##CLASS(KEY, ##__VA_ARGS__)

#endif 
