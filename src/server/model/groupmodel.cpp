#include "groupmodel.hpp"
#include "db.h"
#include "group.hpp"
#include <muduo/base/Logging.h>

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 组装 sql 语句
    char sql[1024] = { 0 };
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    // 执行 sql 语句
    MySQL mysql;
    if (mysql.connect()) 
    {
        if (mysql.update(sql)) 
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 组装 sql 语句
    char sql[1024] = { 0 };
    sprintf(sql, "insert into groupuser(groupid, userid, grouprole) values(%d, %d, '%s')", groupid, userid, role.c_str());

    // 执行 sql 语句
    MySQL mysql;
    if (mysql.connect()) 
    {
        mysql.update(sql);
    }
    return ;
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    /*
    1.先根据userid在groupuser表中查询出该用户所属的群组信息
    2.根据群组信息，查询属于该群组的所有用户的userid，并和user表进行多表联合查询，查出用户的详细信息
    */
    char sql[1024] = { 0 };
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid=%d", userid);

    vector<Group> groupVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res))
            {
                // Group group(atoi(row[0]), row[1], row[2]);
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    // 查询群组中所有的成员信息
    for (Group &group : groupVec)
    {
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a \
        inner join groupuser b on a.id = b.userid where b.groupid=%d", group.getId());
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res))
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                // LOG_INFO << "Group id: " << group.getId() << row[0] << " " << row[1] << " " << row[2] << " " << row[3] << " " << user.getRole();
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
        // LOG_INFO << "Group id: " << group.getId() << " user count: " << group.getUsers().size();
    }
    return groupVec;
}

// 根据指定groupId查询群组用户id列表， 不包括userId自己， 主要用于群聊业务给群组中其他成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = { 0 }; 
    sprintf(sql, "select userid from groupuser where groupid=%d and userid != %d", groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res))
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}