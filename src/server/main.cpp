#include "chatserver.hpp"
#include <iostream>
#include <signal.h>
#include "chatservice.hpp"
using namespace std;

// 处理服务器 CTRL+C 结束后，重置用户状态
void resetHandler(int sig)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    
    server.start(); // 启动服务
    loop.loop(); // 进入事件循环

    return 0;
}
/*
{"msgid":2}
telnet 192.168.150.128 8080
./ChatClient 192.168.150.128 8080
./ChatClient 127.0.0.1 6000
{"msgid":1, "id":22, "password":"123456"} // 登录
{"msgid":3, "name":"li si", "password":"666666"} // 注册
{"msgid":1, "id":23, "password":"666666"}  // 登录
{"msgid":5, "id":22, "from":"zhangsan", "toid":23, "msg":"hello world"} // 发送消息
{"msgid":6, "id":22, "friendid":23} // 添加好友
{"msgid":5, "id":23, "from":"li si", "toid":22, "msg":"挺好的"}

{"msgid":5, "id":22, "from":"zhangsan", "toid":23, "msg":hello world}

select a.id, a.name, a.state from user a inner join friend b on a.id = b.friendid where b.userid=%d
select a.id, a.groupname, a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid=%d

*/