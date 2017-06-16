#include <string>
#include <bitset>

#ifndef __metadata_header_h__
#define __metadata_header_h__

class MetadataHeader
{
  private:
    bitset<2> _frameType;    //0: Payload, 1: On Calibration, 3: Dummy
    bitset<2> _payloadArea;    
    bitset<4> _cellSize;
    bitset<4> _bitNumber;  
    bitset<8> _eccPayload;
  
  public:
    MetadataHeader(int frameType, int payloadWidth, int cellSize, int bitNumber, int eccPayload) 
          : _frameType(frameType), _bitNumber(bitNumber), _eccPayload(eccPayload) { 
            switch(payloadWidth){
              case 160: 
                _payloadArea = std::bitset<2>(0x0);
                break;
              case 320: 
                _payloadArea = std::bitset<2>(0x1);
                break;
              case 480: 
                _payloadArea = std::bitset<2>(0x2);
                break;
            }
            switch(cellSize){
              case 4: 
                _cellSize = std::bitset<4>(0x0);
                break;
              case 8: 
                _cellSize = std::bitset<4>(0x1);
                break;
              case 16: 
                _cellSize = std::bitset<4>(0x2);
                break;
            }
          }
    
    ~MetadataHeader() {}

    std::string getMetadataHeader() { 
      std::string header;
      header.append(_frameType.to_string());
      header.append(_payloadArea.to_string());
      header.append(_cellSize.to_string());
      header.append(_bitNumber.to_string());
      header.append(_eccPayload.to_string());
      return header; 
    }

    std::string getFrameType() { return _frameType.to_string();}
    std::string getPayloadArea()  { return _payloadArea.to_string(); }
    std::string getCellSize(){ return _cellSize.to_string();}
    std::string getBitNumber() { return _bitNumber.to_string(); }
    std::string getECCPayload() { return _eccPayload.to_string(); }
};

#endif