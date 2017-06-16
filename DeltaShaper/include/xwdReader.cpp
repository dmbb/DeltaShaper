#include <iostream>
#include <fstream>
#include <chrono>
#include <netinet/in.h>
#include "xwdReader.h"

using namespace std;

vector<unsigned char> intToBytes(int paramInt)
{
     vector<unsigned char> arrayOfByte(4);
     for (int i = 0; i < 4; i++)
         arrayOfByte[3 - i] = (paramInt >> (i * 8));
     return arrayOfByte;
}

void ReadAllBytes(string filename, vector<char> &result, int colormap_size){
    int header_size, pos;
    
    ifstream ifs(filename, ios::binary|ios::ate);
    pos = ifs.tellg();

    ifs.seekg(ios::beg);

    ifs.read(reinterpret_cast<char*>(&header_size), 4);
    header_size = ntohl(header_size);

    //Do not load header and colormaps. Read the remaining of the file
    ifs.seekg(header_size + colormap_size);
    result.resize(pos-(header_size + colormap_size));

    ifs.read(&result[0], pos-(header_size + colormap_size));

}


void xwdToRGB(string filename, vector<char> &img){
  auto begin = chrono::high_resolution_clock::now();
  int R, G, B, pixel;
  int colormap_size = 3072; //256 entries, 12B each
  int header_size;
  
  
  ReadAllBytes(filename, img, colormap_size);
  
}
