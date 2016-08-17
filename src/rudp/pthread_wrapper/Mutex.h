//
// Created by maxxie on 16-7-13.
//

#ifndef RELIABLEFASTUDP_PTHREADMUTEX_H
#define RELIABLEFASTUDP_PTHREADMUTEX_H
#include "../ReliableFastUDP.h"
#ifndef TARGET_WIN32
#include <pthread.h>
#else
#include <Windows.h>
#endif // TARGET_WIN32

namespace pthread {
    class Mutex {
	public:
#ifndef TARGET_WIN32
		pthread_mutex_t mutex;
#else
		CRITICAL_SECTION mutex;
#endif // TARGET_WIN32
        Mutex();
        void lock();
        void unlock();
    };
}


#endif //RELIABLEFASTUDP_PTHREADMUTEX_H
