//
// Created by maxxie on 16-8-12.
//

#include "../src/rudp/tcp_engine/TcpFilter.h"

int main() {
    TcpFilter filter = TcpFilter("eth1", 2000);
    filter.init();
    sockaddr_in in;
    char *send_buf = "abcdef\0";
    in.sin_port = 2000;
    inet_pton(AF_INET, "10.0.0.2", &in.sin_addr);
    for (int i = 0;i < 100;i++) {
        filter.send((unsigned char *) send_buf, 7, (struct sockaddr *) &in, sizeof(sockaddr_in));
    }
    filter.restore_firewall();
    LOG(INFO) << "okl";
}