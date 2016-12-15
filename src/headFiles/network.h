//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_NETWORK_H
#define ARIES_NETWORK_H

#include <ifaddrs.h>
//#include <netinet/in.h>
#include <arpa/inet.h>

#include "../../thirdParty/usr/include/gflags/gflags.h"
#include "../../thirdParty/usr/include/gflags/gflags_declare.h"
#include "../../thirdParty/usr/include/zmq.h"
#include "../../thirdParty/usr/include/zmq.hpp"
#include "../../thirdParty/usr/include/mpi.h"
#include "../../thirdParty/usr/include/glog/logging.h"

#include "machineNode.h"
#include "machineLink.h"
#include "struct.h"
#include "systemParameter.h"
#include "assistFunction.h"
#include "context.h"

#define ARIES_ZMQ_IO_THREADS 2
#define MAX_ZMQ_HWM (5000)
#define MAX_BUFFER (1024*16)
#define MAX_MACH (128)
#define RDATAPORT 0
#define RACKPORT 1

class network {
public:
    //TODO: 成员变量
    systemParameter *memberSystemParameter;

    std::map<int, machineNode *> memberMachineNodes; // machine nodes
    std::map<int, machineLink *> memberMachineStarLinks; // node graph links
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
    std::map<int, context *> memberSchedulerSendPortMap; // coordinator
    std::map<int, context *> memberSchedulerRecvPortMap; // coordinator

    // star topology only: ring topology does not use this map
    std::map<int, context *> memberWorkerSendPortMap; // coordinator
    std::map<int, context *> memberWorkerRecvPortMap; // coordinator

    //worker or scheduler
    std::map<uint16_t, context *> memberStarRecvPortMap;
    std::map<uint16_t, context *> memberStarSendPortMap;

    //TODO: ring网络拓扑存储结构
    std::map<int, context *> memberRingSendPortMap; // coordinator
    std::map<int, context *> memberRingRecvPortMap; // coordinator

    //TODO: ps网络拓扑存储结构
    std::map<int, context *> memberPsSendPortMap; // ps client/server
    std::map<int, context *> memberPsRecvPortMap; // ps client/server

    //TODO: 构造器
    network();

//    network(int argc, char *argv[]);

    //TODO: 成员函数
    int init(int argc, char *argv[]);


    void findRole();

    machineRole findRole(int nodeID);


    void utilFindValidIP(std::string validIP);

    void getIPList(std::vector<std::string> &ipList);


    void parseNodeFile(std::string &fileName);

    void parsePsNodeFile(std::string &fileName);

    void parsePsLinkFile(std::string &fileName);

    void parseStarLinkFile(std::string &fileName);


    void createStarEthernet(zmq::context_t &contextZmq, std::string &cip);

    void createRingWorkerEthernetAux(zmq::context_t &contextZmq, std::string &cip);

    void createPsStarEthernet(zmq::context_t &contextZmq, int mpiSize, std::string &cip, int serverRank);


    void coordinatorRingWakeUpAux(int dackPort, int rank);

    void workerRingWakeUpAux(int dackPort, int rank);


    int getIDMessage(zmq::socket_t &zport, int rank, int *identity, char *message, int length);

    int getSingleMessage(zmq::socket_t &zport, int rank, char *message, int length);

    bool cppsSendMore(zmq::socket_t &zport, void *message, int length);

    bool cppsSend(zmq::socket_t &zport, void *message, int length);
};

#endif //ARIES_NETWORK_H
