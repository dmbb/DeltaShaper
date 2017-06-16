#include <thread.h>

#ifndef __cal_thread_h__
#define __cal_thread_h__


class CalibrationThread : public Thread {
    int* _state;
    int _caller;
    
  public:
    CalibrationThread(int* state, int caller);

    void* run();
}; 

#endif
