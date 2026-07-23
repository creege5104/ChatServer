#ifndef GROUP_USER_H
#define GROUP_USER_H
#include "user.hpp"
using namespace std;

class GroupUser : public User
{
public:
    void setRole(string role){_role = role;}
    string getRole(){return _role;}
private:
    string _role;
};

#endif