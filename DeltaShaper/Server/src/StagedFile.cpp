#include "StagedFile.h"
#include <fstream>
#include <iostream>
#include <bitset>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <sys/socket.h> 
#include <arpa/inet.h>

using namespace std;


bool compareBySeqNum(const FilePiece &a, const FilePiece &b){
    return a.seqN < b.seqN;
}



StagedFile::StagedFile(){}
StagedFile::~StagedFile(){}

bool StagedFile::isComplete() { return _pieces.size() == _totalSize; }

void StagedFile::initFileStage(int totalSize, int seqNum, string content, string part){
    FilePiece piece;
    piece.seqN = seqNum;
    piece.filePart = content;
    piece.filePiece = part;
    _pieces.push_back(piece);
    _totalSize = totalSize;
}

bool StagedFile::isPiecePresent(int seqN){
    bool found = false;
    for(vector<FilePiece>::iterator it = _pieces.begin(); it!=_pieces.end(); ++it){
        FilePiece p = *it;
        if(p.seqN == seqN)
            found = true;
    }
    return found;
}

void StagedFile::addFilePiece(int seqNum, string content, string part){
    FilePiece piece;
    piece.seqN = seqNum;
    piece.filePart = content;
    piece.filePiece = part;
    _pieces.push_back(piece);
}

void StagedFile::print(){
    cout << "printing pieces content" << endl;
    for(vector<FilePiece>::iterator it = _pieces.begin(); it!=_pieces.end(); ++it){
        FilePiece p = *it;
        cout << p.filePiece << endl;
    }
}

void StagedFile::deliverPacket(int packetID, int caller, string& content){
        //////////////////////////////////////////////////////////////////////
        //Raw socket creation for inserting returning IP packet on the network
        //////////////////////////////////////////////////////////////////////
        string address;
        if(caller)
            address = "10.10.10.10";
        else
            address = "127.0.0.1";

        struct sockaddr_in sin;
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = inet_addr(address.c_str());

        int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

        if(s == -1){
            perror("Failed to create socket. Root?");
            exit(1);
        }


        if (sendto (s, content.data(), content.length() ,  0, (struct sockaddr *) &sin, sizeof (sin)) <0){
            perror("sendto failed");
        }
        else{
            printf ("Packet Sent to %s, Length : %d \n" , address.c_str(), content.length());
        }

        auto timeSent = chrono::high_resolution_clock::now();    
        std::cout << "Packet delivery timestamp: "
          << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
          << '\n';
}

void StagedFile::assembleFile(int fileId, int caller){
    ostringstream ss,name;
    sort(_pieces.begin(), _pieces.end(), compareBySeqNum);
    
    
    for(vector<FilePiece>::iterator it = _pieces.begin(); it!=_pieces.end(); ++it){
        FilePiece p = *it;
        ss << p.filePart;
    }
    

    string packetContent = ss.str();
    deliverPacket(fileId, caller, packetContent);
}
