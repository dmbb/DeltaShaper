#include <vector>
#include <string>

#ifndef __staged_file_h__
#define __staged_file_h__

typedef struct FilePiece {
int seqN;
std::string filePart;
std::string filePiece;
} FilePiece;


class StagedFile {
  private:
   std::vector<FilePiece> _pieces;
   int _totalSize;

   

  public:
    StagedFile();
    ~StagedFile();
 
    bool isComplete();
    
    void initFileStage(int totalSize, int seqNum, std::string content, std::string part);
    
    bool isPiecePresent(int seqN);
    
    void addFilePiece(int seqNum, std::string content, std::string part);
  
    void print();
    
    void deliverPacket(int packetID, int caller, std::string& content);

    void assembleFile(int fileId, int caller);
    
};
#endif