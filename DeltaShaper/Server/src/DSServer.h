#include <string>
#include "StagedFile.h"

#ifndef __dsserver_h__
#define __dsserver_h__

class DSServer {
  private:  
    int _locX;
    int _locY;
    int _caller;
    int _pic_interval;
    
    std::string windowSearch(char* input);
    void resizeWindow(std::string win);
    std::string prepareWindow();

  public:
    DSServer(int locX, int locY, int caller, int pic_interval);

    void Start();
 
};
#endif
