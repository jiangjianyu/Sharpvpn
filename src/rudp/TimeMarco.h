//
// Created by maxxie on 16-8-10.
//

#ifndef SHARPVPN_TIMEMARCO_H
#define SHARPVPN_TIMEMARCO_H

#include "../Sharp.h"
#ifndef TARGET_WIN32
typedef struct timeval time_struct_ms;
#define get_time_ms(t) gettimeofday(&t, 0)
#define set_time_ms(tv, m) tv.tv_sec = (m)/1000; \
                             tv.tv_sec = (m)%1000
#define get_ms(tv_a, tv_b) ((tv_a.tv_sec - tv_b.tv_sec) * 1000 + (tv_a.tv_usec - tv_b.tv_usec) / 1000)
#define get_us(tv_a, tv_b) ((tv_a.tv_sec - tv_b.tv_sec) * 1000000 + (tv_a.tv_usec - tv_b.tv_usec))
#define INIT_US_TIMEER()
#define get_time_us(t) gettimeofday(&t, 0)
typedef struct timeval us_TIME;
#else
typedef DWORD time_struct_ms;

#define get_time_ms(t) t = timeGetTime()
#define get_ms(tv_a, tv_b) ((tv_a) - (tv_b))
#define set_time_ms(tv, m) tv = (m)
#define sleep(s) Sleep((s)*1000)
#define msleep(s) Sleep(s)

typedef LARGE_INTEGER us_TIME;
#define set_time_ms(tv, t) tv = (t)

#define INIT_US_TIMEER() QueryPerformanceFrequency(&cpu_frequency)
#define get_time_us(tv) QueryPerformanceCounter(&tv)
#define get_us(s1, s0) (s1.QuadPart - s0.QuadPart) * 1000000 / cpu_frequency.QuadPart
#endif // TARGET_WIN32
#endif //SHARPVPN_TIMEMARCO_H
