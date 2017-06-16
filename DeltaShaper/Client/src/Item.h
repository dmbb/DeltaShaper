#include <vector>
#ifndef __item_h__
#define __item_h__

class Item
{
  private:
    int _packetID;
    int _packetFrag;
    std::vector<unsigned char> _content;
    int _tframes;
    
  public:
    Item(int packetID, int packetFrag, std::vector<unsigned char> content, int tframes) 
          : _content(content), _packetID(packetID), _packetFrag(packetFrag), _tframes(tframes) {}
    ~Item() {}

    std::vector<unsigned char> getContent() { return _content; }
    int getPacketID() { return _packetID; }
    int getPacketFrag() { return _packetFrag;}
    int getTFrames() { return _tframes; }
};
#endif