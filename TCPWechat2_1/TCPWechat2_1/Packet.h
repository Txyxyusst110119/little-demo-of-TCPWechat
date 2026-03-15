#ifndef PACKET_H
#define PACKET_H

#include <QByteArray>
#include <cstdint>
#include <QDataStream>   // 序列化必须的头文件
#include <QIODevice>     // 补充IO设备头文件
#include "CRC32.h"       // 关联校验类

// 第一步：先定义数据包类型枚举（全局可见，服务器/客户端都要用到）
enum PacketType {
    LOGIN_REQUEST = 1,        // 客户端登录请求（数据体：用户名|密码）
    LOGIN_RESPONSE,           // 服务器登录响应（数据体："success"/"fail:原因"）
    CHAT_MESSAGE,             // 群聊消息（数据体："发送者|消息内容"）
    PRIVATE_CHAT_MESSAGE,     // 新增：私聊消息（数据体："发送者|接收者|消息内容"）
    USER_ONLINE,              // 新用户上线（数据体：上线用户名）
    USER_OFFLINE,             // 用户下线（数据体：下线用户名）
    USER_LIST,                 // 在线用户列表

    PRIVATE_CHAT_REQUEST,  // 私聊请求（数据体：发起者|接收者）
    PRIVATE_CHAT_RESPONSE,  // 私聊响应（数据体：接收者|发起者|agree/refuse:原因）

    IMAGE_TRANSFER = 10, // 图片传输（数据体：发送者|接收者|文件名|文件大小|图片二进制数据）
    IMAGE_RECEIVE_NOTIFY // 图片接收通知（可选，用于提示用户）

};

// 第二步：扩展后的Packet结构体（包含类型字段）
struct Packet {
    static const uint32_t MAGIC_NUM = 0x12345678;

    PacketType type;
    QByteArray data;

    QByteArray pack() const;
    static bool unpack(QByteArray& buffer, Packet& outPacket);
};


#endif // PACKET_H