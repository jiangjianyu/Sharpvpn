//
// Created by maxxie on 16-7-26.
//

#include <fstream>
#include <signal.h>
#include "DaemonRuntime.h"
#include "Sharp.h"
#ifndef TARGET_WIN32
static void sig_handler_exit(int signo) {
    exit(0);
}

int DaemonRuntime::write_pid_file() {
    pid_t pid_read;
    std::ifstream fs(pid_file);
    if (!fs) {
        if ( fs >> pid_read) {
            LOG(ERROR) << "already running program, pid " << pid;
            return -1;
        }
    }
    fs.close();
    std::ofstream fos(pid_file, std::ios_base::out);
    if (!fos) {
        LOG(ERROR) << "error opening file " << pid_file;
        return -1;
    }
    if (!(fos << pid)) {
        LOG(ERROR) << "error writing to file " << pid_file;
        return -1;
    }
    fos.close();
    return 0;
}

int DaemonRuntime::stop() {
    int stopped;
    std::ifstream fs(pid_file);
    pid_t pid;
    if (!fs) {
        return -1;
    }
    if (!(fs >> pid)) {
        return -1;
    }
    if (pid > 0) {
        // make sure pid is not zero or negative
        if (0 != kill(pid, SIGTERM)){
            if (errno == ESRCH) {
                LOG(INFO) << "not running";
                return 0;
            }
            return -1;
        }
        stopped = 0;
        // wait for maximum 10s
        for (int i = 0; i < 200; i++) {
            if (-1 == kill(pid, 0)) {
                if (errno == ESRCH) {
                    stopped = 1;
                    break;
                }
            }
            // sleep 0.05s
            msleep(50);
        }
        if (!stopped) {
            return -1;
        }
        LOG(INFO) << "Stopped";
        if (0 != unlink(pid_file.c_str())) {
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}

int DaemonRuntime::start() {
    pid = fork();
    if (pid == -1) {
        LOG(ERROR) << "Error forking";
        return -1;
    }
    if (pid > 0) {
        // let the child print message to the console first
        signal(SIGINT, sig_handler_exit);
        sleep(5);
        exit(0);
    }

    pid_t ppid = getppid();
    pid = getpid();
    if (0 != write_pid_file()) {
        kill(ppid, SIGINT);
        return -1;
    }

    setsid();
    signal(SIGHUP, SIG_IGN);

    // print on console
    LOG(INFO) << "started";
    kill(ppid, SIGINT);

    // then rediret stdout & stderr
    fclose(stdin);
    FILE *fp;
    fp = freopen(log_file.c_str(), "a", stdout);
    if (fp == NULL) {
        LOG(ERROR) << "error redirecting stdout";
        return -1;
    }
    fp = freopen(log_file.c_str(), "a", stderr);
    if (fp == NULL) {
        LOG(ERROR) << "error redirecting stderr";
        return -1;
    }

    return 0;
}
#else
int DaemonRuntime::start() {
	return 0;
}

int DaemonRuntime::stop() {
	return 0;
}

int DaemonRuntime::write_pid_file() {
	return 0;
}

#endif // TARGET_WIN32


DaemonRuntime::DaemonRuntime(std::string pid_f, std::string log_f) {
    pid_file = pid_f;
    log_file = log_f;
}