//
// Created by maxxie on 16-8-10.
//

#include <thread>
#include "../src/rudp/tcp_engine/TcpFilter.h"

int main() {
    google::InitGoogleLogging("tcpfilter");
    FLAGS_logtostderr = 1;
    TcpFilter *filter = new TcpFilter("eth1", 2000);
    filter->init();
    std::thread([filter](){
        filter->run();
    }).detach();
    unsigned char buf[1500];
    struct sockaddr_in in;
    for (int i = 0;i < 100;i++) {
        int len = filter->recv(buf, NULL, NULL);
        LOG(INFO) << "recv len " << len;
    }
    filter->restore_firewall();
    filter->stop();
    return 0;
}