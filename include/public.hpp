#ifndef PUBLIC_H
#define PUBLIC_H
/*
server 和 client 的公共文件
*/

enum EnMsgType{
    LOGIN_MSG = 1, // 登录消息
    LOGIN_MSG_ACK = 2, // 登录消息确认【成功/失败】
    REG_MSG,   // 注册消息
    REG_MSG_ACK,   // 注册消息确认【成功/失败】
    ONE_CHAT_MSG, // 一对一聊天消息
    ADD_FRIEND_MSG, // 添加好友消息

    CREATE_GROUP_MSG, // 创建群组
    ADD_GROUP_MSG, // 加入群组
    GROUP_CHAT_MSG, // 群聊天

    LOGINOUT_MSG, // 注销
};

#endif // PUBLIC_H