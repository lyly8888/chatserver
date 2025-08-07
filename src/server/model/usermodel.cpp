#include "usermodel.hpp"
#include "db.h"
#include <iostream>
#include <string>
using namespace std;


// User 表的增加方法
bool UserModel::insert(User& user)
{
     // 1. 组装 sql 语句
    char sql[1024] = {0};
    // sprintf 函数将格式化的数据写入字符数组（字符串）中
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')", user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 2. 执行 sql 语句, 【获取连接， 执行 sql 语句， 出作用域自动close，析构函数】
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入数据的自增 ID
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true; // 插入成功
        }
    }
    return false; // 插入失败
}

// User 表的查询方法：根据用户id查询用户信息
User UserModel::query(int id)
{
    // 1. 组装 sql 语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    // 2. 执行 sql 语句, 【获取连接， 执行 sql 语句， 出作用域自动close，析构函数】
    MySQL mysql;
    if (mysql.connect())
    {
        // 查询数据，用主键查，主键不允许重复，所以查到的数据只有一条
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr)
        {
            // 获取数据 fetch 获取
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                // 将查询到的数据封装到 user 对象中
                User user;
                user.setId(atoi(row[0])); // atoi 函数将字符串转换为 int
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res); // 释放资源
                return user;
            }
        }
    }
    return User(); // 查询失败，返回一个空的 user 对象
}
// User 表的更新方法：根据用户id更新用户信息
bool UserModel::updateState(User& user)
{
    // 1. 组装 sql 语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    // 2. 执行 sql 语句, 【获取连接， 执行 sql 语句， 出作用域自动close，析构函数】
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true; // 更新成功
        }
    }
    return false; // 更新失败
}

// 重置用户的状态信息
void UserModel::resetState()
{
    // 1. 组装 sql 语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = 'offline' where state = 'online'");

    // 2. 执行 sql 语句, 【获取连接， 执行 sql 语句， 出作用域自动close，析构函数】
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return ; // 更新成功
        }
    }
    return ; // 更新失败
}