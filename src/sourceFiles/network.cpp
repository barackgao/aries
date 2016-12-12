//
// Created by 高炜 on 16/12/9.
//

#include "../headFiles/network.h"

network::network() {}

int network::init(int argc, char **argv) {
    //TODO: 读取命令行参数
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    //TODO: 初始化glog日志环境
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    //TODO: 初始化mpi
    int mpiRank, mpiSize;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    this->memberMpiRank = mpiRank;
    this->memberMpiSize = mpiSize;
    this->memberSchedulerMachineNumber = FLAGS_schedulerMachineNumber;

    //TODO: 定义角色
    this->findRole();

    //TODO: 初始化系统参数和通信句柄
    this->memberSystemParameter = new systemParameter();
    this->memberSystemParameter->init(argc, argv, this->memberMpiRank, this->memberMpiSize);

    //TODO: 读入结点信息
    parseNodeFile(memberSystemParameter->memberNodeFileName);

    // TODO: 比较网卡IP与配置文件中的IP是否一致，返回一致IP
    std::string validIP;
    utilFindValidIP(validIP);
    LOG(INFO) << "Rank (" << mpiRank << ")'s IP found in node file " << validIP << std::endl;

    //TODO: 读入ps结点，ps链接信息
    if (memberSystemParameter->memberPsLinkFileName.size() > 0 and
        memberSystemParameter->memberPsNodeFileName.size() > 0) {
        parsePsNodeFile(memberSystemParameter->memberPsNodeFileName);
        parsePsLinkFile(memberSystemParameter->memberPsLinkFileName);
        LOG(INFO) << "[*****] ps configuration files(node/link) are parsed" << std::endl;
    } else {
        LOG(INFO) << "No configuration !!!! " << std::endl;
    }

    //TODO: zmq句柄配置
    int ioThreads = ARIES_ZMQ_IO_THREADS;
    zmq::context_t *contextzmq = new zmq::context_t(ioThreads);
    this->memberZmqContext = contextzmq;

    //TODO: 读入网络结构并初始化
    if (memberSystemParameter->memberTopology.compare("star") == 0) {
        //TODO: 读入星型网络结构
        parseStarLinkFile(memberSystemParameter->memberStarLinkFileName);
        //TODO: 创建星型网络
        createStarEthernet(*contextzmq, memberMpiSize, memberMachineNodes[memberMpiSize - 1]->memberIP);
        //TODO: 读入环型网络结构
        parseStarLinkFile(memberSystemParameter->memberRingLinkFileName);
        //TODO: 创建环型网络结构
        createRingWorkerEthernetAux(*contextzmq, mpiSize, memberMachineNodes[memberMpiSize - 1]->memberIP);
        LOG(INFO) << "Star Topology is cretaed with " << mpiSize << " machines (processes) " << std::endl;

    } else {
        LOG(INFO) << "Ring Topology is being created" << std::endl;
        parseStarLinkFile(memberSystemParameter->memberRingLinkFileName);
//        create_ring_ethernet(pshctx, *contextzmq, mpi_size, pshctx->nodes[mpi_size-1]->ip);
    }

    //TODO 创建PS网络结构
    if (memberSystemParameter->memberPsLinkFileName.size() > 0 and
        memberSystemParameter->memberPsNodeFileName.size() > 0) {
        for (int i = 0; i < memberSchedulerMachineNumber; i++) {
            if (memberMpiRank == memberFirstSchedulerMachineID + i or
                memberMachineRole == machineRoleWorker) {
                createPsStarEthernet(*contextzmq, mpiSize,
                                     memberMachineNodes[memberFirstSchedulerMachineID + i]->memberIP,
                                     memberFirstSchedulerMachineID + i);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    LOG(INFO) << " EXIT : in stards_init function MPI RANK :  " << mpiRank << std::endl;
    MPI_Finalize();
}

int network::findRole() {
    machineRole mrole = machineRoleUnknown;

    memberSchedulerMachineNumber = memberSystemParameter->memberSchedulerNumber;
    memberFirstSchedulerMachineID = memberMpiSize - memberSchedulerMachineNumber - 1;
    memberWorkerMachineNumber = memberFirstSchedulerMachineID;
    memberFirstWorkerMachineID = 0;
    memberFirstCoordinatorMachineID = memberMpiSize - 1;

    if (memberMpiRank == memberFirstCoordinatorMachineID) {
        mrole = machineRoleCoordinator;
    } else if (memberMpiRank < memberFirstSchedulerMachineID) {
        mrole = machineRoleWorker;
    } else if (memberMpiRank >= memberFirstSchedulerMachineID && memberMpiRank < memberFirstCoordinatorMachineID) {
        mrole = machineRoleScheduler;
    }

    memberMachineRole = mrole;

    return mrole;

}

machineRole network::findRole(int nodeID) {
    machineRole mrole = machineRoleUnknown;

    if(nodeID == memberFirstCoordinatorMachineID){
        mrole = machineRoleCoordinator;
    }else if(nodeID < memberFirstSchedulerMachineID){
        mrole = machineRoleWorker;
    }else if(nodeID >= memberFirstSchedulerMachineID && nodeID < memberFirstCoordinatorMachineID){
        mrole = machineRoleScheduler;
    }

    return mrole;
}

void network::utilFindValidIP(std::string validIP) {
    std::vector<std::string>ipList;
    getIPList(ipList); // get all local abailable ip address
    // try to find matching IP with any IP in the user input.
    // Assumption : user provide correct IP addresses
    // TODO: add more user-error proof code
    for(auto const &p : ipList){
        for(auto const np : this->memberMachineNodes){
            if(!np.second->memberIP.compare(p)){
                validIP.append(p);
                return;
            }
        }
    }
    exit(0);
    return;
}

// TODO: 获得本机网卡的所有可用IP地址
void network::getIPList(std::vector<std::string> &ipList) {
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;
    char *addressBuffer = NULL;
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            //      char addressBuffer[INET_ADDRSTRLEN];
            addressBuffer = (char *) calloc(INET_ADDRSTRLEN + 1, sizeof(char));
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //printf("'%s': %s\n", ifa->ifa_name, addressBuffer);
            ipList.push_back(*(new std::string(addressBuffer)));
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
            //      char addressBuffer[INET6_ADDRSTRLEN];
            addressBuffer = (char *) calloc(INET6_ADDRSTRLEN + 1, sizeof(char));
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //printf("'%s': %s\n", ifa->ifa_name, addressBuffer);
            ipList.push_back(*(new std::string(addressBuffer)));
        }
    }
    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct);//remember to free ifAddrStruct
    return;
}

void network::parseNodeFile(std::string &fileName){
    std::ifstream in(fileName);
    std::string delim(CONF_FILE_DELIMITER);
    std::string linebuffer;
    while(getline(in, linebuffer)){
        std::vector<std::string> parts;
        utilGetTokens(linebuffer, delim, parts);
        if(!parts[0].compare("#"))  // skip comment line
            continue;
        machineNode *tmp = new machineNode;
        tmp->memberIP.append(parts[0]);
        tmp->memberID = atoi(parts[1].c_str());
//        tmp->funcname.append(parts[2]);
        this->memberMachineNodes[tmp->memberID] = tmp;
    }
    if(this->memberMpiRank == 0){
        for(auto const &p : this->memberMachineNodes){
            LOG(INFO) << "Machine ID(Rank): " << p.first << " -------------------------- ";
            machineNode *tmp = p.second;
            LOG(INFO) << "IP Address: " << tmp->memberIP << std::endl;
            LOG(INFO) << "Rank : " << tmp->memberID << std::endl;
        }
    }
}

void network::parsePsNodeFile(std::string &fileName) {
    std::ifstream in(fileName);
    std::string delim(CONF_FILE_DELIMITER);
    std::string linebuffer;
    while (getline(in, linebuffer)) {
        std::vector<std::string> parts;
        utilGetTokens(linebuffer, delim, parts);
        if (!parts[0].compare("#"))  // skip comment line
            continue;
        machineNode *tmp = new machineNode;
        tmp->memberIP.append(parts[0]);
        tmp->memberID = atoi(parts[1].c_str());
//        tmp->funcname.append(parts[2]);
        this->memberPsNodes[tmp->memberID] = tmp;
    }
    if (this->memberMpiRank == 0) {
        for (auto const &p : this->memberPsNodes) {
            LOG(INFO) << "@@ PS Machine ID(Rank): " << p.first << " -------------------------- ";
            machineNode *tmp = p.second;
            LOG(INFO) << "@@ PS  IP Address: " << tmp->memberIP << std::endl;
            LOG(INFO) << "@@ PS  Rank : " << tmp->memberID << std::endl;
        }
    }
}

// TODO: make an unified link file format for
// ring, reverse ring, start topologies
void network::parsePsLinkFile(std::string &fileName) {
    std::ifstream in(fileName);
    std::string delim(CONF_FILE_DELIMITER);
    std::string linebuffer;
    int linkCount = 0;
    while (getline(in, linebuffer)) {
        std::vector<std::string> parts;
        utilGetTokens(linebuffer, delim, parts);
        if (!parts[0].compare("#"))  // skip comment line
            continue;
        machineLink *tmp = new machineLink;
        tmp->memberSrcNode = atoi(parts[0].c_str());
        tmp->memberSrcPort = atoi(parts[1].c_str());
        tmp->memberDstNode = atoi(parts[2].c_str());
        tmp->memberDstPort = atoi(parts[3].c_str());
        // TODO: if necessary, take port information from the user.
        this->memberPsLinks[linkCount++] = tmp;
    }
    if (this->memberMpiRank == 0) {
        for (auto const &p : this->memberPsLinks) {
            LOG(INFO) << "@@ PS Link: " << p.first << " -------------------------- " << std::endl;
            machineLink *tmp = p.second;
            LOG(INFO) << "@@ PS src node: " << tmp->memberSrcNode << "src port: " << tmp->memberSrcPort << std::endl;
            LOG(INFO) << "@@ PS dst node: " << tmp->memberDstNode << "dst port: " << tmp->memberDstPort << std::endl;
        }
    }
}

void network::parseStarLinkFile(std::string &fileName){
    std::ifstream in(fileName);
    std::string delim(CONF_FILE_DELIMITER);
    std::string linebuffer;
    int linkCount=0;
    while(getline(in, linebuffer)){
        std::vector<std::string> parts;
        utilGetTokens(linebuffer, delim, parts);
        if(!parts[0].compare("#"))  // skip comment line
            continue;
        machineLink *tmp = new machineLink;
        tmp->memberSrcNode = atoi(parts[0].c_str());
        tmp->memberSrcPort = atoi(parts[1].c_str());
        tmp->memberDstNode = atoi(parts[2].c_str());
        tmp->memberDstPort = atoi(parts[3].c_str());
        // TODO: if necessary, take port information from the user.
        this->memberMachineStarLinks[linkCount++] = tmp;
    }
    if(this->memberMpiRank == 0){
        for(auto const &p : this->memberMachineStarLinks){
            LOG(INFO) << "Link: " << p.first << " -------------------------- " << std::endl;
            machineLink *tmp = p.second;
            LOG(INFO) << "src node: " << tmp->memberSrcNode << "src port: " << tmp->memberSrcPort << std::endl;
            LOG(INFO) << "dst node: " << tmp->memberDstNode << "dst port: " << tmp->memberDstPort << std::endl;
        }
    }
}

void network::createStarEthernet(zmq::context_t &contextZmq, int mpiSize, std::string &cip) {
    int hwm, hwmSend;
    size_t hwmSize = sizeof(hwm);
    size_t hwmSendSize = sizeof(hwmSend);
    int rank = this->memberMpiRank;

    char *buffer = (char *) calloc(sizeof(char), MAX_BUFFER); // max message size is limited to 2048 bytes now

    int *idcnt_s = (int *) calloc(sizeof(int), MAX_MACH);
    memset((void *) idcnt_s, 0x0, sizeof(int) * MAX_MACH);

    int *idcnt_w = (int *) calloc(sizeof(int), MAX_MACH);
    memset((void *) idcnt_w, 0x0, sizeof(int) * MAX_MACH);

    machineRole mrole = memberMachineRole;
    char *tmpcstring = (char *) calloc(sizeof(char), 128);

    if (mrole == machineRoleCoordinator) {

        int schedcnt = 0;
        int workercnt = 0;
        int schedcnt_r = 0;
        int workercnt_r = 0;

        for (auto const &p : this->memberMachineStarLinks) {
            machineLink *tmp = p.second;

// for receiving port
            if (tmp->memberDstNode == this->memberMpiRank) {
                int dstPort = tmp->memberDstPort;
                int srcNode = tmp->memberSrcNode;
                zmq::socket_t *pport_s = new zmq::socket_t(contextZmq, ZMQ_ROUTER);
                int setHwm = MAX_ZMQ_HWM;
                pport_s->setsockopt(ZMQ_RCVHWM, &setHwm, sizeof(int));//设置队列长度
                sprintf(tmpcstring, "tcp://*:%d", dstPort);
                pport_s->bind(tmpcstring); // open 5555 for all incomping connection
                pport_s->getsockopt(ZMQ_RCVHWM, (void *) &hwm, &hwmSize);
                LOG(INFO) << "@@@@@@@ Coordinator rank " << this->memberMpiRank
                          << " open a port " << tmpcstring
                          << " FOR RECEIVE PORT -- ptr to zmqsocket(" << pport_s
                          << ") HWM(" << hwm
                          << ")" << std::endl;
                machineRole dstMachRole = this->findRole(srcNode);
                _ringport *recvPort = new class _ringport;
                recvPort->ctx = new context((void *) pport_s, machineRoleCoordinator);
                if (dstMachRole == machineRoleScheduler) {
                    this->memberSchedulerRecvPortMap.insert(std::pair<int, _ringport *>(schedcnt, recvPort));
                    schedcnt++;
                } else if (dstMachRole == machineRoleWorker) {
                    this->memberWorkerRecvPortMap.insert(std::pair<int, _ringport *>(workercnt, recvPort));
                    workercnt++;
                } else {
                    assert(0);
                }
            }

// for sending port
            if (tmp->memberSrcNode == this->memberMpiRank) {
                int srcPort = tmp->memberSrcPort;
                int dstNode = tmp->memberDstNode;
                zmq::socket_t *pport_s = new zmq::socket_t(contextZmq, ZMQ_ROUTER);
                int setHwm = MAX_ZMQ_HWM;
                pport_s->setsockopt(ZMQ_SNDHWM, &setHwm, sizeof(int));//设置队列长度
                sprintf(tmpcstring, "tcp://*:%d", srcPort);
                pport_s->bind(tmpcstring);
                pport_s->getsockopt(ZMQ_RCVHWM, (void *) &hwm, &hwmSize);
                pport_s->getsockopt(ZMQ_SNDHWM, (void *) &hwmSend, &hwmSendSize);
                LOG(INFO) << "Coordinator rank " << this->memberMpiRank
                          << " open a port " << tmpcstring
                          << " FOR SEND PORT -- ptr to zmqsocket(" << pport_s
                          << ") RCVHWM(" << hwm
                          << ") SNDHWM(" << hwmSend
                          << ")" << std::endl;
                machineRole srcMachRole = this->findRole(dstNode);
                _ringport *sendPort = new class _ringport;
                sendPort->ctx = new context((void *) pport_s, machineRoleCoordinator);
                if (srcMachRole == machineRoleScheduler) {
                    this->memberSchedulerSendPortMap.insert(std::pair<int, _ringport *>(schedcnt_r, sendPort));
                    schedcnt_r++;
                } else if (srcMachRole == machineRoleWorker) {
                    this->memberWorkerSendPortMap.insert(std::pair<int, _ringport *>(workercnt_r, sendPort));
                    workercnt_r++;
                } else {
                    assert(0);
                }
            }
        }

        LOG(INFO) << "[Coordinator] Open ports and start hands shaking worker_recvportsize("
                  << this->memberWorkerRecvPortMap.size() << ") worker_sendportsize("
                  << this->memberWorkerSendPortMap.size() << ")" << std::endl;

        for (unsigned int i = 0; i < this->memberWorkerRecvPortMap.size(); i++) {
            int id = 1;
            zmq::socket_t *port_r = this->memberWorkerRecvPortMap[i]->ctx->m_zmqSocket;
            LOG(INFO) << "Coordinator -- wait for " << i
                      << "th worker RECV out of  " << this->memberWorkerRecvPortMap.size()
                      << "  pport(" << port_r
                      << ")" << std::endl;

            getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
            LOG(INFO) << "id: " << id << " buffer: " << buffer << std::endl;
            cppsSendMore(*port_r, (void *) &id, 4);
            std::string msg1("Go");
            cppsSend(*port_r, (void *) msg1.c_str(), MAX_BUFFER - 1);
        }

        LOG(INFO) << "[Coordinator @@@@@] finish worker recv port confirm " << std::endl;

        for (unsigned int i = 0; i < this->memberWorkerSendPortMap.size(); i++) {
            int id = 0;
            zmq::socket_t *port_r = this->memberWorkerSendPortMap[i]->ctx->m_zmqSocket;
            LOG(INFO) << "Coordinator -- wait for " << i
                      << "th worker SENDPORT out of  " << this->memberWorkerSendPortMap.size()
                      << "  pport(" << port_r
                      << ")" << std::endl;

            getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
            LOG(INFO) << "id: " << id << " buffer: " << buffer << std::endl;
            cppsSendMore(*port_r, (void *) &id, 4);
            std::string msg1("Go");
            cppsSend(*port_r, (void *) msg1.c_str(), MAX_BUFFER - 1);
        }

        LOG(INFO) << "[Coordinator @@@@@] finish worker send port confirm " << std::endl;


        for (unsigned int i = 0; i < this->memberSchedulerRecvPortMap.size(); i++) {
            int id = 1;
            zmq::socket_t *port_r = this->memberSchedulerRecvPortMap[i]->ctx->m_zmqSocket;

            LOG(INFO) << "Coordinator -- wait for " << i
                      << "th scheduler RECVPORT out of  " << this->memberSchedulerRecvPortMap.size()
                      << "  pport(" << port_r
                      << ")" << std::endl;

            getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
            LOG(INFO) << "id: " << id << " buffer: " << buffer << std::endl;
            cppsSendMore(*port_r, (void *) &id, 4);
            std::string msg1("Go");
            cppsSend(*port_r, (void *) msg1.c_str(), MAX_BUFFER - 1);
        }


        LOG(INFO) << "[Coordinator @@@@@] finish scheduler recv port confirm " << std::endl;


        for (unsigned int i = 0; i < this->memberSchedulerSendPortMap.size(); i++) {
            int id = 0;
            zmq::socket_t *port_r = this->memberSchedulerSendPortMap[i]->ctx->m_zmqSocket;
            LOG(INFO) << "Coordinator -- wait for " << i
                      << "th scheduler SENDPORT out of  " << this->memberSchedulerSendPortMap.size()
                      << "  pport(" << port_r
                      << ")" << std::endl;

            getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
            LOG(INFO) << "id: " << id << " buffer: " << buffer << std::endl;
            cppsSendMore(*port_r, (void *) &id, 4);
            std::string msg1("Go");
            cppsSend(*port_r, (void *) msg1.c_str(), MAX_BUFFER - 1);
        }


        LOG(INFO) << "[Coordinator @@@@@] finish scheduler send port confirm " << std::endl;

    } else if (mrole == machineRoleWorker or mrole == machineRoleScheduler) {

        sleep(2);
        for (auto const &p : this->memberMachineStarLinks) {
            machineLink *tmp = p.second;

// for receiving port
            if (tmp->memberDstNode == this->memberMpiRank) {
                int srcPort = tmp->memberSrcPort;
                zmq::socket_t *zport = new zmq::socket_t(contextZmq, ZMQ_DEALER);
                int *identity = (int *) calloc(1, sizeof(int));
                *identity = 1;
                zport->setsockopt(ZMQ_IDENTITY, (void *) identity, sizeof(int));

                int *sethwm = (int *) calloc(1, sizeof(int));
                *sethwm = MAX_ZMQ_HWM;
                zport->setsockopt(ZMQ_RCVHWM, sethwm, sizeof(int));

                sprintf(tmpcstring, "tcp://%s:%d", cip.c_str(), srcPort);

                zport->connect(tmpcstring); // open 5555 for all incomping connection
                zport->getsockopt(ZMQ_RCVHWM, (void *) &hwm, &hwmSize);
                LOG(INFO) << "RANK rank %d CONNECT a port  " << this->memberMpiRank
                          << " FOR RECEIVE PORT -- ptr to socket(" << tmpcstring
                          << ") HWM(" << hwm
                          << ") " << std::endl;

                _ringport *recvport = new class _ringport;
                recvport->ctx = new context((void *) zport, mrole);
                this->memberStarRecvPortMap.insert(std::pair<int, _ringport *>(0, recvport));
            }

// for sending port
            if (tmp->memberSrcNode == this->memberMpiRank) {
                int dstPort = tmp->memberDstPort;
                zmq::socket_t *zport = new zmq::socket_t(contextZmq, ZMQ_DEALER);
                int *identity = (int *) calloc(1, sizeof(int));
                *identity = 1;
                zport->setsockopt(ZMQ_IDENTITY, (void *) identity, sizeof(int));

                int sethwm = MAX_ZMQ_HWM;
                zport->setsockopt(ZMQ_SNDHWM, &sethwm, sizeof(int));

                sprintf(tmpcstring, "tcp://%s:%d", cip.c_str(), dstPort);
                zport->connect(tmpcstring); // open 5555 for all incomping connection
                LOG(INFO) << "Rank " << this->memberMpiRank
                          << " CONNECT a port " << tmpcstring
                          << " FOR SEND PORT -- ptr to socket(" << zport
                          << ") " << std::endl;

                _ringport *recvport = new class _ringport;
                recvport->ctx = new context((void *) zport, mrole);
                this->memberStarSendPortMap.insert(std::pair<int, _ringport *>(0, recvport));
            }
        }

        char *buffer = (char *) calloc(sizeof(char), MAX_BUFFER);
        sprintf(buffer, "Heart Beat from rank %d", rank);
        std::string msg(buffer);
        zmq::socket_t *sendport = this->memberStarSendPortMap[0]->ctx->m_zmqSocket;
        printf("Rank(%d) Send HB to SEND PORT ptr -- socket(%p) \n", rank, sendport);
        cppsSend(*sendport, (void *) msg.c_str(), strlen(msg.c_str()));
        getSingleMessage(*sendport, rank, buffer, MAX_BUFFER);
        LOG(INFO) << "buffer: " << buffer << std::endl;
        LOG(INFO) << "[Rank " << rank << "] got confirm for RECV PORT " << std::endl;
        zmq::socket_t *recvport = this->memberStarRecvPortMap[0]->ctx->m_zmqSocket;
        printf("Rank(%d) Send HB to RECV PORT ptr -- socket(%p) \n", rank, recvport);
        cppsSend(*recvport, (void *) msg.c_str(), strlen(msg.c_str()));
        getSingleMessage(*recvport, rank, buffer, MAX_BUFFER);
        LOG(INFO) << "buffer: " << buffer << std::endl;
        LOG(INFO) << "[Rank " << rank << "] got confirm for SEND PORT " << std::endl;

    } else {
        LOG(INFO) << "[Fatal: rank " << rank << "] MACHINE TYPE IS NOT ASSIGNED YET " << std::endl;
        assert(0);
    }
    LOG(INFO) << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ finish create starring ethernet by rank (" << rank
              << ")" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
}

void network::createRingWorkerEthernetAux(zmq::context_t &contextZmq, int mpiSize, std::string &cip) {
    int hwm;
    size_t hwmSize = sizeof(hwm);

    int rank = memberMpiRank;

    int *idcnt_s = (int *) calloc(sizeof(int), MAX_MACH);
    memset((void *) idcnt_s, 0x0, sizeof(int) * MAX_MACH);

    int *idcnt_w = (int *) calloc(sizeof(int), MAX_MACH);
    memset((void *) idcnt_w, 0x0, sizeof(int) * MAX_MACH);

    machineRole mrole = memberMachineRole;

    char *tmpCstring = (char *) calloc(sizeof(char), 128);
    int firstScheduler = memberFirstSchedulerMachineID;
    int schedulerMachine = memberSchedulerMachineNumber;
    int workerMachine = memberWorkerMachineNumber;
    int firstCoordinator = memberFirstCoordinatorMachineID;

    if (mrole == machineRoleCoordinator) {

        for (auto const &p : memberMachineStarLinks) {
            machineLink *tmp = p.second;

            if ((p.second->memberSrcNode >= firstScheduler and p.second->memberSrcNode < firstScheduler + schedulerMachine) or
                // if workers related link, skip
                (p.second->memberDstNode >= firstScheduler and p.second->memberDstNode < firstScheduler + schedulerMachine)) {
                continue;
            }

// for receiving port
            if (tmp->memberDstNode == memberMpiRank) {
                int dstPort = tmp->memberDstPort;
                int srcNode = tmp->memberSrcNode;
                zmq::socket_t *pport_s = new zmq::socket_t(contextZmq, ZMQ_ROUTER);
                int setHwm = MAX_ZMQ_HWM;
                pport_s->setsockopt(ZMQ_RCVHWM, &setHwm, sizeof(int));
                sprintf(tmpCstring, "tcp://*:%d", dstPort);
                pport_s->bind(tmpCstring); // open 5555 for all incomping connection
                pport_s->getsockopt(ZMQ_RCVHWM, (void *) &hwm, &hwmSize);
                LOG(INFO) << "@@@@@@@ Coordinator rank " << memberMpiRank
                          << " open a port " << tmpCstring
                          << " FOR RECEIVE PORT -- ptr to zmqsocket(" << pport_s
                          << ") HWM(" << hwm
                          << ")" << std::endl;

                machineRole dstMachineRole = this->findRole(srcNode);
                _ringport *recvport = new class _ringport;
                recvport->ctx = new context((void *) pport_s, machineRoleCoordinator);
                if (dstMachineRole == machineRoleScheduler) {
                    exit(0);
//                    assert(0);
                } else if (dstMachineRole == machineRoleWorker) {
                    if (srcNode == (workerMachine - 1)) {
//                        assert(pshctx->ring_recvportmap.size() < 2);
                        this->memberRingRecvPortMap.insert(std::pair<int, _ringport *>(RDATAPORT, recvport));
                    } else if (srcNode == 0) {
//                        assert(pshctx->ring_recvportmap.size() < 2);
                        this->memberRingRecvPortMap.insert(std::pair<int, _ringport *>(RACKPORT, recvport));
                    } else {
                        assert(0);
                    }
                } else {
                    assert(0);
                }
            }

// for sending port
            if (tmp->memberSrcNode == memberMpiRank) {
                int dstNode = tmp->memberDstNode;
                int dstPort = tmp->memberDstPort;
                zmq::socket_t *zport = new zmq::socket_t(contextZmq, ZMQ_DEALER);
                int *identity = (int *) calloc(1, sizeof(int));
                *identity = 1;
                zport->setsockopt(ZMQ_IDENTITY, (void *) identity, sizeof(int));
                int setHwm = MAX_ZMQ_HWM;
                zport->setsockopt(ZMQ_SNDHWM, &setHwm, sizeof(int));
                sprintf(tmpCstring, "tcp://%s:%d", memberMachineNodes[dstNode]->memberIP.c_str(), dstPort);
                zport->connect(tmpCstring); // open 5555 for all incomping connection
                LOG(INFO) << "Rank " << memberMpiRank
                          << " CONNECT a port " << tmpCstring
                          << " FOR SEND PORT -- ptr to socket(" << zport
                          << ")" << std::endl;

                machineRole srcMachineRole = this->findRole(dstNode);
                _ringport *sendport = new class _ringport;
                sendport->ctx = new context((void *) zport, machineRoleCoordinator);
                if (srcMachineRole == machineRoleScheduler) {
//                    assert(0);
                } else if (srcMachineRole == machineRoleWorker) {
                    if (dstNode == (workerMachine - 1)) {
//                        assert(pshctx->ring_sendportmap.size() < 2);
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RACKPORT, sendport));
                    } else if (dstNode == 0) {
//                        assert(pshctx->ring_sendportmap.size() < 2);
                        LOG(INFO) << "coordinator put rdataport into sendport for dstnode " << dstNode
                                  << " (sendport[" << sendport->ctx
                                  << "]" << std::endl;
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RDATAPORT, sendport));
                    } else {
//                        assert(0);
                    }
                } else {
//                    assert(0);
                }
            }
        }


        if (this->memberRingSendPortMap.size() != 2) {
            LOG(INFO) << "ctx->rank "<< memberMpiRank
                      <<" breaks rules :  pshctx->ring_sendportmap.size() : " << this->memberRingSendPortMap.size() << std::endl;
        }

//        assert(pshctx->ring_sendportmap.size() == 2);
//        assert(pshctx->ring_recvportmap.size() == 2);
        coordinatorRingWakeUpAux(RDATAPORT, rank);
        coordinatorRingWakeUpAux(RACKPORT, rank);
    } else if (mrole == machineRoleWorker) {

//    int rcnt = 0;
//    int scnt = 0;
        sleep(2);

        for (auto const &p : memberMachineStarLinks) {
            machineLink *tmp = p.second;
            if ((p.second->memberSrcNode >= firstScheduler and p.second->memberSrcNode < firstScheduler + schedulerMachine) or
                // if scheduler related link, skip
                (p.second->memberDstNode >= firstScheduler and p.second->memberDstNode < firstScheduler + schedulerMachine)) {
                continue;
            }

// for receiving port
            if (tmp->memberDstNode == memberMpiRank) {
                int dstPort = tmp->memberDstPort;
                int srcNode = tmp->memberSrcNode;
                zmq::socket_t *pport_s = new zmq::socket_t(contextZmq, ZMQ_ROUTER);
                int setHwm = MAX_ZMQ_HWM;
                pport_s->setsockopt(ZMQ_RCVHWM, &setHwm, sizeof(int));
                sprintf(tmpCstring, "tcp://*:%d", dstPort);
                pport_s->bind(tmpCstring); // open 5555 for all incomping connection
                pport_s->getsockopt(ZMQ_RCVHWM, (void *) &hwm, &hwmSize);
                LOG(INFO) << "@@@@@@@ WORKER rank " << memberMpiRank
                          << " open a port " << tmpCstring
                          << " FOR RECEIVE PORT -- ptr to zmqsocket(" << pport_s
                          << ") HWM(" << hwm
                          << ")" << std::endl;

//mach_role dstmrole = pshctx->find_role(mpi_size, srcnode);
                _ringport *recvport = new class _ringport;
                recvport->ctx = new context((void *) pport_s, machineRoleCoordinator);

                if (memberMpiRank == 0) { // the first worker
                    if (srcNode == firstCoordinator) {
//                        assert(pshctx->ring_recvportmap.size() < 2);
                        this->memberRingRecvPortMap.insert(
                                std::pair<int, _ringport *>(RDATAPORT, recvport)); // for data port
                    } else {
//                        assert(pshctx->ring_recvportmap.size() < 2);
                        this->memberRingRecvPortMap.insert(
                                std::pair<int, _ringport *>(RACKPORT, recvport)); // for data port
                    }
                } else if (memberMpiRank == (workerMachine - 1)) { // the last worker
                    if (srcNode == firstCoordinator) {
//                        assert(pshctx->ring_recvportmap.size() < 2);
                        this->memberRingRecvPortMap.insert(
                                std::pair<int, _ringport *>(RACKPORT, recvport)); // for data port
                    } else {
//                        assert(pshctx->ring_recvportmap.size() < 2);
//                        assert(srcnode == (pshctx->rank - 1));
                        this->memberRingRecvPortMap.insert(
                                std::pair<int, _ringport *>(RDATAPORT, recvport)); // for data port
                    }
                } else { // workers between the first and last workers.
                    int rank = memberMpiRank;
                    if (srcNode == (rank - 1)) {
//                        assert(pshctx->ring_recvportmap.size() < 2);
                        this->memberRingRecvPortMap.insert(
                                std::pair<int, _ringport *>(RDATAPORT, recvport)); // for data port
                    } else {
//                        assert(pshctx->ring_recvportmap.size() < 2);
                        this->memberRingRecvPortMap.insert(
                                std::pair<int, _ringport *>(RACKPORT, recvport)); // for data port
                    }
                }
            }

// for sending port
            if (tmp->memberSrcNode == memberMpiRank) {
                int dstNode = tmp->memberDstNode;
                int dstPort = tmp->memberDstPort;
                zmq::socket_t *zport = new zmq::socket_t(contextZmq, ZMQ_DEALER);
                int *identity = (int *) calloc(1, sizeof(int));
                *identity = 1;
                zport->setsockopt(ZMQ_IDENTITY, (void *) identity, sizeof(int));
                int sethwm = MAX_ZMQ_HWM;
                zport->setsockopt(ZMQ_SNDHWM, &sethwm, sizeof(int));

//sprintf(tmpcstring, "tcp://%s:%d", cip.c_str(), dstport);
                sprintf(tmpCstring, "tcp://%s:%d", memberMachineNodes[dstNode]->memberIP.c_str(), dstPort);
                zport->connect(tmpCstring); // open 5555 for all incomping connection
                LOG(INFO) << "Rank " << memberMpiRank
                          << " CONNECT a port " << tmpCstring
                          << " FOR SEND PORT -- ptr to socket(" << zport
                          << ")" << std::endl;

                _ringport *recvport = new class _ringport;
                recvport->ctx = new context((void *) zport, mrole);
                if (memberMpiRank == 0) { // the first worker
                    if (dstNode == firstCoordinator) {
//                        assert(pshctx->ring_sendportmap.size() < 2);
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RACKPORT, recvport));
                    } else {
//                        assert(pshctx->ring_sendportmap.size() < 2);
//                        assert(dstnode == (pshctx->rank +1));
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RDATAPORT, recvport));
                    }
                } else if (memberMpiRank == (workerMachine - 1)) { // the last worker
                    if (dstNode == firstCoordinator) {
//                        assert(pshctx->ring_sendportmap.size() < 2);
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RDATAPORT, recvport));
                    } else {
//                        assert(pshctx->ring_sendportmap.size() < 2);
//                        assert(dstnode == (pshctx->rank - 1));
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RACKPORT, recvport));
                    }
                } else { // workers between the first and last workers.
                    int rank = memberMpiRank;
                    if (dstNode == (rank + 1)) {
//                        assert(pshctx->ring_sendportmap.size() < 2);
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RDATAPORT, recvport));
                    } else {
//                        assert(pshctx->ring_sendportmap.size() < 2);
//                        assert(dstnode == (rank-1));
                        this->memberRingSendPortMap.insert(std::pair<int, _ringport *>(RACKPORT, recvport));
                    }
                }
            }
        }
        workerRingWakeUpAux(RDATAPORT, rank);
        workerRingWakeUpAux(RACKPORT, rank);

    } else if (mrole == machineRoleScheduler) {
// do noting: schedulr
    } else {
        LOG(INFO) << "[Fatal: rank " << rank << "] MACHINE TYPE IS NOT ASSIGNED YET" << std::endl;
//        assert(0);
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

void network::createPsStarEthernet(zmq::context_t &contextZmq, int mpiSize, std::string &cip, int serverRank) {
    int hwm, hwmSend;
    size_t hwmSize = sizeof(hwm);
    size_t hwmSendSize = sizeof(hwmSend);
    int rank = memberMpiRank;

    char *buffer = (char *)calloc(sizeof(char), MAX_BUFFER); // max message size is limited to 2048 bytes now
    int *idcnt_s = (int *)calloc(sizeof(int), MAX_MACH);
    memset((void *)idcnt_s, 0x0, sizeof(int)*MAX_MACH);

    int *idcnt_w = (int *)calloc(sizeof(int), MAX_MACH);
    memset((void *)idcnt_w, 0x0, sizeof(int)*MAX_MACH);

    machineRole mrole = memberMachineRole;
    char *tmpCstring = (char *)calloc(sizeof(char), 128);

    if(mrole == machineRoleScheduler){ // ps server

        int schedcnt=0;
        int workercnt=0;
        int schedcnt_r=0;
        int workercnt_r=0;

        for(auto const &p : memberPsLinks){
            machineLink *tmp = p.second;

            // for receiving port
            if(tmp->memberDstNode == memberMpiRank){
                int dstPort = tmp->memberDstPort;
                int srcNode = tmp->memberSrcNode;
                zmq::socket_t *pport_s = new zmq::socket_t(contextZmq, ZMQ_ROUTER);
                int setHwm = MAX_ZMQ_HWM;
                pport_s->setsockopt(ZMQ_RCVHWM, &setHwm, sizeof(int));
                sprintf(tmpCstring, "tcp://*:%d", dstPort);
                pport_s->bind (tmpCstring); // open 5555 for all incomping connection
                pport_s->getsockopt(ZMQ_RCVHWM, (void *)&hwm, &hwmSize);
                LOG(INFO) << "@@@@@@@ PS rank " << memberMpiRank
                          << " open a port " << tmpCstring
                          << " FOR RECEIVE PORT -- ptr to zmqsocket(" << pport_s
                          << ") HWM(" << hwm
                          << ")" << std::endl;

                machineRole dstMachineRole = this->findRole(srcNode);
                _ringport *recvPort = new class _ringport;
                recvPort->ctx = new context((void*)pport_s, machineRoleScheduler);
                if(dstMachineRole == machineRoleScheduler){
//                    assert(0);
                    //	  pshctx->ps_recvportmap.insert(pair<int, _ringport*>(schedcnt, recvport));
                    //	  schedcnt++;
                }else if(dstMachineRole == machineRoleWorker){
                    memberPsRecvPortMap.insert(std::pair<int, _ringport*>(workercnt, recvPort));
                    workercnt++;
                }else{
                    LOG(INFO) << "PS [Fatal] SRC NODE (" << srcNode
                              << ")[dstmrole: "<< dstMachineRole
                              <<"] rank " << memberMpiRank << std::endl;
//                    assert(0);
                }
            }

            // for sending port
            if(tmp->memberSrcNode == memberMpiRank){
                int srcPort = tmp->memberSrcPort;
                int dstNode = tmp->memberDstNode;
                zmq::socket_t *pport_s = new zmq::socket_t(contextZmq, ZMQ_ROUTER);
                int sethwm = MAX_ZMQ_HWM;
                pport_s->setsockopt(ZMQ_SNDHWM, &sethwm, sizeof(int));
                pport_s->setsockopt(ZMQ_RCVHWM, &sethwm, sizeof(int));

                sprintf(tmpCstring, "tcp://*:%d", srcPort);
                pport_s->bind (tmpCstring);
                pport_s->getsockopt(ZMQ_RCVHWM, (void *)&hwm, &hwmSize);
                pport_s->getsockopt(ZMQ_SNDHWM, (void *)&hwmSend, &hwmSendSize);
                LOG(INFO) << "PS rank " << memberMpiRank
                          << " open a port "<< tmpCstring
                          << " FOR SEND PORT -- ptr to zmqsocket(" << pport_s
                          << ") RCVHWM( " << hwm
                          << " ) SNDHWM(" << hwmSend
                          << ")" << std::endl;

                machineRole srcMachineRole = this->findRole(dstNode);
                _ringport *sendport = new class _ringport;
                sendport->ctx = new context((void*)pport_s, machineRoleScheduler);
                if(srcMachineRole == machineRoleScheduler){
//                    assert(0);
                    //	  pshctx->ps_sendportmap.insert(pair<int, _ringport*>(schedcnt_r, sendport));
                    //	  schedcnt_r++;
                }else if(srcMachineRole == machineRoleWorker){
                    memberPsSendPortMap.insert(std::pair<int, _ringport*>(workercnt_r, sendport));
                    workercnt_r++;
                }else{
//                    assert(0);
                }
            }
        }

//        strads_msg(ERR, "[PS] Open ports and start hands shaking worker_recvportsize(%ld) worker_sendportsize(%ld)\n",
//                   pshctx->worker_recvportmap.size(),
//                   pshctx->worker_sendportmap.size());

        for(unsigned int i=0; i<memberPsRecvPortMap.size(); i++){
            int id = 1;
            zmq::socket_t *port_r = memberPsRecvPortMap[i]->ctx->m_zmqSocket;
            LOG(INFO) << "PS -- wait for "<< i
                      << "th worker RECV out of  " << memberPsSendPortMap.size()
                      << "  pport(" << port_r
                      << ")" << std::endl;

            getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
//            assert(id == 1);
            cppsSendMore(*port_r, (void *)&id, 4);
            std::string msg1("Go");
            cppsSend(*port_r, (void *)msg1.c_str(), MAX_BUFFER-1);
        }

        LOG(INFO) << "[PS @@@@@] finish worker recv port confirm " << std::endl;

        for(unsigned int i=0; i<memberPsSendPortMap.size(); i++){
            int id = 0;
            zmq::socket_t *port_r = memberPsSendPortMap[i]->ctx->m_zmqSocket;
            LOG(INFO) << "Coordinator -- wait for " << i
                      << "th worker SENDPORT out of " << memberPsSendPortMap.size()
                      << " pport(" << port_r
                      << ")" << std::endl;

            getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
            cppsSendMore(*port_r, (void *)&id, 4);
            std::string msg1("Go");
            cppsSend(*port_r, (void *)msg1.c_str(), MAX_BUFFER-1);
        }
        LOG(INFO) << "[PS @@@@@] finish worker send port confirm" << std::endl;


    }else if(mrole == machineRoleWorker){

        sleep(2);

        int slotid_r = memberPsRecvPortMap.size();
        int slotid_s = memberPsSendPortMap.size();

        for(auto const &p : memberPsLinks){
            machineLink *tmp = p.second;

            // for receiving port
            if(tmp->memberDstNode == memberMpiRank and tmp->memberSrcNode == serverRank ){
                int srcport = tmp->memberSrcPort;
                zmq::socket_t *zport = new zmq::socket_t(contextZmq, ZMQ_DEALER);
                int *identity = (int *)calloc(1, sizeof(int));
                *identity = 1;
                zport->setsockopt(ZMQ_IDENTITY, (void *)identity, sizeof(int));

                int *sethwm = (int *)calloc(1, sizeof(int));
                *sethwm = MAX_ZMQ_HWM;
                zport->setsockopt(ZMQ_RCVHWM, sethwm, sizeof(int));

                //	sprintf(tmpcstring, "tcp://10.54.1.30:%d", srcport);
                sprintf(tmpCstring, "tcp://%s:%d", cip.c_str(), srcport);
                zport->connect (tmpCstring); // open 5555 for all incomping connection
                zport->getsockopt(ZMQ_RCVHWM, (void *)&hwm, &hwmSize);
                LOG(INFO) << "RANK rank " <<memberMpiRank
                          << " CONNECT a port  " << tmpCstring
                          << " FOR RECEIVE PORT -- ptr to socket(" << zport
                          << ") HWM(" << hwm
                          << ")" << std::endl;

                _ringport *recvport = new class _ringport;
                recvport->ctx = new context((void*)zport, mrole);

                int slotid = memberPsRecvPortMap.size();
                assert(slotid == slotid_r);
                memberPsRecvPortMap.insert(std::pair<int, _ringport*>(slotid_r, recvport));
            }

            // for sending port
            if(tmp->memberSrcNode == memberMpiRank and tmp->memberDstNode == serverRank){
                int dstport = tmp->memberDstPort;
                zmq::socket_t *zport = new zmq::socket_t(contextZmq, ZMQ_DEALER);
                int *identity = (int *)calloc(1, sizeof(int));
                *identity = 1;
                zport->setsockopt(ZMQ_IDENTITY, (void *)identity, sizeof(int));
                int setHwm = MAX_ZMQ_HWM;
                zport->setsockopt(ZMQ_SNDHWM, &setHwm, sizeof(int));
                //	sprintf(tmpcstring, "tcp://10.54.1.30:%d", dstport);
                sprintf(tmpCstring, "tcp://%s:%d", cip.c_str(), dstport);
                zport->connect (tmpCstring); // open 5555 for all incomping connection
                LOG(INFO) << "Rank " << memberMpiRank
                          << " CONNECT a port " << tmpCstring
                          << " FOR SEND PORT -- ptr to socket(" << zport
                          << ") " << std::endl;

                _ringport *recvPort = new class _ringport;
                recvPort->ctx = new context((void*)zport, mrole);

                int slotid = memberPsSendPortMap.size();
                assert(slotid == slotid_s);
                memberPsSendPortMap.insert(std::pair<int, _ringport*>(slotid, recvPort));
            }
        }

//        assert((slotid_r+1) == pshctx->ps_recvportmap.size());
//        assert((slotid_s+1) == pshctx->ps_sendportmap.size());

        char *buffer = (char *)calloc(sizeof(char), MAX_BUFFER);
        sprintf(buffer, "PS Heart Beat from rank %d", rank);
        std::string msg(buffer);
        zmq::socket_t *sendport = memberPsSendPortMap[slotid_s]->ctx->m_zmqSocket;
        printf("PS Rank(%d) Send HB to SEND PORT ptr -- socket(%p) \n", rank, sendport);
        cppsSend(*sendport, (void *)msg.c_str(), strlen(msg.c_str()));
        getSingleMessage(*sendport, rank, buffer, MAX_BUFFER);
//        strads_msg(OUT, "PS [Rank %d] got confirm for RECV PORT \n", rank);
        zmq::socket_t *recvport = memberPsRecvPortMap[slotid_r]->ctx->m_zmqSocket;
        printf("PS Rank(%d) Send HB to RECV PORT ptr -- socket(%p) \n", rank, recvport);
        cppsSend(*recvport, (void *)msg.c_str(), strlen(msg.c_str()));
        getSingleMessage(*recvport, rank, buffer, MAX_BUFFER);
        LOG(INFO) << "PS [Rank " << rank
                  << "] got confirm for SEND PORT " << std::endl;

    }else{
        LOG(INFO) << "PS [Fatal: rank " << rank
                  << "] MACHINE TYPE IS NOT ASSIGNED YET" << std::endl;
//        assert(0);
    }

    LOG(INFO) << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ PSPS finish create star  ethernet by rank (" << rank
              << ")  with server rank " << serverRank << std::endl;

    //  MPI_Barrier(MPI_COMM_WORLD);
}

int network::getIDMessage(zmq::socket_t &zport, int rank, int *identity, char *message, int length) {
    int messageLengh = -1;
    for (int i = 0; i < 2; i++) {
        zmq::message_t request;
        zport.recv(&request);
        char *data = (char *) request.data();
        int size = request.size();
        if (i == 0) {
            *identity = *(int *) data;
        }
        if (i == 1) {
            memcpy(message, &data[0], size);
            messageLengh = size;
        }
        int more;
        size_t more_size = sizeof(more);
        zport.getsockopt(ZMQ_RCVMORE, (void *) &more, &more_size);
        if (!more) {
            break;      //  Last message part
        }
    }
    return messageLengh;
}

int network::getSingleMessage(zmq::socket_t &zport, int rank, char *message, int length) {
    int messageLength = -1;
    zmq::message_t request;
    zport.recv(&request);
    char *data = (char *) request.data();
    int size = request.size();
    memcpy(message, &data[0], size);
    messageLength = size;
    int more;
    size_t moreSize = sizeof(more);
    zport.getsockopt(ZMQ_RCVMORE, (void *) &more, &moreSize);
    if (more) {
        exit(1);
//        assert(0);
    }
    return messageLength;
}

bool network::cppsSendMore (zmq::socket_t &zport, void *message, int length) {
    bool rc;
    zmq::message_t request(length);
    memcpy((void *) request.data(), message, length);
    rc = zport.send(request, ZMQ_SNDMORE);
    return (rc);
}

bool network::cppsSend (zmq::socket_t &zport, void *message, int length) {
    bool rc;
    zmq::message_t request(length);
    memcpy((void *) request.data(), message, length);
    rc = zport.send(request);
    return (rc);
}

void network::coordinatorRingWakeUpAux(int dackPort, int rank) {

    char *buffer = (char *) calloc(sizeof(char), MAX_BUFFER);
    sprintf(buffer, "Heart Beat from rank %d", rank);
    std::string msg(buffer);
    zmq::socket_t *sendport = memberRingSendPortMap[dackPort]->ctx->m_zmqSocket;
    cppsSend(*sendport, (void *) msg.c_str(), strlen(msg.c_str()));
    printf("COORDINATOR Rank(%d) Send HB to SEND PORT ptr -- socket(%p) -- Sending is DONE  \n", rank, sendport);
    int id = 1;
    zmq::socket_t *port_r = memberRingRecvPortMap[dackPort]->ctx->m_zmqSocket;
    LOG(INFO) << "[ Coordinator Rank : " << rank
              << " ]  GOT MESSAGE recvport (" << port_r
              << ") " << std::endl;

    getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
    LOG(INFO) << "[ Coordinator Rank : " << rank
              << " ]  GOT MESSAGE recvport (" << port_r
              << ") DONE DONE" << std::endl;

    cppsSendMore(*port_r, (void *) &id, 4);
    std::string msg1("Go ring");
    cppsSend(*port_r, (void *) msg1.c_str(), MAX_BUFFER - 1);
    getSingleMessage(*sendport, rank, buffer, MAX_BUFFER);
    LOG(INFO) << "buffer: " << buffer << std::endl;
    LOG(INFO) << "[Coordinator Rank " << rank
              << "] got confirm for SEND PORT" << std::endl;

}

void network::workerRingWakeUpAux(int dackPort, int rank) {
    char *buffer = (char *) calloc(sizeof(char), MAX_BUFFER);
    sprintf(buffer, "Heart Beat from rank %d", rank);
    std::string msg(buffer);
    zmq::socket_t *sendport = memberRingSendPortMap[dackPort]->ctx->m_zmqSocket;
    cppsSend(*sendport, (void *) msg.c_str(), strlen(msg.c_str()));
    printf("COORDINATOR Rank(%d) Send HB to SEND PORT ptr -- socket(%p) -- Sending is DONE  \n", rank, sendport);
    int id = 1;
    zmq::socket_t *port_r = memberRingRecvPortMap[dackPort]->ctx->m_zmqSocket;
    LOG(INFO) << "[ Rank : " << rank
              << " ]  GOT MESSAGE recvport (" << port_r
              << ")" << std::endl;

    getIDMessage(*port_r, rank, &id, buffer, MAX_BUFFER);
    LOG(INFO) << "[ Worker Rank : " << rank
              << " ]  GOT MESSAGE recvport (" << port_r
              << ") DONE DONE" << std::endl;

    cppsSendMore(*port_r, (void *) &id, 4);
    std::string msg1("Go ring");
    cppsSend(*port_r, (void *) msg1.c_str(), MAX_BUFFER - 1);
    getSingleMessage(*sendport, rank, buffer, MAX_BUFFER);
    LOG(INFO) << "buffer: " << buffer << std::endl;
    LOG(INFO) << "[Worker Rank " << rank
              << "] got confirm for SEND PORT" << std::endl;

}