#include <array>
#include "WorkerThread.h"
#include "TransmissionThread.h"
#include "CalibrationThread.h"

#ifndef __dsclient_h__
#define __dsclient_h__

class DSClient {
  private:  
    int* _w_frame;
    int* _h_frame;
    int* _w_div;
    int* _h_div;
    int _caller;
    int* _nbits;
    int* _framerate;
    int* _next_selector;
    array<WorkerThread*,1> _threadV;
    TransmissionThread* _transmissionThread;
    CalibrationThread* _calibrationThread;
    wqueue<Item*> _queue;
    wqueue<TransmissionID> _transmission_queue;
    std::mutex _mtx;

  public:
    DSClient(int* w_frame, int* h_frame, int* w_div, int* h_div, int caller, int* framerate, int* nbits, int* nextSelector);
 	int Start(int mode);
    void RSFill(unsigned char* file, int pktLen, int packetID);
    void BitFill(unsigned char* file, int pktLen, int packetID);
    void TriBitFill(unsigned char* file, int pktLen, int packetID);
    int getCallerMode() { return _caller; }
};

int handler(struct nfq_q_handle *myQueue, struct nfgenmsg *msg, struct nfq_data *pkt,   void *cbData);
#endif