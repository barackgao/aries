//
// Created by 高炜 on 16/12/9.
//

#include "../headFiles/machineLink.h"

machineLink::machineLink(int srcNode,
                         int srcPort,
                         int dstNode,
                         int dstPort)
        :memberSrcNode(srcNode),
         memberSrcPort(srcPort),
         memberDstNode(dstNode),
         memberDstPort(dstPort) {
}