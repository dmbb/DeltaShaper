# DeltaShaper

DeltaShaper takes advantage of the encrypted video channel of widely used videoconferencing applications, such as Skype, as a carrier for unobservable covert TCP/IP communications. We have built a censorship-resistant system, named DeltaShaper, which offers a data-link interface and supports TCP/IP applications that tolerate low throughput / high latency links. Our results show that is possible to run standard protocols such as FTP, SMTP, or HTTP over Skype video streams. In our [paper](http://web.ist.utl.pt/diogo.barradas/papers/DeltaShaper_PETS17.pdf), we propose and evaluate different alternatives to encode information in the video stream in order to increase available throughput while preserving the packet-level characteristics of the video stream.


The prototype presented in this codebase represents a proof-of-concept of our system.

# Instructions for DeltaShaper Setup


Our prototype is tested in Linux Ubuntu 14.04 LTS (32 bits) and has C++ and Python 2.7 as language requirements.


## 1 - Installing dependencies

DeltaShaper's code depends on several third-party software. These dependencies are listed in `dependencies.txt`. Dependencies must be installed and are working correctly when tested on Linux Ubuntu 14.04 LTS (32 bits).


## 2 - Add missing files

Some files must be added manually to the project structure:


* `DeltaShaper/Utilities` should contain:


 &nbsp;&nbsp;&nbsp;&nbsp;`coverVideo.mp4` (scaled to 640x480). This video serves two purposes:


 &nbsp;&nbsp;&nbsp;&nbsp; &nbsp;&nbsp; 1. To correctly start Skype: If Skype is launched when no video is being fed to the camera device, it will fail to pick video from that device later on;


 &nbsp;&nbsp;&nbsp;&nbsp; &nbsp;&nbsp; 2. - To provide a cover video for DeltaShaper to produce covert frames.


* `DeltaShaper/DeltaShaper/Client/ReferenceStreams` should contain:


 &nbsp;&nbsp;&nbsp;&nbsp; This folder must contain parsed traffic captures (30 seconds each) of several regular Skype streams. These will form sets of reference streams which will be used to compute the EMD against the current DeltaShaper stream and trigger adjustments of the encoding selectors upon calibration.

 &nbsp;&nbsp;&nbsp;&nbsp; The expected structure for these files is the following: One folder like `regular1.mp4_bandwidth_0_30_0`, where `regular1.mp4` corresponds to the name of a sample regular stream, `bandwidth` corresponds to the perturbation executed upon the collected stream and `0` corresponds to the degree of perturbation. (The trailing `_30_0` is present due to our traffic sampling scripts. Since they are not important here, we may trim expected folder names in the future). In this case, this means our sample was collected in an unperturbed network environment. 
 
 
 For a different packet trace captured on a connection constrained to 150 Kbps, a suitable name could be `regular2.mp4_bandwidth_150_30_0`. Keep in mind that all regular streams captured in a given network condition will make part of the reference stream set for those particular network conditions.


 Inside each of these folders we can find `packetCount_50`, a file containing the packet lengths of a 30 seconds Skype traffic capture, binned in sizes k=50, with one packet length per line.

An example structure of these sets of reference streams is available in `DeltaShaper/DeltaShaper/Client/ReferenceStreams`. It depicts packet traces of a single video sample when streamed over different bandwidth restrictions. Here, a single sample represents the reference set for a given network condition.


* `DeltaShaper/DeltaShaper/Client/Data` should contain:


 &nbsp;&nbsp;&nbsp;&nbsp; `coverVideo.mp4` (scaled 640x480) - Video to be used as cover video


----

 Note: You may use the following ffmpeg recipe to scale a .mp4 video:


    ffmpeg -i input.mp4 -vf scale=640:480 -r 30 -pix_fmt yuv420p out.mp4


## 3 - Setting up Skype

To avoid hampering the screen area when running DeltaShaper on a conventional desktop, Skype is run in a virtual display. This also allow us to run Skype in headless servers. However, some automation is needed in order to login, set up Skype definitions and accept the terms of Skype4Py API (in the latter case). For this, we provide desktop automation scripts which allow for these steps to be completed in the absence of visual output.

1. Start the video camera device in /dev/video0: just `sudo modprobe v4l2loopback`
2. Start the virtual display and run Skype: run `Utilities/bgSkype.sh`
3. Login and configure Skype options(through our scripts in a headless machine, otherwise you can just use a VNC connector): compile and run the scripts on `DeltaShaper/SkypeAutomation` as user interactions are prompted by Skype. Skype options must be set so that video starts automatically when a call starts; the server node should be set up _not_ to answer calls automatically; user should be configured to perform auto sign-in. Note that running `rm -rf ~/.Skype` allows you to restart a Skype initial configuration from scratch.


----


Notes:


You may need to re-run `Utilities/bgSkype.sh` for video to be correctly delivered after performing the configurations on Skype. Just kill Skype and run the script. If all was well configured, Skype should now start correctly and auto-login your account.


Skype's prompt for accepting the use of Skype4Py API will only be triggered after trying to setup DeltaShaper pipelines (see 4.3). This will likely force you to stop the pipelines, accept the API through our automation script and then re-run the pipelines.


We have managed to use Skype video in 240p (320x240) and standard VGA (640x480) resolutions. All the higher resolutions he have experimented were sampled down by Skype. Thus, our video pipelines assume a VGA resolution.




## 4 - Setting up the DeltaShaper channel

### 1- Compile DeltaShaper Client & Server code

    cd DeltaShaper/DeltaShaper/Client
    make
    cd ../Server
    make

### 2- Setup netfilter packet interception kernel modules


 &nbsp;&nbsp;&nbsp;&nbsp; On the Client machine: Install the packet interception module in `DeltaShaper/DeltaShaper/Client/src/packet_queuer`. Compile and run `sudo insmod queuer.ko`


 &nbsp;&nbsp;&nbsp;&nbsp; On the Server machine: Install the packet interception module in `DeltaShaper/DeltaShaper/Server/src/packet_queuer`. Compile and run `sudo insmod queuer.ko`



### 3- Start DeltaShaper pipelines:

 &nbsp;&nbsp;&nbsp;&nbsp; 2.1. Start the server component on the server machine: `DISPLAY=:1.0 python general_autoAnswer.py`


 &nbsp;&nbsp;&nbsp;&nbsp; 2.2. Start the client component on the client machine: `DISPLAY=:1.0 python Call.py "CALLEE_HANDLE"`

The server component must be running before the client places its call. Once the client calls the server, the call will be picked up and an initial calibration procedure will start. After this calibration period, IP packets can be exchanged.


### 4- Run a TCP/IP application over DeltaShaper:

When the pipelines are running, a DeltaShaper channel is established. To start exchanging data after the initial calibration phase, you may run a typical TCP/IP application inside a Linux network namespace (in our prototype, this namespace is called TEST).

Example:

    sudo ip netns exec TEST wget "DELTASHAPER_SERVER_IP"/censoredPage.html
    sudo ip netns exec TEST ssh user@"DELTASHAPER_SERVER_IP"
    ... etc.
