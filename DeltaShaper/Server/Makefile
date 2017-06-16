CC			= g++
CFLAGS		= -c -std=c++11 -O3 -I../include
LDFLAGS		= -lpthread -lpng
SOURCES		= ../include/thread.cpp ../include/xwdReader.cpp src/StagedFile.cpp src/WorkerThread.cpp 
OBJECTS		= $(SOURCES:.cpp=.o)

D_SRC      = src/DSServer.cpp
D_OBJ      = $(D_SRC:.cpp=.o)
D_TARGET	= vp

vpBuild: $(D_SRC) $(SOURCES) $(D_TARGET)

$(D_TARGET): $(D_OBJ) $(OBJECTS)
	$(CC) $(D_OBJ) $(OBJECTS) -o $@ $(LDFLAGS)
	
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(D_OBJ) $(OBJECTS) $(D_TARGET) 
