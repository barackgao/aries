//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_NETWORK_H
#define ARIES_NETWORK_H
//
// Created by 高炜 on 16/12/5.
//

#ifndef PETUUM_MODIFY_SHAREDCTX_H
#define PETUUM_MODIFY_SHAREDCTX_H

//#include <pthread.h>
#include <list>

#include "sysParam.h"
#include "machineNode.h"
#include "machineLink.h"
#include "struct.h"
#include "_ringport.h"

#include "thirdParty/zeromq/include/zmq.h"
#include "thirdParty/zeromq/include/zmq.hpp"

#define MAX_ZMQ_HWM (5000)
//#define IHWM (MAX_ZMQ_HWM - 5)

class sharedCtx{
public:
    std::map<int, machineNode *> nodes; // machine nodes
    std::map<int, machineLink *> links; // node graph links
//    std::map<int, machineLink *> rlinks; // node graph links

    std::map<int, machineNode *> psNodes; // machine nodes for ps configuration
    std::map<int, machineLink *> psLinks; // link for ps configuration

    int m_mpiRank;                     // machine id
    machineRole m_machineRole;
    sysParam *m_sysParam;

    int m_schedulerMachineID; // scheduler id from 0 to sizeof(schedulers)-1
    int m_workerMachineID;    // worker id from 0 to sizeof(workers)-1
    int m_coordinatorMachineID; // coordinator id from 0 to sizeof(coordinator)-1

    int m_schedulerMachines;
    int m_firstSchedulerMachine;
    int m_workerMachines;
    int m_firstWorkerMachine;
    int m_firstCoordinatorMachine; // first one and last one since only one coordinator now
    int m_mpiSize;

    zmq::context_t *m_zmqContext;

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

    machineRole findRole(int mpiSize);
    machineRole findRole(int mpiSize, int nodeNumber);
    void printLink(void);
    void printNode(void);
    void printPsLink(void);
    void printPsNode(void);

/*
    pthread_mutex_t m_sendPortLock; // for flag, not for port
    pthread_mutex_t m_recvPortLock; // for flag, not for port
    bool m_sendPortFlag;
    bool m_recvPortFlag;


    // fast ring stuff
    pthread_mutex_t m_fastSendPortLock; // for flag, not for port
    pthread_mutex_t m_fastRecvPortLock; // for flag, not for port
    bool m_fastSendPortFlag;
    bool m_fastRecvPortFlag;
    class _ringport *ringFastSentPort; // once initialized, no more change
    class _ringport *ringFastRecvPort; // once initialized, no more change

    // star topology stuff -- can be used for any other topology as well.
    pthread_mutex_t m_starSendPortLock; // for flag, not for port
    pthread_mutex_t m_starRecvPortLock; // for flag, not for port
    bool m_starSendPortFlag;
    bool m_starRecvPortFlag;
    pthread_mutex_t m_starSend; // for flag, not for port
    pthread_mutex_t m_starRecv; // for flag, not for port


    std::map<int, _ringport *> coordinatorSendPortMap; // workers and scheduler
    std::map<int, _ringport *> coordinatorRecvPortMap; // workers and scheduler





    idval_pair *m_idvalp_buf;

//    std::map<std::string, dshardctx *>m_shardmap; // alias and dshardctx mapping
    std::map<int64_t, staleinfo *>m_stale;

    pthread_mutex_t m_freethrds_lock;
    int64_t m_freethrds;
    schedulerMachineCtx *m_schedulerCtx;
    coordinatorMachineCtx *m_coordinatorCtx;
    workerMachineCtx *m_workerCtx;

    pthread_mutex_t m_ringTokenSendLock;
    pthread_mutex_t m_ringTokenRecvLock;
    int64_t m_ringTokenSend;
    int64_t m_ringTokenRecv;

    pthread_cond_t m_upsignal_syncput;
    pthread_cond_t m_upsignal_syncget;
    pthread_mutex_t m_lock_syncput;
    pthread_mutex_t m_lock_syncget;

    long m_max_pend;
    long m_async_count;
    pthread_mutex_t m_lock_async_count;
    sharedCtx *__ctx;
    void *sub_comm_ptr;

    void (*ps_callback_func)(sharedCtx *, std::string &, std::string &);
    void (*ps_server_pgasyncfunc)(std::string &, std::string &, sharedCtx *);
    void (*ps_server_putsyncfunc)(std::string &, std::string &, sharedCtx *);
    void (*ps_server_getsyncfunc)(std::string &, std::string &, sharedCtx *);

    void *m_tablectx;
*/

};

#endif //PETUUM_MODIFY_SHAREDCTX_H

#endif //ARIES_NETWORK_H
