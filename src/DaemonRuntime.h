//
// Created by maxxie on 16-7-26.
//

#ifndef SHARPVPN_DAEMONRUNTIME_H
#define SHARPVPN_DAEMONRUNTIME_H


#include <string>
#include "Sharp.h"
typedef enum {
    daemon_null,
    daemon_start,
    daemon_stop,
    daemon_restart
}DaemonMode;

class DaemonRuntime {
private:
    std::string pid_file;
    std::string log_file;
#ifndef TARGET_WIN32
    pid_t pid;
#endif
    int write_pid_file();
public:
    DaemonRuntime(std::string pid_file, std::string log_file);
    int start();
    int stop();
};


#endif //SHARPVPN_DAEMONRUNTIME_H
