//
// Created by maxxie on 16-8-10.
//

#ifndef SHARPVPN_TCPFILTER_H
#define SHARPVPN_TCPFILTER_H

#include <pcap/pcap.h>
#include <sys/socket.h>
#include "../NetStruct.h"
#include "../pthread_wrapper/Mutex.h"
#include "../pthread_wrapper/ConditionVariable.h"
#include <libnet.h>
class TcpFilter {
public:
    pcap_t *capture_engine;
    libnet_t *libnet_engine;
    char errbuf[PCAP_ERRBUF_SIZE];
    pthread::Mutex mutex;
    pthread::ConditionVariable cv;
    struct bpf_program fp;
    std::string dev;
    int port;
    uint32_t seq = 1;
    netstruct::PackageSegmentList package_list = netstruct::PackageSegmentList(10000);

    static void get_package(u_char *args, const struct pcap_pkthdr *header, const u_char *package);
    static TcpFilter *filter;
    TcpFilter(std::string dev, int port);
    int init();
    void run();
    void stop();
    int init_firewall();
    int restore_firewall();
    int recv(unsigned char *buf, sockaddr *addr, socklen_t *len);
    int send(unsigned char *buf, int len, sockaddr *addr, socklen_t _len);
};


#endif //SHARPVPN_TCPFILTER_H
