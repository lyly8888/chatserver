#include "friendmodel.hpp"
#include <muduo/base/Logging.h>
#include "db.h"
#include <vector>
using namespace std;

//添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 组装 sql 语句
    char sql[1024] = { 0 };
    sprintf(sql, "insert into friend(userid, friendid) values(%d, %d)", userid, friendid);

    // 执行 sql 语句
    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return ;
        }  
    }

}


//返回用户好友列表 friendid, friendname, state
vector<User> FriendModel::query(int userid)
{
    // 1. 组装 sql 语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on a.id = b.friendid where b.userid=%d", userid);

    // 2. 执行 sql 语句, 【获取连接， 执行 sql 语句， 出作用域自动close，析构函数】
    MySQL mysql;
    vector<User> vec;
    if (mysql.connect())
    {
        // 查询数据，用主键查，主键不允许重复，所以查到的数据只有一条
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            // 获取数据 fetch 获取
            MYSQL_ROW row = mysql_fetch_row(res);
            // 把 uiserid 用户的所有离线消息存储到 vector 中
            while (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
                
                row = mysql_fetch_row(res); // 获取下一行
            }
            mysql_free_result(res); // 释放res
            return vec;
        }
    }
    return vec; // 查询失败，返回一个空的 vector
}