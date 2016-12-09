//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_MACHINELINK_H
#define ARIES_MACHINELINK_H

class machineLink{
public:
    //TODO: 成员变量
    //int memberID;
    int memberSrcNode;
    int memberSrcPort;
    int memberDstNode;
    int memberDstPort;

    //TODO: 构造器
    machineLink(int srcNode,
                int srcPort,
                int dstNode,
                int dstPort)
            :memberSrcNode(srcNode),
             memberSrcPort(srcPort),
             memberDstNode(dstNode),
             memberDstPort(dstPort){}
};
#endif //ARIES_MACHINELINK_H
