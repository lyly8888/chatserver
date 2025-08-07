#ifndef USER_H
#define USER_H

#include <string>
using namespace std;


// 匹配 User 表的 ORM 类
class User{
public:
    User(int id=-1, string name="", string password="", string state="offline")
    {
        this->id = id;
        this->name = name;
        this->password = password;
        this->state = state;
    }
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string password) { this->password = password; }
    void setState(string state) { this->state = state; }



    int getId() const { return this->id; }
    string getName() const { return name; }
    string getPwd() const { return password; }
    string getState() const { return state; }



protected:
    int id;
    string name;
    string password;
    string state;

};



#endif // USER_H