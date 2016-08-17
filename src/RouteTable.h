//
// Created by maxxie on 16-7-27.
//

#ifndef SHARPVPN_ROUTETABLE_H
#define SHARPVPN_ROUTETABLE_H
#include <cstdint>
#include <string>
#include "Sharp.h"

#ifdef TARGET_WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <Iphlpapi.h>
#endif


typedef struct RouteRow {
    std::string dst;
    std::string netmask;
    int netmask_sharp = -1;
    std::string gateway;
	dev_type dev;
} RouteRow;

class RouteTable {
private:
#ifdef TARGET_WIN32
	PMIB_IPFORWARDROW default_route_row = NULL;
	MIB_IPFORWARDROW default_origin_backup;
	MIB_IPFORWARDROW tunnel_route;
#endif // TARGET
	bool modified = false;
public:
    static RouteRow get_default_route();
    int add_route_row(RouteRow row, bool host);
#ifdef TARGET_WIN32
	int add_route_row_complete(RouteRow row, bool host);
#endif
#ifndef TARGET_WIN32
	std::string tunnel_gateway;
    std::string default_gateway;
	dev_type default_dev;
#endif
    int del_route_row(RouteRow row);
    int set_default_as_tunnel(dev_type tunnel_name, std::string tunnel_ip);
    int set_chnroute(std::string filename, std::string remote_ip);
	int delete_chnroute(std::string filename, std::string remote_ip);
	int init_table();
};


#endif //SHARPVPN_ROUTETABLE_H
