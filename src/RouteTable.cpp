//
// Created by maxxie on 16-7-27.
//


#include "RouteTable.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "Sharp.h"

#ifndef TARGET_WIN32
#include <sys/socket.h>
#include <net/route.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#endif

#if defined(TARGET_LINUX) || defined(TARGET_MACOSX)
int RouteTable::set_default_as_tunnel(dev_type tunnel_name, std::string _tunnel_gateway) {
    tunnel_gateway = _tunnel_gateway;
	RouteRow row;
	row.dst = "0.0.0.0";
    row.netmask_sharp = 1;
	row.gateway = tunnel_gateway;
	row.netmask = "128.0.0.0";
	row.dev = tunnel_name;
	add_route_row(row, false);
	row.dst = "128.0.0.0";
	add_route_row(row, false);
	return 0;
}

int RouteTable::init_table() {
    /* it's not necessary to init the route table in mac os */
    return 0;
}

#endif

#ifdef TARGET_LINUX
RouteRow RouteTable::get_default_route() {
    RouteRow row;
    std::vector<RouteRow> row_list;
    std::ifstream ifs("/proc/net/route");
    if (!ifs) {
        row.dst = "";
        return row;
    }
    char buf[1500];
    unsigned long last;
    while (true) {
        if(!ifs.getline(buf, 1500)) {
            break;
        }
        std::vector<std::string> arr;
        std::string tem(buf);
        last = tem.find_last_not_of(' ');
        tem = tem.substr(0, last);
        unsigned long start = 0;

        for (unsigned long i = 0;i < tem.length();i++) {
            if (tem[i] != '\t')
                continue;
            if (i - start > 0) {
                arr.push_back(tem.substr(start, i - start));
            }
            start = i + 1;
        }
        row.dev = arr[0];
        row.dst = arr[1];
        row.gateway = arr[2];
        row.netmask = arr[7];
        row_list.push_back(row);
    }
    row.dst = "";
    for (std::vector<RouteRow>::iterator itr = row_list.begin();itr != row_list.end();++itr) {
        if (itr->dst == "00000000") {
            row.dst = "0.0.0.0";
            row.dev = itr->dev;
            struct in_addr in;
            in.s_addr = strtol(itr->gateway.c_str(), NULL, 16);
            row.gateway = inet_ntoa(in);
            in.s_addr = strtol(itr->netmask.c_str(), NULL, 16);
            row.netmask = inet_ntoa(in);
        }
    }
    return row;
}

int RouteTable::add_route_row(RouteRow row, bool host) {
    struct rtentry route;
    memset(&route, 0, sizeof(rtentry));
    struct sockaddr_in *dstaddr = (sockaddr_in*)&route.rt_dst;
    struct sockaddr_in *gateway = (sockaddr_in*)&route.rt_gateway;
    struct sockaddr_in *netmask = (sockaddr_in*)&route.rt_genmask;
    int sock_f = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (inet_pton(AF_INET, row.dst.c_str(), &dstaddr->sin_addr) < 0) {
        LOG(ERROR) << "invalid ipaddr " << row.dst;
        return -1;
    }
    if (inet_pton(AF_INET, row.gateway.c_str(), &gateway->sin_addr) < 0) {
        LOG(ERROR) << "invalid gateway " << row.gateway;
        return -1;
    }
    if (row.netmask_sharp == -1) {
        if (inet_pton(AF_INET, row.netmask.c_str(), &netmask->sin_addr) < 0) {
            LOG(ERROR) << "invalid netmask " << row.netmask;
            return -1;
        }
    } else {
        netmask->sin_addr.s_addr = 0;
        for (int i = 0;i < row.netmask_sharp;i++) {
            netmask->sin_addr.s_addr |= 1 << (31 - i);
        }
        netmask->sin_addr.s_addr = htonl(netmask->sin_addr.s_addr);
    }
    dstaddr->sin_family = AF_INET;
    gateway->sin_family = AF_INET;
    netmask->sin_family = AF_INET;
    route.rt_flags = RTF_UP | RTF_GATEWAY;
    if (host) {
        route.rt_flags |= RTF_GATEWAY | RTF_HOST;
    }
    route.rt_metric = 100;
    route.rt_dev = new char[row.dev.length() + 1];
    memcpy(route.rt_dev, row.dev.c_str(), row.dev.length() + 1);
    if (ioctl(sock_f, SIOCADDRT, &route) < 0) {
        //already exists
        if (errno == 17)
            return 0;
        LOG(ERROR) << "Error adding route " << row.dst << " errno: " << errno;
        return -1;
    }
    return 0;
}

int RouteTable::del_route_row(RouteRow row) {
    struct rtentry route;
    memset(&route, 0, sizeof(rtentry));
    struct sockaddr_in *dstaddr = (sockaddr_in*)&route.rt_dst;
    struct sockaddr_in *gateway = (sockaddr_in*)&route.rt_gateway;
    struct sockaddr_in *netmask = (sockaddr_in*)&route.rt_genmask;
    int sock_f = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (inet_pton(AF_INET, row.dst.c_str(), &dstaddr->sin_addr) < 0) {
        LOG(ERROR) << "invalid ipaddr " << row.dst;
        return -1;
    }
    if (inet_pton(AF_INET, row.gateway.c_str(), &gateway->sin_addr) < 0) {
        LOG(ERROR) << "invalid gateway " << row.gateway;
        return -1;
    }
    if (row.netmask_sharp == -1) {
        if (inet_pton(AF_INET, row.netmask.c_str(), &netmask->sin_addr) < 0) {
            LOG(ERROR) << "invalid netmask " << row.netmask;
            return -1;
        }
    } else {
        netmask->sin_addr.s_addr = 0;
        for (int i = 0;i < row.netmask_sharp;i++) {
            netmask->sin_addr.s_addr |= 1 << (31 - i);
        }
        netmask->sin_addr.s_addr = htonl(netmask->sin_addr.s_addr);
    }
    dstaddr->sin_family = AF_INET;
    gateway->sin_family = AF_INET;
    netmask->sin_family = AF_INET;
    route.rt_dev = new char[row.dev.length() + 1];
    memcpy(route.rt_dev, row.dev.c_str(), row.dev.length() + 1);
    if (ioctl(sock_f, SIOCDELRT, &route) < 0) {
        //already exists
        if (errno == 17)
            return 0;
        LOG(ERROR) << "Error deleteing route " << row.dst << " errno: " << errno;
        return -1;
    }
    return 0;
}

#endif

#ifdef TARGET_WIN32
#define PROTO_TYPE_UCAST     0
#define PROTOCOL_ID(Type, VendorId, ProtocolId) (((Type & 0x03)<<30)|((VendorId & 0x3FFF)<<16)|(ProtocolId & 0xFFFF))
#define PROTO_VENDOR_ID      0xFFFF

int RouteTable::init_table() {
	PMIB_IPFORWARDTABLE p_ip_forward_table = NULL;
	DWORD dwSize = 0;
	BOOL bOrder = FALSE;
	DWORD dwStatus = 0;

	dwStatus = GetIpForwardTable(p_ip_forward_table, &dwSize, bOrder);
	if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
		// Allocate the memory for the table
		if (!(p_ip_forward_table = (PMIB_IPFORWARDTABLE)malloc(dwSize))) {
			LOG(ERROR) << "Malloc failed. Out of memory.";
			return -1;
		}
		// Now get the table.
		dwStatus = GetIpForwardTable(p_ip_forward_table, &dwSize, bOrder);
	}
	if (dwStatus != ERROR_SUCCESS) {
		LOG(ERROR) << "getIpForwardTable failed.";
		if (p_ip_forward_table)
			free(p_ip_forward_table);
		return -1;
	}

	default_route_row = (PMIB_IPFORWARDROW)malloc(sizeof(MIB_IPFORWARDROW));
	if (!default_route_row) {
		LOG(ERROR) << "error allocating mem";
		return -1;
	}
	for (int i = 0; i < p_ip_forward_table->dwNumEntries; i++) {
		if (p_ip_forward_table->table[i].dwForwardDest == 0) {
			memcpy(default_route_row, &(p_ip_forward_table->table[i]),
				sizeof(MIB_IPFORWARDROW));
		}
	}
	memcpy(&default_origin_backup, default_route_row,
		sizeof(MIB_IPFORWARDROW));
	modified = true;
	return 0;
}

int RouteTable::add_route_row_complete(RouteRow row, bool host) {
	PMIB_IPFORWARDTABLE p_ip_forward_table = NULL;
	PMIB_IPFORWARDROW p_row = NULL;
	DWORD dwSize = 0;
	BOOL bOrder = FALSE;
	DWORD dwStatus = 0;

	dwStatus = GetIpForwardTable(p_ip_forward_table, &dwSize, bOrder);
	if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
		// Allocate the memory for the table
		if (!(p_ip_forward_table = (PMIB_IPFORWARDTABLE)malloc(dwSize))) {
			LOG(ERROR)<<"Malloc failed. Out of memory.";
			return -1;
		}
		// Now get the table.
		dwStatus = GetIpForwardTable(p_ip_forward_table, &dwSize, bOrder);
	}
	if (dwStatus != ERROR_SUCCESS) {
		LOG(ERROR) << "getIpForwardTable failed.";
		if (p_ip_forward_table)
			free(p_ip_forward_table);
		return -1;
	}
	in_addr gateway_in, dst_in, netmask_in;
	if (inet_pton(AF_INET, row.gateway.c_str(), &gateway_in) < 0) {
		LOG(ERROR) << "invalid gateway ip " << row.gateway;
		return -1;
	}
	if (inet_pton(AF_INET, row.dst.c_str(), &dst_in) < 0) {
		LOG(ERROR) << "invalid gateway ip " << row.dst;
		return -1;
	}
	if (row.netmask_sharp == -1) {
		if (inet_pton(AF_INET, row.netmask.c_str(), &netmask_in) < 0) {
			LOG(ERROR) << "invalid netmask " << row.netmask;
			return -1;
		}
	}
	else {
		netmask_in.s_addr = 0;
		for (int i = 0; i < row.netmask_sharp; i++) {
			netmask_in.s_addr |= 1 << (31 - i);
		}
		netmask_in.s_addr = htonl(netmask_in.s_addr);
	}
	p_row = (PMIB_IPFORWARDROW)malloc(sizeof(MIB_IPFORWARDROW));
	if (!p_row) {
		LOG(ERROR) << "error allocating mem";
		return -1;
	}
	for (int i = 0; i < p_ip_forward_table->dwNumEntries; i++) {
		if (p_ip_forward_table->table[i].dwForwardDest == 0) {
			memcpy(p_row, &(p_ip_forward_table->table[i]),
				sizeof(MIB_IPFORWARDROW));
		}
	}
	p_row->dwForwardNextHop = gateway_in.s_addr;
	p_row->dwForwardDest = dst_in.s_addr;
	p_row->dwForwardMask = netmask_in.s_addr;
	p_row->dwForwardIfIndex = row.dev;
	p_row->dwForwardMetric1 = 100;
	dwStatus = CreateIpForwardEntry(p_row);
	if (dwStatus != NO_ERROR) {
		LOG(INFO) << "invalid parameter";
	}
	if (p_ip_forward_table)
		free(p_ip_forward_table);
	if (p_row)
		free(p_row);
	return 0;
}

int RouteTable::del_route_row(RouteRow row) {
	in_addr dst, gateway, netmask;
	if (inet_pton(AF_INET, row.dst.c_str(), &dst) < 0) {
		LOG(ERROR) << "Invalid dst address";
		return -1;
	}
	if (inet_pton(AF_INET, row.gateway.c_str(), &gateway) < 0) {
		LOG(ERROR) << "Invalid gateway address";
		return -1;
	}
	if (row.netmask_sharp == -1) {
		if (inet_pton(AF_INET, row.netmask.c_str(), &netmask) < 0) {
			LOG(ERROR) << "invalid netmask " << row.netmask;
			return -1;
		}
	}
	else {
		netmask.s_addr = 0;
		for (int i = 0; i < row.netmask_sharp; i++) {
			netmask.s_addr |= 1 << (31 - i);
		}
		netmask.s_addr = htonl(netmask.s_addr);
	}
	default_route_row->dwForwardDest = dst.s_addr;
	default_route_row->dwForwardMask = netmask.s_addr;
	//default_route_row->dwForwardNextHop = gateway.s_addr;
	if (DeleteIpForwardEntry(default_route_row) != NO_ERROR) {
		//LOG(ERROR) << "error deleting route";
		return -1;
	}
	return 0;
}

RouteRow RouteTable::get_default_route() {
	RouteRow row;
	row.dev = 7;
	row.dst = "0.0.0.0";
	row.gateway = "10.0.0.1";
	return row;
}

int RouteTable::add_route_row(RouteRow row, bool host) {
	in_addr dst, gateway, netmask;
	if (inet_pton(AF_INET, row.dst.c_str(), &dst) < 0) {
		LOG(ERROR) << "Invalid dst address";
		return -1;
	}
	if (inet_pton(AF_INET, row.gateway.c_str(), &gateway) < 0) {
		LOG(ERROR) << "Invalid gateway address";
		return -1;
	}
	if (row.netmask_sharp == -1) {
		if (inet_pton(AF_INET, row.netmask.c_str(), &netmask) < 0) {
			LOG(ERROR) << "invalid netmask " << row.netmask;
			return -1;
		}
	}
	else {
		netmask.s_addr = 0;
		for (int i = 0; i < row.netmask_sharp; i++) {
			netmask.s_addr |= 1 << (31 - i);
		}
		netmask.s_addr = htonl(netmask.s_addr);
	}
	default_route_row->dwForwardDest = dst.s_addr;
	default_route_row->dwForwardMask = netmask.s_addr;
	//default_route_row->dwForwardNextHop = gateway.s_addr;
	if (CreateIpForwardEntry(default_route_row) != NO_ERROR) {
		//LOG(ERROR) << "error adding route to table";
		return -1;
	}
	return 0;
}

int RouteTable::set_default_as_tunnel(dev_type tunnel_name, std::string tunnel_gateway) {
	PMIB_IPFORWARDTABLE p_ip_forward_table = NULL;
	PMIB_IPFORWARDROW p_row = NULL;
	DWORD dwSize = 0;
	BOOL bOrder = FALSE;
	DWORD dwStatus = 0;
	dwStatus = GetIpForwardTable(p_ip_forward_table, &dwSize, bOrder);
	if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
		// Allocate the memory for the table
		if (!(p_ip_forward_table = (PMIB_IPFORWARDTABLE)malloc(dwSize))) {
			LOG(ERROR) << "Malloc failed. Out of memory.";
			return -1;
		}
		// Now get the table.
		dwStatus = GetIpForwardTable(p_ip_forward_table, &dwSize, bOrder);
	}
	if (dwStatus != ERROR_SUCCESS) {
		LOG(ERROR) << "getIpForwardTable failed.";
		if (p_ip_forward_table)
			free(p_ip_forward_table);
		return -1;
	}

	for (int i = 0; i < (int)p_ip_forward_table->dwNumEntries; i++) {
		if (p_ip_forward_table->table[i].dwForwardDest == 0) {
			if (DeleteIpForwardEntry(&p_ip_forward_table->table[i]) != NO_ERROR) {
				LOG(ERROR) << "error deleting default route";
				return -1;
			}
		}
	}
	in_addr gateway;
	if (inet_pton(AF_INET, tunnel_gateway.c_str(), &gateway) < 0) {
		LOG(ERROR) << "invalid gateway ";
		return -1;
	}
	memcpy(&tunnel_route, default_route_row, sizeof(MIB_IPFORWARDROW));
	tunnel_route.dwForwardDest = 0;
	tunnel_route.dwForwardIfIndex = tunnel_name;
	//tunnel_route.dwForwardNextHop = gateway.s_addr;
	DWORD status;
	//if ((status = CreateIpForwardEntry(&tunnel_route)) != NO_ERROR) {
	//	LOG(ERROR) << "error setting route table";
	//	return -1;
	//}
	std::stringstream ss;
	ss << "route add 0.0.0.0 mask 0.0.0.0 " << tunnel_gateway << " if " << tunnel_name;
	WinExec(ss.str().c_str(), SW_HIDE);

	ss.str("");
	ss << "netsh interface ip set dns name=" << tunnel_name << " source=static addr=127.0.0.1";
	WinExec(ss.str().c_str(), SW_HIDE);
	
	ss.str("");
	ss << "netsh interface ip set dns name=" << default_origin_backup.dwForwardIfIndex << " source=static addr=127.0.0.1";
	WinExec(ss.str().c_str(), SW_HIDE);
	return 0;
}

#endif // TARGET_WIN32

#ifdef TARGET_MACOSX

#define BUFLEN (sizeof(struct rt_msghdr) + 1024)

#define ROUNDUP(a, size) (((a) & ((size) - 1)) ? (1 + ((a) | ((size) - 1))) : (a))
#define NEXT_SA(ap) ap = (sockaddr*) \
    ((caddr_t) ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof(u_long)) : sizeof(u_long)))


int RouteTable::add_route_row(RouteRow row, bool host) {
    struct rt_msghdr *rtm;
    struct sockaddr *sa, *tri_info[RTAX_MAX];
    struct sockaddr_in *sin;
    int sockfd = socket(AF_ROUTE, SOCK_RAW, 0);
    char *buf = new char[BUFLEN];
    rtm = (struct rt_msghdr*)buf;
    rtm->rtm_msglen = sizeof(struct rt_msghdr) + 3 * sizeof(struct sockaddr_in);
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_ADD;
    rtm->rtm_flags = RTF_GATEWAY | RTF_UP;
    rtm->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    rtm->rtm_pid = getpid();
    rtm->rtm_seq = 9999;
    rtm->rtm_errno = 0;
    sin = (struct sockaddr_in *)(rtm + 1);
    sin->sin_len = sizeof(struct sockaddr_in);
    sin->sin_family = AF_INET;
    inet_pton(AF_INET, row.dst.c_str(), &sin->sin_addr);
    sin = (struct sockaddr_in *)(rtm + 1) + 1;
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(struct sockaddr_in);
    inet_pton(AF_INET, row.gateway.c_str(), &sin->sin_addr);
    sin = (struct sockaddr_in *)(rtm + 1) + 2;
    sin->sin_len = sizeof(struct sockaddr_in);
    sin->sin_family = AF_INET;
    if (row.netmask_sharp == -1) {
        if (inet_pton(AF_INET, row.netmask.c_str(), &sin->sin_addr) < 0) {
            LOG(ERROR) << "invalid netmask " << row.netmask;
            return -1;
        }
    } else {
        sin->sin_addr.s_addr = 0;
        for (int i = 0;i < row.netmask_sharp;i++) {
            sin->sin_addr.s_addr |= 1 << (31 - i);
        }
        sin->sin_addr.s_addr = htonl(sin->sin_addr.s_addr);
    }
    write(sockfd, rtm, rtm->rtm_msglen);
    read(sockfd, rtm, BUFLEN);
    if (rtm->rtm_type == RTM_ADD && rtm->rtm_seq == 9999) {
//        LOG(INFO) << "Get answer";
//        LOG(INFO) << "return error " << rtm->rtm_errno;
    }
    close(sockfd);
    return 0;
}

RouteRow RouteTable::get_default_route() {
    struct rt_msghdr *rtm;
    struct sockaddr *sa;
    struct sockaddr *rti_info[RTAX_MAX];
    struct sockaddr_in *sin;
    int sockfd = socket(AF_ROUTE, SOCK_RAW, 0);
    char *buf = new char[BUFLEN];
    rtm = (struct rt_msghdr*)buf;
    rtm->rtm_msglen = sizeof(struct rt_msghdr) + 1 * sizeof(struct sockaddr_in);
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = RTA_DST | RTA_IFP;
    rtm->rtm_pid = getpid();
    rtm->rtm_seq = 9997;
    rtm->rtm_errno = 0;
    sin = (struct sockaddr_in *)(rtm + 1);
    sin->sin_len = sizeof(struct sockaddr_in);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = 0;
    write(sockfd, rtm, rtm->rtm_msglen);
    read(sockfd, rtm, BUFLEN);
    if (rtm->rtm_type == RTM_GET && rtm->rtm_seq == 9997) {
//        LOG(INFO) << "Get answer";
//        LOG(INFO) << "return error " << rtm->rtm_errno;
    }
    sa = (struct sockaddr *)(rtm + 1);
    for (int i = 0;i < RTAX_MAX;i++) {
        if (rtm->rtm_addrs & (1 << i)) {
            rti_info[i] = sa;
            NEXT_SA(sa);
        } else {
            rti_info[i] = NULL;
        }
    }
    in_addr in;
    RouteRow row;
    char output[200];
    if (rti_info[RTAX_GATEWAY] != NULL) {
        in.s_addr = *((uint32_t*)(rti_info[RTAX_GATEWAY]->sa_data + 2));
        row.gateway = inet_ntoa(in);
    } else {
        row.dst = "err gateway";
        return row;
    }
    if (rti_info[RTAX_IFP] != NULL) {
        row.dev = rti_info[RTAX_IFP]->sa_data + 2;
    } else {
        row.dst = "err interface";
        return row;
    }
    row.dst = "0.0.0.0";
    row.netmask = "0.0.0.0";
    row.netmask_sharp = 0;
    return row;
}

int RouteTable::del_route_row(RouteRow row) {
    struct rt_msghdr *rtm;
    struct sockaddr *sa, *tri_info[RTAX_MAX];
    struct sockaddr_in *sin;
    int sockfd = socket(AF_ROUTE, SOCK_RAW, 0);
    char *buf = new char[BUFLEN];
    rtm = (struct rt_msghdr*)buf;
    rtm->rtm_msglen = sizeof(struct rt_msghdr) + 3 * sizeof(struct sockaddr_in);
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_DELETE;
    rtm->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    rtm->rtm_pid = getpid();
    rtm->rtm_seq = 9998;
    rtm->rtm_errno = 0;
    sin = (struct sockaddr_in *)(rtm + 1);
    sin->sin_len = sizeof(struct sockaddr_in);
    sin->sin_family = AF_INET;
    inet_pton(AF_INET, row.dst.c_str(), &sin->sin_addr);
    sin = (struct sockaddr_in *)(rtm + 1) + 1;
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(struct sockaddr_in);
    inet_pton(AF_INET, row.gateway.c_str(), &sin->sin_addr);
    sin = (struct sockaddr_in *)(rtm + 1) + 2;
    sin->sin_len = sizeof(struct sockaddr_in);
    sin->sin_family = AF_INET;
    if (row.netmask_sharp == -1) {
        if (inet_pton(AF_INET, row.netmask.c_str(), &sin->sin_addr) < 0) {
            LOG(ERROR) << "invalid netmask " << row.netmask;
            return -1;
        }
    } else {
        sin->sin_addr.s_addr = 0;
        for (int i = 0;i < row.netmask_sharp;i++) {
            sin->sin_addr.s_addr |= 1 << (31 - i);
        }
        sin->sin_addr.s_addr = htonl(sin->sin_addr.s_addr);
    }
    write(sockfd, rtm, rtm->rtm_msglen);
    read(sockfd, rtm, BUFLEN);
    if (rtm->rtm_type == RTM_DELETE && rtm->rtm_seq == 9998) {
//        LOG(INFO) << "Get answer";
//        LOG(INFO) << "return error " << rtm->rtm_errno;
    }
    close(sockfd);
    return 0;
}

#endif


int RouteTable::set_chnroute(std::string filename, std::string remote_ip) {
    std::ifstream ifs(filename);
    int netmask_num;
    RouteRow row = get_default_route();
    row.dst = remote_ip;
    row.netmask = "255.255.255.255";
    row.netmask_sharp = 32;
#ifndef TARGET_WIN32
    default_gateway = row.gateway;
    default_dev = row.dev;
#endif
    add_route_row(row, true);
    modified = true;
	if (filename == "")
		return 0;
    if (!ifs) {
        LOG(ERROR) << "error opening chnroute file";
        return -1;
    }
    char buf[100];
    while (true) {
        if (!ifs.getline(buf, 100)) {
            break;
        }
        for (int i = 0;i < 100;i++) {
            if (buf[i] == '/') {
                buf[i] = 0;
                row.dst = buf + 0;
                netmask_num = strtol(buf + i + 1, NULL, 0);
                row.netmask_sharp = netmask_num;
            }
        }
        add_route_row(row, false);
    }
    ifs.clear();
    ifs.close();
    return 0;
}

int RouteTable::delete_chnroute(std::string filename, std::string remote_ip) {
	if (!modified)
		return 0;
#ifdef TARGET_WIN32
	if (DeleteIpForwardEntry(&tunnel_route) != NO_ERROR) {
		LOG(ERROR) << "error deleting tunnel route";
	}
	if (CreateIpForwardEntry(&default_origin_backup) != NO_ERROR) {
		LOG(ERROR) << "error recovering original route";
	}
#endif // TARGET_WIN32

	if (filename == "")
		return 0;
    int netmask_num;
//    RouteRow row = get_default_route();
//    row.dst = remote_ip;
//    row.netmask = "255.255.255.255";
//    add_route_row(row, true);
    RouteRow row;
#if defined(TARGET_LINUX) || defined(TARGET_MACOSX)
    row.dst = "0.0.0.0";
    row.netmask_sharp = 1;
    row.gateway = tunnel_gateway;
    row.dev = default_dev;
    row.netmask = "128.0.0.0";
    del_route_row(row);
    row.dst = "128.0.0.0";
    del_route_row(row);
    row.gateway = default_gateway;
#endif
    std::ifstream ifs(filename);
    if (!ifs) {
        LOG(ERROR) << "error opening file " << filename;
        return -1;
    }
    char buf[100];
    while (true) {
        if (!ifs.getline(buf, 100)) {
            break;
        }
        for (int i = 0;i < 100;i++) {
            if (buf[i] == '/') {
                buf[i] = 0;
                row.dst = buf + 0;
                netmask_num = strtol(buf + i + 1, NULL, 0);
                row.netmask_sharp = netmask_num;
            }
        }
        del_route_row(row);
    }
	modified = false;
#ifdef TARGET_WIN32
	std::stringstream ss;
	ss << "netsh interface ip set dns name=" << default_origin_backup.dwForwardIfIndex << " source=dhcp";
	WinExec(ss.str().c_str(), SW_HIDE);
#endif
    ifs.clear();
    ifs.close();
    return 0;
}