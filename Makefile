CPP     = g++
CFLAGS  = -Wall -g -c -std=c++0x -Wno-deprecated
INCLUDE = -I .
TARGET  = bin/dotest
LIBVAR  = -lpthread
SRC     = $(wildcard *.cpp)
OBJ     = $(SRC:.cpp=.o)

#all:$(OBJ) $(LIB)
#       $(CPP) $(CFLAGS) $(INCLUDE) -o $(TARGET) $(OBJ) $(LIBVAR)

.cpp.o:
	$(CPP) $(CFLAGS) -o $*.o $(INCLUDE) $*.cpp

$(TARGET):$(OBJ)
	$(CPP) -o $(TARGET) $(OBJ) $(LIBVAR)
	rm -rf *.o

all:$(TARGET)

clean:
	find -name "*.o" -exec rm {} \;
	rm -rf $(TARGET)
