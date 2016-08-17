#include <iostream>

using namespace std;
#include "SharpTunnel.h"
#include "SharpVpn.h"
#include "DaemonRuntime.h"
#include <csignal>

SharpVpn *vpn;
#ifndef TARGET_WIN32
static void sig_handler(int signo) {
    vpn->stop();
}
#endif

int main(int argc, char **args) {
#ifdef TARGET_WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		std::cout << "WSAStartup failed with error " << err << "\n";
		return 1;
	}
#endif // TARGET_WIN32
    unsigned char buf[40];
    FLAGS_log_dir = "./";
    google::InitGoogleLogging(args[0]);
    google::InstallFailureSignalHandler();
    FLAGS_logtostderr = 1;
    VpnArgs vpn_args;
    vpn_args.error = true;
    DaemonMode daemon_mode = daemon_null;
    for (int i = 1;i < argc;i++) {
        if (!strcmp(args[i], "-c")) {
            vpn_args = SharpVpn::parse_file_args(args[i+1]);
            i++;
        } else if(!strcmp(args[i], "-d")) {
            if (!strcmp(args[i+1], "start")) {
                daemon_mode = daemon_start;
            } else if (!strcmp(args[i+1], "stop")) {
                daemon_mode = daemon_stop;
            } else if (!strcmp(args[i+1], "restart")) {
                daemon_mode = daemon_restart;
            } else {
                LOG(ERROR) << "wrong para " << args[i];
                return -1;
            }
            i++;
        }
    }
    if (vpn_args.error) {
        LOG(ERROR) << "not enough para, type -h for help";
        return -1;
    }
    DaemonRuntime runtime = DaemonRuntime(vpn_args.pid_file, vpn_args.log_file);
    if (daemon_mode == daemon_stop) {
        runtime.stop();
        return -1;
    }
    if (daemon_mode == daemon_restart) {
        runtime.stop();
    }
    vpn = new SharpVpn(vpn_args);
	if (vpn->init() < 0)
		return -1;
    if (daemon_mode == daemon_start || daemon_mode == daemon_restart) {
        runtime.start();
    }
#ifndef TARGET_WIN32
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
#endif
    vpn->run();
    return 0;
}