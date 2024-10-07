CXX=g++
CXXFLAGS=-std=c++17 -g -pedantic -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer
LDLIBS=

# 0 for output in autograder, 1 for no output in autograder
OUT=1


SRCS=server.cpp client.cpp
DEPS=BoundedBuffer.cpp common.cpp TCPRequestChannel.cpp Histogram.cpp HistogramCollection.cpp
BINS=$(SRCS:%.cpp=%.exe)
OBJS=$(DEPS:%.cpp=%.o)


all: clean $(BINS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.exe: %.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(patsubst %.exe,%,$@) $^ $(LDLIBS)


.PHONY: clean print-var collect-minfo test

clean:
	rm -f server client *.tst *.o *.csv received/*

print-var:
	echo $(OUT)

collect-minfo:
	uname -a > minfo.txt && lscpu >> minfo.txt && git add minfo.txt && git commit -m "Machine information"

test: all
	mkdir -p received/
	chmod u+x pa4-tests.sh
	./pa4-tests.sh
