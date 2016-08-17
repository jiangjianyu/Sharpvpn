//
// Created by maxxie on 8/14/16.
//

#include <iostream>
#include "../src/SharpTunnel.h"

int main() {
    SharpTunnel tunnel("utun0", "172.16.0.2", 1400);
    tunnel.init(false);
    tunnel.set_ip(0x0200070a);
    int a;
    std::cin >> a;
}