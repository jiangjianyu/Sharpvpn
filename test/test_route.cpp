//
// Created by maxxie on 16-7-27.
//

#include <iostream>
#include "../src/RouteTable.h"

int main() {
    RouteTable table;
    RouteRow row;
    row.dst = "0.0.0.0";
    row.netmask = "128.0.0.1";
	row.netmask_sharp = 1;
    row.gateway = "10.7.0.1";
    row.dev = 7;
	table.init_table();

    table.add_route_row(row, true);
//    table.del_route_row(row);
//    RouteRow row_de = table.get_default_route();
//    LOG(INFO) << row_de.dst << " " << row_de.gateway << " " << row_de.dev;
//    int a;
//    std::cin >> a;
//    table.set_default_as_tunnel("eth1");
//    RouteRow r = table.get_default_route();
//    std::cout << r.dst << " " << r.gateway << " " << r.dev << " " << r.netmask;
    //RouteTable::set_chnroute("/etc/chnroute.txt", "159.203.238.143");
    return 0;
}