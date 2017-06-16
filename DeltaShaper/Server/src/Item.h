#include <string>
#ifndef __item_h__
#define __item_h__

class Item
{
  private:
    int _packetID;
    int _fragSize;
    int _packetFrag;
    std::string _content;
    int _tframes;
    
  public:
    Item(int packetID, int fragSize, int packetFrag, std::string content, int tframes) 
          : _content(content), _packetID(packetID), _fragSize(fragSize), _packetFrag(packetFrag), _tframes(tframes) {}
    ~Item() {}

    std::string getContent() { return _content; }
    int getPacketID() { return _packetID; }
    int getFragSize() { return _fragSize; }
    int getPacketFrag() { return _packetFrag;}
    int getTFrames() { return _tframes; }
};
#endif