#include "CalibrationThread.h"  
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

int readPipe(int fd, int* state)
    {
        ssize_t bytes;
        int newState;
        size_t total_bytes = 0;
        char buffer[10];

        while(true){
            bytes = read(fd, buffer, sizeof(buffer));
            if (bytes > 0) {
                total_bytes += (size_t)bytes;
                newState = buffer[0] - '0';
            } else {
                if (errno == EWOULDBLOCK) {
                    break;
                } else {
                    perror("read error");
                    return EXIT_FAILURE;
                }
            }
        }
        return newState;
    }

CalibrationThread::CalibrationThread(int* state, int caller) : _state(state), _caller(caller) {
    // create pipe to monitor
    int res = system("mkfifo /tmp/cal_pipe");
}


void* CalibrationThread::run() {
    
    int fd_a;
    int nfd;
    fd_set read_fds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    const int calInterval = 600; //(x2)
    int timeToCal = calInterval;

    // open file descriptors of named pipes to watch
    fd_a = open("/tmp/cal_pipe", O_RDWR | O_NONBLOCK);
    if (fd_a == -1) {
        perror("open error");
        return NULL;
    }

    while(true){
        FD_ZERO(&read_fds);
        FD_SET(fd_a, &read_fds);
        nfd = select(fd_a+1, &read_fds, NULL, NULL, &tv);
        if (nfd != 0) {
            if (nfd == -1) {
                perror("select error");
                return NULL;
            }
            if (FD_ISSET(fd_a, &read_fds)) {
                int newState = readPipe(fd_a, _state);

                if(newState != *_state)
                    *_state = newState;
            }
        }

        //Periodic Calibration
	// UNCOMMENT THIS CODE SECTION FOR ENABLING

        /*timeToCal-= 1;
        if(timeToCal == 0 && _caller){
            *_state = 1;
            timeToCal = 600;
            cout << "[+] Starting periodic calibration" << endl;
        }*/
            
        usleep(500000);
    }

    return NULL;
}
