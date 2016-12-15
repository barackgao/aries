//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_SYSTEMPARAMETER_H
#define ARIES_SYSTEMPARAMETER_H

#include <map>
#include <sys/stat.h>
#include <fstream>

#include "../../thirdParty/usr/include/glog/logging.h"
#include "../../thirdParty/usr/include/gflags/gflags.h"
#include "../../thirdParty/usr/include/gflags/gflags_declare.h"
#include "../../thirdParty/usr/include/mpi.h"

#include "machineNode.h"
#include "assistFunction.h"

DECLARE_string(machineFileName);
DECLARE_string(nodeFileName);
DECLARE_string(starlinkFileName);
DECLARE_string(ringLinkFileName);
DECLARE_string(topology);
DECLARE_string(psNodeFileName);
DECLARE_string(psLinkFileName);
DECLARE_int64(schedulerMachineNumber);

#define CONF_FILE_DELIMITER "\n \t"
#define SRCPORT_BASE (47000)
#define DETPORT_BASE (38000)

enum machineType { memberCoordinator=300, memberScheduler, memberWorker };

class systemParameter{
public:
    //TODO: 成员变量
    std::string memberMachineFileName; //系统－结点文件
    std::string memberNodeFileName; //系统
    std::string memberStarLinkFileName; //系统
    std::string memberRingLinkFileName; //系统
    std::string memberTopology; //系统－拓扑
    std::string memberPsNodeFileName;  // by system -- from mach file
    std::string memberPsLinkFileName;  // by system -- from mach file
    int memberSchedulerNumber;

    //TODO: 构造器
    systemParameter();
    systemParameter(std::string &machineFileName,
                    std::string &nodeFileName,
                    std::string &linkFileName,
                    std::string &ringLinkFileName,
                    std::string &topology,
                    std::string &psNodeFileName,
                    std::string &psLinkFileName,
                    int schedulerNumber);

    //TODO: 成员函数
    void print(void);
    void init(int argc,char* argv[],int mpiRank, int mpiSize);
    void createConfigFile(int mpiRank, int mpiSize);
};

#endif //ARIES_SYSTEMPARAMETER_H
