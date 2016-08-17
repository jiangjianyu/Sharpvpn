//
// Created by maxxie on 16-7-14.
//

#include "../src/rudp/ReliableSocket.h"
#include <iostream>
#ifdef TARGET_LINUX
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>

#endif // TARGET_LINUX

#ifdef TARGET_WIN32
#include <Ws2tcpip.h>
#endif // TARGET_WIN32


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
    ReliableSocket soc = ReliableSocket(50, 4);
    struct sockaddr_in addr;
    inet_pton(AF_INET,"159.203.238.143", &addr.sin_addr);
//	inet_pton(AF_INET, "10.0.0.2", &addr.sin_addr);
//	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    addr.sin_port = htons(50000);
    addr.sin_family = AF_INET;
	if (soc.connect_addr((struct sockaddr*)&addr) < 0) {
		return 1;
	}
    unsigned char da[1500];
    time_struct_ms tv, tv2;
	get_time_ms(tv);
    for (int i = 0;i < 10000;i++) {
        int r = soc.recv_package(da);
        assert(da[r - 1] == i % 255);
    }
	get_time_ms(tv2);
    std::cout << "time spend: " << get_ms(tv2, tv) << std::endl;
//    soc.close_addr();
    soc.recv_package(da);
    sleep(1);
    return 0;
}
