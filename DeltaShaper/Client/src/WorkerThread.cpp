#include "WorkerThread.h"  
#include "FrameHeader.h"  
#include "MetadataHeader.h"
#include <png++/png.hpp>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <chrono>
#include <math.h> 
#include <streambuf>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ezpwd/rs>
#include <dirent.h>

const int rs_payload = 223;
const int DUMMY_FRAMES = 50;

string bitstringToASCII(string frameData){
    std::stringstream sstream(frameData);
    std::string output;
    std::bitset<8> bits;
    while(1){
        sstream >> bits;
        if(sstream.good()){
            char c = char(bits.to_ulong());
            output += c;
        }else break;
    }

    return output;
}

string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}

Selector getBestSelector(){
    //Parse Captures, compute EMD and choose the best selector
    string emdResults = exec("python ./../Client/Calibrator.py");
    cout << "Selectors' EMD cost: " << endl;
    cout << emdResults << endl;
    cout << "===========================: " << endl;


    //Interpret result and build corresponding selector
    vector<string> res;
    string rawSelector;
    Selector selector;
    string a1, a2, cells, bits;

    //Extract best selector
    std::size_t start = 0, end = 0;
    while ((end = emdResults.find("\n", start)) != string::npos) {
        emdResults= emdResults.substr(start, end - start);
        start = end + 1;
    }

    //Split selector from emd cost
    start = 0, end = 0;
    while ((end = emdResults.find(" ", start)) != string::npos) {
        rawSelector = emdResults.substr(start, end - start);
        start = end + 1;
    }
    
    //Split selector components
    start = 0, end = 0;
    while ((end = rawSelector.find("_", start)) != string::npos) {
        res.push_back(rawSelector.substr(start, end - start));
        start = end + 1;
    }
    res.push_back(rawSelector.substr(start));

    //Split area components
    start = 0, end = 0;
    while ((end = res[0].find("x", start)) != string::npos) {
        a1 = res[0].substr(start, end - start);
        start = end + 1;
    }
    a2 = res[0].substr(start);
    
    cells = res[1];
    bits = res[2];

    //Build selector
    selector.payloadArea = make_pair(atoi(a1.c_str()),atoi(a2.c_str()));
    selector.cellSize = atoi(cells.c_str());
    selector.bitNumber = atoi(bits.c_str());

    cout << "A new selector has been found: " << selector.payloadArea.first << "x"
    << selector.payloadArea.second << "_"
    << selector.cellSize << "_"
    << selector.bitNumber <<endl;

    return selector;
}


WorkerThread::WorkerThread(int* state, wqueue<Item*>& queue, wqueue<TransmissionID>& transmission_queue, mutex& mtx, int caller, int* w_div, int* h_div, int* w_frame, int* h_frame, int* framerate, int* nbits, int* nextSelector) : 
    _state(state), _queue(queue), _transmission_queue(transmission_queue), _mtx(mtx), _caller(caller), _w_div(w_div), _h_div(h_div), _w_frame(w_frame), _h_frame(h_frame) , _framerate(framerate), _nbits(nbits), _next_selector(nextSelector) {}


void* WorkerThread::run() {
    bool calibratedRound = false;

    //Create folder to hold calibration dummy data as part of a single packet ID = 0
    ifstream data("./../Client/Data/data0");
    if(!data){
        int data_result = system("mkdir ./../Client/Data/data0");
    }
    ifstream default_pic("./../Client/Data/data0/320x240_8_6");
    if(!default_pic){
        int default_result = system("mkdir ./../Client/Data/data0/320x240_8_6");
    }
    dummyColorFill(0, *_w_frame, *_h_frame, *_w_frame/ *_w_div , *_nbits);

    ifstream dataCal("./../Client/CalibrationRaw");
    if(!dataCal){
        int data_result = system("mkdir ./../Client/CalibrationRaw");
    }
    ifstream baseCal("./../Client/CalibrationBaselines");
    if(!baseCal){
        int base_result = system("mkdir ./../Client/CalibrationBaselines");
    }

    //Build subset of selectors to be used for calibration purposes
    vector<Selector> selectors;
    
    std::array<int,1> cellSizes = {8}; 
    std::array<int,1> bitNumbers = {6}; 
    std::array<pair<int,int>,1> payloadAreas = {make_pair(320,240)}; 
	

    /*Just one parameter selection per the example above. 
      The calibration procedure can take more parameters into account as the following example:

    std::array<int,3> cellSizes = {4, 8, 16}; 
    std::array<int,9> bitNumbers = {1, 2, 3, 4, 5, 6, 7, 8, 9}; 
    std::array<pair<int,int>,2> payloadAreas = { make_pair(320,240), make_pair(160,120) }; */



    for(int a = 0; a < payloadAreas.size(); a++)
        for(int c = 0; c < cellSizes.size(); c++)
            for(int b = 0; b < bitNumbers.size(); b++){
                Selector selector;
                selector.payloadArea = payloadAreas[a];
                selector.cellSize = cellSizes[c];
                selector.bitNumber = bitNumbers[b];
                selectors.push_back(selector);
            }




    // Remove 1 item at a time and process it. Blocks if no items are 
    // available to process.
    while (1){
        //cout << "State is: " << *_state << endl;
        if(*_state == 0){ //Regular work
            
            if(calibratedRound) //Reset calibration indicator to allow further calibration
                calibratedRound = false;

            if(_queue.size() != 0){
                Item* item = _queue.remove();
                /*auto timeSent = chrono::high_resolution_clock::now();    
                std::cout << "(WorkerThread) To encode: Packet timestamp: "
                  << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
                  << '\n';*/

                ostringstream ss;

                _mtx.lock();
                if(item->getPacketID() != -1){
                    ss << "./../Client/Data/data" + to_string(item->getPacketID());

                    //Create folder to hold "imagified" packet
                    ifstream data(ss.str().c_str());
                    if(!data){
                        ostringstream cmd;
                        cmd << "mkdir " << ss.str();
                        int data_result = system(cmd.str().c_str());
                    }
                _mtx.unlock();
                    //Encode packet fragment into an image
                    colorFill(item->getPacketID(), item->getPacketFrag(), item->getTFrames(), item->getContent());
                    
                    //Add fragment to the queue
                    TransmissionID tid;
                    tid.packetID = item->getPacketID();
                    tid.fragmentID = item->getPacketFrag();
                    _transmission_queue.add(tid);

                    delete item;
                    /*auto timeSent = chrono::high_resolution_clock::now();    
                std::cout << "(WorkerThread) After encoding + queue insertion timestamp: "
                  << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
                  << '\n';*/
                }else{
                    delete item;
                    return NULL;
                    }
            }
        } else if (*_state == 1 && !calibratedRound){ //Calibration process
            cout << "[C] On to calibration" << endl;
            *_next_selector = 0;
            /*cout << "Width: " << *_w_frame <<endl;
            cout << "Height: " << *_h_frame <<endl;
            cout << "W_DIV: " << *_w_div <<endl;
            cout << "H_DIV: " << *_h_div <<endl;
            cout << "FRAMERATE: " << *_framerate <<endl;*/




            /* For each cell/nbit combo
                - fill dummy images with config c --> 1 Frame/sec
                - place dummy images in transmission queue
                - start packetCapture for config c

               Parse captures && compute EMD
               Choose the config yielding the lowest EMD
               Update parameters accordingly
            */

            //Establish Current Baseline
            cout << "[C] Overlaying metadata band on background video" << endl;
            int res = system("sudo sh ./../Client/SnowmixScripts/overlayMetadata.sh");

            sleep(5); //Give time to stabilize stream
            ostringstream cmd, cmd2;
            cmd << "sudo tcpdump -i eth0 -n -G 30 -W 1 -w ./../Client/CalibrationBaselines/base.pcap";
            res = system(cmd.str().c_str());
            bool deletedOverlay = false;

            for(int i = 0; i < selectors.size(); i++){
                //Encode packet fragments into images
                cout << "WORKER, selector is " << *_next_selector << endl;
                cout << "WORKER, i is " << i << endl;
                while(true){
                    if(*_next_selector == i)
                        break;
                    sleep(1);
                }

                for(int frag = 0; frag < DUMMY_FRAMES; frag++){
                    calibrationColorFill(frag, selectors[i].payloadArea.first, selectors[i].payloadArea.second, selectors[i].cellSize, selectors[i].bitNumber, 255);
                    dummyColorFill(frag, selectors[i].payloadArea.first, selectors[i].payloadArea.second, selectors[i].cellSize, selectors[i].bitNumber);
                    //Add fragment to the queue
                    TransmissionID tid;
                    tid.packetID = 0;
                    tid.fragmentID = frag;
                    _transmission_queue.add(tid);
                }
                if(!deletedOverlay){
                    sleep(10); //Allow for new calibration frames to be picked up by TransmissionThread before deleting metadata band
                    res = system("sudo sh ./../Client/SnowmixScripts/deleteOverlay.sh");
                    deletedOverlay = true;
                }
                // Capture packets
                sleep(5); //Give time to stabilize stream
                ostringstream cmd;
                cmd << "sudo tcpdump -i eth0 -n -G 30 -W 1 -w ./../Client/CalibrationRaw/"
                    << to_string(selectors[i].payloadArea.first) << "x" << to_string(selectors[i].payloadArea.second) << "_"
                    << to_string(selectors[i].cellSize) << "_" << to_string(selectors[i].bitNumber);
                int res = system(cmd.str().c_str());
            }

            Selector elected = getBestSelector();

            //Change encoding parameters
            *_w_frame = elected.payloadArea.first;
            *_h_frame = elected.payloadArea.second;
            *_w_div = *_w_frame / elected.cellSize;
            *_h_div = *_h_frame / elected.cellSize;
            *_nbits = elected.bitNumber;

            calibratedRound = true;
            if(_caller)
                *_state = 0;
        }
        usleep(100000);
    }
}


void RSWorkerThread::colorFill(int packetId, int packetFrag, int totalFrag, vector<unsigned char> content){
    int i = 0;
    int header_size = 5;
    int cell_indicator_size = 2;
    int nbits = *_nbits;
    string bitString, mdBitString;
    vector<unsigned char> data, encodedContent, headerData;
    png::image<png::rgba_pixel> image(*_w_frame, *_h_frame);
    
    int step = *_w_frame / *_w_div;

    auto begin = chrono::high_resolution_clock::now();


    //################# Metadata Band Drawing ########################
    MetadataHeader mdHeader = MetadataHeader(0x0, *_w_frame, step, nbits, rs_payload);
    mdBitString = mdHeader.getMetadataHeader();
    int metadataCellSize = 8;
    int j = 0;
    //Draw upper metadata band 
    for (png::uint_32 x = 0; x < image.get_width(); x+= metadataCellSize){
        if(j < mdBitString.length()){
            //Fill cell
            for(int vp = 0; vp < metadataCellSize ; vp++){
                for(int wp = 0; wp < metadataCellSize ; wp++){
                    if(mdBitString[j] - '0' == 1)
                        image[vp][x+wp] = png::rgba_pixel(255, 255, 255, 255);
                    else
                        image[vp][x+wp] = png::rgba_pixel(0, 0, 0, 255);
                }
            }
        } else{
            for(int vp = 0; vp < metadataCellSize ; vp++){
                for(int wp = 0; wp < metadataCellSize ; wp++){
                    image[vp][x+wp] = png::rgba_pixel(0, 255, 0, 255);
                }
            }
        }
        j++;
    }
    //#################################################################
    //Build totalCell indicator

    int eccBlocks = ceil((content.size() + header_size + cell_indicator_size)/(float)rs_payload);
    int totalBytes = content.size() + header_size + cell_indicator_size + eccBlocks * (255 - rs_payload);
    int totalCells = ceil(totalBytes * 8 / (float)nbits);
    std::bitset<16> cellsToRead (totalCells);
    string tempCells = bitstringToASCII(cellsToRead.to_string());

    for(int b = 0; b < tempCells.length();b++)
        headerData.push_back(tempCells[b]);

    cout << "Total cells = " << totalCells << endl;
    //###################Print Original Bitstring #############
    /*string bitstring;
    for(int b = 0; b < content.size(); b++){
      bitstring.append((bitset<8>(content[b])).to_string());
    }
    cout << "Original Bitstring" << bitstring << endl; */
    //#########################################################

    FrameHeader header = FrameHeader(packetId, content.size(), packetFrag, totalFrag);    
    string tempHeaderConv = bitstringToASCII(header.getFrameHeader());
    
    for(int b = 0; b < tempHeaderConv.length();b++)
        headerData.push_back(tempHeaderConv[b]);

    headerData.insert(headerData.end(), content.begin(), content.end());
    
    //Fill small unique RS block with junk to generate a full block
    if((content.size() + header_size + cell_indicator_size) < rs_payload){
        int capacity = ((*_w_div * (*_h_div - 1)) / 8) * nbits;
        int missing = 0;
        if(capacity < 255)
            missing = capacity - (content.size() + header_size + cell_indicator_size + (255 - rs_payload));
        else
            missing = rs_payload - (content.size() + header_size + cell_indicator_size);
        
        string filler;
        for(int i = 0; i < missing; i++)
            filler += '!' + (rand() % 90);;
        headerData.insert(headerData.end(), filler.begin(), filler.end());
    }

    content = headerData;

    /*totalBytes = headerData.size() + eccBlocks * (255 - rs_payload);
    totalCells = totalBytes * 8 / nbits;
    cout << "Content len = " << totalBytes << " total cells = " << totalCells << endl;*/
    //cout << "Raw content + Header len: " << content.size() << endl;
    
    ezpwd::RS<255,rs_payload> rs;


    while (1){
        if(content.size() > rs_payload){
            vector<unsigned char> data(content.begin(), content.begin() + rs_payload);
            rs.encode(data);
            encodedContent.insert(encodedContent.end(), data.begin(), data.end());
            std::vector<unsigned char> subvec(content.begin() + rs_payload, content.end());
            content = subvec;
        }else{
            data = content;
            rs.encode(data);
            encodedContent.insert(encodedContent.end(), data.begin(), data.end());
            break;
        }
    }
    //cout << encodedContent << endl;
    //cout << "Encoded content length: " << encodedContent.size() << endl;
    
    
    //add encoded data to bitstring
    for(int b = 0; b < encodedContent.size(); b++){
      bitString.append((bitset<8>(encodedContent[b])).to_string());
   }


    //cout << index << " " << content << endl;
    //cout << "Bitstring after encoding" << bitString << endl;
    //cout << "bitstring length after encoding: " << bitString.length() << endl;
    for (png::uint_32 y = metadataCellSize; y < image.get_height(); y+= step){  //Starting in 8 to allow for 8-sized cell upper band
        for (png::uint_32 x = 0; x < image.get_width(); x+= step){
            if(i < bitString.length()){
                
                std::bitset<8> R;
                R.set(0,0);
                R.set(1,0);
                R.set(2,0);
                R.set(3,0);
                R.set(4,1);
                std::bitset<8> G;
                G.set(0,0);
                G.set(1,0);
                G.set(2,0);
                G.set(3,0);
                G.set(4,1);
                std::bitset<8> B;
                B.set(0,0);
                B.set(1,0);
                B.set(2,0);
                B.set(3,0);
                B.set(4,1);


                for(int j=0; j< nbits; j++){
                    if(j%3 == 0){
                        if(bitString[i+j] - '0' == 1)
                            R.set(7- j/3);
                            //cout << "j/Nbits " << R << endl;
                    }else if(j%3 == 1){
                        if(bitString[i+j] - '0' == 1){
                            //cout << "nbits: " << nbits << "j " << j << "DIV" << nbits/j << endl;
                            G.set(7- j/3);
                        }
                    }else if(j%3 == 2){
                        if(bitString[i+j] - '0' == 1)
                            B.set(7- j/3);
                    }
                }
        //cout << "bits:"<<bitString[i]<<bitString[i+1]<<bitString[i+2]<<endl;
                for(int vp = 0; vp < step; vp++){
                    for(int wp = 0; wp < step; wp++){
                        image[y+vp][x+wp] = png::rgba_pixel(R.to_ulong(), G.to_ulong(), B.to_ulong(),255);
                    }
                }
        i+=nbits;
            }else{
                std::bitset<8> R;
                R.set(0,0);
                R.set(1,0);
                R.set(2,0);
                R.set(3,0);
                R.set(4,1);
                std::bitset<8> G;
                G.set(0,0);
                G.set(1,0);
                G.set(2,0);
                G.set(3,0);
                G.set(4,1);
                std::bitset<8> B;
                B.set(0,0);
                B.set(1,0);
                B.set(2,0);
                B.set(3,0);
                B.set(4,1);

                for(int j=0; j< nbits; j++){
                    if(j%3 == 0){
                        if(rand()%2 == 1)
                            R.set(7- j/3);
                            //cout << "j/Nbits " << R << endl;
                    }else if(j%3 == 1){
                        if(rand()%2 == 1){
                            //cout << "nbits: " << nbits << "j " << j << "DIV" << nbits/j << endl;
                            G.set(7- j/3);
                        }
                    }else if(j%3 == 2){
                        if(rand()%2 == 1)
                            B.set(7- j/3);
                    }
                }
                for(int vp = 0; vp < step; vp++){
                    for(int wp = 0; wp < step; wp++){
                        image[y+vp][x+wp] = png::rgba_pixel(R.to_ulong(), G.to_ulong(), B.to_ulong(),255);
                    }
                }
            }
        }
    }

    auto end = chrono::high_resolution_clock::now();    
    auto dur = end - begin;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    //cout << "(Client Worker-Thread Before Saving image) Elapsed time: " << ms << "ms" << endl;

    image.write("./../Client/Data/data" + std::to_string(packetId) + "/frag" + std::to_string(packetFrag) + ".png");

    end = chrono::high_resolution_clock::now();    
    dur = end - begin;
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    //cout << "(Client Worker-Thread After Saving image) Elapsed time: " << ms << "ms" << endl;
    
    /*auto timeSent = chrono::high_resolution_clock::now();    
        std::cout << "(WorkerThread) After encoding timestamp: "
          << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
          << '\n';*/
}


void WorkerThread::calibrationColorFill(int packetFrag, int w, int h, int cellSize, int nbits, int opacity){
    int i = 0;
    string mdBitString;
    png::image<png::rgba_pixel> image(w, h);
    
    int step = cellSize;

    //#########################################################################################
    //########################## Dummy Frames for parameter calibration #######################
    //#########################################################################################

    //################# Metadata Band Drawing ########################
    MetadataHeader mdHeader = MetadataHeader(0x1, w, cellSize, nbits, 223);
    mdBitString = mdHeader.getMetadataHeader();
    int metadataCellSize = 8;

    int j = 0;
    //Draw upper metadata band 
    for (png::uint_32 x = 0; x < image.get_width(); x+= metadataCellSize){
        if(j < mdBitString.length()){
            //Fill cell
            for(int vp = 0; vp < metadataCellSize; vp++){
                for(int wp = 0; wp < metadataCellSize; wp++){
                    if(mdBitString[j] - '0' == 1)
                        image[vp][x+wp] = png::rgba_pixel(255, 255, 255, opacity);
                    else
                        image[vp][x+wp] = png::rgba_pixel(0, 0, 0, opacity);
                }
            }
        } else{
            for(int vp = 0; vp < metadataCellSize; vp++){
                for(int wp = 0; wp < metadataCellSize; wp++){
                    image[vp][x+wp] = png::rgba_pixel(0, 255, 0, opacity);
                }
            }
        }
        j++;
    }
    //#################################################################


    for (png::uint_32 y = metadataCellSize; y < image.get_height(); y+= step){
        for (png::uint_32 x = 0; x < image.get_width(); x+= step){
            std::bitset<8> R;
            R.set(0,0);
            R.set(1,0);
            R.set(2,0);
            R.set(3,0);
            R.set(4,1);
            std::bitset<8> G;
            G.set(0,0);
            G.set(1,0);
            G.set(2,0);
            G.set(3,0);
            G.set(4,1);
            std::bitset<8> B;
            B.set(0,0);
            B.set(1,0);
            B.set(2,0);
            B.set(3,0);
            B.set(4,1);

            for(int j=0; j< nbits; j++){
                if(j%3 == 0){
                    if(rand()%2 == 1)
                        R.set(7- j/3);
                        //cout << "j/Nbits " << R << endl;
                }else if(j%3 == 1){
                    if(rand()%2 == 1){
                        //cout << "nbits: " << nbits << "j " << j << "DIV" << nbits/j << endl;
                        G.set(7- j/3);
                    }
                }else if(j%3 == 2){
                    if(rand()%2 == 1)
                        B.set(7- j/3);
                }
            }
            for(int vp = 0; vp < step; vp++){
                for(int wp = 0; wp < step; wp++){
                    image[y+vp][x+wp] = png::rgba_pixel(R.to_ulong(), G.to_ulong(), B.to_ulong(),opacity);
                }
            }
        }
    }

    image.write("./../Client/Data/data0/frag" + std::to_string(packetFrag) + ".png");
}


void WorkerThread::dummyColorFill(int packetFrag, int w, int h, int cellSize, int nbits){
    int i = 0;
    string mdBitString;
    png::image<png::rgba_pixel> image(w, h);
    
    int step = cellSize;
    //#########################################################################################
    //########################## Dummy Frames matching chosen selector ########################
    //#########################################################################################
    //################# Metadata Band Drawing ########################
    MetadataHeader mdHeader = MetadataHeader(0x3, w, cellSize, nbits, 223);
    mdBitString = mdHeader.getMetadataHeader();
    int metadataCellSize = 8;

    int j = 0;
    //Draw upper metadata band 
    for (png::uint_32 x = 0; x < image.get_width(); x+= metadataCellSize){
        if(j < mdBitString.length()){
            //Fill cell
            for(int vp = 0; vp < metadataCellSize; vp++){
                for(int wp = 0; wp < metadataCellSize; wp++){
                    if(mdBitString[j] - '0' == 1)
                        image[vp][x+wp] = png::rgba_pixel(255, 255, 255, 255);
                    else
                        image[vp][x+wp] = png::rgba_pixel(0, 0, 0, 255);
                }
            }
        } else{
            for(int vp = 0; vp < metadataCellSize; vp++){
                for(int wp = 0; wp < metadataCellSize; wp++){
                    image[vp][x+wp] = png::rgba_pixel(0, 255, 0, 255);
                }
            }
        }
        j++;
    }
    //#################################################################


    for (png::uint_32 y = metadataCellSize; y < image.get_height(); y+= step){
        for (png::uint_32 x = 0; x < image.get_width(); x+= step){
            std::bitset<8> R;
            R.set(0,0);
            R.set(1,0);
            R.set(2,0);
            R.set(3,0);
            R.set(4,1);
            std::bitset<8> G;
            G.set(0,0);
            G.set(1,0);
            G.set(2,0);
            G.set(3,0);
            G.set(4,1);
            std::bitset<8> B;
            B.set(0,0);
            B.set(1,0);
            B.set(2,0);
            B.set(3,0);
            B.set(4,1);

            for(int j=0; j< nbits; j++){
                if(j%3 == 0){
                    if(rand()%2 == 1)
                        R.set(7- j/3);
                        //cout << "j/Nbits " << R << endl;
                }else if(j%3 == 1){
                    if(rand()%2 == 1){
                        //cout << "nbits: " << nbits << "j " << j << "DIV" << nbits/j << endl;
                        G.set(7- j/3);
                    }
                }else if(j%3 == 2){
                    if(rand()%2 == 1)
                        B.set(7- j/3);
                }
            }
            for(int vp = 0; vp < step; vp++){
                for(int wp = 0; wp < step; wp++){
                    image[y+vp][x+wp] = png::rgba_pixel(R.to_ulong(), G.to_ulong(), B.to_ulong(),255);
                }
            }
        }
    }


    ifstream dummySelectors("./../Client/Data/data0/" + std::to_string(w) + "x" + std::to_string(h) + "_" + std::to_string(cellSize) + "_" + std::to_string(nbits));
    if(!dummySelectors){
        ostringstream cmd;
        cmd << "mkdir ./../Client/Data/data0/" << std::to_string(w) << "x" + std::to_string(h) << "_" + std::to_string(cellSize) << "_" << std::to_string(nbits);
        int dummy_result = system(cmd.str().c_str());
    }

    image.write("./../Client/Data/data0/" + std::to_string(w) + "x" + std::to_string(h) + "_" + std::to_string(cellSize) + "_" + std::to_string(nbits) +"/frag" + std::to_string(packetFrag) + ".png");
}


void BitWorkerThread::colorFill(int packetId, int packetFrag, int totalFrag, vector<unsigned char> content){
    int i = 0;
    int nbits = *_nbits;
    string bitString;
    vector<unsigned char> data, headerData;
    png::image<png::rgba_pixel> image(*_w_frame, *_h_frame);
    
    int step = *_w_frame / *_w_div;
    //FrameType,PacketId,PacketFragNum,TotalPacketFrag

    FrameHeader header = FrameHeader(packetId, 0, packetFrag, totalFrag);

    
    bitString.append(header.getFrameHeader());

    
    //add data to bitstring
    for(int b = 0; b < content.size(); b++){
      bitString.append((bitset<8>(content[b])).to_string());
   }


    //cout << index << " " << content << endl;
    //cout << "Bitstring after encoding" << bitString << endl;
    //cout << "bitstring length: " << bitString.length() << endl;
    for (png::uint_32 y = 0; y < image.get_height(); y+= step){
        for (png::uint_32 x = 0; x < image.get_width(); x+= step){
            if(i < bitString.length()){

                std::bitset<8> R;
                R.set(0,0);
                R.set(1,0);
                R.set(2,0);
                R.set(3,0);
                R.set(4,1);
                std::bitset<8> G;
                G.set(0,0);
                G.set(1,0);
                G.set(2,0);
                G.set(3,0);
                G.set(4,1);
                std::bitset<8> B;
                B.set(0,0);
                B.set(1,0);
                B.set(2,0);
                B.set(3,0);
                B.set(4,1);

                /*std::bitset<8> R;
                R.set(6,1);
                std::bitset<8> G;
                G.set(6,1);
                std::bitset<8> B;
                B.set(6,1);*/

                for(int j=0; j< nbits; j++){
                    if(j%3 == 0){
                        if(bitString[i+j] - '0' == 1)
                            R.set(7- j/3);
                            //cout << "j/Nbits " << R << endl;
                    }else if(j%3 == 1){
                        if(bitString[i+j] - '0' == 1){
                            //cout << "nbits: " << nbits << "j " << j << "DIV" << nbits/j << endl;
                            G.set(7- j/3);
                        }
                    }else if(j%3 == 2){
                        if(bitString[i+j] - '0' == 1)
                            B.set(7- j/3);
                    }
                }
        //cout << "bits:"<<bitString[i]<<bitString[i+1]<<bitString[i+2]<<endl;
                for(int vp = 0; vp < step; vp++){
                    for(int wp = 0; wp < step; wp++){
                        image[y+vp][x+wp] = png::rgba_pixel(R.to_ulong(), G.to_ulong(), B.to_ulong(),255);
                    }
                }
        i+=nbits;
            }else{
                for(int vp = 0; vp < step; vp++){
                        for(int wp = 0; wp < step; wp++){
                            image[y+vp][x+wp] = png::rgba_pixel(255, 255, 255, 255);
                        }
                    }
            }
        }
    }

    

    image.write("./../Client/Data/data" + std::to_string(packetId) + "/frag" + std::to_string(packetFrag) + ".png");
}

