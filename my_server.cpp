#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>

#include "httpMessage.cpp"

char argv_host[1024];
char argv_port[256];
char argv_dir[1024];
char null_addr[INET_ADDRSTRLEN]={'\0'};

pid_t cpid;

long file_length(char *f)
{
    struct stat st;
    stat(f, &st);
    return st.st_size;
}

char* getIP(char hostname[],char hostport[]){
  struct addrinfo hints;
  struct addrinfo* res;

  static char result[INET_ADDRSTRLEN]={'\0'};
  static char ipstr[INET_ADDRSTRLEN] = {'\0'};

  //result="";

    // prepare hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  int status = 0;
  if ((status = getaddrinfo(hostname, hostport, &hints, &res)) != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
    return result;
  }

  for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
    // convert address to IPv4 address
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;

    // convert the IP to a string and print it:
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    std::cout << "  " << ipstr << std::endl;
    //result=ipstr;
    // std::cout << "  " << ipstr << ":" << ntohs(ipv4->sin_port) << std::endl;
  }

  freeaddrinfo(res); // free the linked list

  return ipstr;
}

int
main(int argc,char* argv[])
{
  if(argc==4){
    strcpy(argv_host,argv[1]);
    strcpy(argv_port,argv[2]);
    strcpy(argv_dir,argv[3]);
  }
  else if(argc==1){
    strcpy(argv_host,"localhost");
    strcpy(argv_port,"4000");
    strcpy(argv_dir,".");
  }
  else{
    std::cerr << "usage: invalid arguments" << std::endl;
    return 1;
  }

  char host_ip[INET_ADDRSTRLEN]={'\0'};
  strcpy(host_ip,getIP(argv_host,argv_port));
  if(strcmp(host_ip,null_addr)==0){
    return 2;
  }
  std::cout<<"host_ip:"<<host_ip<<std::endl;

  // create a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv_port));     // short, network byte order
  addr.sin_addr.s_addr = inet_addr(host_ip);
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  int flag=::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) ;
  if (flag == -1){
    perror("bind");
    return 2;
  }

  int cp_count=0;
  // set socket to listen status
  while(1){
    if (listen(sockfd, 1) == -1) {
      perror("listen");
      return 3;
    }

    // accept a new connection
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int clientSockfd = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);

    if (clientSockfd == -1) {
      perror("accept");
      return 4;
    }

    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    std::cout << "Accept a connection from: " << ipstr << ":" <<
      ntohs(clientAddr.sin_port) << std::endl;

    // read/write data from/into the connection
    bool isEnd = false;
    char buf[1024] = {'\0'};
    std::stringstream ss;
    cp_count+=1;
    cpid=fork();
    if(cpid==-1){
      perror("Fork failure:");
      return 3;
    }
    else if(cpid==0){
      std::cout<<"This is the "<<cp_count<<"-th child process!"<<std::endl;
      std::cout<<"Connection at port:"<<clientSockfd<<"!"<<std::endl;
      HttpRequest httpRequest;
      while (!isEnd) {
        memset(buf, '\0', sizeof(buf));

        if (recv(clientSockfd, buf, 1024, 0) == -1) {
          perror("recv");
          return 5;
        }

        isEnd=httpRequest.feed(buf);
        ss << buf << std::endl;
        std::cout << buf << std::endl;

        if (ss.str() == "close\n")break;

        ss.str("");
        break;
      }
      httpRequest.consume();
      char url[1024]={'\0'};
      strcpy(url,httpRequest.getUrl());
      cout<<"Result of getUrl():"<<url<<endl;

      std::cout<<"Finish reading the request!"<<std::endl;

      char filename[1024]={'\0'};
      strcpy(filename,argv_dir);
      strcat(filename,url);
      std::cout<<"Open file:"<<filename<<std::endl;
      int fd=open(filename,O_RDONLY|O_APPEND,0644);
      HttpResponse httpResponse;
      if(fd==-1){
        // perror("Open file:");
        // _exit(2);
        //404 NOT FOUND
        httpResponse.setStatus("404");
        httpResponse.setDescription("NOT FOUND");
        httpResponse.setHeader("Server", "Apache/2.4.18 (FreeBSD)");
        httpResponse.setHeader("Content-Length","91");
        httpResponse.setHeader("Connection", "close");

      }
      else{
        long contentLength=file_length(filename);
        httpResponse.setStatus("200");
        httpResponse.setDescription("OK");
        httpResponse.setHeader("Server", "Apache/2.4.18 (FreeBSD)");
        httpResponse.setHeader("Content-Length",to_string(contentLength));
        httpResponse.setHeader("Connection", "close");

        // string badresponse = "zhonghuarenmingongheguowansui";
        const char* hr_header = httpResponse.encode();
        // int count1=send(clientSockfd, badresponse.c_str(), strlen(hr_header), 0) ;
        int count1=send(clientSockfd, hr_header, strlen(hr_header), 0);
        if (count1== -1) {
          perror("Send Header");
          return 6;
        }
        while(1){
          int count=read(fd,buf,sizeof(buf));

          std::cout<<std::endl<<count<<" chars read!"<<std::endl;
          if(count==0)break;
          int count1=send(clientSockfd, buf, count, 0) ;
          if (count1== -1) {
            perror("Send");
            return 6;
          }
          // if(count<20)break;
        }
          
        close(clientSockfd);
        std::cout<<"Socket closing..."<<std::endl<<std::endl;
        _exit(0);
      }
    }
    else{
      close(clientSockfd);
    }
  }
  return 0;
}
