//
// Created by maxxie on 16-7-22.
//

#include "SharpPackageParser.h"

SharpPackageParser SharpPackageParser::parse_package(unsigned char *raw_tunnel_package) {
    /* return nothing when package carry wrong format of message */
    return SharpPackageParser(raw_tunnel_package);
}

SharpPackageParser::SharpPackageParser(unsigned char *tunnel_package) {
    raw = tunnel_package;
    ipv4_hdr = (ipv4_hdr_t*)raw;
	if ((ipv4_hdr->ver & 0xf0) == 0x60) {
		ip_type = ipv6;
	}
	else if ((ipv4_hdr->ver & 0xf0) == 0x40) {
		ip_type = ipv4;
	}
	else {
		ip_type = unknown_ipver;
	}
	if ((ipv4_hdr->daddr & 0x000000f0) == 0x000000e0) {
		multicast = true;
	}
	else {
		multicast = false;
	}
}
