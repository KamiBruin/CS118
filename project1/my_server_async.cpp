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

#include "httpMessage.h"

struct Connection{
  int fdNum;
  int responseFdNum;
  char* responesHeader;
  int headerCount;
  int headerSent;
  char readBuffer[1024];
  char writeBuffer[1024];
  HttpRequest httpRequest;
  HttpResponse httpResponse;
  //timer
  struct Connection *next;
  struct Connection *prev;

  int writeOK;
  int writeCount;
  int sent;
};

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
  int maxSockfd=sockfd;

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

  fd_set readFds;
  fd_set writeFds;
  fd_set errFds;
  fd_set watchFds;

  fd_set readFdss;
  fd_set writeFdss;
  fd_set errFdss;

  FD_ZERO(&readFds);
  FD_ZERO(&writeFds);
  FD_ZERO(&errFds);

  FD_ZERO(&readFdss);
  FD_ZERO(&writeFdss);
  FD_ZERO(&errFdss);
  FD_ZERO(&watchFds);


  FD_SET(sockfd,&watchFds);
  FD_SET(sockfd, &readFdss);
  FD_SET(sockfd,&errFdss);
  FD_SET(sockfd,&writeFdss);

  if (listen(sockfd, 20) == -1) {
    perror("listen");
    return 3;
  }


  struct Connection curCon;
  curCon.fdNum=sockfd;
  curCon.responseFdNum=-1;
  memset(curCon.readBuffer, '\0', sizeof(curCon.readBuffer));
  memset(curCon.writeBuffer,'\0', sizeof(curCon.writeBuffer));
  curCon.next=NULL;
  curCon.prev=NULL;
  curCon.writeCount=0;
  curCon.writeOK=0;
  curCon.sent=0;
  struct Connection* headCon=&curCon;


  bool isEnd = false;
  struct timeval tv;
  int nReadyFds = 0;
    
  tv.tv_sec = 3;
  tv.tv_usec = 0;

  // readFds = watchFds;
  // writeFds= watchFds;
  // errFds = watchFds;
  // int cp_count=0;
  // set socket to listen status
  while(1){
    readFds=readFdss;
    writeFds=writeFdss;
    errFds=errFdss;
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    cout<<"Begin select."<<endl;
    if ((nReadyFds = select(maxSockfd + 1, &readFds, &writeFds, &errFds, &tv)) == -1) {
      perror("select");
      return 4;
    }  	
    if (nReadyFds == 0) {
      std::cout << "No data is received for 3 seconds!" << std::endl;
    }
    else {
      struct Connection* tmpCon=headCon;
      for(; tmpCon != NULL; tmpCon=tmpCon->next) {
        cout<<"Checking connection:"<<tmpCon->fdNum<<endl;
        int fd=tmpCon->fdNum;
        // get one socket for reading
        if (FD_ISSET(fd, &readFds)) {
          cout<<"\t"<<fd<<" is ready to read."<<endl;
          if (fd == sockfd) { // this is the listen socket
            struct sockaddr_in clientAddr;
            socklen_t clientAddrSize = sizeof(clientAddr);
            int clientSockfd = accept(fd, (struct sockaddr*)&clientAddr, &clientAddrSize);
            if (clientSockfd == -1) {
              perror("accept");
              return 5;
            }
            char ipstr[INET_ADDRSTRLEN] = {'\0'};
            inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
            std::cout << "Accept a connection from: " << ipstr << ":" <<
              ntohs(clientAddr.sin_port) << " with Socket ID: " << clientSockfd << std::endl;

            // update maxSockfd
            if (maxSockfd < clientSockfd)
              maxSockfd = clientSockfd;

            // add the socket into the socket set
            FD_SET(clientSockfd, &readFdss);
            FD_SET(clientSockfd, &errFdss);
            cout<<"Setting new connection."<<endl;
            struct Connection* newCon=new Connection;
            newCon->fdNum=clientSockfd;
            newCon->responseFdNum=-1;
            memset(newCon->readBuffer, '\0', sizeof(newCon->readBuffer));
            memset(newCon->writeBuffer,'\0', sizeof(newCon->writeBuffer));
            newCon->next=headCon;
            newCon->prev=NULL;
            newCon->writeCount=0;
            newCon->writeOK=0;
            newCon->sent=0;
            newCon->responesHeader=new char[1024];
            headCon->prev=newCon;
            headCon=newCon;
            cout<<"Finishing new connection."<<endl;
            continue;
          }
          else { // this is the normal socket
            HttpRequest* httpRequest=&tmpCon->httpRequest;
            char tmpBuffer[1024]={'\0'};
            memset(tmpBuffer, '\0', sizeof(tmpBuffer));
            int recvLen=recv(fd,tmpBuffer, sizeof(tmpBuffer),0);
            if(recvLen==-1){
              perror("recv");
              return 6;
            }
            if(recvLen==0){
              close(tmpCon->responseFdNum);
              close(fd);
              FD_CLR(fd, &readFdss);
              FD_CLR(fd,&writeFdss);
              FD_CLR(fd,&errFdss);
              FD_CLR(tmpCon->responseFdNum,&readFdss);
              cout<<"Deleting node"<<endl;
              // struct connection *deadCon=tmpCon;
              if(tmpCon->prev!=NULL)tmpCon->prev->next=tmpCon->next;
              if(tmpCon->next!=NULL)tmpCon->next->prev=tmpCon->prev;
              if(tmpCon==headCon)headCon=tmpCon->next;
              std::cout<<"Socket closing..."<<std::endl<<std::endl;
            }
            strcat(tmpCon->readBuffer,tmpBuffer);
            const char * remain=NULL;
            // isEnd=tmpCon->httpRequest.feed(tmpBuffer,remain);
            isEnd=httpRequest->feed(tmpBuffer,remain);
            std::cout << tmpBuffer << std::endl;
            cout<<"isEnd:"<<isEnd<<endl;
            if(isEnd){
              tmpCon->headerSent=0;
              // if(!tmpCon->httpRequest.consume()){
              if(!httpRequest->consume()){
                HttpResponse* httpResponse=&tmpCon->httpResponse;
                httpResponse->setStatus("400");
                httpResponse->setDescription("BAD REQUEST");
                httpResponse->setHeader("Server", "Apache/2.4.18 (FreeBSD)");
                //httpResponse.setHeader("Content-Length","91");
                httpResponse->setHeader("Connection", "close");
                char* hr_header = httpResponse->encode();
                strcpy(tmpCon->responesHeader,hr_header);
                FD_SET(tmpCon->fdNum, &writeFdss);
                tmpCon->headerCount=strlen(tmpCon->responesHeader);
              }
              else{
                char url[1024]={'\0'};
                strcpy(url,tmpCon->httpRequest.getUrl());
                cout<<"Result of getUrl():"<<url<<endl;

                std::cout<<"Finish reading the request!"<<std::endl;

                char filename[1024]={'\0'};
                strcpy(filename,argv_dir);
                strcat(filename,url);
                std::cout<<"Open file:"<<filename<<std::endl;
                int responseFd=open(filename,O_RDONLY|O_APPEND,0644);
                HttpResponse* httpResponse=&tmpCon->httpResponse;
                if(responseFd==-1){
                  // perror("Open file:");
                  // _exit(2);
                  //404 NOT FOUND
                  httpResponse->setStatus("404");
                  httpResponse->setDescription("NOT FOUND");
                  httpResponse->setHeader("Server", "Apache/2.4.18 (FreeBSD)");
                  httpResponse->setHeader("Content-Length","0");
                  httpResponse->setHeader("Connection", "close");
                  char* hr_header = httpResponse->encode();
                  strcpy(tmpCon->responesHeader,hr_header);
                  FD_SET(tmpCon->fdNum, &writeFdss);
                  tmpCon->headerCount=strlen(tmpCon->responesHeader);
                }
                else{
                  tmpCon->responseFdNum=responseFd;
                  FD_SET(responseFd, &readFdss);
                  long contentLength=file_length(filename);
                  httpResponse->setStatus("200");
                  httpResponse->setDescription("OK");
                  httpResponse->setHeader("Server", "Apache/2.4.18 (FreeBSD)");
                  httpResponse->setHeader("Content-Length",to_string(contentLength));
                  httpResponse->setHeader("Connection", "close");

                  char* hr_header = httpResponse->encode();
                  cout<<hr_header<<endl;
                  FD_SET(tmpCon->fdNum, &writeFdss);
                  strcpy(tmpCon->responesHeader,hr_header);
                  cout<<tmpCon->responesHeader<<endl;
                  tmpCon->headerCount=strlen(tmpCon->responesHeader);
                } 
              }
            }
            // else {
            //   std::cout << "Socket " << fd << " receives: " << buf << std::endl;
            // }
          }
        }
        if(FD_ISSET(fd,&writeFds)){
          cout<<"\t"<<fd<<" is ready to write."<<endl;
          // cout<<tmpCon->responesHeader<<endl;
          // cout<<"\t"<<strlen(tmpCon->responesHeader)<<endl;
          // if(strlen(tmpCon->responesHeader)==0){
          //   continue;
          // }
          if(tmpCon->headerSent<tmpCon->headerCount){
            cout<<"Writing response header:"<<endl;
            cout<<tmpCon->responesHeader<<endl;
            int count1=send(fd, tmpCon->responesHeader+tmpCon->headerSent, tmpCon->headerCount-tmpCon->headerSent, 0) ;
            if (count1== -1){
              perror("Send");
              // return 6;
            }
            tmpCon->headerSent+=count1;
            cout<<"HeaderCount:"<<tmpCon->headerCount<<",HeaderSent:"<<tmpCon->headerSent<<endl;
            // tmpCon->responesHeader=tmpCon->responesHeader+count1;
          }
          else if(tmpCon->responseFdNum==-1){
            cout<<"File not found,exit..."<<endl;
            close(tmpCon->responseFdNum);
            close(fd);
            FD_CLR(fd, &readFdss);
            FD_CLR(fd,&writeFdss);
            FD_CLR(fd,&errFdss);
            FD_CLR(tmpCon->responseFdNum,&readFdss);
            cout<<"Deleting node"<<endl;
            // struct connection *deadCon=tmpCon;
            if(tmpCon->prev!=NULL)tmpCon->prev->next=tmpCon->next;
            if(tmpCon->next!=NULL)tmpCon->next->prev=tmpCon->prev;
            if(tmpCon==headCon)headCon=tmpCon->next;
            std::cout<<"Socket closing..."<<std::endl<<std::endl;
          }
          else if(tmpCon->writeCount){
            cout<<"Wrinting HTTP body:"<<endl;
            int count1=send(fd, tmpCon->writeBuffer+tmpCon->sent, tmpCon->writeCount, 0) ;
            if (count1== -1){
              perror("Send");
              // return 6;
            }
            // tmpCon->writeBuffer=tmpCon->writeBuffer.substr(count1);
            tmpCon->sent+=count1;
            tmpCon->writeCount-=count1;
          }
          else{
            cout<<"Try to read from file:"<<tmpCon->responseFdNum<<endl;
            if(1 || FD_ISSET(tmpCon->responseFdNum,&readFds)){
              cout<<"Reading from file:"<<endl;
              int count1=read(tmpCon->responseFdNum,tmpCon->writeBuffer,sizeof(tmpCon->writeBuffer));
              if(count1==-1){
                perror("Read from file");
                // continue;
                // return 6;
              }
              else if(count1==0){
                close(tmpCon->responseFdNum);
                close(fd);
                FD_CLR(fd, &readFdss);
                FD_CLR(fd,&writeFdss);
                FD_CLR(fd,&errFdss);
                FD_CLR(tmpCon->responseFdNum,&readFdss);
                cout<<"Deleting node"<<endl;
                // struct connection *deadCon=tmpCon;
                if(tmpCon->prev!=NULL)tmpCon->prev->next=tmpCon->next;
                if(tmpCon->next!=NULL)tmpCon->next->prev=tmpCon->prev;
                if(tmpCon==headCon)headCon=tmpCon->next;
                std::cout<<"Socket closing..."<<std::endl<<std::endl;
              }
              else{
                tmpCon->sent=0;
                tmpCon->writeCount=count1;
                int count2=send(fd, tmpCon->writeBuffer, tmpCon->writeCount, 0) ;
                if(count2== -1){
                  perror("Send");
                  // return 6;
                }
                // tmpCon->writeBuffer=tmpCon->writeBuffer.substr(count2);
                tmpCon->sent+=count2;
                tmpCon->writeCount-=count2;
              }

            }
            // else{
            //   close(tmpCon->responseFdNum);
            //   close(fd);
            //   FD_CLR(fd, &watchFds);
            //   FD_CLR(tmpCon->responseFdNum,&watchFds);
            //   // struct connection *deadCon=tmpCon;
            //   tmpCon->prev->next=tmpCon->next;
            //   tmpCon->next->prev=tmpCon->prev;
            //   std::cout<<"Socket closing..."<<std::endl<<std::endl;
            // }
          }
        }
      }
    }
  }
  return 0;
}
