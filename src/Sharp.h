//
// Created by maxxie on 16-7-19.
//

#ifndef SHARPVPN_SHARP_H
#define SHARPVPN_SHARP_H

//#define TARGET_WIN32
//#define TARGET_LINUX

#ifdef 	__gnu_linux__
#define TARGET_LINUX
#elif defined(__APPLE__)
#define TARGET_MACOSX
#endif

//#ifdef 	_WIN32
//#define TARGET_WIN32
//#endif

#ifdef TARGET_WIN32
typedef unsigned long ssize_t;
typedef int dev_type;
#define msleep(s) Sleep(s)
#define GLOG_NO_ABBREVIATED_SEVERITIES

#define WIN32_NETWORK_INIT() { \
	WORD wVersionRequested; \
	WSADATA wsaData; \
	int err; \
	wVersionRequested = MAKEWORD(2, 2); \
	err = WSAStartup(wVersionRequested, &wsaData); \
	if (err != 0) { \
		std::cout << "WSAStartup failed with error " << err << "\n"; \
		return 1; \
	} \
}
#else

#include <string>
typedef std::string dev_type;
#endif // TARGET_WIN32

namespace c_file {
    extern "C" ssize_t read(int __fd, void *__buf, size_t __nbytes);
    extern "C" ssize_t write(int __fd, const void *__buf, size_t __n);
}

#define MAX(a, b) ((a) > (b))? (a) : (b)
#define MIN(a, b) ((a) < (b))? (a) : (b)

#ifdef NO_GLOG
#include "PipeLogger.h"
#else
#include <glog/logging.h>
#endif
#define MAX_MTU 1600

#ifndef TARGET_WIN32
#define msleep(s) usleep((s) * 1000)
#endif // TARGET_LINUX

#endif //SHARPVPN_SHARPVPN_H
