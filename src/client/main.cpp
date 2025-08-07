#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <muduo/base/Logging.h>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <semaphore.h> // 信号量
#include <atomic> // 原子操作

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"


// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态是否成功
atomic_bool g_isLoginSuccess{false};


// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间(聊天信息需要添加时间信息)
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现， main线程用作发送线程， 子线程用作接收线程
int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        // 当./chatClient wrong_args > somefile.txt时，cout会写进文件，确保了即使用户试图将程序的正常输出重定向，这个错误提示依然会醒目地显示在终端上。
        cerr << "command error, example: ./chatClient 192.168.150.128 8080" << endl; 
        exit(-1); // 立即终止整个程序的执行，不同于return
    }
    // 解析通过命令行传入的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client 端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client 端要连接的server端的ip和port
    sockaddr_in server;
    memset(&server, 0, sizeof(server)); // memset函数将一块内存中的内容全部设置为指定的值【起始，0， size大小】

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client 端连接server端
    if(connect(clientfd, (sockaddr*)&server, sizeof(server)) == -1)
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功后，启动接收线程，专门接收服务器发送过来的消息
    thread readTask(readTaskHandler, clientfd); // pthread_create linux
    readTask.detach(); // pthread_detach

    // main 线程用作接收用户输入，发送数据
    for(;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "=================================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "=================================" << endl;
        cout << "please enter your choice: " ;

        int choice;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车，避免字符串读取到回车

        switch(choice)
        {
            case 1: // 登录
            {
                // TODO
                int id = 0;
                char pwd[50] = {0};
                cout << "please enter your id: " ;
                cin >> id;
                cin.get(); // 读掉缓冲区残留的回车，避免字符串读取到回车
                cout << "please enter your password: " ;
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                g_isLoginSuccess = false;

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                LOG_INFO << "send login msg: " << request;
                if(len == -1)
                {
                    cerr << "send login msg error" << endl;
                    close(clientfd);
                    exit(-1);
                }
                sem_wait(&rwsem); // 等待信号量，由子线程处理完登陆的响应后，通知这里

                if(g_isLoginSuccess)
                {
                    // 进入主聊天页面
                    isMainMenuRunning = true;
                    mainMenu(clientfd);

                }
            }
            break;
            case 2: // 注册
            {
                // TODO
                char name[50] = {0};
                char password[50] = {0};
                cout << "please enter your name: " ;
                cin.getline(name, 50); // cin.getline()函数读取一行字符串，直到遇到换行符为止，并将换行符替换为空字符；而cin>>遇到空格、非法字符会结束输入。
                cout << "please enter your password: " ;
                cin.getline(password, 50);

                json js;
                js["msgid"] = REG_MSG;   
                js["name"] = name;
                js["password"] = password;
                string request = js.dump(); // .dump() 的作用：把 JSON 对象转成字符串，parse 将字符串转成 JSON 对象
                // int len = send(clientfd, request.c_str(), request.size(), 0);
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len == -1)
                {
                    cerr << "send reg msg error" << endl;
                    close(clientfd);
                    exit(-1);
                }
                sem_wait(&rwsem); // 等待信号量，由子线程处理完注册的响应后，通知这里
            }
            break;
            case 3: // 退出
            {
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
                break;
            }
            default:
            {
                // TODO
                cout << "invalid input" << endl;
                break;
            }
        }
    }
}
// 处理登录响应
void doLoginResponse(json &responsejs)
{
    // json responsejs = json::parse(js);
    // LOG_INFO << "recv from server Login success: " <<responsejs.dump();
    if(responsejs["errno"].get<int>() != 0) // 登录失败
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else // 登录成功
    {
        
        // 记录当前登录的用户信息
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户好友列表信息
        if(responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for(auto &str : vec)
            {
                json js = json::parse(str);

                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户群组列表信息
        if(responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();
            vector<string> vec = responsejs["groups"];
            // LOG_INFO << "groups " << responsejs["groups"].dump();
            for(auto &str : vec)
            {
                json js = json::parse(str); // 群信息

                Group group;
                group.setId(js["id"].get<int>());
                group.setName(js["groupname"].get<string>());
                group.setDesc(js["groupdesc"].get<string>());

                vector<string> vec2 = js["users"];

                for(auto &str2 : vec2)
                {
                    json js2 = json::parse(str2); // 群成员信息
                    // LOG_INFO << "users" << js2.dump();
                    GroupUser user;
                    user.setId(js2["id"].get<int>());
                    user.setName(js2["name"]);
                    user.setState(js2["state"]);
                    user.setRole(js2["role"]);

                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        
        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线信息 个人聊天离线消息、群组离线消息
        if(responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for(string &str : vec)
            {
                json js = json::parse(str); // time + id + name + msg

                if(js["msgid"].get<int>() == ONE_CHAT_MSG) // 个人聊天离线消息
                {
                    cout << js["time"] << " [" << js["id"] << "] " << js["name"] << " said : " << js["msg"] << endl;
                }
                else if(js["msgid"].get<int>() == GROUP_CHAT_MSG) // 群组聊天离线消息
                {
                    cout << js["time"] << " 群消息 ["<< js["groupid"] << "]: " << " [" << js["id"] << "] " << js["name"] << " said : " << js["msg"] << endl;
                }
                
            }
        }
        g_isLoginSuccess = true;
    }
}
// 处理注册响应
void doRegResponse(json &response)
{
    if(response["errno"].get<int>() != 0) // 注册失败
    {
        cerr << "name already exist, register failed" << endl;
    }
    else // 注册成功
    {
        cout << "name register success, userid is " << response["id"] << " , do not forget it"<< endl;
    }
}
// 接收线程
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 不满足while条件，进入阻塞状态
        if(len == -1 || len == 0)
        {
            cerr << "recv server response error" << endl;
            close(clientfd);
            exit(-1);
        }
        
        // 接收ChatServer返回的json数据
        json js = json::parse(buffer);
        // LOG_INFO << "recv server response: " << js.dump();
        if(js["msgid"].get<int>() == ONE_CHAT_MSG) // 单聊消息
        {
            cout << js["time"] << " [" << js["id"] << "] " << js["name"] << " said : " << js["msg"] << endl;
            continue;
        }
        if(js["msgid"].get<int>() == GROUP_CHAT_MSG) // 群聊消息
        {
            cout << js["time"] << " 群消息 ["<< js["groupid"] << "]: " << " [" << js["id"] << "] " << js["name"] << " said : " << js["msg"] << endl;
            continue;
        }
        if(js["msgid"].get<int>() == LOGIN_MSG_ACK) // 登录成功
        {
            doLoginResponse(js); // 处理登录响应
            sem_post(&rwsem); // 通知主线程，登录结果处理完成
            continue;
        }
        if(js["msgid"].get<int>() == REG_MSG_ACK) // 注册成功
        {
            doRegResponse(js); // 处理注册响应
            sem_post(&rwsem); // 通知主线程，注册结果处理完成
            continue;
        }
        
    }
}
// "help" command handler
void help(int, string);
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "quit" command handler
void loginout(int, string);



// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令， 格式help"},
    {"chat", "一对一聊天，格式 chat:friendid:message"},
    {"addfriend", "添加好友，格式 addfriend:friendid"},
    {"creategroup", "创建群组，格式 creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式 addgroup:groupid"},
    {"groupchat", "群组聊天，格式 groupchat:groupid:message"},
    {"loginout", "退出客户端，格式 loginout"}
};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help(-1, "");
    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":");
        if(idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx); // substr(start, length)
        }

        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            cerr << "invalid input command" << endl;
            continue;
        }

        // 调用命令对应的处理函数，mainMenu对修改封闭，不需要关心命令具体如何实现，添加新功能不需要修改它
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx - 1));
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "=============================login user===============================" << endl;
    cout << "current login user => id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << endl;
    cout << "=============================frient list===============================" << endl;
    if(!g_currentUserFriendList.empty())
    {
        for (vector<User>::iterator it = g_currentUserFriendList.begin(); it != g_currentUserFriendList.end(); it++)
        {
            cout << "friend id: " << it->getId() << ", friend name: " << it->getName() << ", friend state: " << it->getState() << endl;
        }
    }
    cout << "=============================group list===============================" << endl;
    if(!g_currentUserGroupList.empty())
    {
        for(auto group : g_currentUserGroupList)
        {
            cout << "group id: " << group.getId() << ", group name: " << group.getName() << ", group desc: " << group.getDesc() << endl;
            for(GroupUser &user : group.getUsers())
            {
                cout << "group user id: " << user.getId() << ", group user name: " << user.getName() 
                << ", group user state: " << user.getState() << ", group user role: " << user.getRole()  << endl;
            }
        }
    }
    cout << "======================================================================" << endl;
}

// "help" command handler
void help(int fd=-1, string str="")
{
    cout << "show all command list" << endl;
    for (auto &it : commandMap)
    {
        cout << it.first << " : " << it.second << endl;
    } 
}
// "chat" command handler
void chat(int clientfd, string str) // chat:friendid:message
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr << "chat command invalid" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }

}
// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
        js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}
// "creategroup" command handler
void creategroup(int clientfd, string str) // creategroup:groupname:groupdesc
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr << "creategroup command invalid" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, string str) // addgroup:groupid
{
    int groupid = atoi(str.c_str());
    
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }

}
// "groupchat" command handler
void groupchat(int clientfd, string str) // groupchat:groupid:message
{
    int idx = str.find(":");
    if(idx == -1)
    {
        cerr << "groupchat command invalid" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(len == -1)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}
// "quit" command handler
void loginout(int clientfd, string="") // loginout
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    LOG_INFO << "send loginout msg -> " << buffer ;
    if(len == -1)
    {
        cerr << "send loginout msg error -> " << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
    
}
// 获取系统时间(聊天信息需要添加时间信息)
string getCurrentTime()
{
    auto tt = chrono::system_clock::to_time_t(chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char data[60] = {0};
    sprintf(data, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return string(data);
    
}