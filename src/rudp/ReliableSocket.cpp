//
// Created by jyjia on 2016/7/12.
//


#include <thread>
#include <iostream>
#include <csignal>
#include <cstring>
#include "ReliableSocket.h"

#ifndef TARGET_WIN32
#include <netinet/in.h>
#include<arpa/inet.h>
#include <netdb.h>
#else
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <ws2tcpip.h>
us_TIME cpu_frequency;
#endif // TARGET_WIN32

#define min(a, b) ((a) < (b))? (a) : (b)

namespace netstruct {
	class Sleeper {
	private:
		us_TIME start;
		int send_len = 0;
		int sleep_len;
		long sleep_time;
	public:
		Sleeper(int __sleep_len, long __sleep_time) {
			sleep_len = __sleep_len;
			sleep_time = __sleep_time;
		}
		void init() {
			get_time_us(start);
		}
		void send_sleep(int len) {
            send_len += len;
			if (send_len > sleep_len) {
				us_TIME now;
				get_time_us(now);
				long time_pass = get_us(now, start);
				if (time_pass < sleep_time) {
					std::this_thread::sleep_for(std::chrono::microseconds(sleep_time - time_pass));
				}
				get_time_us(start);
                send_len = 0;
			}
		}
	};
}


int ReliableSocket::max_segment = MAX_SEGMENT(1496);

ReliableSocket::ReliableSocket(int down_speed, int up_speed, std::string key) {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    download_speed = down_speed;
    upload_speed = up_speed;
    socket_type = RegularSocket;
    worker_running = false;
	crypto = new SharpCrypto(key);
	segment_recv.crypto = crypto;
}

int ReliableSocket::bind_addr(const struct sockaddr *bind_addr) {
    memcpy(local_addr, bind_addr, remote_len);
    int r = bind(sock, bind_addr, sizeof(*bind_addr));
    if (r == 0)
        socket_type = BindedSocket;
    if (r == 0)
        LOG(INFO) << "Bind socket to address successfully\n";
    else
        LOG(INFO) << "Bind socket to address fail\n";
    return r;
}

int ReliableSocket::connect_addr(struct sockaddr *server_addr) {
	time_struct_ms s0, s1;
	timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
    remote_len = sizeof(*server_addr);
    remote_addr = new sockaddr();
    connected_addr = new sockaddr();
    memcpy(remote_addr, server_addr, remote_len);
    memcpy(connected_addr, remote_addr, remote_len);
    PackageConnectionCreate create = PackageConnectionCreate(download_speed);
    LOG(INFO) << "Sending request to create connection";
	get_time_ms(s0);
    sendto(sock, (char*)create.bytes, create.length(), 0, remote_addr, remote_len);
    PackageConnectionCreateR create_r = PackageConnectionCreateR(100, 50020);
    int count = 0;
    int r;
	fd_set read_set;
	while (count < 5) {
		FD_ZERO(&read_set);
		FD_SET(sock, &read_set);
        tv.tv_sec = 1;
		select(sock + 1, &read_set, NULL, NULL, &tv);
		if (FD_ISSET(sock, &read_set)) {
			r = recvfrom(sock, (char *)create_r.bytes, MAX_MTU, 0, NULL, NULL);
			if (r > 0)
				break;
		}
		LOG(ERROR) << "error connecting to server, retry\n";
		get_time_ms(s0);
		sendto(sock, (char*)create.bytes, create.length(), 0, remote_addr, remote_len);
		count++;
	}
    /* check if recvfrom timeout */
    if(count >= 5)
        return -1;
    send_speed = min(create_r.create_r->download_speed, upload_speed);
    get_time_ms(s1);
    long rtt = get_ms(s1, s0);
    set_rto(rtt);
    struct sockaddr_in* addr = (struct sockaddr_in*)connected_addr;
    addr->sin_port = htons(create_r.create_r->port);
    LOG(INFO) << "connection established to port " << create_r.create_r->port;

	/* create a local socket to get exit requests */
	local_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in *local_in = (sockaddr_in*)local_addr;
	inet_pton(AF_INET, "127.0.0.1", &local_in->sin_addr);
	bool binded = false;
	local_in->sin_family = AF_INET;
	for (int i = 2000; i < 2010; i++) {
		local_in->sin_port = htons(i);
		if (bind(local_sock, local_addr, remote_len) == 0) {
			binded = true;
			break;
		}
	}
	if (!binded) {
		LOG(ERROR) << "error binding the control port 2000-2010\n";
		return -1;
	}

    worker_running = true;
    start_thread_sender();
    start_thread_receiver();
    start_thread_ackhandler();
    start_thread_segment();
    return 0;
}

ReliableSocket::~ReliableSocket() {
    delete remote_addr;
    delete connected_addr;
}

void ReliableSocket::set_rto(long rtt) {
	rto = 2*rtt;
	if (rto < 30)
		rto = 30;
    LOG(INFO) << "delay: " << rtt << "ms";
}

int ReliableSocket::close_addr() {
    PackageConnectionClose close_p = PackageConnectionClose();
    /* Does not need to ensure that the connection is really closed,
     * server will close them in a short time anyway */
    sendto(sock, (char*)close_p.bytes, close_p.length(), 0, connected_addr, remote_len);
    LOG(INFO) << "Sending request to close connection\n";
    worker_running = false;
    stop_all_thread();
    closesocket(sock);
    if (socket_type == AcceptableSocket) {
        //close all client socket
    }
    return 0;
}

void ReliableSocket::start_thread_sender() {
    /* Main Sender */
	INIT_US_TIMEER();
    std::thread([this]() {
		int fast_start_count = 0;
        PackageData *package_send;
        int list_package_size = 1024 * 10;
        int list_package_us = (double)list_package_size / send_speed * 8;
        int list_package_count;
		netstruct::Sleeper sleeper = netstruct::Sleeper(list_package_size, list_package_us);
		sleeper.init();
		PackagePing ping_package;
        ping_package.ping->ping_back = false;
        time_struct_ms tv;
		int package_count = 0;

        get_time_ms(ping_send_time);
        //ping_send_count++;
        sendto(sock, (char*)ping_package.bytes, ping_package.length(), 0,
               connected_addr, remote_len);
        LOG(INFO) << "Ping Remote";

        while (worker_running) {
            //every 50 package
			package_count++;
			if (package_count % 50 == 0) {
				package_count = 0;
				get_time_ms(tv);
				if (get_ms(tv, ping_send_time) > 20000) {
/*					if (ping_send_count >= 10 || send_not_receive >= 1000) {
						//stop
						stop_all_thread();
						LOG(INFO) << "can not receive response from remote, exit";
						return;
					}*/
					get_time_ms(ping_send_time);
					//ping_send_count++;
					sendto(sock, (char*)ping_package.bytes, ping_package.length(), 0,
						connected_addr, remote_len);
//					LOG(INFO) << "Ping Remote";
				}
			}
            list_package_count = 0;
            /* get from queue */
            send_mutex.lock();
            while (send_pool.size() == 0) {
                if (!worker_running) {
                    send_mutex.unlock();
                    LOG(INFO) << "sender thread exit";
                    return;
                }
                send_cv.wait(&send_mutex);
            }
            package_send = send_pool.pop();
            send_mutex.unlock();

			if (package_send == nullptr) {
				continue;
			}

            /* update una and send */
            package_send->data->una = recv_ack_id;
			package_send->send_count++;
            //send_not_receive++;
            list_package_count += package_send->length();
            sendto(sock, (char*)package_send->bytes, package_send->length(), 0,
                    connected_addr, remote_len);

			/* Fast recover, speed down?*/
			if (package_send->send_count >= 2) {
				list_package_count += package_send->length();
				sendto(sock, (char*)package_send->bytes, package_send->length(), 0,
					connected_addr, remote_len);
			}
//                LOG(INFO) << "send package " << package_send->data->package_id << " " << package_send->base->type << " " << package_send->length();
			//Fast start
			if (fast_start) {
				sendto(sock, (char*)package_send->bytes, package_send->length(), 0,
					connected_addr, remote_len);
				fast_start_count++;
				if (fast_start_count == 10) {
					fast_start_count = 0;
					fast_start = false;
				}
			}
//                package_send->send_after++;
            get_time_ms(package_send->send_time);

            /* add to ack watching list */
            ack_mutex.lock();
            ack_pool.push(package_send);
            ack_mutex.unlock();
            ack_cv.signal();
			sleeper.send_sleep(list_package_count);
        }
    }).detach();

	/* Ack Sender */
	std::thread([this]() {
		PackageAck ack_package = PackageAck(40);
		while (worker_running) {
			ack_send_mutex.lock();
			netstruct::AckQueue ack_read_list = ack_send_queue;
			ack_send_queue.rebase();
			ack_send_mutex.unlock();
			while (ack_read_list.size()) {
				ack_package.load(35, this->recv_ack_id, ack_read_list);
				ack_package.ack->send_from = send_from;
				sendto(sock, (char*)ack_package.bytes, ack_package.length(), 0, connected_addr, remote_len);
				sendto(sock, (char*)ack_package.bytes, ack_package.length(), 0, connected_addr, remote_len);
			}
			msleep(5);
		}
	}).detach();

}

void ReliableSocket::start_thread_ackhandler() {
    std::thread([this](){
        PackageData *ack_data;
		time_struct_ms tv;
        long time_spend;
        bool resend;
        while(worker_running) {
            /* get from queue */
            ack_mutex.lock();
            while (ack_pool.size() == 0) {
                if (!worker_running) {
                    ack_mutex.unlock();
                    LOG(INFO) << "ackhandler thread exit";
                    return;
                }
                ack_cv.wait(&ack_mutex);
            }
            ack_data = ack_pool.pop();
            ack_mutex.unlock();

            /* while una of the receiver is less than package id */
            resend = false;
            if (ack_data->send_count > 20) {
                send_pool.erase(ack_data->data->package_id);
				if (ack_data->data->package_id > send_from) {
					send_from = ack_data->data->package_id + 1;
				}
				LOG(INFO) << "drop package " << (long)ack_data->data->package_id;
                continue;
            }
            while (send_ack_id < ack_data->data->package_id) {
                ack_recv_mutex.lock();
                if (ack_recv_pool.find(ack_data->data->package_id) != 0) {
                    ack_recv_mutex.unlock();
                    break;
                }
                ack_recv_mutex.unlock();

                get_time_ms(tv);
                time_spend = get_ms(tv, ack_data->send_time);
                if (time_spend < rto) {
                    msleep(rto - time_spend);
                } else {
                    /* connection timeout, add to resend queue */
                    //LOG(INFO) << "Ack timeout, resend package " << (int)ack_data->data->package_id << ","<< ack_data->send_count;
                    send_mutex.lock();
                    send_pool.push(ack_data->data->package_id);
                    send_mutex.unlock();
                    send_cv.signal();
                    resend = true;
                    break;
                }
            }
            if (!resend) {
                send_pool.erase(ack_data->data->package_id);
            }
        }
        LOG(INFO) << "ackhandler thread exit\n";
    }).detach();
}

/*
void ReliableSocket::start_thread_heart_beat() {
    std::thread([this](){
		exit_heart_beat = false;
        int count = 0;
        time_struct_ms s0, s1;
        sleep(2);
        count = 0;
        bool miss = false;
        while (worker_running) {
            ping_mutex.lock();
            if (latest_ping != nullptr) {
				count = 0;
                delete latest_ping;
                latest_ping = nullptr;
            }
            ping_mutex.unlock();
            PackagePing ping = PackagePing();
            LOG(INFO) << "Sending Ping Package";
            get_time_ms(s0);
            miss = false;
            sendto(sock, ping.get_buf(), ping.length(),
                   0, connected_addr, remote_len);

            ping_mutex.lock();
            if (latest_ping == nullptr) {
                ping_cv.wait_time(&ping_mutex, rto);
            }
            //timeout

            if (latest_ping == nullptr) {
                count++;
                miss = true;
            } else {
                delete latest_ping;
                latest_ping = nullptr;
				count = 0;
            }
            ping_mutex.unlock();
            if (count >= 10) {
                // Connection Close
                stop_all_thread();
                sleep(2);
                closesocket(sock);
                LOG(INFO) << "heart beat thread exit";
				exit_heart_beat = true;
                return;
            }
            if (!miss) {
                get_time_ms(s1);
                set_rto(get_ms(s1, s0));
            }
			if (!worker_running) {
				break;
			}
            sleep(20);
        }
		LOG(INFO) << "heart beat thread exit";
		exit_heart_beat = true;
    }).detach();
}
*/

void ReliableSocket::start_thread_receiver() {
    std::thread([this]() {
		char buf[MAX_MTU];
        int r;
		fd_set read_set;
		timeval select_tv;
		time_struct_ms tv;
		long lasting_time;
		PackageType type;
		uint64_t recv_package_id;
		uint64_t recv_una;
		uint64_t ack_id;
		uint64_t ack_una;
		uint64_t max_ack_id;
		uint64_t send_from_recv;

		PackagePing ping_buf;
		PackageData *data_buf;
		PackageAck ack_buf = PackageAck(40);
        while (worker_running) {
			FD_ZERO(&read_set);
			FD_SET(sock, &read_set);
			if (socket_type == RegularSocket || socket_type == BindedSocket) {
				FD_SET(local_sock, &read_set);
			}
			select_tv.tv_sec = 20;
			select_tv.tv_usec = 0;
			select(sock + 1, &read_set, NULL, NULL, &select_tv);
			if ((socket_type == RegularSocket || socket_type == BindedSocket) && 
				FD_ISSET(local_sock, &read_set)) {
				LOG(INFO) << "exit msg recv\n";
				break;
			}
			if (FD_ISSET(sock, &read_set)) {
				r = recvfrom(sock, buf, MAX_MTU, 0, connected_addr, &remote_len);
			}
			else {
				continue;
			}
            //r = recvfrom(sock, package.get_buf(), MAX_MTU, 0, connected_addr, &remote_len);
            if (r > 0) {
                //send_not_receive = 0;
				type = parse_package_type((unsigned char*)buf);
				switch (type) {
				case Ping:
					ping_buf.load((unsigned char*)buf, r);
					if (!ping_buf.ping->ping_back) {
						//LOG(INFO) << "PACKAGE PROCESSING:Recv ping";
						ping_buf.ping->ping_back = true;
						sendto(sock, (char*)ping_buf.bytes, ping_buf.length(), 0, connected_addr, remote_len);
					}
					else {
						//LOG(INFO) << "PACKAGE PROCESSING:Recv ping back";
						get_time_ms(tv);
						lasting_time = get_ms(tv, ping_send_time);
                        //ping_send_count = 0;
						if (lasting_time < 2 * rto) {
							set_rto(lasting_time);
						}
					}
					break;
				case Data:
					data_buf = new PackageData((unsigned char*)buf, r);
					recv_package_id = data_buf->data->package_id;
					recv_una = data_buf->data->una;
//					LOG(INFO) << "Receive Data Package " << data_buf->data->package_id;
					recv_mutex.lock();
					/* Duplicated data package
					* Drop Package */
					if (recv_pool.find(data_buf->data->package_id) == nullptr &&
						recv_ack_id < recv_package_id) {
						recv_pool.set(recv_package_id, data_buf);
					}
					else {
						delete data_buf;
					}
					while (recv_pool.find(recv_ack_id + 1) != nullptr) {
						recv_ack_id++;
					}
					recv_mutex.unlock();
					recv_cv.signal();
					//            LOG(INFO) << "Send ack " << recv_package_id << " una " << recv_ack_id;
					ack_send_mutex.lock();
					ack_send_queue.push(recv_package_id);
					ack_send_mutex.unlock();

					/* if this procedure is running with multiple thread, mutex is needed */
					//            ack_recv_mutex.lock();
					send_ack_id = MAX(send_ack_id, recv_una);
					while (ack_recv_pool.find(send_ack_id + 1) != 0) {
						send_ack_id++;
					}
					//            ack_recv_mutex.unlock();
					break;
				case Ack:
					ack_buf.load((unsigned char*)buf, r);
					ack_una = ack_buf.ack->una;
					send_from_recv = ack_buf.ack->send_from;
                    max_ack_id = 0;
					/* if una number is illegal. */
					if (ack_una > send_id) {
						ack_una = 0;
					}
					if (send_from_recv > recv_ack_id) {
						recv_ack_id = send_from_recv;
					}
/*                    LOG(INFO) << "recv ack una " << ack_una;
					for (int i = 0;i < ack_buf.ack->ack_count;i++) {
						LOG(INFO) << "recv ack " << ack_buf.ack_list[i];
					}*/

					ack_recv_mutex.lock();
					/* If ack has already received, or ack number is illegal. */
					for (int i = 0; i < ack_buf.ack->ack_count; i++) {
						ack_id = ack_buf.ack_list[i];
						if (ack_id <= send_ack_id || ack_id > send_id) {
							continue;
						}
						max_ack_id = MAX(max_ack_id, ack_id);
						if (ack_recv_pool.find(ack_id) != 0) {
							continue;
						}
						ack_recv_pool.insert(ack_id);
					}
					send_ack_id = MAX(ack_una, send_ack_id);
					while (ack_recv_pool.find(send_ack_id + 1) != 0) {
						send_ack_id++;
					}
					ack_recv_mutex.unlock();

					/* fast resend */
					if (resend_now < send_ack_id)
						resend_now = send_ack_id;
					if (max_ack_id > resend_now + 2) {
						uint64_t resend_id = resend_now + 1;
						while (resend_id < max_ack_id) {
							ack_recv_mutex.lock();
							bool found = ack_recv_pool.find(resend_id) != 0;
							ack_recv_mutex.unlock();
							if (!found) {
                                //LOG(INFO) << "Fast Resend package " << (int)resend_id;
								send_mutex.lock();
								send_pool.push(resend_id);
								send_mutex.unlock();
								send_cv.signal();
							}
							resend_id++;
						}
						resend_now = max_ack_id;
					}
					break;
				case ConnectionClose:
					LOG(INFO) << "PACKAGE PROCESSING:connection close\n";
					stop_all_thread();
					closesocket(sock);
					break;
				default:
					LOG(ERROR) << "PACKAGE PROCESSING:Unknown package, dropping\n";
					break;
				}
            }
        }
        LOG(INFO) << "receiver thread exit";
    }).detach();
}



void ReliableSocket::start_thread_segment() {
	std::thread([this]() {
		netstruct::PackageSegment *segment;
		PackageData *package;
		unsigned char buf[MAX_MTU];
		int start;
		while (worker_running) {
			msleep(20);
			if (segment_list.size() <= 0)
				continue;
			segment_mutex.lock();
			netstruct::PackageSegmentList list = segment_list;
			segment_list.rebase();
			segment_mutex.unlock();
			while (send_id - send_ack_id > 8000) {
				msleep(10);
			}
			while (list.size() > 0) {
				start = 2;
				segment = list.front();
				while (list.size() > 0 && max_segment - 2 - start > segment->len) {
					*((uint16_t*)(buf + start - 2)) = segment->len;
					memcpy(buf + start, segment->buf, segment->len);
					list.pop();
					start += segment->len;
					delete segment;
					*((uint16_t*)(buf + start)) = 0;
					start += 2;
					segment = list.front();
				}
				if (start > 2) {
					package = new PackageData(start + SHARPVPN_CRYPTO_OVERHEAD);
					crypto->encrypt(buf, package->get_data(), start);
					send_mutex.lock();
					package->data->package_id = send_id;
					send_pool.push(package, send_id);
					send_id++;
					send_mutex.unlock();
					send_cv.signal();
				}
			}
		}
	}).detach();
}

int ReliableSocket::send_package(unsigned char *__buf, int __size) {
	if (!worker_running)
		return -1;
	netstruct::PackageSegment *seg = new netstruct::PackageSegment(__buf, __size);
	segment_mutex.lock();
	segment_list.push(seg);
	segment_mutex.unlock();
	return 0;
}

//void ReliableSocket::process_package(PackageBase *package) {
//    
//}

int ReliableSocket::recv_package(unsigned char *buf) {
    PackageData *return_data;
	int len = segment_recv.next_segment(buf);
	if (len != 0) {
		return len;
	}

    recv_mutex.lock();
	while (recv_pool.find(recv_id) == nullptr) {
		if (!worker_running) {
			recv_mutex.unlock();
			return -1;
		}
		if (recv_id < recv_ack_id) {
			recv_id++;
			continue;
		}
		recv_cv.wait(&recv_mutex);
	}
	return_data = recv_pool[recv_id];
    int recv_now = recv_id++;
    recv_mutex.unlock();
	segment_recv.load(return_data->get_data(), return_data->data_len);
	recv_pool.erase(recv_now);
	return segment_recv.next_segment(buf);
}

ReliableSocket* ReliableSocket::accept_connection() {
    uint64_t unique_id;
    if (socket_type != AcceptableSocket)
        return nullptr;
    PackageConnectionCreate create = PackageConnectionCreate(100);
    PackageConnectionCreateR create_r = PackageConnectionCreateR(50001, 100);
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(sock, &read_set);
    select(sock + 1, &read_set, NULL, NULL, NULL);
    if (FD_ISSET(sock, &read_set)) {
        recvfrom(sock, (char*)create.bytes, MAX_MTU, 0, connected_addr, &remote_len);
        if (create.base->type != ConnectionCreate) {
            LOG(INFO) << "not connection create package, droping";
        }
        else {
            unique_id = ((sockaddr_in*)connected_addr)->sin_port;
            unique_id = unique_id << 32 | ((sockaddr_in*)connected_addr)->sin_addr.s_addr;
            int sub_send_speed = min(create.create->download_speed, upload_speed);
            LOG(INFO) << "new request";
            struct sockaddr new_addr;
            memcpy(&new_addr, local_addr, remote_len);
            int po = get_free_port();
            ((sockaddr_in *)&new_addr)->sin_port = htons(po);
            int sock_cr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            while (bind(sock_cr, &new_addr, remote_len) != 0) {
                po = get_free_port();
                //        LOG(INFO) << "trying to bind client to port " << po << "\n";
                ((sockaddr_in *)&new_addr)->sin_port = htons(po);
            }
            LOG(INFO) << "new connection is binded to port " << po;
            create_r.create_r->port = po;
            create_r.create_r->download_speed = sub_send_speed;
            sendto(sock, (char*)create_r.bytes, create_r.length(), 0, connected_addr, remote_len);
            return new ReliableSocket(sub_send_speed, sock_cr, connected_addr, &new_addr, 200, crypto->get_key());
        }
    }
	return nullptr;
}

ReliableSocket::ReliableSocket(int speed_send, int sock_r, sockaddr *co_addr, sockaddr *local, long rt, std::string key) {
    send_speed = speed_send;
    set_rto(rt);
    socket_type = ServerSocket;
    sock = sock_r;
    memcpy(local_addr, local, remote_len);
    memcpy(remote_addr, co_addr, remote_len);
    memcpy(connected_addr, co_addr, remote_len);
    worker_running = true;
	crypto = new SharpCrypto(key);
	segment_recv.crypto = crypto;
    start_thread_sender();
    start_thread_ackhandler();
    start_thread_receiver();
    start_thread_segment();
}

int ReliableSocket::get_free_port() {
    int po = free_port_now + free_port_start;
    free_port_now = (free_port_now + 1) % 100;
    return po;
}

int ReliableSocket::listen_addr() {
    socket_type = AcceptableSocket;
    return 0;
}

int ReliableSocket::bind_addr(std::string host, int port) {
    sockaddr addr;
    socklen_t len;
    get_ipinfo(host, port, &addr, &len);
    return bind_addr(&addr);
}

int ReliableSocket::connect_addr(std::string host, int port) {
    sockaddr addr;
    socklen_t len;
    get_ipinfo(host, port, &addr, &len);
    return connect_addr(&addr);
}

int ReliableSocket::get_ipinfo(std::string host_s, int port, sockaddr *addr, socklen_t *addrlen) {
    struct addrinfo hints;
    struct addrinfo *res;
    int r;
    const char *host = host_s.c_str();
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
	hints.ai_family = AF_INET;
    if (0 != (r = getaddrinfo(host, NULL, &hints, &res))) {
        LOG(ERROR) << "SOCKET INITIALIZATION:getaddrinfo, error";
        return -1;
    }

    if (res->ai_family == AF_INET)
        ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(port);
    else if (res->ai_family == AF_INET6)
        ((struct sockaddr_in6 *)res->ai_addr)->sin6_port = htons(port);
    else {
        LOG(ERROR) << "SOCKET INITIALIZATION:unknown ai_family " << res->ai_family;
        freeaddrinfo(res);
        return -1;
    }
    memcpy(addr, res->ai_addr, res->ai_addrlen);
    *addrlen = res->ai_addrlen;
    return 0;
}

void ReliableSocket::stop_all_thread() {
    worker_running = false;
    send_cv.signal();
    ack_cv.signal();
    recv_cv.signal();
    int sock_cl = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char buf[5];
	sendto(sock_cl, buf, 1, 0, local_addr, remote_len);
	sendto(sock_cl, buf, 1, 0, local_addr, remote_len);
}

bool ReliableSocket::is_alive() {return worker_running; }