#include "DSClient.h"
#include <signal.h>
#include <fstream>
#include <cmdline.h>
#include <netinet/in.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <errno.h>
#include <arpa/inet.h>
#include <linux/ip.h>

using namespace std;

static int signaled = 0;
static int state = 0;   //0: Regular 1: On Calibration (starts with caller value)

const int rs_payload = 223;
const int mode = 0; //0 = RS(ECC), 1 = Bit(NoECC),

void int_handler(int signum){
    signaled = 1;
    cout << "Stopping...(" << signum << ")" << endl;
    cout << "Deleting data folders..." << endl;
    string cmd = "rm -rf ./../Client/Data/data*";
    int result = system(cmd.c_str());
    cmd = "rm -rf ./../Client/CalibrationRaw/";
    result = system(cmd.c_str());
    cmd = "rm -rf ./../Client/CalibrationFiltered/";
    result = system(cmd.c_str());
    //exit(0);  
}

DSClient::DSClient(int* w_frame, int* h_frame, int* w_div, int* h_div, int caller, int* framerate, int* nbits, int* nextSelector) : 
     _w_div(w_div), _h_div(h_div), _w_frame(w_frame), _h_frame(h_frame), _caller(caller), _framerate(framerate), _nbits(nbits), _next_selector(nextSelector) {}


int handler(struct nfq_q_handle *myQueue, struct nfgenmsg *msg, struct nfq_data *pkt, void *cbData) {
    unsigned int saddr, daddr;
    int pktID = 0;
    struct nfqnl_msg_packet_hdr *header;
    struct iphdr *ip;
    DSClient *client = (DSClient *) cbData;

    if( header = nfq_get_msg_packet_hdr(pkt) )
        pktID = ntohl(header->packet_id);
        nfq_get_payload ( ( struct nfq_data * ) pkt, (unsigned char**)&ip );


    unsigned char *pktData;
    char* address;
    int pktLen = nfq_get_payload(pkt, &pktData);
    
    if(client->getCallerMode())
        address = inet_ntoa(*(in_addr*)&ip->saddr);
    else
        address = inet_ntoa(*(in_addr*)&ip->daddr);

    if(strcmp(address,"10.10.10.10") == 0){
        cout << "Sending data[" << pktLen << "]" <<endl;


        auto timeSent = chrono::high_resolution_clock::now();    
        std::cout << "(DSClient) Got packet from NFQUEUE: "
          << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
          << '\n';

        switch(mode){
            case 0: 
                client->RSFill(pktData, pktLen, pktID);
                break;
            case 1: 
                client->BitFill(pktData, pktLen, pktID);
                break;
            case 2: 
                client->TriBitFill(pktData, pktLen, pktID);
        }
    //Steal packets originating from the container. (These are not delivered back to the network) 
    //Ex: Ping - veth0 receives packet from veth1 but the packet is stolen before actual deliver
    //            to end interface
    //Accept everything else
    
        return nfq_set_verdict(myQueue, pktID, NF_STOLEN, pktLen, pktData);
    }
    else
        return nfq_set_verdict(myQueue, pktID, NF_ACCEPT, pktLen, pktData);
}


int DSClient::Start(int mode){
    struct nfq_handle *nfqHandle;
    struct nfq_q_handle *myQueue;
    struct nfnl_handle *netlinkHandle;

    int fd, res;
    char buf[4096];

    //set up network container
    ifstream container("/var/run/netns/TEST");
    if(!container){
        string cmd = "./../Client/network_container.sh";
        int container_result = system(cmd.c_str());
    }

    //copy metadata png to tmp folder for easy snowmix access
    ifstream metadata("/tmp/metadata.png");
    if(!metadata){
        int metadata_result = system("cp ./../Client/Data/metadata.png /tmp/metadata.png");
    }

    //set up Folder to store encoded frames
    ifstream data("./../Client/Data");
    if(!data){
        string cmd = "mkdir ./../Client/Data";
        int data_result = system(cmd.c_str());
    }

    // _queue connection
    if (!(nfqHandle = nfq_open())) {
        perror("Error in nfq_open()");
        return(1);
    }

    // bind this handler
    if (nfq_bind_pf(nfqHandle, AF_INET) < 0) {
        perror("Error in nfq_bind_pf()");
        return(1);
    }

    // define a handler
    if (!(myQueue = nfq_create_queue(nfqHandle, 0, &handler, this))) {
        perror("Error in nfq_create_queue()");
        return(1);
    }

    // turn on packet copy mode
    if (nfq_set_mode(myQueue, NFQNL_COPY_PACKET, 0xffff) < 0) {
        perror("Could not set packet copy mode");
        return(1);
    }

    netlinkHandle = nfq_nfnlh(nfqHandle);
    fd = nfnl_fd(netlinkHandle);

    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);

    //start threads
    _transmissionThread = new TransmissionThread(&signaled, _transmission_queue, _w_frame, _h_frame, _w_div, _h_div, _nbits, *_framerate, _next_selector);
    _transmissionThread->start();

    _calibrationThread = new CalibrationThread(&state, _caller);
    _calibrationThread->start();

    //Worker threads number can be found in header file
    for (int i = 0; i < _threadV.size(); i++){
        switch(mode){
            case 0: 
                _threadV[i] = new RSWorkerThread(&state, _queue, _transmission_queue, _mtx, _caller, _w_div, _h_div, _w_frame, _h_frame, _framerate, _nbits, _next_selector);
                break;
            case 1: 
                _threadV[i] = new BitWorkerThread(&state, _queue, _transmission_queue, _mtx, _caller, _w_div, _h_div, _w_frame, _h_frame, _framerate, _nbits, _next_selector);
                break;
        }
        _threadV[i]->start();
    }

    // Restore the old signal mask only for this thread.
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    
    struct sigaction s;
    s.sa_handler = int_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);
    sigaction(SIGTERM, &s, NULL);
    
    //main cycle
    while ((res = recv(fd, buf, sizeof(buf), 0)) && res >= 0 && signaled == 0){
        nfq_handle_packet(nfqHandle, buf, res);
    }

    nfq_destroy_queue(myQueue);
    nfq_close(nfqHandle);

    TransmissionID tid;
    tid.packetID = -1;
    tid.fragmentID = -1;
    _transmission_queue.add(tid);
    printf("Added terminator\n");
    _transmissionThread->join();
    printf("It joined\n");
    return 0;
}

void DSClient::RSFill(unsigned char* file, int pktLen, int packetID){
    int packetFrag = 0;    
    int nBytes = ((*_w_div * (*_h_div-1))/8)* *_nbits;
    int cell_indicator_size = 2;
    int header_size = 5;    
    int max_frame_bytes = nBytes;
    vector<unsigned char> content;
    
    Item* item; 
    int full_ecc_blocks = max_frame_bytes/255.0; //For RS


    int bytes_per_frame;
    if(full_ecc_blocks == 0)  //Payload frame can't fit a whole 255 block
        bytes_per_frame = max_frame_bytes - (255 - rs_payload) - (header_size + cell_indicator_size); 
    else if(255*full_ecc_blocks + (255 - rs_payload) >= max_frame_bytes) //Payload frame fits N ECC blocks but has no space for another due
        bytes_per_frame = rs_payload * full_ecc_blocks - (header_size + cell_indicator_size);    // to lack of space for ECC bytes
    else //Payload frame fits N ECC Blocks and has space for a partial one
        bytes_per_frame = max_frame_bytes - ((255 - rs_payload) * (full_ecc_blocks + 1)) - (header_size + cell_indicator_size);

    
    //Put packet data into a vector
    for(int i = 0; i < pktLen; i++)
        content.push_back(file[i]);


    //cout << "(DSClient)Packet length : " << pktLen << endl;
    //cout << "(DSClient)Content length : " << content.size()<<endl;
    //cout << "bytes_per_frame : " << bytes_per_frame << endl;;

    int tframes = ceil(content.size()/(float)bytes_per_frame);
    while(1){
        if (content.size() > bytes_per_frame){
            std::vector<unsigned char> subvec(content.begin(), content.begin() + bytes_per_frame);
            item = new Item(packetID, packetFrag, subvec, tframes);
            _queue.add(item);
            packetFrag++;
            std::vector<unsigned char> subvec2(content.begin() + bytes_per_frame, content.end());
            content = subvec2;
        }else{
            item = new Item(packetID, packetFrag, content, tframes);
            _queue.add(item);
            break;
        }
    }

}

void DSClient::BitFill(unsigned char* file, int pktLen, int packetID){
    int packetFrag = 0;
    int nBytes = ((*_w_div * *_h_div)/8)* *_nbits;
    int header_size = 5;    
    
    vector<unsigned char> content;
    for(int i = 0; i < pktLen; i++)
        content.push_back(file[i]);
    //cout << "File length after b64 encoding (Bytes): " << content.length() << endl;
    //cout << "Max bytes in a frame (-8 header): " << nBytes - 8 << endl;
    //cout << content << endl;
    
    Item* item;
    int tframes = ceil(content.size()/((float)nBytes-header_size)); //accounting for header
    cout << "Total frames: " << tframes << endl;

    while(1){
        if (content.size() > nBytes){
            std::vector<unsigned char> subvec(content.begin(), content.begin() + nBytes-header_size);
            int ncells = ceil(((subvec.size() + header_size )*8)/(float)*_nbits);
            cout << " NCells: " << ncells << endl;
            item = new Item(packetID, packetFrag, subvec, tframes);
            _queue.add(item);
            packetFrag++;
            std::vector<unsigned char> subvec2(content.begin() + nBytes-header_size, content.end());
            content = subvec2;
        }else{
            int ncells = ceil(((content.size() + header_size )*8)/(float)*_nbits);
            cout << " NCells: " << ncells << endl;
            item = new Item(packetID, packetFrag, content, tframes);
            _queue.add(item);
            break;
        }
    }
}


int main(int argc, char *argv[]){
    
    cmdline::parser parser;

    parser.add<string>("caller", 'r', "Caller 1, Callee 0", true, "");
    parser.add<string>("framerate", 'f', "Payload FPS", true, "");
    
    parser.parse_check(argc, argv);
    

    const int caller = atoi((parser.get<string>("caller")).c_str());
	
    //Select starting parameters
    int framerate = atoi((parser.get<string>("framerate")).c_str());
    int w_div = 40;
    int h_div = 30;
    int w = 320;
    int h = 240;
    int nbits = 6;
    int nextSelector = 0;

    cout << "Starting DeltaShaper client." << endl << "Press Ctrl-C to stop this program." << endl;
    
    DSClient* client = new DSClient(&w, &h, &w_div, &h_div, caller, &framerate, &nbits, &nextSelector);
    
    client->Start(mode);
    
    cout << "Exiting" << endl;
    return 0;
}
