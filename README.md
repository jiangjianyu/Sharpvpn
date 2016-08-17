#Sharpvpn

Sharpvpn is a fast, convenient and safe vpn based on libsodium. Sharpvpn mainly focus on providing vpn service between two devices connected in network with high latency and high rate package loss. It was motivated by multiple project: Shadowvpn, Chinadns, finalspeed, etc.

Sharpvpn use libsodium to encrypt data packets, chinadns to prevent from dns pollution and a self-defined-protocol rudp to make use of the bandwidth.



##Install

Dependency: libsodium glog libnet libpcap jsoncpp

###Linux

````
sudo apt-get install cmake libsodium-dev libglog-dev libnet1-dev libpcap-dev libjsoncpp-dev
cmake ./
make
````



###Windows

Please download the prebuild version of Sharpvpn-windows(link)



###macOS

````bash
brew install cmake libsodium libglog libnet libpcap jsoncpp
cmake ./
make
````



##Configuration

There is only one configuration file, and a sample shows below.

```json

{
  "server":"127.0.0.1",
  "port":50010,
  "tunnel_ip":"10.7.0.1",
  "tunnel_name":"tun0",
  "mtu":1496,
  "download_speed":100,
  "upload_speed":100,
  "password":"my_password",
  "mode":"client",
  "chinadns_host":"127.0.0.1",
  "token":"maxxie-1"
}
```

Explanation of the fields above:

| Name           | Explanation                              |
| :------------- | :--------------------------------------- |
| server         | the address of the server                |
| port           | server port                              |
| tunnel_ip      | ip address of the tunnel (for server only) |
| tunnel_name    | name of the tunnel, tun0 for linux, utun0 for macOS |
| mtu            | mtu of your network, 1496 works in most case |
| download_speed | download bandwidth of your network(Mbps) |
| upload_speed   | upload bandwidth of your network(Mbps)   |
| password       | key for encryption                       |
| mode           | running mode, client or server           |
| chinadns_host  | the address chinadns is listened         |
| chnroute       | chnroute file                            |
| token          | a token to identify different client(for client only) |

##Usage

```
./Sharpvpn -c /etc/sharpvpn/(client/server).json -d start
./Sharpvpn -c /etc/sharpvpn/(client/server).json -d stop
```
