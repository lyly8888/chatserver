#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// User 表的数据操作类
// 定义一个UserModel类
class UserModel{
public:

    // User 表的增加方法
    bool insert(User& user);

    // User 表的查询方法：根据用户id查询用户信息
    User query(int id);

    // User 表的更新方法：根据用户id更新用户信息
    bool updateState(User& user);

    // 重置用户的状态信息
    void resetState();

private:

};


#endif