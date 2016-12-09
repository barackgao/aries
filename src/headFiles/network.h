//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_NETWORK_H
#define ARIES_NETWORK_H

#include "../../thirdParty/usr/include/gflags/gflags.h"
#include "../../thirdParty/usr/include/zmq.hpp"
#include "../../thirdParty/usr/include/mpi.h"
#include "../../thirdParty/usr/include/glog/logging.h"

#include "machineNode.h"
#include "machineLink.h"
#include "struct.h"
#include "systemParameter.h"

class network{
public:
    //TODO: 成员变量
    systemParameter* memberSystemParameter;

    std::map<int, machineNode *> memberMachineNodes; // machine nodes
    std::map<int, machineLink *> memberMachineLinks; // node graph links
//    std::map<int, machineLink *> memberMachineRingLinks; // node graph links

    std::map<int, machineNode *> memberPsNodes; // machine nodes for ps configuration
    std::map<int, machineLink *> memberPsLinks; // link for ps configuration

    int memberMpiRank;                     // machine id
    machineRole memberMachineRole;

    /*
    int memberSchedulerMachineID; // scheduler id from 0 to sizeof(schedulers)-1
    int memberWorkerMachineID;    // worker id from 0 to sizeof(workers)-1
    int memberCoordinatorMachineID; // coordinator id from 0 to sizeof(coordinator)-1
    */

    int memberSchedulerMachineNumber;
    int memberFirstSchedulerMachineID;
    int memberWorkerMachineNumber;
    int memberFirstWorkerMachineID;
    int memberFirstCoordinatorMachineID; // first one and last one since only one coordinator now
    int memberMpiSize;

    zmq::context_t *memberZmqContext;

    //TODO: star网络拓扑存储结构
    // rank number - ringport map
    std::map<int, _ringport *> schedulerSendPortMap; // coordinator
    std::map<int, _ringport *> schedulerRecvPortMap; // coordinator

    // star topology only: ring topology does not use this map
    std::map<int, _ringport *> workerSendPortMap; // coordinator
    std::map<int, _ringport *> workerRecvPortMap; // coordinator

    //worker or scheduler
    std::map<uint16_t, _ringport *> starRecvPortMap;
    std::map<uint16_t, _ringport *> starSendPortMap;

    //TODO: ring网络拓扑存储结构
    std::map<int, _ringport *> ringSendPortMap; // coordinator
    std::map<int, _ringport *> ringRecvPortMap; // coordinator

    //TODO: ps网络拓扑存储结构
    std::map<int, _ringport *> psSendPortMap; // ps client/server
    std::map<int, _ringport *> psRecvPortMap; // ps client/server

    //TODO: 构造器
    network();
    network(int argc, char* argv[]);

    //TODO: 成员函数
    int init(int argc, char* argv[]);
    int findRole();
};

#endif //ARIES_NETWORK_H
