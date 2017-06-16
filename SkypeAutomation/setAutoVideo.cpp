#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>


using namespace std;

string windowIDSearch(char* input)
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

void tickAutoVideo(string win){
    ostringstream ss,action;
    
    /* Attempt starting video w/mouse action*/
    
    action << "DISPLAY=:1 xdotool windowfocus --sync " << win;
    int xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --sync --window " << win << " 40 335";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --sync --window " << win << " 185 84";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --sync --window " << win << " 630 435";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --sync --window " << win << " 810 435";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());  
}


void openOptions(string win){
    ostringstream ss,action;
    
    action << "DISPLAY=:1 xdotool windowfocus --sync " << win;
    int xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool key --window " << win << " " << "ctrl+o";
    xdo_result = system(action.str().c_str());
    action.str(std::string());

}
void autoVideo(string username){
    FILE *fpipe;
    char line[512];   
    
    //Open Options Window      
    if ( !(fpipe = (FILE*)popen("DISPLAY=:1 xwininfo -tree -root","r")) ) {  
      perror("Pipe is leaking...");
      exit(1);
    }

    string win;
    ostringstream windowName;
    
    windowName << username << " - Skype";
    while ( fgets( line, sizeof line, fpipe)) {
     if(strstr(line,windowName.str().c_str())){
        win = windowIDSearch(line);
        cout << "Window: " << win << endl;
        openOptions(win);
        cout << "Spawned Options Window" << endl;
        break;
     }
    }
    pclose(fpipe);
    sleep(2);
    
    
    //Switch AutoAnswer tick
    FILE *fpipe2;
    char line2[512]; 
    windowName.str(std::string());
    if ( !(fpipe2 = (FILE*)popen("DISPLAY=:1 xwininfo -tree -root","r")) ) {  
      perror("Pipe is leaking...");
      exit(1);
    }
    
    windowName << "Options";
    while ( fgets( line2, sizeof line2, fpipe2)) {
     if(strstr(line2,windowName.str().c_str())){
        win = windowIDSearch(line2);
        cout << "Window: " << win << endl;
        tickAutoVideo(win);
        cout << "(Un)Ticked AutoVideo Box" << endl;
        break;
     }
    }
    pclose(fpipe2);
    
}

int main(int argc, char *argv[]) {
    char* username;
    
    if(argc == 2){
        username = argv[1];
        autoVideo(username);
    } else{
        cout << "Missing arguments: Username" << endl;
        return 0;
    }
    return 0;
}
