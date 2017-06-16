#include "TransmissionThread.h"  
#include <string>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sstream>
#include <chrono>
#include <sys/types.h>
#include <gst/gst.h>
#include <cv.h>
#include <highgui.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


static GMainLoop *loop;

const gchar* format = "BGRA";
const guint width = 320;
const guint height = 240;
const int DUMMY_FRAMES = 40;


TransmissionThread::TransmissionThread(int* signaled, wqueue<TransmissionID>& queue, int* w_frame, int* h_frame, int* w_div, int* h_div, int* nbits, int framerate, int* nextSelector) : _signaled(signaled), _transmission_queue(queue) , _w_frame(w_frame), _h_frame(h_frame), _w_div(w_div), _h_div(h_div), _nbits(nbits), _framerate(framerate), _next_selector(nextSelector) {}

void TransmissionThread::InitPipelines(int *snowmixPID, int *outputPID, int *backgroundInputPID, int *ffmpegPID){
    
    /* Init Snowmix Live Video Mixer */
    pid_t pid = fork();

    switch (pid){
    case -1: // error
        perror("Error forking");
        exit(1);

    case 0: // child process
        if(setpgid(0, 0) == -1)
                perror("setpgid error:");
        char *envp[] ={"SNOWMIX=/usr/local/lib/Snowmix-0.5.1", "HOME=/", (char*)0};
        execle("/usr/local/bin/snowmix","/usr/local/bin/snowmix", "./../Client/SnowmixScripts/InitPipeline.ini", (char*)0, envp); //start the pipeline on child
        perror("execle Snowmix");
        exit(1);
    }
    *snowmixPID = pid;

    sleep(2);

    /* Send Snowmix output to pipe */
    pid = fork();

    switch (pid){
    case -1: // error
        perror("Error forking");
        exit(1);

    case 0: // child process
        if(setpgid(0, 0) == -1)
                perror("setpgid error:");
        char *envp[] ={"SNOWMIX=/usr/local/lib/Snowmix-0.5.1", "HOME=/", (char*)0};
        execle("./../Client/SnowmixScripts/output2pipe.sh","./../Client/SnowmixScripts/output2pipe.sh", (char*)0, envp); //start the pipeline on child
        perror("execle output2pipe");
        exit(1);
    }
    *outputPID = pid;

    sleep(2);

    /* Init background chat feed */
    pid = fork();

    switch (pid){
    case -1: // error
        perror("Error forking");
        exit(1);

    case 0: // child process
        if(setpgid(0, 0) == -1)
                perror("setpgid error:");
        char *envp[] ={"SNOWMIX=/usr/local/lib/Snowmix-0.5.1", "HOME=/", (char*)0};
        execle("./../Client/SnowmixScripts/input2feed.sh","./../Client/SnowmixScripts/input2feed.sh", "1", "0", "./../Client/Data/coverVideo.mp4", (char*)0, envp); //start the pipeline on child
        perror("execle input2feed");
        exit(1);
    }
    *backgroundInputPID = pid;
    
    sleep(2);
    
    /* Init ffmpeg mirror pipeline */
    pid = fork();

    switch (pid){
    case -1: // error
        perror("Error forking");
        exit(1);

    case 0: // child process
        execl("/usr/bin/ffmpeg","/usr/bin/ffmpeg", "-nostats", "-nostdin", "-loglevel" ,"quiet", "-i", "./../Client/SnowmixScripts/videoPipe", "-r", "30" , "-pix_fmt", "yuv420p" ,"-threads" ,"0" ,"-f", "v4l2", "/dev/video0", (char*)0); //start the pipeline on child
        perror("execl ffmpeg");
        exit(1);
    }
    *ffmpegPID = pid;
}


static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer *user_data) {
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    IplImage* img = NULL;
    GstMapInfo map;
    ostringstream ss;
    UserData *d = (UserData*) user_data;
    bool dataAvailable = false;


    //Play video-encoded packet 
    if (d->queue->size() != 0){
        TransmissionID camera_ready = d->queue->remove();
        if(camera_ready.packetID == -1){
            cout << "Terminator on queue" << endl;
            printf("Killing Pipelines with PIDs: %d, %d, %d, %d\n", d->snowmixPID, d->outputPID, d->backgroundPID, d->ffmpegPID);
            kill(-d->snowmixPID,SIGKILL);
            kill(-d->outputPID,SIGKILL);
            kill(-d->backgroundPID,SIGKILL);
            kill(-d->ffmpegPID,SIGINT);
            int result = system("rm /run/shm/shmpipe*");
            g_main_loop_quit (loop);
            printf("end \n");
            
        }else{
            if(camera_ready.packetID == 0 && camera_ready.fragmentID == (DUMMY_FRAMES-1)){
                (*d->nextSelector) += 1;
                cout << "Updated selector to " << (*d->nextSelector) << endl;
            }
            ss << "./../Client/Data/data" << to_string(camera_ready.packetID) << "/frag" << to_string(camera_ready.fragmentID) << ".png";
            cout << "[C] Playing Packet:" << to_string(camera_ready.packetID) << " , Part:" << to_string(camera_ready.fragmentID) << endl;
            img=cvLoadImage(ss.str().c_str(), CV_LOAD_IMAGE_UNCHANGED);
            dataAvailable = true;
            auto timeSent = chrono::high_resolution_clock::now();    
            std::cout << "[C] (TransmissionThread) Data timestamp: "
          << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
          << '\n';
        }
    } else{
        //No new data, load placeholder
        /*cout << "Window width: " << (*d->w_frame) << endl;
        cout << "W Cells width: " << (*d->w_div) << endl;
        cout << "Window Height: " << (*d->h_frame) << endl;
        cout << "W Cells height: " << (*d->h_div) << endl;
        cout << "Bits: " << (*d->nbits) << endl;*/

        int cellSize = (*d->w_frame) / (*d->w_div);


        ss << "./../Client/Data/data0/" << std::to_string((*d->w_frame)) << "x" << std::to_string((*d->h_frame)) << "_" << std::to_string(cellSize) << "_" << std::to_string((*d->nbits)) << "/frag0.png";
        img=cvLoadImage(ss.str().c_str(), CV_LOAD_IMAGE_UNCHANGED); 

        //DEBUG check timestamp of dummy image load
        auto timeSent = chrono::high_resolution_clock::now();    
            /*std::cout << "(TransmissionThread) Dummy timestamp: "
          << std::chrono::duration_cast<chrono::milliseconds>(timeSent.time_since_epoch()).count()
          << '\n';*/
    }


    size = img->height * img->width * img->nChannels;

    buffer = gst_buffer_new_allocate (NULL, size, NULL);
    gst_buffer_map (buffer, &map, GST_MAP_WRITE);
    memcpy( (guchar *)map.data, (guchar *)img->imageData,  gst_buffer_get_size( buffer ) );

    /* Buffer Presentation Time */
    GST_BUFFER_PTS (buffer) = timestamp; 

    /* Duration of Buffer Data */
    if(dataAvailable == true) //Buffer duration is 1/Framerate
        GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, d->framerate);
    else 
        GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, d->framerate);

    /* Next buffer timestamp */
    timestamp += GST_BUFFER_DURATION (buffer);

    /* Push Buffer Data to the pipeline */
    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);


    if (ret != GST_FLOW_OK) {
    /* something went wrong, stop pushing */
        cout << "[C] Uh-oh, something went wrong with the GStreamer pipeline" << endl;
        g_main_loop_quit (loop);
    }


    gst_buffer_unmap(buffer, &map);
    gst_buffer_unref (buffer);
    cvReleaseImage(&img);
}

void* TransmissionThread::run() {
    int snowmixPID, outputPID, backgroundInputPID, ffmpegPID;
    ostringstream ss;
    struct dirent *de;
    pid_t pid;
    InitPipelines(&snowmixPID, &outputPID, &backgroundInputPID, &ffmpegPID);
    UserData cb_data;
    cb_data.queue = &_transmission_queue;
    cb_data.snowmixPID = snowmixPID;
    cb_data.backgroundPID = backgroundInputPID;
    cb_data.outputPID = outputPID;
    cb_data.ffmpegPID = ffmpegPID;
    cb_data.framerate = _framerate;
    cb_data.w_frame = _w_frame;
    cb_data.h_frame = _h_frame;
    cb_data.w_div = _w_div;
    cb_data.h_div = _h_div;
    cb_data.nbits = _nbits;
    cb_data.nextSelector = _next_selector;


    // Clear Control Pipe for this connection to Snowmix
    remove("/tmp/feed-2-control-pipe");

    GstElement *pipeline, *appsrc, *conv, *videosink;

    // Init GStreamer 
    gst_init (NULL, NULL);
    loop = g_main_loop_new (NULL, FALSE);

    // Setup pipeline 
    pipeline = gst_pipeline_new ("pipeline");
    appsrc = gst_element_factory_make ("appsrc", "source");
    conv = gst_element_factory_make ("videoconvert", "conv");
    videosink = gst_element_factory_make ("shmsink", "shmsink");

    gint sharedMemorySize = width * height * 4 * 32;

    // Elements Setup 
    g_object_set (G_OBJECT (videosink), "socket-path", "/tmp/feed-2-control-pipe",
                                    "shm-size", sharedMemorySize,
                                    "wait-for-connection", 0,
                                    "sync", 1,
                                    NULL);

    g_object_set (G_OBJECT (appsrc), "caps",
        gst_caps_new_simple ("video/x-raw",
                     "format", G_TYPE_STRING, format,
                     "width", G_TYPE_INT, width,
                     "height", G_TYPE_INT, height,
                     "pixel-aspect-ratio", G_TYPE_STRING, "1/1",
                     "interlace-mode", G_TYPE_STRING, "progressive",
                     "framerate", GST_TYPE_FRACTION, 30, 1,
                     NULL), NULL);
    gst_bin_add_many (GST_BIN (pipeline), appsrc, conv, videosink, NULL);
    gst_element_link_many (appsrc, conv, videosink, NULL);

    // Setup appsrc & Register callbacks
    g_object_set (G_OBJECT (appsrc),
        "stream-type", 0,
        "format", GST_FORMAT_TIME, NULL);
    g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), &cb_data);

    
    // Play 
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);

    // Clean the mess 
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline));
    g_main_loop_unref (loop);
}
