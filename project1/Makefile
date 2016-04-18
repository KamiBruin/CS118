CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=EDIT_MAKE_FILE
CLASSES=


my_server: my_server.cpp
	g++ -o my_server -Wall -g -pthread -std=c++11 my_server.cpp

my_client: my_client.cpp
	g++ -o my_client -Wall -g -pthread -std=c++11 my_client.cpp

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM web-server web-client *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
