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

void acceptUseTerms(string win){
    ostringstream ss,action;

    action << "DISPLAY=:1 xdotool windowfocus --sync " << win;
    int xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool mousemove --window " << win << " 340 240";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
    
    action << "DISPLAY=:1 xdotool click --window " << win << " 1";
    xdo_result = system(action.str().c_str());
    action.str(std::string());
}
void acceptTerms(){
    FILE *fpipe;
    char line[512];         
    if ( !(fpipe = (FILE*)popen("DISPLAY=:1 xwininfo -tree -root","r")) ) {  
      perror("Pipe is leaking...");
      exit(1);
    }

    string win;
    while ( fgets( line, sizeof line, fpipe)) {
     if(strstr(line,"Terms of Use")){
        win = windowIDSearch(line);
        cout << "Window: " << win << endl;
        acceptUseTerms(win);
        break;
     }
    }
    pclose(fpipe);
}

int main(int argc, char *argv[]) {
    acceptTerms();
    return 0;
}
