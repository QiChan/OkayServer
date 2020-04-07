#ifndef __COMMON_SINGLETON_H__
#define __COMMON_SINGLETON_H__

#include <mutex>

#if defined(__GNUC__) && (!defined(__clang__))
#define ATTRIBUTE_USED__ __attribute__((__used__))
#else
#define ATTRIBUTE_USED__ // no-op
#endif

// 懒汉模式
template<typename T>
class Singleton
{
    protected:
        Singleton() = default;
        virtual ~Singleton() = default;
        Singleton(const Singleton<T>&) = delete;
        Singleton<T>& operator= (const Singleton<T>&) = delete;

    private:
        class CGarbo
        {
            public:
                CGarbo() = default;
                ~CGarbo()
                {
                    if (Singleton<T>::m_pInstance)
                    {
                        // 单例的释放
                        delete Singleton<T>::m_pInstance;
                        Singleton<T>::m_pInstance = nullptr;
                    }
                }
                CGarbo(const CGarbo&) = delete;
                CGarbo& operator= (const CGarbo&) = delete;
        };

        static T* m_pInstance;
        static CGarbo Garbo ATTRIBUTE_USED__;        // 定义一个静态成员变量，程序结束时，系统会自动调用它的析构函数
        static std::once_flag oc;   // 用来保证线程安全

    private:
        template<typename... Args>
        static void Init(Args&&... args)
        {
            m_pInstance = new T(std::forward<Args>(args)...);
        }

    public:
        template<typename... Args>
        static T* GetInstance(Args&&... args)
        {
            // 保证只调用一次
            std::call_once(oc, Singleton<T>::Init<Args...>, std::forward<Args>(args)...);
            return m_pInstance;
        }
};

template<typename T>
T* Singleton<T>::m_pInstance = nullptr;

template<typename T>
typename Singleton<T>::CGarbo Singleton<T>::Garbo;

template<typename T>
std::once_flag Singleton<T>::oc;

#endif
