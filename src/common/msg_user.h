#ifndef __COMMON_MSG_USER_H__
#define __COMMON_MSG_USER_H__

#include "service.h"

class MsgUser : public std::enable_shared_from_this<MsgUser>
{
    public:
        MsgUser();
        virtual ~MsgUser() = default;
        MsgUser(const MsgUser&) = delete;
        MsgUser& operator= (const MsgUser&) = delete;

    public:
        void        InitUser(Service* service, uint64_t uid, uint32_t handle);
        uint64_t    uid() const { return uid_; }
        uint32_t    GetHandle() const { return handle_; }
        void        ClearHandle() { handle_ = 0; }
        bool        IsInit() const { return is_init_ok_; }
        void        SetInit(bool init) { is_init_ok_ = init; }
        void        SetIP(const std::string& ip) { real_ip_ = ip; }
        void        SendToUser(const Message& msg);
        const std::string& GetIP() const { return real_ip_; }

    public:
        virtual void VLoadUserInfo() = 0;

    private:
        bool        CheckLoadUserInfo();

    protected:
        Service*    service_;

    private:
        uint64_t    uid_;
        uint32_t    handle_;   
        std::string real_ip_;
        bool        is_init_ok_;      // 是否已初始化完成
        int         loading_userinfo_interval_;    

        static const int kLoadingUserInfoInterval = 3;
};

#endif
