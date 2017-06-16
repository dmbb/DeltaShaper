#include <wqueue.h>
#include <thread.h>
#include "TransmissionID.h"

#ifndef __t_thread_h__
#define __t_thread_h__

typedef struct UserData {
int snowmixPID;
int backgroundPID;
int outputPID;
int ffmpegPID;
int framerate;
int* w_frame;
int* h_frame;
int* w_div;
int* h_div;
int* nbits;
int* nextSelector;
wqueue<TransmissionID> *queue;
} UserData;

class TransmissionThread : public Thread {
    wqueue<TransmissionID>& _transmission_queue;
    int* _signaled;
    int* _w_frame;
	int* _h_frame;
	int* _w_div;
	int* _h_div;
	int* _nbits;
    int _framerate;
    int* _next_selector;
    
  public:
    TransmissionThread(int* signaled, wqueue<TransmissionID>& queue, int* w_frame, int* h_frame, int* w_div, int* h_div, int* nbits, int framerate, int* nextSelector);
    
    void InitPipelines(int *snowmixPID, int *outputPID, int *backgroundInputPID, int *ffmpegPID);

    void* run();
}; 

#endif
