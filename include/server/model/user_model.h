#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
#include "db.h"
class UserModel
{
public:
    bool insert(User &user);
    User query(int id);
    bool updateState(User &user);
    bool delete_(int id);
    void resetState(); //重置用户的状态信息

private:
};

#endif