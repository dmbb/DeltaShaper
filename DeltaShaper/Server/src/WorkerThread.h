#include <thread.h>
#include <wqueue.h>
#include <string>
#include <mutex>
#include <map>
#include "StagedFile.h"
#include "Item.h"

const int WIN_WIDTH = 640;
const int WIN_HEIGHT = 480;

class WorkerThread : public Thread {
  protected:
    wqueue<string>& _queue;
    std::map<int,StagedFile>& _fileStore;
    std::mutex& _mtx; 
    int _locX;
    int _locY;
    int _caller;

    void deliverPacket(int packetID);
    std::string bitstringToASCII(string frameData);
    virtual Item divdecodeImage(vector<char>& img) = 0;
    
  public:
    WorkerThread(wqueue<std::string>& queue, std::map<int, StagedFile>& fileStore, int locX, int locY, int caller, std::mutex& mtx);
    
    void* run();
};

class RSWorkerThread : public WorkerThread {
  public:
    RSWorkerThread(wqueue<std::string>& queue, std::map<int, StagedFile>& fileStore, int locX, int locY, int caller, std::mutex& mtx) : WorkerThread(queue, fileStore, locX, locY, caller, mtx){}
    Item divdecodeImage(vector<char>& img);
};

class BitWorkerThread : public WorkerThread {
  public:
    BitWorkerThread(wqueue<std::string>& queue, std::map<int, StagedFile>& fileStore, int locX, int locY, int caller, std::mutex& mtx) : WorkerThread(queue, fileStore, locX, locY, caller, mtx){}
    Item divdecodeImage(vector<char>& img);
};
