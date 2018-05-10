CXXFLAGS?=	-O2 -g -Wall
CC?=		gcc
CXX?=		g++
LIBS?=
LDFLAGS?=
PREFIX?=	/usr/local
VERSION=1.5
TMPDIR=/tmp/webbench-$(VERSION)

webbench: WebBench.o 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o WebBench WebBench.o $(LIBS)

WebBench.o: WebBench.cpp
	$(CC) -c WebBench.cpp

clean:
	-rm -f *.o webbench *~ core *.core tags

