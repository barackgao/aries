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
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD,&mpiSize);
    this->memberMpiRank = mpiRank;
    this->memberMpiSize = mpiSize;
    this->memberSchedulerMachineNumber = FLAGS_schedulerMachineNumber;

    //TODO: 定义角色
    this->findRole();

    //TODO: 初始化系统参数和通信句柄
    this->memberSystemParameter = new systemParameter();
    this->memberSystemParameter->init(argc, argv, this->memberMpiRank, this->memberMpiSize);

}

int network::findRole() {
    machineRole mrole = machineRoleUnknown;
    int schedulerMachineNumber = this->memberSystemParameter->;
        int firstSchedulerMachineID = memberMpiSize -  - 1;
        int firstCoordinatorMachineID = memberMpiSize - 1;

        memberSchedulerMachineNumber = memberSystemParameter->memberSchedulerNumber;
        memberFirstSchedulerMachineID = memberMpiSize - memberSchedulerMachineNumber - 1;
        memberWorkerMachineNumber = memberFirstSchedulerMachineID;
        memberFirstWorkerMachineID = 0;
        memberFirstCoordinatorMachineID = memberMpiSize - 1;

        if(memberMpiRank == memberFirstCoordinatorMachineID){
            mrole = machineRoleCoordinator;
        }
        else if(memberMpiRank < memberFirstSchedulerMachineID){
            mrole = machineRoleWorker;
        }
        else if(memberMpiRank >= memberFirstSchedulerMachineID && memberMpiRank < memberFirstCoordinatorMachineID){
            mrole = machineRoleScheduler;
        }

        memberMachineRole = mrole;

        return mrole;

    }
}

sharedCtx(int mpiRank,sysParam *params):
        m_mpiRank(mpiRank), m_machineRole(machineRoleUnknown),
        /*
        m_sport_lock(PTHREAD_MUTEX_INITIALIZER),
        m_rport_lock(PTHREAD_MUTEX_INITIALIZER),
        m_sport_flag(false), m_rport_flag(false),
        ring_sport(NULL), ring_rport(NULL),
        m_fsport_lock(PTHREAD_MUTEX_INITIALIZER),
        m_frport_lock(PTHREAD_MUTEX_INITIALIZER),
        m_fsport_flag(false), m_frport_flag(false),
        ring_fsport(NULL), ring_frport(NULL),
        m_starsport_lock(PTHREAD_MUTEX_INITIALIZER),
        m_starrport_lock(PTHREAD_MUTEX_INITIALIZER),
        m_starsend(PTHREAD_MUTEX_INITIALIZER),
        m_starrecv(PTHREAD_MUTEX_INITIALIZER),
         */
        m_sysParam(params)/*, m_idvalp_buf(NULL),
            m_freethrds_lock(PTHREAD_MUTEX_INITIALIZER),
            m_freethrds(0),
            m_schedctx(NULL), m_coordctx(NULL), m_workerctx(NULL),
            m_ringtoken_send_lock(PTHREAD_MUTEX_INITIALIZER),
            m_ringtoken_recv_lock(PTHREAD_MUTEX_INITIALIZER),
            m_upsignal_syncput(PTHREAD_COND_INITIALIZER),
            m_upsignal_syncget(PTHREAD_COND_INITIALIZER),
            m_lock_syncput(PTHREAD_MUTEX_INITIALIZER),
            m_lock_syncget(PTHREAD_MUTEX_INITIALIZER),
            m_lock_async_count(PTHREAD_MUTEX_INITIALIZER),
            __ctx(this),
            sub_comm_ptr(NULL)*/{

    m_schedulerMachineID = -1;
    m_workerMachineID = -1;
    m_coordinatorMachineID = -1;

    m_zmqContext=NULL;

    m_machineRole = machineRoleUnknown;
    /*
    m_ringtoken_send = IHWM;
    m_ringtoken_recv = 0;

    ps_callback_func = NULL;
    ps_server_pgasyncfunc=NULL;
    ps_server_putsyncfunc=NULL;
    ps_server_getsyncfunc=NULL;

    m_tablectx = 0;
    m_max_pend = MAX_ZMQ_HWM/10;
    assert(m_max_pend > 10); // too small increase max amq hwm
    m_async_count = 0;
    */
}

//
// Created by 高炜 on 16/12/5.
//

#include "networkInit.h"

// TODO: 创建网络环境，返回通信句柄
sharedCtx* networkInit(int argc, char* argv[]){


    sharedCtx *pshctx = new sharedCtx(mpiRank,params);

    pshctx->m_mpiSize = mpiSize;


    //TODO:
//    pshctx->bind_params(params);
//    pshctx->fork_machagent();

    //TODO: 读入结点信息
    parseNodeFile(params->m_nodeFile, *pshctx);

    //TODO: 验证IP
    std::string validIP;
    utilFindValidIP(validIP, *pshctx);
    LOG(INFO) << "Rank (" << mpiRank << ")'s IP found in node file " << validIP << std::endl;

    //TODO: 读入ps结点，ps链接信息
    if(params->m_psLinkFile.size() > 0 and params->m_psNodeFile.size() > 0){
        parsePsNodeFile(params->m_psNodeFile, *pshctx);
        parsePsLinkFile(params->m_psLinkFile, *pshctx);
        LOG(INFO) << "[*****] ps configuration files(node/link) are parsed" << std::endl;
    }else{
        LOG(INFO) << "No configuration !!!! " << std::endl;
    }

    //TODO: zmq句柄配置
    int io_threads= PETUUM_ZMQ_IO_THREADS;
    zmq::context_t *contextzmq = new zmq::context_t(io_threads);
    pshctx->m_zmqContext = contextzmq;

    //TODO: 读入网络结构并初始化
    if(params->m_topology.compare("star") == 0){
        //TODO: 读入星型网络结构
        parseStarLinkFile(params->m_linkFile, *pshctx);
        //TODO: 创建星型网络
        createStarEthernet(pshctx, *contextzmq, mpiSize, pshctx->nodes[mpiSize-1]->m_IP);
        parseStarLinkFile(params->m_rLinkFile, *pshctx);
        createRingWorkerEthernetAux(pshctx, *contextzmq, mpiSize, pshctx->nodes[mpiSize-1]->m_IP);
        LOG(INFO) << "Star Topology is cretaed with " << mpiSize << " machines (processes) " << std::endl;

    }
    else{
        LOG(INFO) << "Ring Topology is being created" << std::endl;
        parseStarLinkFile(params->m_linkFile, *pshctx);
//        create_ring_ethernet(pshctx, *contextzmq, mpi_size, pshctx->nodes[mpi_size-1]->ip);
    }

    if(params->m_psLinkFile.size() > 0 and params->m_psNodeFile.size() > 0){
        for(int i=0; i < pshctx->m_schedulerMachines; i++){
            if(pshctx->m_mpiRank == pshctx->m_firstSchedulerMachine + i or pshctx->m_machineRole == machineRoleWorker ){
                createPsStarEthernet(pshctx, *contextzmq, mpiSize, pshctx->nodes[pshctx->m_firstSchedulerMachine + i]->m_IP, pshctx->m_firstSchedulerMachine + i);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    LOG(INFO) <<  " EXIT : in stards_init function MPI RANK :  " << mpiRank << std::endl;
    MPI_Finalize();
    return pshctx;
}

// TODO: 比较网卡IP与配置文件中的IP是否一致，返回一致IP
void utilFindValidIP(std::string &validIP, sharedCtx &shctx){
    std::vector<std::string>ipList;
    getIPList(ipList); // get all local abailable ip address
    // try to find matching IP with any IP in the user input.
    // Assumption : user provide correct IP addresses
    // TODO: add more user-error proof code
    for(auto const &p : ipList){
        for(auto const np : shctx.nodes){
            if(!np.second->m_IP.compare(p)){
                validIP.append(p);
                return;
            }
        }
    }
    exit(0);
    return;
}

// TODO: 获得本机网卡的所有可用IP地址
void getIPList(std::vector<std::string> &ipList){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    char *addressBuffer=NULL;
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            //      char addressBuffer[INET_ADDRSTRLEN];
            addressBuffer = (char *)calloc(INET_ADDRSTRLEN + 1, sizeof(char));
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //printf("'%s': %s\n", ifa->ifa_name, addressBuffer);
            ipList.push_back(*(new std::string(addressBuffer)));
        } else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            //      char addressBuffer[INET6_ADDRSTRLEN];
            addressBuffer = (char *)calloc(INET6_ADDRSTRLEN + 1, sizeof(char));
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //printf("'%s': %s\n", ifa->ifa_name, addressBuffer);
            ipList.push_back(*(new std::string(addressBuffer)));
        }
    }
    if (ifAddrStruct!=NULL)
        freeifaddrs(ifAddrStruct);//remember to free ifAddrStruct
    return;
}

