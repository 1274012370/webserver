CXX = g++
CFLAGS = -std=c++11

OBJS = ./src/*.cpp main.cpp

TARGET = server
HEAD = ./head
all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -I $(HEAD)