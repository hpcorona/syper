CC=g++
CFLAGS=
LDFLAGS=-fPIC -lfcgi -lzmq
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)

all: clean libsyper.so

clean:
	rm *.o

%.o: %.cpp
	$(CC) -std=c++0x -c $(CFLAGS) $<

libsyper.so: $(OBJS) 
	$(CC) -shared -Wl,-soname,libsyper.so.1 $(LDFLAGS) -o libsyper.so $^
