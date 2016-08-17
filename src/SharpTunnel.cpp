//
// Created by maxxie on 16-7-19.
//

#include "SharpTunnel.h"
#include "Sharp.h"
#include "RouteTable.h"
#include <fcntl.h>
#include <sys/types.h>

#ifdef TARGET_LINUX
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/ioctl.h>
#endif

#ifdef TARGET_WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <Windows.h>
#endif // TARGET_WIN32

#ifdef TARGET_MACOSX
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/sys_domain.h>
#include <netinet/ip.h>
#include <sys/uio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <libnet.h>

#endif



SharpTunnel::SharpTunnel(std::string dev_n, std::string ip, int dev_mtu) {
    dev_name = dev_n;
    ip_address = ip;
    mtu = dev_mtu;
}

#ifdef TARGET_MACOSX
int SharpTunnel::init(bool setip) {
    struct ctl_info ctl_info;
    struct sockaddr_ctl sc;
    int fd;
    int utunnum;

    if (sscanf(dev_name.c_str(), "utun%d", &utunnum) != 1) {
        LOG(ERROR) << "invalid utun device name ";
        return -1;
    }

    memset(&ctl_info, 0, sizeof(ctl_info));
    if (strlcpy(ctl_info.ctl_name, UTUN_CONTROL_NAME,
                sizeof(ctl_info.ctl_name)) >= sizeof(ctl_info.ctl_name)) {
        LOG(INFO) << "error setting up utun";
        return -1;
    }

    fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);

    if (fd == -1) {
        LOG(ERROR) << "error creating control socket";
        return -1;
    }

    if (ioctl(fd, CTLIOCGINFO, &ctl_info) == -1) {
        close(fd);
        LOG(INFO) << "error sending control msg";
        return -1;
    }

    sc.sc_id = ctl_info.ctl_id;
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    sc.sc_unit = utunnum + 1;

    if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) == -1) {
        close(fd);
        LOG(ERROR) << "error connecting to control";
        return -1;
    }
    tun = fd;
    running = true;
    if (setip)
        return set_ip();
    return 0;
}

int SharpTunnel::set_ip() {
    struct ifreq ifr;
    struct sockaddr_in addr;
    int stat, s;

    memset(&ifr, 0, sizeof(ifr));
    memset(&addr, 0, sizeof(addr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);

    addr.sin_family = AF_INET;
    s = socket(addr.sin_family, SOCK_DGRAM, 0);
    stat = inet_pton(addr.sin_family, ip_address.c_str(), &addr.sin_addr);
    tunnel_ip = ntohl(addr.sin_addr.s_addr);
    struct sockaddr_in gate;
    gate.sin_family = AF_INET;
    gate.sin_addr.s_addr = htonl((tunnel_ip & 0xffffff00) + 1);

    if (stat == 0) {
        LOG(ERROR) << "invalid ip";
        return -1;
    }

    if (stat == -1) {
        LOG(ERROR) << "invalid family";
        return -1;
    }

//    ifr.ifr_addr = *(struct sockaddr *) &addr;
//
//    //if (ioctl(fd, SIOCSIFADDR, (caddr_t) &ifr) == -1)
//    if (ioctl(s, SIOCSIFADDR, (caddr_t) &ifr) == -1) {
//        LOG(ERROR) << "set ip fail";
//        return -1;
//    }
//
//    memset(&ifr, 0, sizeof(ifr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = htonl(ntohl(addr.sin_addr.s_addr & 0xffffff) + 1);
//    ifr.ifr_ifru.ifru_dstaddr = *(struct sockaddr *) &addr;
//    if (ioctl(s, SIOCSIFDSTADDR, &ifr) == -1) {
//        LOG(ERROR) << "set dst ip fail";
//        return -1;
//    }
//
//
//    memset(&ifr, 0, sizeof(ifr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    ifr.ifr_ifru.ifru_mtu = mtu;
//    if (ioctl(s, SIOCSIFMTU, (caddr_t) &ifr) == -1) {
//        LOG(ERROR) << "set mtu fail";
//        return -1;
//    }
//
//
//    memset(&ifr, 0, sizeof(ifr));
//    memset(&addr, 0, sizeof(addr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = 0xffffff;
//    addr.sin_len = sizeof(struct sockaddr_in);
//    ifr.ifr_addr = *(struct sockaddr *) &addr;
//    if (ioctl(s, SIOCSIFNETMASK, &ifr) == -1) {
//        LOG(ERROR) << "set netmask fail";
//        return -1;
//    }
//
//    //bring up tunnel
//    memset(&ifr, 0, sizeof(ifr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    ifr.ifr_ifru.ifru_flags |= IFF_UP;
//    if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
//        LOG(ERROR) << "bring up tunnel fail";
//        return -1;
//    }
    std::stringstream ss;
    ss << "ifconfig " << dev_name << " " << ip_address << " "
       << inet_ntoa(gate.sin_addr) << " mtu " << mtu << " netmask 255.255.255.0 up";
    system(ss.str().c_str());
//    close(s);
    return 0;
}

int SharpTunnel::shutdown() {
    if (!running)
        return 0;
    close(tun);
    LOG(INFO) << "tunnel close successfully";
    return 0;
}
int SharpTunnel::set_ip(uint32_t ip) {
    struct ifaliasreq ifa;
    struct ifreq ifr;
    struct sockaddr_in addr;
    int s;

    //set ip address
//    memset(&ifr, 0, sizeof(ifr));
//    memset(&ifa, 0, sizeof(ifa));
//    memset(&addr, 0, sizeof(addr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    strncpy(ifa.ifra_name, dev_name.c_str(), IFNAMSIZ);
//    addr.sin_family = AF_INET;
//    s = socket(addr.sin_family, SOCK_DGRAM, 0);
//    addr.sin_addr.s_addr = ip;
//    ifr.ifr_ifru.ifru_addr = *(struct sockaddr *) &addr;
//    if (ioctl(s, SIOCSIFADDR, &ifr) == -1) {
//        LOG(ERROR) << "set ip fail";
//        return -1;
//    }
//
//    memset(&ifr, 0, sizeof(ifr));
//    memset(&addr, 0, sizeof(addr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = htonl(ntohl(ip & 0xffffff) + 1);
//    ifr.ifr_ifru.ifru_dstaddr = *(struct sockaddr *) &addr;
//    if (ioctl(s, SIOCSIFDSTADDR, &ifr) == -1) {
//        LOG(ERROR) << "set dst ip fail";
//        return -1;
//    }
//
//    memset(&ifr, 0, sizeof(ifr));
//    memset(&addr, 0, sizeof(addr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = 0xfeffff;
//    addr.sin_len = sizeof(struct sockaddr_in);
//    ifr.ifr_addr = *(struct sockaddr *) &addr;
//    if (ioctl(s, SIOCSIFNETMASK, &ifr) == -1) {
//        LOG(ERROR) << "set netmask fail";
//        return -1;
//    }
//
//    //set mtu
//    memset(&ifr, 0, sizeof(ifr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    ifr.ifr_ifru.ifru_mtu = mtu;
//    if (ioctl(s, SIOCSIFMTU, (caddr_t) &ifr) == -1) {
//        LOG(ERROR) << "set mtu fail";
//        return -1;
//    }

    //set netmask
/*    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    struct sockaddr_in* addrp = (struct sockaddr_in*)&ifr.ifr_ifru.ifru_addr;
    addrp->sin_family = AF_INET;
    if (inet_pton(addr.sin_family, "255.255.255.0", &addrp->sin_addr) == 0) {
        LOG(ERROR) << "invalid netmask";
        return -1;
    }
    if (ioctl(s, SIOCSIFNETMASK, &ifr) == -1) {
        LOG(ERROR) << "set netmask fail";
        return -1;
    }*/

    //bring up tunnel
//    memset(&ifr, 0, sizeof(ifr));
//    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
//    ifr.ifr_ifru.ifru_flags |= IFF_UP;
//    if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
//        LOG(ERROR) << "bring up tunnel fail";
//        return -1;
//    }
    in_addr in, in_ip;
    in.s_addr = ip & 0xffffff;
    in.s_addr = htonl(ntohl(in.s_addr) + 1);
    in_ip.s_addr = ip;
    tunnel_gateway = inet_ntoa(in);

    std::stringstream ss;
    ss << "ifconfig " << dev_name << " " << inet_ntoa(in_ip) << " " << tunnel_gateway
       << " mtu " << mtu << " netmask 255.255.255.0 up";
    system(ss.str().c_str());
    return 0;
}

ssize_t SharpTunnel::read(unsigned char *buf, size_t size) {
    uint32_t type;
    struct iovec iv[2];
    int read_r;

    iv[0].iov_base = &type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = size;

    read_r = readv(tun, iv, 2);
    if (read_r <= 0) {
        return read_r;
    } else if (read_r < sizeof(uint32_t)) {
        return 0;
    }
    return read_r - sizeof(uint32_t);
}

ssize_t SharpTunnel::write(unsigned char *buf, size_t size) {
    uint32_t type;
    struct iovec iv[2];
    struct ip *iph;
    iph = (struct ip*)buf;
    if (iph->ip_v == 6) {
        type = htonl(AF_INET6);
    } else {
        type = htonl(AF_INET);
    }
    iv[0].iov_base = &type;
    iv[0].iov_len = sizeof(type);
    iv[1].iov_base = buf;
    iv[1].iov_len = size;
    int write_r = writev(tun, iv, 2);
    if (write_r <= 0) {
        return write_r;
    } else if (write_r < sizeof(uint32_t)) {
        return 0;
    }
    return write_r - sizeof(uint32_t);
}
#endif

#ifdef TARGET_LINUX
int SharpTunnel::shutdown(){
	if (!running)
		return 0;
    close(this->tun);
    LOG(INFO) << "tunnel closed successfully";
    return 0;
}

int SharpTunnel::init(bool setip) {
    struct ifreq ifr;
    int fd, e;
    const char *dev = dev_name.c_str();
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        LOG(ERROR) << "can not open /dev/net/tun";
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *         IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if(*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if ((e = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        LOG(ERROR) << "can not setup tun device: " << dev_name;
        close(fd);
        return -1;
    }
    tun = fd;
    LOG(INFO) << "create tunnel successfully\n";
    if (setip) {
        if (set_ip() < 0)
            return -1;
    }
	running = true;
    return 0;
}

int SharpTunnel::set_ip() {
    struct ifreq ifr;
    struct sockaddr_in addr;
    int stat, s;

    memset(&ifr, 0, sizeof(ifr));
    memset(&addr, 0, sizeof(addr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);

    addr.sin_family = AF_INET;
    s = socket(addr.sin_family, SOCK_DGRAM, 0);
    stat = inet_pton(addr.sin_family, ip_address.c_str(), &addr.sin_addr);
    tunnel_ip = ntohl(addr.sin_addr.s_addr);

    if (stat == 0) {
        LOG(ERROR) << "invalid ip";
        return -1;
    }

    if (stat == -1) {
        LOG(ERROR) << "invalid family";
        return -1;
    }

    ifr.ifr_addr = *(struct sockaddr *) &addr;

    //if (ioctl(fd, SIOCSIFADDR, (caddr_t) &ifr) == -1)
    if (ioctl(s, SIOCSIFADDR, (caddr_t) &ifr) == -1) {
        LOG(ERROR) << "set ip fail";
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    ifr.ifr_ifru.ifru_mtu = mtu;
    if (ioctl(s, SIOCSIFMTU, (caddr_t) &ifr) == -1) {
        LOG(ERROR) << "set mtu fail";
        return -1;
    }

    //set netmask
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    struct sockaddr_in* addrp = (struct sockaddr_in*)&ifr.ifr_ifru.ifru_netmask;
    addrp->sin_family = AF_INET;
    if (inet_pton(addr.sin_family, "255.255.255.0", &addrp->sin_addr) == 0) {
        LOG(ERROR) << "invalid netmask";
        return -1;
    }
    if (ioctl(s, SIOCSIFNETMASK, &ifr) == -1) {
        LOG(ERROR) << "set netmask fail";
        return -1;
    }

    //bring up tunnel
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    ifr.ifr_ifru.ifru_flags |= IFF_UP;
    if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
        LOG(ERROR) << "bring up tunnel fail";
        return -1;
    }
    return 0;
}

int SharpTunnel::set_ip(uint32_t ip) {
    struct ifreq ifr;
    struct sockaddr_in addr;
    int s;

    //set ip address
    memset(&ifr, 0, sizeof(ifr));
    memset(&addr, 0, sizeof(addr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    addr.sin_family = AF_INET;
    s = socket(addr.sin_family, SOCK_DGRAM, 0);
    addr.sin_addr.s_addr = ip;
    ifr.ifr_addr = *(struct sockaddr *) &addr;
    if (ioctl(s, SIOCSIFADDR, (caddr_t) &ifr) == -1) {
        LOG(ERROR) << "set ip fail";
        return -1;
    }


    //set mtu
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    ifr.ifr_ifru.ifru_mtu = mtu;
    if (ioctl(s, SIOCSIFMTU, (caddr_t) &ifr) == -1) {
        LOG(ERROR) << "set mtu fail";
        return -1;
    }

    //set netmask
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    struct sockaddr_in* addrp = (struct sockaddr_in*)&ifr.ifr_ifru.ifru_netmask;
    addrp->sin_family = AF_INET;
    if (inet_pton(addr.sin_family, "255.255.255.0", &addrp->sin_addr) == 0) {
        LOG(ERROR) << "invalid netmask";
        return -1;
    }
    if (ioctl(s, SIOCSIFNETMASK, &ifr) == -1) {
        LOG(ERROR) << "set netmask fail";
        return -1;
    }

    //bring up tunnel
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
    ifr.ifr_ifru.ifru_flags |= IFF_UP;
    if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
        LOG(ERROR) << "bring up tunnel fail";
        return -1;
    }
    in_addr in;
    in.s_addr = ip & 0xffffff;
    in.s_addr = htonl(ntohl(in.s_addr) + 1);

    tunnel_gateway = inet_ntoa(in);
    return 0;
}

ssize_t SharpTunnel::read(unsigned char *buf, size_t size) {
    return c_file::read(tun, buf, size);
}

ssize_t SharpTunnel::write(unsigned char *buf, size_t size) {
    return c_file::write(tun, buf, size);
}

#endif

#ifdef TARGET_WIN32
#define FILE_DEVICE_UNKNOWN 0x00000022
#define FILE_ANY_ACCESS 0
#define METHOD_BUFFERED 0
#define TAP_CODE(device_type, function, method, access) ((device_type << 16) | (access << 14) | (function << 2) | (method))
#define TAP_WIN_CONTROL_CODE(request, method) TAP_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)
#define TAP_WIN_IOCTL_GET_MAC               TAP_WIN_CONTROL_CODE (1, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_VERSION           TAP_WIN_CONTROL_CODE (2, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_MTU               TAP_WIN_CONTROL_CODE (3, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_INFO              TAP_WIN_CONTROL_CODE (4, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_CONFIG_POINT_TO_POINT TAP_WIN_CONTROL_CODE (5, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_SET_MEDIA_STATUS      TAP_WIN_CONTROL_CODE (6, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_CONFIG_DHCP_MASQ      TAP_WIN_CONTROL_CODE (7, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_GET_LOG_LINE          TAP_WIN_CONTROL_CODE (8, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_CONFIG_DHCP_SET_OPT   TAP_WIN_CONTROL_CODE (9, METHOD_BUFFERED)
#define TAP_WIN_IOCTL_CONFIG_TUN            TAP_WIN_CONTROL_CODE (10, METHOD_BUFFERED)
typedef unsigned char MACADDR[6];

int SharpTunnel::init(bool set_ip) {
	uint32_t status = 1;
	DWORD out_len = 0;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	PIP_ADAPTER_ADDRESSES currentAddress = NULL;
	PCHAR tap_id = NULL;


	/* This procedure first find the interface named TAP_Windows Adapter V9, 
	   get the guid of the device and the interface id(to set ip address) 
	   */
	pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(sizeof(IP_ADAPTER_ADDRESSES));

	// Make an initial call to GetAdaptersAddresses to get the 
	// size needed into the outBufLen variable
	if (GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &out_len) == ERROR_BUFFER_OVERFLOW) {
		free(pAddresses);
		pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(out_len);
	}

	if (pAddresses == NULL) {
		LOG(ERROR) << "TUNNEL DEVICE SEARCH:Error allocating memory";
		return -1;
	}

	dwRetVal = GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &out_len);
	if (dwRetVal == NO_ERROR) {
		// If successful, get the interface id of the interface
		ifIndex = 0;
		currentAddress = pAddresses;
		while (currentAddress) {
			if (wcscmp(currentAddress->Description, L"TAP-Windows Adapter V9") == 0) {
				ifIndex = currentAddress->IfIndex;
				tap_id = new char[strlen(currentAddress->AdapterName) + 1];
				memcpy(tap_id, currentAddress->AdapterName, strlen(currentAddress->AdapterName) + 1);
				break;
			}
			currentAddress = currentAddress->Next;
		}
		if (!ifIndex) {
			LOG(ERROR) << "TUNNEL DEVICE SEARCH:Error finding index of the interface";
			return -1;
		}
		LOG(INFO) << "TUNNEL DEVICE SEARCH:Interface ID " << ifIndex;
	}
	if (pAddresses) {
		free(pAddresses);
	}
	
	/* Start device by device io, the control info is 
	   defined in windows-tap.h in openvpn/windows-tap
	   we use p2p mode of the openvpn tap
	   */
	sockaddr_in addr, network, netmask;
	std::string tap_name_prefix = "\\\\.\\GLOBAL\\";
	std::string tap_name_suffix = ".tap";
	std::string tap_id_string = tap_id;
	std::string tap_name_s = tap_name_prefix + tap_id_string + tap_name_suffix;
	PWCHAR tap_name = new WCHAR[100];
	size_t len;
	mbstowcs_s(&len, tap_name, 100,tap_name_s.c_str(), tap_name_s.length() + 1);
	tun = CreateFile(tap_name, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, NULL);
	if (tun == INVALID_HANDLE_VALUE) {
		LOG(ERROR) << "TUNNEL INITIALIZATION:open device failed, error code " << GetLastError();
		return -1;
	}

	if (!DeviceIoControl(tun, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status,
		sizeof(uint32_t), &status, sizeof(status), &out_len,
		NULL)) {
		LOG(ERROR) << "TUNNEL INITIALIZATION:set device status failed";
		return -1;
	}
	
	/* get mac address of the tunnel *
	MACADDR macaddr;
	if (!DeviceIoControl(tun, TAP_WIN_IOCTL_GET_MAC, &status,
		sizeof(status), macaddr, sizeof(MACADDR), &out_len, 0)) {
		LOG(ERROR) << "error getting mac address ";
		return -1;
	}
	*/

	//set mtu
	std::stringstream ss;
	ss.str("");
	ss << "netsh interface ipv4 set subinterface " << ifIndex
		<< " mtu=" << mtu << " store=persistent";
	//LOG(INFO) << ss.str();
	WinExec(ss.str().c_str(), SW_HIDE);

	running = true;
	return 0;
}

int SharpTunnel::set_ip() {
	//Not implemented in win32
	return 0;
}

int SharpTunnel::set_ip(uint32_t ip) {
	sockaddr_in netmask;
	sockaddr_in network;
	DWORD out_len;

	/* set local ip address, netmask and 
	   remote network of the tunnel */
	int dwRetVal;

	//config info : local_ip, network, netmask
	unsigned long config_info[3];
	if (inet_pton(AF_INET, "255.255.255.0", &netmask.sin_addr) == 0) {
		LOG(ERROR) << "TUNNEL IP ASSIGNMENT:invalid netmask";
		return -1;
	}
	network.sin_addr.s_addr = ip & netmask.sin_addr.s_addr;
	config_info[0] = ip;
	config_info[1] = network.sin_addr.s_addr;
	config_info[2] = netmask.sin_addr.s_addr;

	void *outbuffer = malloc(12);
	if (!DeviceIoControl(tun, TAP_WIN_IOCTL_CONFIG_TUN, config_info,
		sizeof(config_info), outbuffer, 32, &out_len, NULL)) {
		LOG(ERROR) << "TUNNEL IP ASSIGNMENT:set device status info failed";
		return -1;
	}

	/* set ip address of the adapter */
	if ((dwRetVal = AddIPAddress(ip, netmask.sin_addr.s_addr,
		ifIndex, &NTEContext, &NTEInstance)) != NO_ERROR &&
		dwRetVal != ERROR_OBJECT_ALREADY_EXISTS) {
		LOG(ERROR) << "TUNNEL IP ASSIGNMENT:Error setting adatper ip, error code " << dwRetVal;
		return -1;
	}
	
	//in_addr ip_in;
	//ip_in.s_addr = ip;
	//char ip_buf_2[20];
	//const char *i_b;
	//i_b = inet_ntop(AF_INET, &ip_in, ip_buf_2, 20);

	//std::stringstream ss;
	//ss.str("");
	//ss << "netsh interface ip set address name=" << ifIndex 
	//	<< " source=static addr=" << i_b << " 255.255.255.0" << " 10.7.0.1";
	//WinExec(ss.str().c_str(), SW_HIDE);

	free(outbuffer);
	in_addr gate;
	gate.s_addr = htonl(ntohl(network.sin_addr.s_addr) + 1);
	char ip_buf[20];
	const char *s_b;
	if ((s_b = inet_ntop(AF_INET, &gate, ip_buf, 20)) == NULL) {
		LOG(ERROR) << "TUNNEL IP ASSIGNMENT:Errir getting tunnel gateway";
		return -1;
	}
	tunnel_gateway = s_b;
	return 0;
}
ssize_t SharpTunnel::read(unsigned char *buf, size_t size) {
	unsigned long len = 0;
	bool status = 0;
	onRead.hEvent = CreateEvent(NULL, true, false, NULL);
	status = ReadFile(tun, buf, size, &len, &onRead);
	if (!status) {
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(onRead.hEvent, INFINITE);
			GetOverlappedResult(tun, &onRead, &len, false);
		}
	}
	//LOG(INFO) << "Read data " << len;
	return len;
}

ssize_t SharpTunnel::write(unsigned char *buf, size_t size) {
	unsigned long len = 0;
	bool status;
	onWrite.hEvent = CreateEvent(NULL, true, false, NULL);
	status = WriteFile(tun, buf, size, &len, &onWrite);
	if (!status) {
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(onWrite.hEvent, INFINITE);
			GetOverlappedResult(tun, &onWrite, &len, false);
		}
	}
	//LOG(INFO) << "Write data " << len;
	return len;
}

int SharpTunnel::shutdown() {
	if (!running)
		return 0;
	uint32_t status = 0;
	DWORD out_len = 0;
	int status_f = 0;
	if (!DeviceIoControl(tun, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status_f,
		sizeof(uint32_t), &status_f, sizeof(status_f), &out_len,
		NULL)) {
		LOG(ERROR) << "TUNNEL SHUTDOWN:set device status failed";
		status = -1;
	}

	
	if (DeleteIPAddress(NTEContext) != NO_ERROR) {
		LOG(ERROR) << "TUNNEL SHUTDOWN:error deleting address of adapter";
		status = -1;
	}
	
	//std::stringstream ss;
	//ss.str("");
	//ss << "netsh interface ip set address name=" << ifIndex << " source=dhcp";
	//WinExec(ss.str().c_str(), SW_HIDE);

	CloseHandle(tun);
	LOG(INFO) << "TUNNEL SHUTDOWN:tunnel closed successfully";
	running = false;
	return 0;
}
#endif // TARGET_WIN32


uint32_t SharpTunnel::get_local_tunnel_ip() {
	return tunnel_ip;
}

