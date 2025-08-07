#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;
using namespace std;

// 聊天服务器类
class ChatServer{
public:
    // 初始化服务器对象
    ChatServer(EventLoop* loop, 
                const InetAddress& listenAddr, 
                const string& nameArg);
    // 启动服务器
    void start();
    


private:
    // 当有新的客户端连接、断开时，会调用这个回调函数【上报连接相关信息的回调函数】
    void onConnection(const TcpConnectionPtr& conn);
    // 当有新的客户端读、写消息时，会调用这个回调函数【上报读写相关信息的回调函数】
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

    TcpServer _server; // 组合muduo库，实现服务器功能的类对象
    EventLoop* _loop; //指向事件循环的指针
};




#endif // CHATSERVER_H