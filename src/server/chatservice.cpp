#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <map>
using namespace std;
using namespace placeholders;
using namespace muduo;


// 获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}
// 注册消息以及对应的回调函数
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    // 连接redis 服务器
    if(_redis.connect())
    {
        // 设置回调，当redis服务器有消息推送时，调用回调函数
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }


}
// 服务器异常，重置业务
void ChatService::reset()
{
    // 将所有的用户状态设置为离线
    _userModel.resetState();
}
// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志， msgid 没有对应处理器
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        
        // string errstr = "msgid: " + to_string(msgid) + " can not find handler!";
        // 返回一个默认的处理器, 空操作
        return [=](auto a, auto b, auto c){ // [=] 按值捕获
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
    
}


// 处理登录业务 id password
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do login service!";
    int id = js["id"].get<int>(); // js["id"] 返回的是 字符串 类型，需要转换成 int
    string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId() != -1 && user.getPwd() == pwd)
    {
        if(user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this user already login! 请重新输入新账号";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            { // 就保证这点加锁就行，不需要加锁整个函数
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id 登录成功，向 redis 订阅 通道
            _redis.subscribe(id); // 订阅该id的消息

            // 登录成功，更新用户状态信息 offline -> online
            user.setState("online");
            _userModel.updateState(user);
            // 返回登录成功信息
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 清空离线消息
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友列表,并返回给相应用户
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec2;
                for(auto &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询该用户所在群组信息，并返回给相应用户
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty())
            {
                vector<string> groupV;
                for(Group &group : groupuserVec)
                {
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();

                    vector<string> userV;
                    for(GroupUser &user : group.getUsers())
                    {
                        json jsUser;
                        jsUser["id"] = user.getId();
                        jsUser["name"] = user.getName();
                        jsUser["state"] = user.getState();
                        jsUser["role"] = user.getRole();
                        userV.push_back(jsUser.dump());
                    }
                    groupjs["users"] = userV;
                    groupV.push_back(groupjs.dump());
                }
                response["groups"] = groupV;
            }
            LOG_INFO << "Login response json: " << response.dump();
            // .dump() 的作用：把 JSON 对象转成字符串，parse 将字符串转成 JSON 对象
            conn->send(response.dump());
        }
    }
    else
    {
        // 登录失败： 用户名不存在或者密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password error!";
        // .dump() 的作用：把 JSON 对象转成字符串，parse 将字符串转成 JSON 对象
        conn->send(response.dump());
        
    }
}

// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "do reg service!";
    // 1. 解析客户端json数据
    // 2. 组成用户对象
    // 3. 注册新用户
    // 4. 注册成功, 返回客户端及相应消息，注册失败，返回客户端及相应消息
    string name = js["name"];
    string password = js["password"];
    User user;
    user.setName(name);
    user.setPwd(password);
    bool state =  _userModel.insert(user); // 注册用户
    if(state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        // .dump() 的作用：把 JSON 对象转成字符串，parse 将字符串转成 JSON 对象
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        // .dump() 的作用：把 JSON 对象转成字符串，parse 将字符串转成 JSON 对象
        conn->send(response.dump());
    }
}
// 处理客户端注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do loginout service!";
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            // 从map中删除用户连接信息
            _userConnMap.erase(it);
        }
    }

    // 在redis中取消订阅频道
    _redis.unsubscribe(userid);

    // 更新用户状态信息
    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updateState(user); // 更新用户状态信息
    
}
// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex); // 加锁，不要影响其他连接的操作

        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if(it->second == conn)
            {
                user.setId(it->first);
                // 从map中删除用户连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 在redis中取消订阅频道
    _redis.unsubscribe(user.getId());

    // 更新用户状态信息
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user); // 更新用户状态信息
    }
}
// 处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            // toid 在线，服务器主动推送消息给toid，不能放在锁外面，否则如果发送的时候，用户突然不在线了，就会出错
            it->second->send(js.dump());
            return;
        }
    }
    // 查询toid是否在线，如果在线，直接推送消息, 在其他服务器上
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
    }

    // toid 不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());


}
// 添加好友业务 msgid  id  friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 存储好友信息
    _friendModel.insert(userid, friendid);
    _friendModel.insert(friendid, userid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    Group group;
    group.setName(name);
    group.setDesc(desc);
    if(_groupModel.createGroup(group))
    {
        // 创建群组成功，将群组创建者加入到群组中
        _groupModel.addGroup(userid, group.getId(), "creator");

        // json response;
        // response["msgid"] = CREATE_GROUP_MSG;
        // response["errno"] = 0;
        // response["errmsg"] = "create group success!";
        // // .dump() 的作用：把 JSON 对象转成字符串，parse 将字符串转成 JSON 对象
        // conn->send(response.dump());
    }
    else
    {
        // json response;
        // response["msgid"] = CREATE_GROUP_MSG;
        // response["errno"] = 1;
        // response["errmsg"] = "create group failed!";
        // conn->send(response.dump());
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
    json response;
    response["msgid"] = ADD_GROUP_MSG;
    response["errno"] = 0;
    response["errmsg"] = "add group success!";
    conn->send(response.dump());
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for(auto id : useridVec)
    {
        // LOG_INFO << "grout chat id = " << id;
        // lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if(it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询是否在线，如果在线，直接推送消息, 在其他服务器上
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线消息
                _offlineMsgModel.insert(id, js.dump());
            }
            
        }
    }

    // json response;
    // response["msgid"] = GROUP_CHAT_MSG;
    // response["errno"] = 0;
    // response["errmsg"] = "group chat success!";
    // conn->send(response.dump());

}

// 从 redis 消息队列中获取订阅消息, 别的服务器推送过来的消息 或者 自己服务器推送过来的消息，
// 但是大概率不是自己服务器推送过来的消息，因为对于消息要先查看_userConnMap，如果自己服务器推送过来的消息，那么一定在_userConnMap中
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    // json js = json::parse(msg); 可以不用反序列化，因为还要发送

    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储离线消息
    _offlineMsgModel.insert(userid, msg);
}
