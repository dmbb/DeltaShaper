#include <string>
#include <wqueue.h>
#include <thread.h>
#include <chrono>
#include <iostream>
#include <sstream>


class PhotoThread : public Thread {
    int _index;
    wqueue<string>& _queue;
    string _callId;
    int _pic_interval;
    
  public:
    PhotoThread(wqueue<string>& queue, string callId, int pic_interval) : _index(1), _queue(queue), _callId(callId), _pic_interval(pic_interval) {}
 
    void* run() {
        int xwd_result;
        while (1){    
            auto begin = chrono::high_resolution_clock::now();
            ostringstream ss;

            /*auto timeSent = chrono::high_resolution_clock::now();    
            std::cout << "(PhotoThread) Took picture: "
            << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
            << '\n';*/

            ss << "DISPLAY=:1 xwd -id " << _callId << " -silent > win" << _index;
            xwd_result = system(ss.str().c_str());
            
            if(xwd_result < 0){
                cout << "The call was ended" << endl;
                exit(0);       
            }


            ss.clear();
            ss.str("");
            
            ss << "win" << _index;
            _queue.add(ss.str().c_str());
            
            _index+=1;

            //Measure elapsed Time
            /*auto end = chrono::high_resolution_clock::now();    
            auto dur = end - begin;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            cout << "(PhotoThread) Elapsed time: " << ms << "ms" << endl;*/

            usleep(_pic_interval);
          }
   }
}; 
