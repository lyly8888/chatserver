#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>

using namespace std;


// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接, 不是连接，而是获取连接的句柄/资源
    MySQL();
    // 释放数据库连接资源
    ~MySQL();
    // 连接数据库
    bool connect();
    // 更新操作（包括增删改）
    bool update(string sql);
    // 查询操作
    MYSQL_RES* query(string sql);
    // 获取连接句柄
    MYSQL* getConnection();
private:
    MYSQL *_conn;
};




#endif // DB_H