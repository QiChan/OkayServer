#ifndef __COMMON_SOCKET_TYPE_H__
#define __COMMON_SOCKET_TYPE_H__

#include <cstdint>

enum class SocketType : uint8_t
{
    TCP_SOCKET = 1,
    ROBOT_SOCKET = 2,
    NULL_SOCKET
};

#endif 
