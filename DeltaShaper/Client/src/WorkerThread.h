#include "Item.h"
#include "TransmissionID.h"
#include <thread.h>
#include <wqueue.h>
#include <mutex>
#include <tuple>
#include <vector>

#ifndef __w_thread_h__
#define __w_thread_h__

typedef struct Selector {
std::pair<int,int> payloadArea;
int cellSize;
int bitNumber;
} Selector;



class WorkerThread : public Thread {
  protected:  
    int* _state;
    wqueue<Item*>& _queue;
    wqueue<TransmissionID>& _transmission_queue;
    std::mutex& _mtx;
    int _caller;
    int* _w_frame;
    int* _h_frame;
    int* _w_div;
    int* _h_div;
    int* _framerate;
    int* _nbits;
    int* _next_selector;

    virtual void colorFill(int packetId, int packetFrag, int totalFrag, std::vector<unsigned char> content) = 0;

  public:
    WorkerThread(int* state, wqueue<Item*>& queue, wqueue<TransmissionID>& transmission_queue, std::mutex& mtx, int caller, int* w_div, int* h_div, int* w_frame, int* h_frame, int* framerate, int* nbits, int* nextSelector);
    void calibrationColorFill(int packetFrag, int w, int h, int cellSize, int nbits, int opacity);
    void dummyColorFill(int packetFrag, int w, int h, int cellSize, int nbits);
    void* run();
};

class RSWorkerThread : public WorkerThread {
  public:
    RSWorkerThread(int* state, wqueue<Item*>& queue, wqueue<TransmissionID>& transmission_queue, std::mutex& mtx, int caller, int* w_div, int* h_div, int* w_frame, int* h_frame, int* framerate, int* nbits, int* nextSelector) : WorkerThread(state, queue, transmission_queue, mtx, caller, w_div, h_div, w_frame, h_frame, framerate, nbits, nextSelector){}
    void colorFill(int packetId, int packetFrag, int totalFrag, std::vector<unsigned char> content);
};

class BitWorkerThread : public WorkerThread {
  public:
    BitWorkerThread(int* state, wqueue<Item*>& queue, wqueue<TransmissionID>& transmission_queue, std::mutex& mtx, int caller, int* w_div, int* h_div, int* w_frame, int* h_frame, int* framerate, int* nbits, int* nextSelector) : WorkerThread(state, queue, transmission_queue, mtx, caller, w_div, h_div, w_frame, h_frame, framerate, nbits, nextSelector){}
    void colorFill(int packetId, int packetFrag, int totalFrag, std::vector<unsigned char> content);
};

#endif