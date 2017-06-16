#include <string>
#include <bitset>
#ifndef __frame_header_h__
#define __frame_header_h__

class FrameHeader
{
  private:
    std::bitset<16> _packetId;   //IP packet identifier
    std::bitset<16> _fragSize;   //Size of the fragment at hand
    std::bitset<4> _packetFrag;  //Packet fragment No. Several payload frames may be needed to carry an IP packet
    std::bitset<4> _totalFrag;   //Total number of packet fragments
  
  public:
    FrameHeader(int packetId, int fragSize, int packetFrag, int totalFrag) 
          : _packetId(packetId), _fragSize(fragSize), _packetFrag(packetFrag), _totalFrag(totalFrag) { 
          }
    
    ~FrameHeader() {}

    std::string getFrameHeader() { 
      std::string header;
      header.append(_packetId.to_string());
      header.append(_fragSize.to_string());
      header.append(_packetFrag.to_string());
      header.append(_totalFrag.to_string());
      return header; 
    }

    std::string getPacketId()  { return _packetId.to_string(); }
    std::string getFragSize()  { return _fragSize.to_string(); }
    std::string getPacketFrag(){ return _packetFrag.to_string();}
    std::string getTotalFrag() { return _totalFrag.to_string(); }
};

#endif