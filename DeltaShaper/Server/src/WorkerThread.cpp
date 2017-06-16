#include "WorkerThread.h"
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <bitset>
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <algorithm>
#include <vector>
#include <ezpwd/rs>
#include <xwdReader.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>


const int rs_payload = 223;
WorkerThread::WorkerThread(wqueue<string>& queue, map<int, StagedFile>& fileStore, int locX, int locY, int caller, mutex& mtx) : 
        _queue(queue), _fileStore(fileStore), _locX(locX), _locY(locY), _caller(caller), _mtx(mtx) {}


void* WorkerThread::run() {
    string img;
    ofstream myfile;
    StagedFile staged;
    
    while (1){
        img = _queue.remove();
        ostringstream ss,e;
        

        vector<char> rgb_matrix;

        auto begin = chrono::high_resolution_clock::now();

        auto timeSent = chrono::high_resolution_clock::now();    
            /*std::cout << "(WorkerThread) Before decoding: "
            << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
            << '\n';*/

        xwdToRGB(img, rgb_matrix);

        
        //Measure decoding time
        auto end = chrono::high_resolution_clock::now();    
        auto dur = end - begin;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        //cout << "(XWD Decoder . Worker-Thread) Elapsed time: " << ms << "ms" << endl;

        Item frameData = divdecodeImage(rgb_matrix);


        /* Place frame payload into the staging area until the full packet is received
        *****************************************************************************/
        if(!frameData.getContent().empty() && !(frameData.getContent() == "ERROR")){
            
            ss << "0" << img;
            /* DEBUG: Keep partial packet data on disk
            ***********************************
            myfile.open (ss.str().c_str());
            myfile << frameData.getContent();
            myfile.close();*/
            
            _mtx.lock(); //FULL MAP LOCK

            if(!_fileStore.count(frameData.getPacketID())){ //file is not already there
                staged = StagedFile();
                staged.initFileStage(frameData.getTFrames(), frameData.getPacketFrag(), frameData.getContent(), ss.str().c_str());
                _fileStore.emplace(frameData.getPacketID(), staged);
                if(_fileStore[frameData.getPacketID()].isComplete())
                        _fileStore[frameData.getPacketID()].assembleFile(frameData.getPacketID(), _caller);
            
            }else { //file is already there. Check for new part & whether it is now complete
                if(!_fileStore[frameData.getPacketID()].isPiecePresent(frameData.getPacketFrag())){
                    _fileStore[frameData.getPacketID()].addFilePiece(frameData.getPacketFrag(), frameData.getContent(), ss.str().c_str());
                    if(_fileStore[frameData.getPacketID()].isComplete()){
                        _fileStore[frameData.getPacketID()].assembleFile(frameData.getPacketID(), _caller);
                    }else{
                         //cout << "Added " << ss.str().c_str()<< endl;
                    }
                }else { //remove duplicate
                    //cout << "Already here" << endl; 
                    //int d = remove(ss.str().c_str());
                }
             }
            _mtx.unlock();         
        } 
        int d = remove(img.c_str()); //remove xwd dump
        end = chrono::high_resolution_clock::now();    
        dur = end - begin;
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
        //cout << "(WorkerThread) Decoding Elapsed time: " << ms << "ms" << endl;
        timeSent = chrono::high_resolution_clock::now();    
            /*std::cout << "(WorkerThread) After decoding: "
            << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
            << '\n';*/
    }

}

string WorkerThread::bitstringToASCII(string frameData){
    std::stringstream sstream(frameData);
    std::string output;
    std::bitset<8> bits;
    while(1)
    {
        sstream >> bits;
        if(sstream.good()){
            char c = char(bits.to_ulong());
            output += c;
        }else break;
    }
    
    return output;
}


Item RSWorkerThread::divdecodeImage(vector<char> &img){
    int R,G,B,avg;
    int step;
    int nbits, w_frame, h_frame;
    string mdBitString;

    auto begin = chrono::high_resolution_clock::now();    


    //######################## Metadata Band Recovery #############################
    for(int x = _locX*4; x < _locX*4 + 20*8*4; x+=8*4){ //20 mdCells -> 8 pix (1cell) at a time
        R=0; G=0; B=0; avg=0;

        for(int i = 0; i < 8*4; i+=4){
            for( int j = 0; j < 8*4;j+=4){

                if(j<6*4 && j>=2*4 && i<6*4 && i>=2*4){
                    R += static_cast<int>(static_cast<unsigned char>(img[(j*WIN_WIDTH) +x+i + 2]));
                    G += static_cast<int>(static_cast<unsigned char>(img[(j*WIN_WIDTH) +x+i + 1]));
                    B += static_cast<int>(static_cast<unsigned char>(img[(j*WIN_WIDTH) +x+i + 0]));
                }
            } 
        }
        //Average with neighbour values      
        R = R/(16);
        G = G/(16);
        B = B/(16);
        
        bitset<8> Red (R);
        bitset<8> Green (G);
        bitset<8> Blue (B);

        if ((R > 200) && (G > 200) && (B > 200))
            mdBitString += "1";
        else
            mdBitString += "0";
    }

    //cout << "Metadata: " << mdBitString << endl;

    //################# Extract metadata and assign decoder parameters #################
    bitset<2> frameType (mdBitString.substr(0,2));
    mdBitString = mdBitString.substr(2);
    bitset<2> payloadArea (mdBitString.substr(0,2));
    mdBitString = mdBitString.substr(2);
    bitset<4> cellSize (mdBitString.substr(0,4));
    mdBitString = mdBitString.substr(4);
    bitset<4> bitNumber (mdBitString.substr(0,4));
    mdBitString = mdBitString.substr(4);
    bitset<8> eccPayload (mdBitString.substr(0,8));


    switch(static_cast<int>(frameType.to_ulong())){
        case 0: //Payload Data
            if(!_caller){
                int fd;
                string pipe = "/tmp/cal_pipe";

                //Insert calibration 
                fd = open(pipe.c_str(), O_WRONLY);
                int status = write(fd, "0", sizeof("0"));
                if(!status)
                    cout << "[S] Write to cal_pipe failed" << endl;
                close(fd);
            }
            break;
        case 1: //Calibration
            if(!_caller){
                int fd;
                string pipe = "/tmp/cal_pipe";

                //Insert calibration 
                fd = open(pipe.c_str(), O_WRONLY);
                int status = write(fd, "1", sizeof("1"));
                if(!status)
                    cout << "[S] Write to cal_pipe failed" << endl;
                close(fd);
            }
            return Item(1, 0, 0, "ERROR", 0);
        case 3: //Dummy Data
            if(!_caller){
                int fd;
                string pipe = "/tmp/cal_pipe";

                //Insert calibration 
                fd = open(pipe.c_str(), O_WRONLY);
                int status = write(fd, "0", sizeof("0"));
                if(!status)
                    cout << "[S] Write to cal_pipe failed" << endl;
                close(fd);
            }
            return Item(1, 0, 0, "ERROR", 0);
    }

    switch(static_cast<int>(payloadArea.to_ulong())){
        case 0: 
            w_frame = 160;
            h_frame = 120;
            break;
        case 1: 
            w_frame = 320;
            h_frame = 240;
            break;
        case 2: 
            w_frame = 480;
            h_frame = 368;
            break;
    }

    switch(static_cast<int>(cellSize.to_ulong())){
        case 0: 
            step = 4;
            break;
        case 1: 
            step = 8;
            break;
        case 2: 
            step = 16;
            break;
    }

    nbits = static_cast<int>(bitNumber.to_ulong());
    eccPayload = static_cast<int>(eccPayload.to_ulong());


    int innerAreaSide = step/2;
    int innerAreaStart = step/4;
    int innerAreaEnd = step/4 + innerAreaSide;

    /*cout << "Nbits: " << nbits << endl;
    cout << "Cell: " << step << endl;
    cout <<  "W_frame: " << w_frame << endl;
    cout << "h_frame:" << h_frame << endl;
    cout << "ECC" << eccPayload << endl;*/
    //#################################### Payload Decoding #####################################
    
    //Read 1st RS payload block and extract full payload size
    int blockCells = 255 * 8 / nbits;
    int cellCounter = blockCells;
    string firstBlockData;
    int y = 8*WIN_WIDTH*4;
    int x = _locX*4;
    bool brokeInner = false;

    for(; y < _locY*WIN_WIDTH*4 + h_frame*WIN_WIDTH*4; y+=(step*WIN_WIDTH*4)){
        if(brokeInner){
            y-=(step*WIN_WIDTH*4);
            break;
        }
        for(x = _locX*4; x < _locX*4 + w_frame*4; x+=step*4){
            if(!cellCounter){
                brokeInner = true;
                break;
            }
            R=0; G=0; B=0; avg=0;

            for(int i = 0; i < step*4; i+=4){
                for( int j = 0; j < step*4;j+=4){

                    if(j<innerAreaEnd*4 && j>=innerAreaStart*4 && i<innerAreaEnd*4 && i>=innerAreaStart*4){
                        R += static_cast<int>(static_cast<unsigned char>(img[y+ (j*WIN_WIDTH) +x+i + 2]));
                        G += static_cast<int>(static_cast<unsigned char>(img[y+ (j*WIN_WIDTH) +x+i + 1]));
                        B += static_cast<int>(static_cast<unsigned char>(img[y+ (j*WIN_WIDTH) +x+i + 0]));
                    }
                } 
            }
            //Average with neighbour values      
            R = R/(innerAreaSide*innerAreaSide);
            G = G/(innerAreaSide*innerAreaSide);
            B = B/(innerAreaSide*innerAreaSide);
            
            //cout << "R " << R << " G " << G << " B " << B << "y: " << y << "x: " << x << endl;
            bitset<8> Red (R);
            bitset<8> Green (G);
            bitset<8> Blue (B);


            for(int j=0; j< nbits; j++){
                if(j%3 == 0){
                    firstBlockData += to_string(Red[7- j/3]);
                }else if(j%3 == 1){
                    firstBlockData += to_string(Green[7-j/3]);
                }else if(j%3 == 2){
                    firstBlockData += to_string(Blue[7-j/3]);
                }
            }
            cellCounter--; //Read 1 cell
        }
    }

    //################ Cut bitstring last unused bits (acertos)############
    int len = firstBlockData.length();
    int out = 0;
    while(len%8 != 0){
        out++;
        len--;
    }

    string frameData = firstBlockData.substr(len,firstBlockData.length());
    while(out > 0){
        firstBlockData.erase(firstBlockData.size() - 1);
        out--;
    }

    //############################# Decode RS ECC #########################
    ezpwd::RS<255,rs_payload> rs;
    string encodedContent = bitstringToASCII(firstBlockData);

    //cout << "Server: bitstring len " << firstBlockData.length() << endl;
    //cout << "Server: bitstring " << firstBlockData << endl;
    //cout << "Server: Encoded Packet Length: " <<  encodedContent.length() << endl;
    
    string data, decoded;

    while (1){
        if(encodedContent.length() > 255){
            data = encodedContent.substr(0,255);
            int fixed = rs.decode( data );
            if(fixed < 0){
                cout << "OVERWHELMED" << endl;
                return Item(1, 0, 0, "ERROR", 0);}
            cout << "errors: " << fixed <<endl;
            data.resize( data.size() - rs.nroots() );
            decoded.append(data);
            encodedContent = encodedContent.substr(255);
        }else{
            data = encodedContent;
            int fixed = rs.decode( data );
            if(fixed < 0){
                cout << "OVERWHELMED" << endl;
                return Item(1, 0, 0, "ERROR", 0);}
            cout << "errors: " << fixed <<endl;
            data.resize( data.size() - rs.nroots() );
            decoded.append(data);
            break;
        }
    }


    //cout << "Decoded length ASCII " << decoded.length() << endl;
    


    //######## Convert Decoded content to bitstring to recover header #############
    string bitString;
    for(int b = 0; b < decoded.length(); b++){
        bitString.append((bitset<8>(decoded[b])).to_string());
    }
    //cout << "Bitstring size = " << bitString.length()<<endl;
    string h = bitString.substr(0,56); //Header
    firstBlockData = bitString.substr(56); //Content
    firstBlockData = bitstringToASCII(firstBlockData);

    //Extract header 
    bitset<16> total_cells (h.substr(0,16));
    int totalCells = total_cells.to_ulong();
    //cout << "Total cells (Parameter): " << totalCells << endl;
    h = h.substr(16);
    bitset<16> packetId (h.substr(0,16));
    h = h.substr(16);
    bitset<16> fragSize (h.substr(0,16));
    h = h.substr(16);
    bitset<4> packetFrag (h.substr(0,4));
    h = h.substr(4);
    bitset<4> packetTotalFrag (h.substr(0,4));
    h = h.substr(4);

    firstBlockData = firstBlockData.substr(0,fragSize.to_ulong());

    if(totalCells <= blockCells) //We do not need to read more, one block carried the whole message
        return Item(static_cast<int>(packetId.to_ulong()), static_cast<int>(fragSize.to_ulong()), static_cast<int>(packetFrag.to_ulong()), 
            firstBlockData, static_cast<int>(packetTotalFrag.to_ulong()));

    totalCells -= blockCells;
    brokeInner = false;
    //Otherwise, We read the remaining payload data
    for(; y < _locY*WIN_WIDTH*4 + h_frame*WIN_WIDTH*4; y+=(step*WIN_WIDTH*4)){
        if(brokeInner)
            break;

        for(;x < _locX*4 + w_frame*4; x+=step*4){
            if(!totalCells){
                brokeInner = true;
                break;
            }

            R=0; G=0; B=0; avg=0;

            for(int i = 0; i < step*4; i+=4){
                for( int j = 0; j < step*4;j+=4){

                    if(j<innerAreaEnd*4 && j>=innerAreaStart*4 && i<innerAreaEnd*4 && i>=innerAreaStart*4){
                        R += static_cast<int>(static_cast<unsigned char>(img[y+ (j*WIN_WIDTH) +x+i + 2]));
                        G += static_cast<int>(static_cast<unsigned char>(img[y+ (j*WIN_WIDTH) +x+i + 1]));
                        B += static_cast<int>(static_cast<unsigned char>(img[y+ (j*WIN_WIDTH) +x+i + 0]));
                    }
                } 
            }
            //Average with neighbour values      
            R = R/(innerAreaSide*innerAreaSide);
            G = G/(innerAreaSide*innerAreaSide);
            B = B/(innerAreaSide*innerAreaSide);
            
            //cout << "R " << R << " G " << G << " B " << B << "y: " << y << "x: " << x << endl;
            bitset<8> Red (R);
            bitset<8> Green (G);
            bitset<8> Blue (B);


            for(int j=0; j< nbits; j++){
                if(j%3 == 0){
                    frameData += to_string(Red[7- j/3]);
                }else if(j%3 == 1){
                    frameData += to_string(Green[7-j/3]);
                }else if(j%3 == 2){
                    frameData += to_string(Blue[7-j/3]);
                }
            }
            totalCells--;
        }
        x = _locX*4;
    }

    //cout << "Raw Bitstring " << frameData << endl;
    //cout << "Raw Bitstring length "<< frameData.length() << endl;

    //################ Cut bitstring last unused bits (acertos)############
    while(frameData.length()%8 != 0)
        frameData.erase(frameData.size() - 1);
    //cout << "After cut Bitstring Size " << frameData.length() << endl;
    //cout << "After Cut Bitstring " << frameData << endl;
    

    //############################# Decode RS ECC #########################
    encodedContent = bitstringToASCII(frameData);
    
    string decodedPayload;

    while (1){
        if(encodedContent.length() > 255){
            data = encodedContent.substr(0,255);
            int fixed = rs.decode( data );
            if(fixed < 0){
                cout << "OVERWHELMED" << endl;
                return Item(1, 0, 0, "ERROR", 0);}
            cout << "errors: " << fixed <<endl;
            data.resize( data.size() - rs.nroots() );
            decodedPayload.append(data);
            encodedContent = encodedContent.substr(255);
        }else{
            data = encodedContent;
            int fixed = rs.decode( data );
            if(fixed < 0){
                cout << "OVERWHELMED" << endl;
                return Item(1, 0, 0, "ERROR", 0);}
            cout << "errors: " << fixed <<endl;
            data.resize( data.size() - rs.nroots() );
            decodedPayload.append(data);
            break;
        }
    }


    //cout << "Decoded length ASCII " << decoded.length() << endl;
    
    firstBlockData.append(decodedPayload);
    
    //##################### Recovered Packet Length #######################################
    //cout << "(WorkerEnd) Recovered Packet LEN: " << bitstringToASCII(frameData).length() << endl;

    //################## Bitstring after ASCII conversion bit iteration ##################
    /*string bitstring;
        for(int b = 0; b < bitstringToASCII(frameData).length(); b++)
            bitstring.append((bitset<8>(frameData[b])).to_string());
    cout << "Bitstring after ASCII conversion(Worker End) " << bitstring << endl; */
    //####################################################################################
    
    /// Decoding Delay Checker
    auto end = chrono::high_resolution_clock::now();    
    auto dur = end - begin;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    //cout << "(Frame Decoder) Elapsed time: " << ms << "ms" << endl;

    return Item(static_cast<int>(packetId.to_ulong()), static_cast<int>(fragSize.to_ulong()), static_cast<int>(packetFrag.to_ulong()), 
        firstBlockData, static_cast<int>(packetTotalFrag.to_ulong()));
}



