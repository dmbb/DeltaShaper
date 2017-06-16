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

void loginProcedure(string win, string username, string password){
    ostringstream ss,action;
    
    action << "DISPLAY=:1 xdotool windowfocus --sync " << win;
    int xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    
    action << "DISPLAY=:1 xdotool mousemove --window " << win << " 350 185";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    sleep(3);

    action << "DISPLAY=:1 xdotool windowfocus --sync " << win << " ;DISPLAY=:1 xdotool getmouselocation --shell";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --window " << win << " 240 150";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool type --window " << win << " " << username;
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --sync --window " << win << " 240 200";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool type --window " << win << " \'" << password << "\'";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --sync --window " << win << " 515 447";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --sync --window " << win << " 360 260";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
}
void autoLogin(string username, string password){
    FILE *fpipe;
    char line[512];         
    if ( !(fpipe = (FILE*)popen("DISPLAY=:1 xwininfo -tree -root","r")) ) {  
      perror("Pipe is leaking...");
      exit(1);
    }

    string win;
    while ( fgets( line, sizeof line, fpipe)) {
     if(strstr(line,"4.3 for Linux")){
        win = windowIDSearch(line);
        cout << "Window: " << win << endl;
        loginProcedure(win, username, password);
        break;
     }
    }
    pclose(fpipe);
}

int main(int argc, char *argv[]) {
    
    if(argc == 3){
        string username(argv[1]);
        string password(argv[2]);
    cout << username<<endl;
    autoLogin(username,password);
    } else{
        cout << "Missing arguments: User, password" << endl;
        return 0;
    }
    return 0;
}
