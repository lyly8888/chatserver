#ifndef FRIIENDMODEL_H
#define FRIENDMODEL_H

#include <string>
#include <vector>
#include "user.hpp"
using namespace std;

// 维护好友信息的操作接口方法
class FriendModel{

public:
    //添加好友关系
    void insert(int userid, int friendid);


    //返回用户好友列表 friendid, friendname, state
    vector<User> query(int userid);

private:

};


#endif