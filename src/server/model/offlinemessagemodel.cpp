#include "offlinemessagemodel.hpp"
#include "db.h"
#include <muduo/base/Logging.h>

using namespace std;


//存储用户的离线消息
bool OfflineMsgModel::insert(int userid, string msg)
{
    // 组装 sql 语句
    char sql[1024] = { 0 };
    sprintf(sql, "insert into offlinemessage(userid, message) values(%d, '%s')", userid, msg.c_str());

    // 执行 sql 语句
    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            LOG_INFO << "insert offlinemessage success";
            return true;
        }
        else
        {
            LOG_INFO << "insert offlinemessage failed";
        }
    }
    else
    {
        LOG_INFO << "insert offlinemessage connect db failed";
    }
}

//删除用户的所有离线消息
bool OfflineMsgModel::remove(int userid)
{
    // 组装 sql 语句
    char sql[1024] = { 0 };
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);

    // 执行 sql 语句
    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return true;
        }
    }
}

//查询用户的所有离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    // 1. 组装 sql 语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    // 2. 执行 sql 语句, 【获取连接， 执行 sql 语句， 出作用域自动close，析构函数】
    MySQL mysql;
    vector<string> vec;
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
                vec.push_back(row[0]);
                row = mysql_fetch_row(res); // 获取下一行
            }
            mysql_free_result(res); // 释放res
            return vec;
        }
    }
    return vec; // 查询失败，返回一个空的 vector

}