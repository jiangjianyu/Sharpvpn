//
// Created by maxxie on 16-7-20.
//

#include <thread>
#include "SharpVpn.h"
#include "SharpPackageParser.h"
#include <fstream>
#ifndef TARGET_WIN32
#include <netinet/in.h>
#endif // TARGET_LINUX

#ifdef TARGET_MACOSX
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif



typedef struct SharpDhcpInfo {
    uint32_t ip;
} SharpDhcpInfo;

static void func_socket(SharpVpn *vpn, ReliableSocket *connected_socket) {
    ssize_t r;
    unsigned char tun_buf[MAX_MTU];
//    unsigned char udp_buf[MAX_MTU];
    char token[24];
    int token_len;
    SharpDhcpInfo info;
    SharpUserInfo user_info;

    if (vpn->mode == SharpvpnModeServer) {
        for (std::map<std::string, SharpUserInfo>::iterator itr = vpn->user_map.begin();
             itr != vpn->user_map.end();) {
            uint32_t ip = itr->second.ip;
            if (!vpn->ip_map[ip]->is_alive()) {
                delete vpn->ip_map[ip];
                vpn->ip_map.erase(ip);
                itr = vpn->user_map.erase(itr);
            } else {
                itr++;
            }
        }
        token_len = connected_socket->recv_package((unsigned char*)token);
        std::string token_s = token;
        uint32_t ip;
        if (vpn->user_map.find(token_s) == vpn->user_map.end()) {
            ip = vpn->get_useable_ip();
            LOG(INFO) << "assigned new ip for client";
            info.ip = ip;
            user_info.ip = ip;
            vpn->user_map[token_s] = user_info;
            vpn->ip_map[ip] = connected_socket;
        } else {
            ip = vpn->user_map[token_s].ip;
            LOG(INFO) << "assigned used ip for client";
            info.ip = ip;
            vpn->ip_map[ip]->close_addr();
            msleep(500);
            delete vpn->ip_map[ip];
            vpn->ip_map[ip] = connected_socket;
        }
        connected_socket->send_package((unsigned char*)&info, sizeof(SharpDhcpInfo));
    }
    while (connected_socket->is_alive()) {
        r = connected_socket->recv_package(tun_buf);
        if (r == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // do nothing
            } else if (errno == ENETUNREACH || errno == ENETDOWN ||
                       errno == EPERM || errno == EINTR) {
                // just log, do nothing
            } else {
                // TODO rebuild socket
                break;
            }
        } else if (r == 0) {
            continue;
        }
//        de_r = vpn->get_crypto()->decrypt(tun_buf, udp_buf, r);
//        if (!de_r) {
//            continue;
//        }
        vpn->get_tunnel()->write(tun_buf, r);
    }
    if (vpn->mode == SharpvpnModeClient)
        vpn->stop();
}

int SharpVpn::init() {
    int return_status;
    return_status = tunnel->init(mode == SharpvpnModeServer);
    if (return_status < 0)
        return return_status;
    if (mode == SharpvpnModeServer) {
		if (sock->bind_addr(host, port) < 0)
			return -1;
        sock->listen_addr();
    }
    return 0;
}

SharpVpn::SharpVpn(VpnArgs args) {
    mode = args.mode;
    host = args.bind_host;
    port = args.bind_port;
    tunnel = new SharpTunnel(args.tunnel_name, args.tunnel_ip, TUNNEL_MTU(args.mtu));
	ReliableSocket::max_segment = MAX_SEGMENT(args.mtu);
    sock = new ReliableSocket(args.download_speed, args.upload_speed, args.encryption_key);
//    crypto = new SharpCrypto(args.encryption_key);
    vpn_args = args;
}

int SharpVpn::run() {
    running = true;
    ReliableSocket *connected_socket;
    if (mode == SharpvpnModeServer) {
        std::thread([this](){
            ReliableSocket *client_socket;
            SharpUserInfo user_info;
            while (running) {
                client_socket = sock->accept_connection();
                /* assign ip address */
                if (client_socket == nullptr) {
                    continue;
                }
                std::thread(func_socket, this, client_socket).detach();
            }
        }).detach();

    } else {
        /* connect and dhcp */
        SharpDhcpInfo info;
        LOG(INFO) << "Connecting to remote server...";
		vpn_status = VpnConnectingServer;
		if (handler != nullptr)
			(*handler)(handler_in, VpnConnectingServer);
        if (sock->connect_addr(host, port) < 0) {
            LOG(ERROR) << "Connecting to remote server failed.";
            return -1;
        }
        LOG(INFO) << "Connecting to remote server successfully.";
		vpn_status = VpnConnected;
		if (handler != nullptr)
			(*handler)(handler_in, VpnConnected);
        connected_socket = sock;
        connected_socket->send_package((unsigned char *)vpn_args.token.c_str(), vpn_args.token.length());
        connected_socket->recv_package((unsigned char*)&info);
        tunnel->set_ip(info.ip);
        if (vpn_args.chinadns_host != "") {
            chinadns = Chinadns("127.0.0.1", 53, vpn_args.chnroute_file);
            chinadns.start();
        }
        table.init_table();
        table.set_chnroute(vpn_args.chnroute_file, host);
        msleep(30);
#ifdef TARGET_WIN32
		table.set_default_as_tunnel(tunnel->get_if_index(), tunnel->get_tunnel_gateway());
#else
		table.set_default_as_tunnel(vpn_args.tunnel_name, tunnel->get_tunnel_gateway());
#endif
        std::thread(func_socket, this, connected_socket).detach();
    }

    std::thread([this](){
        unsigned char tun_buf[MAX_MTU];
//        unsigned char udp_buf[MAX_MTU];
        ssize_t r;
        ssize_t en_r;
        ReliableSocket *tunnel_socket;
        if (mode == SharpvpnModeClient) {
            tunnel_socket = sock;
        }
        while (running) {
            r = tunnel->read(tun_buf, MAX_MTU);
            if (r <= 0) {
                continue;
            }
            SharpPackageParser parser = SharpPackageParser::parse_package(tun_buf);
            if (parser.ip_type != ipv4 || parser.multicast) {
                continue;
            }
//            en_r = crypto->encrypt(tun_buf, udp_buf, r);
            if (mode == SharpvpnModeServer){
                if (ip_map.find(parser.ipv4_hdr->daddr) != ip_map.end()) {
                    tunnel_socket = ip_map[parser.ipv4_hdr->daddr];
                    if (!tunnel_socket->is_alive()) {
                        LOG(INFO) << "wrong dest address, can not send this package";
                        continue;
                    }
                } else {
                    LOG(INFO) << "wrong dest address, can not send this package";
                    continue;
                }
            }
            tunnel_socket->send_package(tun_buf, r);
        }
    }).join();
    return 0;
}

VpnArgs SharpVpn::parse_file_args(std::string file) {
    Json::Reader reader;
    Json::Value root;
    std::ifstream is;
    is.open(file, std::ios_base::binary);
    VpnArgs args = get_default_args();
    args.error = false;
    if (reader.parse(is, root)) {
        if (!root["mode"].isNull()) {
            args.mode = (root["mode"].asString() == "server") ? SharpvpnModeServer: SharpvpnModeClient;
        }
        if (!root["server"].isNull()) {
            args.bind_host = root["server"].asString();
        }
        if (!root["port"].isNull()) {
            args.bind_port = root["port"].asInt();
        }
        if (!root["tunnel_name"].isNull()) {
            args.tunnel_name = root["tunnel_name"].asString();
        }
        if (!root["mtu"].isNull()) {
            args.mtu = root["mtu"].asInt();
        }
        if(!root["tunnel_ip"].isNull()) {
            args.tunnel_ip = root["tunnel_ip"].asString();
        }
        if (!root["download_speed"].isNull()) {
            args.download_speed = root["download_speed"].asInt();
        }
        if (!root["upload_speed"].isNull()) {
            args.upload_speed = root["upload_speed"].asInt();
        }
        if (!root["password"].isNull()) {
            args.encryption_key = root["password"].asString();
        }
        if (!root["log"].isNull()) {
            args.log_file = root["log"].asString();
        }
        if (!root["chnroute"].isNull()) {
            args.chnroute_file = root["chnroute"].asString();
        }
        if (!root["chinadns_host"].isNull()) {
            args.chinadns_host = root["chinadns_host"].asString();
        }
        if (!root["token"].isNull()) {
            args.token = root["token"].asString();
        }
    } else {
        LOG(ERROR) << "parsing json file fail";
        args.error = true;
    }
    is.close();
    return args;
}

VpnArgs SharpVpn::get_default_args() {
    VpnArgs args;
    args.bind_host = "0.0.0.0";
    args.bind_port = 50010;
    args.download_speed = 100;
    args.upload_speed = 100;
    args.encryption_key = "my_password";
    args.mode = SharpvpnModeServer;
    args.mtu = 1496;
    args.tunnel_ip = "10.7.0.1";
    args.tunnel_name = "tun0";
    args.pid_file = "/var/run/sharpvpn.pid";
    args.log_file = "sharpvpn.log";
    args.chnroute_file = "";
    args.chinadns_host = "";
    args.token = "default_token";
    return args;
}

uint32_t SharpVpn::get_useable_ip() {
    uint32_t proposed_ip;
    for (int i = 1;i < 200;i++) {
        proposed_ip = htonl(tunnel->get_local_tunnel_ip() + i);
        if (ip_map.find(proposed_ip) == ip_map.end()) {
            return proposed_ip;
        }
    }
    return 0;
}

int SharpVpn::refresh_args_file(VpnArgs args, std::string file) {
    Json::Value root;
    Json::FastWriter writer;
    root["server"] = args.bind_host;
    root["port"] = args.bind_port;
    root["tunnel_ip"] = args.tunnel_ip;
    root["tunnel_name"] = args.tunnel_name;
    root["mtu"] = args.mtu;
    root["download_speed"] = args.download_speed;
    root["upload_speed"] = args.upload_speed;
    root["password"] = args.encryption_key;
	root["chnroute"] = args.chnroute_file;
	root["chinadns_host"] = args.chinadns_host;
    root["token"] = args.token;
    if (args.chinadns_host != "")
        root["chinadns_host"] = args.chinadns_host;
    if (args.mode == SharpvpnModeClient)
        root["mode"] = "client";
    else
        root["mode"] = "server";
    root["chnroute"] = args.chnroute_file;
    std::ofstream ofs;
    ofs.open(file);
    if (!ofs) {
        return -1;
    }
    ofs << writer.write(root);
    ofs.close();
    return 0;
}

int SharpVpn::stop() {
    if (!running) {
        return 0;
    }
    this->running = false;
#ifndef TARGET_WIN32
    std::stringstream ss;
    unsigned long last_dot = vpn_args.tunnel_ip.find_last_of(".", vpn_args.tunnel_ip.length());
    std::string end_ip = vpn_args.tunnel_ip.substr(0, last_dot);
    ss << "timeout 0.1 " << "ping -c1 " << end_ip << ".254";
    system(ss.str().c_str());
#endif
    msleep(500);
    tunnel->shutdown();
	if (vpn_args.mode == SharpvpnModeClient && vpn_status == VpnConnected)
		table.delete_chnroute(vpn_args.chnroute_file, vpn_args.bind_host);
	chinadns.stop();
    sock->close_addr();
    msleep(500);
	vpn_status = VpnClosed;
	if (handler != nullptr)
		(*handler)(handler_in, VpnClosed);
    return 0;
}

SharpVpn::~SharpVpn() {
    delete this->tunnel;
//    delete this->crypto;
    delete this->sock;
}
