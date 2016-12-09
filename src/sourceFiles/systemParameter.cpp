//
// Created by 高炜 on 16/12/9.
//

#include "../headFiles/systemParameter.h"

DECLARE_string(machineFileName);

/*
DEFINE_string(machineFileName, "singlemach.vm", "MPI machfile");
DEFINE_string(nodeFileName, "", "Node Conf file");
DEFINE_string(starlinkFileName, "", "Link Conf file");
DEFINE_string(ringLinkFileName, "", "Link Conf file");
DEFINE_string(topology, "star", "specify topology : star / ring");
DEFINE_string(psNodeFileName, "", "PS Node Conf file");
DEFINE_string(psLinkFileName, "", "PS Link Conf file");
DEFINE_int64(schedulerMachineNumber, 1, "the number of scheduler machines, by default 1 ");


DECLARE_string(nodeFile);
DECLARE_string(linkFile);
DECLARE_string(ringLinkFile);
DECLARE_string(topology);
DECLARE_string(psNodeFile);
DECLARE_string(psLinkFile);
DECLARE_int32(schedulers);
*/

systemParameter::systemParameter() {
}

systemParameter::systemParameter(std::string &machineFileName,
                                 std::string &nodeFileName,
                                 std::string &starLinkFileName,
                                 std::string &ringLinkFileName,
                                 std::string &topology,
                                 std::string &psNodeFileName,
                                 std::string &psLinkFileName,
                                 int schedulerMachineNumber)
        :memberMachineFileName(machineFileName),
         memberNodeFileName(nodeFileName),
         memberStarLinkFileName(starLinkFileName),
         memberRingLinkFileName(ringLinkFileName),
         memberTopology(topology),
         memberPsNodeFileName(psNodeFileName),
         memberPsLinkFileName(psLinkFileName),
         memberSchedulerNumber(schedulerMachineNumber) {
}

void systemParameter::print(void) {
    LOG(INFO) << " mach file       : " << memberMachineFileName  << std::endl;
    LOG(INFO) << " node file       : " << memberNodeFileName     << std::endl;
    LOG(INFO) << " star link file  : " << memberStarLinkFileName << std::endl;
    LOG(INFO) << " ring link file  : " << memberRingLinkFileName << std::endl;
    LOG(INFO) << " topology        : " << memberTopology         << std::endl;
    LOG(INFO) << " ps node file    : " << memberPsNodeFileName   << std::endl;
    LOG(INFO) << " ps link file    : " << memberPsLinkFileName   << std::endl;
    LOG(INFO) << " scheduler number: " << memberSchedulerNumber  << std::endl;
}

void systemParameter::init(int argc,char* argv[],int mpiRank, int mpiSize){
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();
    gflags::ParseCommandLineFlags(&argc,&argv,true);
    FLAGS_logtostderr = 1;

    this->memberMachineFileName.append((const char*) FLAGS_machineFileName);
    this->createConfigFile(mpiRank,mpiSize);

}

void systemParameter::createConfigFile(int mpiRank, int mpiSize) {
    //TODO: 创建配置文件
    mkdir("./conf", 0777);
    char *nodeFileName     = (char *)calloc(sizeof(char), 128);
    char *starLinkFileName = (char *)calloc(sizeof(char), 128);
    char *ringLinkFileName = (char *)calloc(sizeof(char), 128);
    char *psNodeFileName   = (char *)calloc(sizeof(char), 128);
    char *psLinkFileName   = (char *)calloc(sizeof(char), 128);
    sprintf(nodeFileName,   "./conf/node_m%d", mpiRank);
    sprintf(starLinkFileName,  "./conf/starlink_m%d", mpiRank);
    sprintf(ringLinkFileName,  "./conf/ringlink_m%d", mpiRank);
    sprintf(psNodeFileName, "./conf/psnode_m%d", mpiRank);
    sprintf(psLinkFileName, "./conf/pslink_m%d", mpiRank);

    this->memberNodeFileName.append((const char *)nodeFileName);
    this->memberStarLinkFileName.append((const char *)starLinkFileName);
    this->memberRingLinkFileName.append((const char *)ringLinkFileName);
    this->memberPsNodeFileName.append((const char *)psNodeFileName);
    this->memberPsLinkFileName.append((const char *)psLinkFileName);

    //TODO: 读入结点配置文件
    std::vector<machineNode *> machineNodes;
    int nodeCount = 0;

    std::ifstream in(memberMachineFileName);
    std::string delimiter(CONF_FILE_DELIMITER);
    std::string lineBuffer;
    while(getline(in, lineBuffer)){
        std::vector<std::string>parts;
        utilGetTokens(lineBuffer, delimiter, parts);
        if(!parts[0].compare("#"))  // skip comment line
            continue;

        machineNode *tmp = new machineNode;
        tmp->memberIP.append(parts[0]);
        tmp->memberID = nodeCount;
//        tmp->functionName.append("default-func");
        machineNodes.push_back(tmp);
        nodeCount++;
    }

    //TODO: 生成结点信息配置文件
    std::ofstream fout(memberNodeFileName);
    for(int i=0; i < machineNodes.size(); i++){
        fout << machineNodes[i]->memberIP << " " << machineNodes[i]->memberID /*<< " " << machineNodes[i]->funcname.c_str()*/ << std::endl;
    }
    fout.close();

    //TODO: 生成星型拓扑配置文件
    fout.open(memberStarLinkFileName);
    int cdRank = mpiSize -1;
    int srcPort = SRCPORT_BASE ;
    int dstPort = DETPORT_BASE ;
    for(int i = 0; i < cdRank; i++){
        fout << cdRank << " " << srcPort++ << " " << i << " " << dstPort++ << std::endl;
    }
    for(int i = 0; i < cdRank; i++){
        fout << i << " " << srcPort++ << " " << cdRank << " " << dstPort++ << std::endl;
    }
    fout.close();

    //TODO: 生成环型拓扑配置文件
    fout.open(memberRingLinkFileName);
    int workers = mpiSize - memberSchedulerNumber - 1;
    int schedulers = memberSchedulerNumber;
    for(int i = workers; i < workers+schedulers; i++){
        fout << i << " " << srcPort++ << " " << cdRank << " " << dstPort++ << std::endl;
    }
    for(int i = workers; i < workers+schedulers; i++){
        fout << cdRank << " " << srcPort++ << " " << i << " " << dstPort++ << std::endl;
    }
    fout << cdRank << " " << srcPort++ << " " << 0 << " " << dstPort++ << std::endl;
    for(int i = 0; i < workers; i++){
        if(i < workers - 1){
            fout << i << " " << srcPort++ << " " << i + 1 << " " << dstPort++ << std::endl;
        }else{
            fout << i << " " << srcPort++ << " " << cdRank << " " << dstPort++ << std::endl;
        }
    }
    for(int i = 0; i < workers; i++){
        if(i == 0 ){
            fout << i << " " << srcPort++ << " " << cdRank << " " << dstPort++ << std::endl;
        }else{
            fout << i << " " << srcPort++ << " " << i - 1 << " " << dstPort++ << std::endl;
        }
    }
    fout << cdRank << " " << srcPort++ << " " << workers-1 << " " << dstPort++ << std::endl;
    fout.close();

    //TODO: 生成ps结点配置文件
    fout.open(memberPsNodeFileName);
    for(int i = 0; i < machineNodes.size()-1; i++){
        fout << machineNodes[i]->memberIP << " " << machineNodes[i]->memberID /*<< " " << machineNodes[i]->funcname.c_str()*/ << std::endl;
    }
    fout.close();

    //TODO: 生成ps链接配置文件
    fout.open(memberPsLinkFileName);
    int clients = mpiSize - memberSchedulerNumber - 1;
    int servers = memberSchedulerNumber;
    for(int i = 0; i < clients; i++){
        for(int j = clients; j < clients+servers; j++)
            fout << i << " " << srcPort++ << " " << j << " " << dstPort++ << std::endl;
    }
    for(int i=0; i<clients; i++){
        for(int j=clients; j< clients+servers; j++)
            fout << j << " " << srcPort++ << " " << i << " " << dstPort++ << std::endl;
    }
    fout.close();

    MPI_Barrier(MPI_COMM_WORLD);
}

void systemParameter::utilGetTokens(const std::string& str, const std::string& delimiter, std::vector<std::string>& tokens) {
    size_t start, end = 0;
    while (end < str.size()) {
        start = end;
        while (start < str.size() && (delimiter.find(str[start]) != std::string::npos)) {
            start++;
            // skip initial empty space
        }
        end = start;
        while (end < str.size() && (delimiter.find(str[end]) == std::string::npos)) {
            end++;
            //skip end of few words
        }
        if (end-start != 0) {  // ignore zero string.
            tokens.push_back(std::string(str, start, end-start));
        }
    }
}