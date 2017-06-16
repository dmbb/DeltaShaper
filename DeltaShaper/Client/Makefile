CC			= g++
CFLAGS		= -c -std=c++11 -O3 -I../include `pkg-config --cflags opencv gstreamer-1.0`
LDFLAGS		= -lpthread -lpng -lnetfilter_queue -lnfnetlink `pkg-config --libs  opencv gstreamer-1.0`
SOURCES		= ../include/thread.cpp ../include/xwdReader.cpp src/WorkerThread.cpp src/TransmissionThread.cpp src/CalibrationThread.cpp
OBJECTS		= $(SOURCES:.cpp=.o)

D_SRC      = src/DSClient.cpp
D_OBJ      = $(D_SRC:.cpp=.o)
D_TARGET	= div


divBuild: $(D_SRC) $(SOURCES) $(D_TARGET)

$(D_TARGET): $(D_OBJ) $(OBJECTS)
	$(CC) $(D_OBJ) $(OBJECTS) -o $@ $(LDFLAGS)
	
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(D_OBJ) $(OBJECTS) $(D_TARGET) 
