//
// Created by 高炜 on 16/12/9.
//
/*
#ifndef ARIES_SYSTEMPARAMETER_H
#define ARIES_SYSTEMPARAMETER_H
//
// Created by 高炜 on 16/12/5.
//

#ifndef PETUUM_MODIFY_SYSPARAM_H
#define PETUUM_MODIFY_SYSPARAM_H

#include <map>

#include "../../thirdParty/usr/include/glog/logging.h"
#include "../../thirdParty/usr/include/gflags/gflags.h"

enum machineType { m_coordinator=300, m_scheduler, m_worker };

class systemParameter{
public:
    std::string m_machineFile; //系统－结点文件
    std::string m_nodeFile; //系统
    std::string m_linkFile; //系统
    std::string m_rLinkFile; //系统
    std::string m_topology; //系统－拓扑
    std::string m_psNodeFile;  // by system -- from mach file
    std::string m_psLinkFile;  // by system -- from mach file
    std::map<machineType, std::string> m_machineString;
    int64_t m_schedulers;

    systemParameter(std::string &machineFile,
             std::string &nodeFile,
             std::string &linkFile,
             std::string &rLinkFile,
             std::string &topology,
             std::string &psNodeFile,
             std::string &psLinkFile,
             int64_t schedulers)
            :m_machineFile(machineFile),
             m_nodeFile(nodeFile),
             m_linkFile(linkFile),
             m_rLinkFile(rLinkFile),
             m_topology(topology),
             m_psNodeFile(psNodeFile),
             m_psLinkFile(psLinkFile),
             m_schedulers(schedulers) {
        m_machineString.insert(std::pair<machineType, std::string>(m_coordinator, "coordinator"));
        m_machineString.insert(std::pair<machineType, std::string>(m_scheduler, "scheduler"));
        m_machineString.insert(std::pair<machineType, std::string>(m_worker, "worker"));
    }

    void print(void);
    systemParameter* init(int argc,char* argv[],int mpiRank);
};

//sysParam* paramInit(int argc,char* argv[],int mpiRank);

#endif //PETUUM_MODIFY_SYSPARAM_H

#endif //ARIES_SYSTEMPARAMETER_H
