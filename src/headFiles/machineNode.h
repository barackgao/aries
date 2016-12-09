//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_MACHINENODE_H
#define ARIES_MACHINENODE_H

#include <iostream>

class machineNode {
public:
    //TODO: 成员变量
    std::string memberIP;
    int memberID;
//    void *(*function)(void *);
//    std::string functionName;

    //TODO: 构造器
    machineNode();
    machineNode(std::string IP, int ID);
};

#endif //ARIES_MACHINENODE_H
