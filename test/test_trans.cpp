//
// Created by maxxie on 16-7-14.
//


#include <iostream>
#include "../src/rudp/ReliableSocket.h"
#ifdef TARGET_LINUX
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // TARGET_LINUX
#ifdef TARGET_WIN32
#include <Ws2tcpip.h>
#endif // TARGET_WIN32

int main(int argc, char **args) {
    //Test Server
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

    ReliableSocket soc = ReliableSocket(100, 100);
    struct sockaddr_in addr;
    addr.sin_port = htons(50000);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
	if (soc.bind_addr((struct sockaddr *) &addr) < 0) {
		return -1;
	}
    soc.listen_addr();
    ReliableSocket *client = soc.accept_connection();
    unsigned char da[1500];
    int send_len = 400;
    for (int i = 0;i < 10000;i++) {
        da[send_len - 1] = i % 255;
        client->send_package(da, send_len);
    }
    client->recv_package(da);
    sleep(1);
    return 0;
}