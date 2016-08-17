//
// Created by maxxie on 16-7-19.
//

#ifndef SHARPVPN_SHARPTUNNEL_H
#define SHARPVPN_SHARPTUNNEL_H


#include <string>
#include "Sharp.h"
#ifndef TARGET_WIN32
typedef int TunnelHandle;
#else
#include <WinSock2.h>
#include <Windows.h>
typedef HANDLE TunnelHandle;
#endif // TARGET_WIN32

class SharpTunnel {
private:
#ifdef TARGET_WIN32
	DWORD ifIndex;
	//to delete address
	ULONG NTEContext = 0;
	ULONG NTEInstance = 0;
	OVERLAPPED onRead = { 0 };
	OVERLAPPED onWrite = { 0 };
#endif // TARGET_WIN32
	TunnelHandle tun;
	bool running = false;
    int mtu;
    std::string dev_name;
    std::string ip_address;
    std::string tunnel_gateway;
    /* a host order ipv4 address */
    uint32_t tunnel_ip;

    int set_ip();
public:
    SharpTunnel(std::string dev_n, std::string ip, int dev_mtu);
    ssize_t read(unsigned char *buf, size_t size);
    ssize_t write(unsigned char *buf, size_t size);
    uint32_t get_local_tunnel_ip();
    std::string get_tunnel_gateway() {return tunnel_gateway;}
    int set_ip(uint32_t ip);
#ifdef TARGET_WIN32
	DWORD get_if_index() { return ifIndex; }
#endif // TARGET_WIN32

    /* Init a tunnel
     * return 0 if it succeed
     */
    int init(bool set_ip);
    int shutdown();
};


#endif //SHARPVPN_SHARPTUNNEL_H
