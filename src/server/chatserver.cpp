#include "chatserver.hpp"
#include <functional>
#include <iostream>
#include <string>
#include "json.hpp"
#include "chatservice.hpp"
#include <muduo/base/Logging.h>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, 
                const InetAddress& listenAddr, 
                const string& nameArg)
                : _server(loop, listenAddr, nameArg)
                , _loop(loop)
{
    // 注册连接、断开回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 注册读、写回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置线程数量
    _server.setThreadNum(4);

}

// 启动服务器
void ChatServer::start()
{
    _server.start();
}

// 当有新的客户端连接、断开时，会调用这个回调函数【上报连接相关信息的回调函数】
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    if(!conn->connected())
    {
        // 客户端断开连接
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown(); // 关闭连接， 释放socket 资源
    }
    else
    {
        // 用户连接成功
    }


}
// 当有新的客户端读、写消息时，会调用这个回调函数【上报读写相关信息的回调函数】
void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 数据反序列化，解析客户端请求
    json js = json::parse(buf);
    // 达到的目的：完全解耦 ，将业务处理与网络通信分离
    // 根据客户端请求 js["msgid"]，获取业务处理器，conne js time
    LOG_INFO << " server recv js[msgid]: " << js["msgid"].get<int>();
    auto msgHandler =  ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，执行相应的业务处理
    msgHandler(conn, js, time);


}