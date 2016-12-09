//
// Created by 高炜 on 16/12/9.
//

#include "../headFiles/systemParameter.h"


DEFINE_string(machineFile, "singlemach.vm", "MPI machfile");
DEFINE_string(nodeFile, "", "Node Conf file");
DEFINE_string(linkFile, "", "Link Conf file");
DEFINE_string(ringLinkFile, "", "Link Conf file");
DEFINE_string(topology, "star", "specify topology : star / ring");
DEFINE_string(psNodeFile, "", "PS Node Conf file");
DEFINE_string(psLinkFile, "", "PS Link Conf file");
DEFINE_int64(schedulers, 1, "the number of scheduler machines, by default 1 ");

/*
DECLARE_string(machineFile);
DECLARE_string(nodeFile);
DECLARE_string(linkFile);
DECLARE_string(ringLinkFile);
DECLARE_string(topology);
DECLARE_string(psNodeFile);
DECLARE_string(psLinkFile);
DECLARE_int32(schedulers);
*/

void systemParameter::print(void) {
    LOG(INFO) << " mach file : " << m_machineFile;
    LOG(INFO) << " node file  : " << m_nodeFile;
    LOG(INFO) << " star file  : " << m_linkFile;
    LOG(INFO) << " rlink file : " << m_rLinkFile;
    LOG(INFO) << " topology : " << m_topology;
    LOG(INFO) << " ps node file : " << m_psNodeFile;
    LOG(INFO) << " ps link file : " << m_psLinkFile;
    LOG(INFO) << " schedulers : " << m_schedulers;
    LOG(INFO) << " machstring : ";
    std::map<machineType, std::string>::iterator iter = m_machineString.begin();
    while (iter != m_machineString.end()) {
        LOG(INFO) << "type: " << iter->first << " string: " << iter->second;
        iter++;
    }
}

systemParameter* systemParameter::init(int argc,char* argv[],int mpiRank){
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();
    gflags::ParseCommandLineFlags(&argc,&argv,true);
    FLAGS_logtostderr = 1;

    systemParameter *sysparam = new systemParameter(FLAGS_machineFile,
                                      FLAGS_nodeFile,
                                      FLAGS_linkFile,
                                      FLAGS_ringLinkFile,
                                      FLAGS_topology,
                                      FLAGS_psNodeFile,
                                      FLAGS_psLinkFile,
                                      FLAGS_schedulers);

    if(mpiRank == 0){
        sysparam->print();
    }

    return sysparam;
}