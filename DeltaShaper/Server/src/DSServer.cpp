#include "DSServer.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <mutex>
#include <bitset>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <vector>
#include <map>
#include <cmdline.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "WorkerThread.h"
#include "PhotoThread.h"

const int mode = 0; //0 = RS(ECC), 1 = Bit(NoECC)
const int worker_thread_num = 1;

using namespace std;

DSServer::DSServer(int locX, int locY, int caller, int pic_interval) : 
    _locX(locX), _locY(locY), _caller(caller), _pic_interval(pic_interval) {}


string DSServer::windowSearch(char* input)
{
    char* match;
    string winId;
    char* pch = strtok (input," ");
    while (pch != NULL){
        if(strstr(pch,"0x")){
            winId = pch;
            break;
        }
        pch = strtok (NULL, " ");
    }
    return winId;
}

string DSServer::prepareWindow(){
    cout << "Preparing window capture..." << endl;
    FILE *fpipe;
    char line[512];         
    if ( !(fpipe = (FILE*)popen("DISPLAY=:1 xwininfo -tree -root","r")) ) {  
      perror("Pipe is leaking...");
      exit(1);
    }

    string win;

    while ( fgets( line, sizeof line, fpipe)) {
     if(strstr(line,"Call with")){
        win = windowSearch(line);
        resizeWindow(win);
        break;
     }
    }
    pclose(fpipe);

    sleep(5);  //Allow for window to stabilize before launching threads
    return win;
}

void DSServer::resizeWindow(string win){
    //Set window to 640x480 and drag mouse out of it

    ostringstream ss,click,move,activate;

    ss << "DISPLAY=:1 xdotool windowsize " << win << " " << WIN_WIDTH << " " << WIN_HEIGHT;
    int xdo_result = system(ss.str().c_str());  

    string xdo = "DISPLAY=:1 xdotool mousemove --sync 1000 1000";
    xdo_result = system(xdo.c_str());
}


void DSServer::Start(){
    wqueue<string> _queue;
    map<int, StagedFile> fileStore;
    mutex mtx; 

    //Capture call window & resize it
    string win = prepareWindow();

    //Launch threads
    array<WorkerThread*,worker_thread_num> threadV;
    PhotoThread* photoThread = new PhotoThread(_queue, win, _pic_interval);
    photoThread->start();

    for (int i = 0; i < threadV.size(); i++){
      switch(mode){
          case 0: 
              threadV[i] = new RSWorkerThread(_queue, fileStore, _locX, _locY, _caller, mtx);
              break;
          case 1: 
              threadV[i] = new BitWorkerThread(_queue, fileStore, _locX, _locY, _caller, mtx);
              break;
      }
        threadV[i]->start();
    }

    cout << "[S] Server Started" << endl;
    if(_caller){
        int fd;
        string pipe = "/tmp/cal_pipe";

        //Insert calibration 
        fd = open(pipe.c_str(), O_WRONLY);
        int status = write(fd, "1", sizeof("1"));
        if(!status)
            cout << "[S] Write to cal_pipe failed" << endl;
        close(fd);
        cout << "[S] Starting initial system calibration" << endl;
    }

    string cmd;
    while (getline(cin, cmd)){
        if (cmd.empty())
            exit(0);
    }

}

int main(int argc, char *argv[]) {

   cmdline::parser parser;

   parser.add<string>("caller", 'r', "Caller 1, Callee 0", true, "");
   parser.add<string>("pic_interval", 'p', "Skype Viewport-Grabbing Interval (micro secs)", true, "");
   parser.parse_check(argc, argv);

   int caller = atoi((parser.get<string>("caller")).c_str());
   int pic_interval = atoi((parser.get<string>("pic_interval")).c_str());
	
   //Default starting parameters
   int w_frame = 320;
   int h_frame = 240;
   int w_div = 40;
   int h_div = 30;
   int loc_x = 0;
   int loc_y = 0;

   DSServer* server = new DSServer(loc_x, loc_y, caller, pic_interval);
   server->Start();
   
    return 0;
}
