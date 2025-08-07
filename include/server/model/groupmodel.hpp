#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);

    // 查询用户所在群组信息
    vector<Group> queryGroups(int userid);

    // 根据指定groupId查询群组用户id列表， 不包括userId自己， 主要用于群聊业务给群组中其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);
    

};

#endif