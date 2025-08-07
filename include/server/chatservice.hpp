#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include <muduo/net/TcpConnection.h>

#include <unordered_map>
#include <functional>
// #include "../thirdparty/json.hpp"
#include "json.hpp"
#include "usermodel.hpp"
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;
using json = nlohmann::json;

// 处理消息的事件回调函数类型
using MsgHandler = function<void(const TcpConnectionPtr& conn, json &js, Timestamp)>;

// 聊天服务器业务类, 要单例模式
class ChatService{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常，重置业务
    void reset();

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 从 redis 消息队列中获取订阅消息
    void handleRedisSubscribeMessage(int channel, string message);


private:
    
    ChatService();
    // 存储消息 id 和对应的业务处理函数
    unordered_map<int, MsgHandler> _msgHandlerMap; // 用户容器

    // 用户数据操作类对象
    UserModel _userModel;

    // 记录在线用户的通信连接，涉及线程安全
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap线程安全
    mutex _connMutex;

    // 离线消息表操作类对象
    OfflineMsgModel _offlineMsgModel;

    // 好友表操作类对象
    FriendModel _friendModel;

    // 群组表操作类对象
    GroupModel _groupModel;

    // redis操作对象
    Redis _redis;

};


#endif // CHATSERVICE_H