CC=g++
RM=rm -f
CFLAGS=-std=c++17 $(shell pkg-config --cflags gstreamer-1.0 tinyxml2)
LDFLAGS=-pthread
LDLIBS=$(shell pkg-config --libs gstreamer-1.0 tinyxml2) -lstdc++fs

SRCS=main.cpp ChickenCam.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: CFLAGS+= -g
all: LDFLAGS+= -g
all: ChickenCam

release: CFLAGS+= -O3
release: LDFLAGS+= -O3
release: ChickenCam

ChickenCam: $(OBJS)
	$(CC) $(LDFLAGS) -o ChickenCam $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) ChickenCam