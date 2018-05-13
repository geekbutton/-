CXXFLAGS?=	-O2 -g -Wall
CC?=		gcc
CXX?=		g++
LIBS?=
LDFLAGS?=
PREFIX?=	/usr/local
VERSION=1.5
TMPDIR=/tmp/webbench-$(VERSION)

webbench: WebBench.o Socket.o 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o WebBench WebBench.o Socket.o$(LIBS)

WebBench.o: WebBench.cpp Socket.h
	$(CXX) -c WebBench.cpp

Socket.o: Socket.cpp Socket.h
	$(CXX) -c Socket.cpp

clean:
	-rm -f *.o webbench *~ core *.core tags

