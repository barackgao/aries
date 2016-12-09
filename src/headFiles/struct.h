//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_STRUCT_H
#define ARIES_STRUCT_H

enum machineRole {
    machineRoleWorker,
    machineRoleCoordinator,
    machineRoleScheduler,
//    machineRoleSchedulerCoordinator,
    machineRoleUnknown
};

#include <stdint.h>

#define USER_MSG_SIZE (16*1024-72)

//the following are UBUNTU/LINUX ONLY terminal color codes.
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


/*
enum uobj_type {
    m_uobj_update,
    m_uobj_objetc
};

 /*
typedef struct{
    int64_t id;
    double value;
}idval_pair;

typedef struct {
    int64_t size;
    int64_t phaseno;
    uobj_type  type; // user defined type that allows user to parse the data entry
    void *data;
}staleinfo;
*/

enum mtype {
    MSG_MR,
    MSG_DONE,
    MSG_USER
};
enum message_type{
    SYSTEM_HB,
    USER_DATA,
    SYSTEM_DSHARD,
    SYSTEM_SCHEDULING,
    USER_WORK,
    USER_UPDATE,
    USER_PROGRESS_CHECK
};
typedef struct{
    mtype type;
    unsigned int long seqno;
    unsigned int long timestamp;
    long cmdid;
    message_type msg_type;
    int src_rank;
    int dst_rank;
    unsigned int long len; // user can specify lentgh of valid bytes in data
    int64_t remain;
    char data[USER_MSG_SIZE];
}mbuffer;
#endif //ARIES_STRUCT_H
