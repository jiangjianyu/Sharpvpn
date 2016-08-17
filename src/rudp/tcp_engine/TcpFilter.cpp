//
// Created by maxxie on 16-8-10.
//

#include <glog/logging.h>
#include <thread>
#include "TcpFilter.h"
#include "PackageHeader.h"

TcpFilter* TcpFilter::filter;

int TcpFilter::init() {
    const char *_dev = dev.c_str();
    bpf_u_int32 mask;
    bpf_u_int32 net;
//    _dev = pcap_lookupdev(errbuf);
//    if (_dev == NULL) {
//        LOG(INFO) << "error finding device";
//        return -1;
//    }
    if (pcap_lookupnet(_dev, &net, &mask, errbuf) == -1) {
        LOG(INFO) << "error geting netmask";
        return -1;
    }
    capture_engine = pcap_open_live(_dev, 3000, 1, 1000, errbuf);
    if (capture_engine == NULL) {
        LOG(INFO) << "error opening capture engine";
        return -1;
    }
    std::stringstream ss;
    ss << "tcp dst port " << port;
    if (pcap_compile(capture_engine, &fp, ss.str().c_str(), 0, net) == -1) {
        LOG(INFO) << "error compiling filter";
        return -1;
    }
    if (pcap_setfilter(capture_engine, &fp) == -1) {
        LOG(INFO) << "error setting filter";
        return -1;
    }
    LOG(INFO) << "open engine in dev " << dev;
    filter = this;
    init_firewall();
    return 0;
}

int TcpFilter::recv(unsigned char *buf, sockaddr *addr, socklen_t *_len) {
    mutex.lock();
    while(package_list.size() == 0) {
        cv.wait(&mutex);
    }
    netstruct::PackageSegment *segment = package_list.pop();
    mutex.unlock();
    const struct sniff_ip *ip = (struct sniff_ip*)(segment->buf + SIZE_ETHERNET);
    const struct sniff_tcp *tcp;
    unsigned char *payload;
    u_int size_ip, size_tcp;
    size_ip = IP_HL(ip) * 4;
    if (size_ip < 20) {
        return -1;
    }
    tcp = (struct sniff_tcp*)(segment->buf + SIZE_ETHERNET + size_ip);
    size_tcp = TH_OFF(tcp) * 4;
    if (size_tcp < 4) {
        return -1;
    }
    payload = segment->buf + SIZE_ETHERNET + size_ip + size_tcp;
    int len = segment->len - SIZE_ETHERNET - size_ip - size_tcp;
    if (addr != NULL && _len != NULL) {
        if (ip->ip_vhl & 0xf0 == 0x40) {
            sockaddr_in *addr_4 = (sockaddr_in *) addr;
            addr_4->sin_family = AF_INET;
            addr_4->sin_port = tcp->th_dport;
            addr_4->sin_addr = ip->ip_dst;
            *_len = sizeof(sockaddr_in);
        } else {
            //ipv6 is not supported now
        }
    }
    memcpy(buf, payload + SIZE_ETHERNET + size_ip + size_tcp, len);
    return len;
}

void TcpFilter::get_package(u_char *args, const struct pcap_pkthdr *header, const u_char *package) {
    if (header->len <= 0) {
        LOG(INFO) << "get one empty package";
        return;
    }
    netstruct::PackageSegment *segment = new netstruct::PackageSegment((unsigned char*)package, header->len);
    filter->mutex.lock();
    filter->package_list.push(segment);
    filter->mutex.unlock();
    filter->cv.signal();
}

TcpFilter::TcpFilter(std::string __dev, int __port) {
    dev = __dev;
    port = __port;
}

int TcpFilter::init_firewall() {
    std::stringstream ss;
    ss << "/sbin/iptables -t filter -A INPUT -p tcp --dport " << port << " -j DROP";
    int ret = system(ss.str().c_str());
    return 0;
}

int TcpFilter::restore_firewall() {
    std::stringstream ss;
    ss << "/sbin/iptables -t filter -D INPUT -p tcp --dport " << port << " -j DROP";
    system(ss.str().c_str());
    return 0;
}

void TcpFilter::run() {
    pcap_loop(capture_engine, -1,TcpFilter::get_package, NULL);
}

void TcpFilter::stop() {
    pcap_breakloop(capture_engine);
}

int TcpFilter::send(unsigned char *buf, int len, sockaddr *addr, socklen_t _len) {
    uint16_t id, id_ip;
    int bytes_written;
    libnet_engine = libnet_init(LIBNET_RAW4, dev.c_str(), errbuf);
    libnet_seed_prand(libnet_engine);
    id = (uint16_t)libnet_get_prand(LIBNET_PR16);
    sockaddr_in *d_sockaddr = (sockaddr_in*)addr;
    uint16_t dport = d_sockaddr->sin_port;
    if (libnet_build_tcp(port, dport, seq++, 0, 0, 1024, 0, 0,
                         LIBNET_TCP_H, (uint8_t*)buf, len, libnet_engine, 0) == -1) {
        LOG(INFO) << "error building tcp";
        return -1;
    }
    libnet_seed_prand(libnet_engine);
    id_ip = (uint16_t)libnet_get_prand(LIBNET_PR16);
    if (libnet_build_ipv4(LIBNET_IPV4_H, 0, id_ip, 0, 64,
                          IPPROTO_TCP, 0, libnet_get_ipaddr4(libnet_engine),
                          d_sockaddr->sin_addr.s_addr,NULL, 0, libnet_engine,0) == -1) {
        LOG(INFO) << "error sending package";
    }
    if ((bytes_written = libnet_write(libnet_engine)) == -1) {
        LOG(INFO) << "error writing to adapter";
    }
    libnet_destroy(libnet_engine);
    return bytes_written;
}