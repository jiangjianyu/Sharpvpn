//
// Created by maxxie on 16-7-13.
//

#include "Mutex.h"

using namespace pthread;

#ifndef TARGET_WIN32
Mutex::Mutex() {
    pthread_mutex_init(&mutex, 0);
}

void Mutex::lock() {
    pthread_mutex_lock(&mutex);
}

void Mutex::unlock() {
    pthread_mutex_unlock(&mutex);
}
#else
Mutex::Mutex() {
	InitializeCriticalSection(&mutex);
}

void Mutex::lock() {
	EnterCriticalSection(&mutex);
}

void Mutex::unlock() {
	LeaveCriticalSection(&mutex);
}
#endif // TARGET_WIN32
