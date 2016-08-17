//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_RELIABLESOCKET_H
#define RELIABLEFASTUDP_RELIABLESOCKET_H

#include <string>
#include <queue>
#include <map>
#include <list>
#include "pthread_wrapper/ConditionVariable.h"
#include "Package.h"
#include "NetStruct.h"
#include "../SharpCrypto.h"

#ifndef TARGET_WIN32
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <queue>
#define closesocket(sock) close(sock)
#else
typedef int socklen_t;
#endif

#define MAX_SEGMENT(max_mtu) max_mtu - 20 - 8 - sizeof(PackageBaseFormat) \
                                 - sizeof(PackageDataFormat) - SHARPVPN_CRYPTO_OVERHEAD
#define TUNNEL_MTU(max_mtu) MAX_SEGMENT(max_mtu) - 4 - 4
typedef enum SocketType{
    RegularSocket,
    BindedSocket,
    ServerSocket,
    AcceptableSocket
}SocketType;

typedef struct ClientSocketInfo {
	uint64_t client_id;
	int sock;
	int speed;
	struct sockaddr connected_addr;
	struct sockaddr local_addr;
} ClientSocketInfo;

class ReliableSocket {
protected:
    SocketType socket_type;
    int sock;
	//Only used in client side
	int local_sock;
    int download_speed;
    int upload_speed;
    int send_speed;
    bool worker_running = false;
	bool fast_start = true;

	SharpCrypto *crypto;
    struct sockaddr *remote_addr = new sockaddr();
    struct sockaddr *connected_addr = new sockaddr();
    struct sockaddr *local_addr = new sockaddr();
    socklen_t remote_len = sizeof(*remote_addr);

    long rto;
    void set_rto(long rtt);

	pthread::Mutex segment_mutex;
	netstruct::PackageSegmentList segment_list = netstruct::PackageSegmentList(100000);
	netstruct::SegmentParser segment_recv;

    uint64_t send_id = 1;
    pthread::Mutex send_mutex;
    pthread::ConditionVariable send_cv;
    netstruct::PackageQueueMap send_pool = netstruct::PackageQueueMap(100000);

    uint64_t resend_now = 0;

    uint64_t recv_ack_id = 0;
    uint64_t send_ack_id = 0;
	/* packages with id less than send_from 
	   are well received or droped */
	uint64_t send_from = 0;
    pthread::Mutex ack_mutex;
    pthread::ConditionVariable ack_cv;
    netstruct::AckWaitList ack_pool = netstruct::AckWaitList(100000);

	pthread::Mutex ack_send_mutex;
	netstruct::AckQueue ack_send_queue = netstruct::AckQueue(100000);

    pthread::Mutex ack_recv_mutex;
    netstruct::IdSet ack_recv_pool = netstruct::IdSet(100000);

//    pthread::Mutex ping_mutex;
//    pthread::ConditionVariable ping_cv;
//    PackagePing *latest_ping = nullptr;

	time_struct_ms ping_send_time;

    uint64_t recv_id = 1;
    pthread::Mutex recv_mutex;
    pthread::ConditionVariable recv_cv;
    netstruct::PackageMap recv_pool = netstruct::PackageMap(100000);

    void start_thread_sender();
    void start_thread_ackhandler();
	void start_thread_segment();
//    void start_thread_heart_beat();
    void start_thread_receiver();
    //void process_package(PackageBase *package);

    void stop_all_thread();

    const int free_port_start = 50020;
    int free_port_now = 0;
    int get_free_port();

    /* Return a connected socket */
    ReliableSocket(int speed_send, int sock, sockaddr *connected_addr, sockaddr *local_addr, long rt, std::string key);
    int get_ipinfo(std::string host, int port, sockaddr *addr, socklen_t *socklen);

public:
    static int max_segment;
    ReliableSocket(int download_speed, int upload_speed, std::string key);

    ~ReliableSocket();
    int recv_package(unsigned char *buf);
    int send_package(unsigned char *buf, int size);
	
    int bind_addr(const struct sockaddr *bind_addr);
    int bind_addr(std::string host, int port);
    int connect_addr(std::string host, int port);
    int connect_addr(struct sockaddr *server_addr);
    int close_addr();
    int listen_addr();
    ReliableSocket* accept_connection();
    bool is_alive();
};


#endif //RELIABLEFASTUDP_RELIABLESOCKET_H
