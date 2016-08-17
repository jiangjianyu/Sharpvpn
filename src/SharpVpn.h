//
// Created by maxxie on 16-7-20.
//

#ifndef SHARPVPN_SHARPVPN_H
#define SHARPVPN_SHARPVPN_H

#include <string>
#include <map>
#include "SharpTunnel.h"
#include "SharpCrypto.h"
#include "RouteTable.h"
#include "rudp/ReliableSocket.h"
#include "chinadns/chinadns-cpp.h"

typedef enum SharpvpnMode {
    SharpvpnModeServer,
    SharpvpnModeClient
} SharpvpnMode;

typedef enum VpnStatus {
	VpnNotRunning,
	VpnConnectingServer,
	VpnConnected,
	VpnClosed
}VpnStatus;

typedef struct VpnArgs {
    SharpvpnMode mode;
    std::string bind_host;
    int bind_port;
    std::string tunnel_ip;
    std::string tunnel_name;
    int mtu;
    int download_speed;
    int upload_speed;
    std::string encryption_key;
    std::string pid_file;
    std::string log_file;
    std::string chnroute_file;
    std::string chinadns_host;
	std::string token;
    bool error;
}VpnArgs;

typedef struct SharpUserInfo {
	uint32_t ip;
} SharpUserInfo;

typedef void status_handler(void *old_in, VpnStatus status);

class SharpVpn {
private:
    std::string host;
    int port;
    VpnArgs vpn_args;
	VpnStatus vpn_status = VpnNotRunning;
	status_handler *handler = nullptr;
	void *handler_in = nullptr;
    SharpTunnel *tunnel;
    ReliableSocket *sock;
//    SharpCrypto *crypto;
    Chinadns chinadns;
	RouteTable table;
    bool running;

public:
	std::map<std::string, SharpUserInfo> user_map;
	std::map<uint32_t, ReliableSocket*> ip_map;
	int ip_used = 0;
	uint32_t get_useable_ip();

    SharpvpnMode mode;
    SharpVpn(VpnArgs args);
	SharpTunnel *get_tunnel() {return tunnel;}
//	SharpCrypto *get_crypto() {return crypto;}
    int init();
    int run();
	void init_status_change_handler(status_handler *_handler, void *in) { handler = _handler; handler_in = in; }
    static VpnArgs parse_file_args(std::string file);
    static int refresh_args_file(VpnArgs args, std::string file);
    static VpnArgs get_default_args();
    int stop();
    int get_status() { return running; }
    ~SharpVpn();
};



#endif //SHARPVPN_SHARPVPN_H
