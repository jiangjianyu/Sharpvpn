//
// Created by maxxie on 16-7-13.
//

#ifndef RELIABLEFASTUDP_PTHREADCONDITIONVARIABLE_H
#define RELIABLEFASTUDP_PTHREADCONDITIONVARIABLE_H

#include <sys/types.h>
#include "Mutex.h"
namespace pthread {

    class ConditionVariable {
    private:
#ifndef TARGET_WIN32
		pthread_cond_t condition_variable;
#else
		CONDITION_VARIABLE condition_variable;
#endif // TARGET_WIN32
    public:
        ConditionVariable();
        void wait(Mutex *mutex);

#ifndef TARGET_MACOSX
        int wait_time(Mutex *mutex, unsigned long msec);
#endif
        void signal();
        void broadcast();
    };

}


#endif //RELIABLEFASTUDP_PTHREADCONDITIONVARIABLE_H
