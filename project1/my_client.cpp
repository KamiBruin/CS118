#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "httpMessage.h"

using namespace std;
string getipaddress(string hostname,string portnum)
{
    struct addrinfo hints;
    struct addrinfo* res;
    cout<<"this is hostname in the sub:"<<hostname<<endl;
    cout<<"this is hostname in the sub:"<<hostname.c_str()<<endl;
    cout<<"this is portnum in the sub:"<<portnum<<endl;
    cout<<"this is portnum in the sub:"<<portnum.c_str()<<endl;
    // prepare hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    // get address
    int status = 0;
    if ((status = getaddrinfo(hostname.c_str(), "80", &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return "err";
    }
    
    std::cout << "IP addresses for " <<hostname<< ": " << std::endl;
    string ip= "";
    for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
        // convert address to IPv4 address
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
        
        // convert the IP to a string and print it:
        char ipstr[INET_ADDRSTRLEN] = {'\0'};
        inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
        std::cout << "  " << ipstr << std::endl;
        ip=ipstr;
        // std::cout << "  " << ipstr << ":" << ntohs(ipv4->sin_port) << std::endl;
        
    }
    
    freeaddrinfo(res); // free the linked list
    
    return ip;
}


int main(int argc, char *argv[])
{
    // create a socket using TCP IP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // struct sockaddr_in addr;
    // addr.sin_family = AF_INET;
    // addr.sin_port = htons(40001);     // short, network byte order
    // addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
    // if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    //   perror("bind");
    //   return 1;
    // }
    if (argc != 2) {
        std::cerr << "usage: showip hostname" << std::endl;
        return 1;
    }
    string url = argv[1];
    int start = int(url.find("//"));
    int hostend = int(url.find(":",start));
    int portend = int(url.find("/",start + 2));
    int len = int(url.size());
    string hostname;
    string portnum;
    string requestname;
    try{
        hostname= url.substr(start + 2, hostend - start - 2);
        portnum= url.substr(hostend + 1, portend - hostend - 1);
        requestname= url.substr(portend, len - portend);
    }
    catch(exception e)
    {
        
    }

    // cout<<"this is start "<<start<<endl;
    // cout<<"this is hostend "<<hostend<<endl;
    // cout<<"this is portend "<<portend<<endl;
    // cout<<"hostname:"<<hostname<<endl;
    // cout<<"portnum:"<<portnum<<endl;
    // cout<<"requestname:"<<requestname<<endl;

    if (hostend == -1){
        hostend = portend;
        portnum = "80";
        hostname = url.substr(start + 2, hostend - start - 2);
    }
    if (portend == -1){requestname = "/index.html";}  
    if (start == -1){
        if (hostend == -1) hostname = url.substr(0,len);
        else  hostname = url.substr(0,hostend+ 1);
    }  
    cout<<"this is hostname "<<hostname<<endl;
    cout<<"this is portnum "<<portnum<<endl;
    cout<<"this is request "<<requestname<<endl;
    cout<<endl;
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(portnum.c_str()));     // short, network byte order
    string ipaddr = getipaddress(hostname,portnum);
    //cout<<"this is ip"<<ipaddr<<endl;
    serverAddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
    cout<<"finish1"<<endl;
    //connect to the server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        return 2;
    }
    cout<<"finish2"<<endl;
    //to get the client address
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
        perror("getsockname");
        return 3;
    }
    cout<<"finish3"<<endl;
    //to return the normal client address
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    //cout<<"finish4"<<endl;
    //cout<<ipstr<<endl;
    std::cout << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;
    
    
    // // send/receive data to/from connection
    
    
    std::string input;
    HttpRequest httpRequest;
    httpRequest.setUrl(requestname);
    httpRequest.setHeader("User-Agent", "Wget/1.15 (linux-gnu)");
    
    httpRequest.setHeader("Connection", "Keep-Alive");
    const char* test_request = httpRequest.encode();
    cout<<"this is test_request:"<<test_request;
    //strcpy(test_request,"GET /index.html HTTP/1.0\nHost:www.baidu.com\n\n\n");
    std::stringstream ss;

    if (send(sockfd, test_request, strlen(test_request), 0) == -1) {
        perror("send");
        return 4;
    }

    char buf[1024] = {'\0'};
    char *remain = NULL;
    HttpResponse httpResponse2;
    int numofremain = 0;
    while (1) {
        memset(buf, '\0', strlen(buf));

        int count=recv(sockfd, buf, 512, 0);

        if (count== -1) {
            perror("recv");
            return 5;
        }
        //cout << "echo: ";
        cout << buf << std::endl;
        numofremain = httpResponse2.feed(buf,remain);
        //cout<<"this is hunger nor not:"<<st;
        if (numofremain != 0) break;
        if(count==0)break;
    }
    
    httpResponse2.consume();
    string status = httpResponse2.getStatus();
    cout<<"status"<<status<<endl;
    if (status[0] != '2' && status[0] != '3'){
        cout<<"failed message"<<endl;
        return 0;
    }
    else {
        cout<<"cong! retrevied message"<<endl;
        cout<<"this is statues:"<<httpResponse2.getStatus()<<endl;
        // cout<<"this is description:"<<httpResponse2.getDesciption()<<endl;

        // cout<<"this is header type:"<<httpResponse2.getHeader("Content-Type")<<endl;
        // cout<<"this is header server:"<<httpResponse2.getHeader("Server")<<endl;
        // cout<<"this is content length:"<<httpResponse2.getHeader("Content-Length")<<endl;
        // cout<<"this is connection:"<<httpResponse2.getHeader("Connection")<<endl;
        
        cout<<endl;
        int contentlen = atoi(httpResponse2.getHeader("Content-Length").c_str());
        cout<<"the whole content length should be:"<<contentlen<<endl;
        char content[1024] = {0};
        int toreceivelen = contentlen;
        cout<<"this is the first time of remain length:"<<numofremain<<endl;
        int filenum = requestname.rfind('/');
        string filename = requestname.substr(filenum + 1,requestname.size() - filenum - 1);
        //cout<<"this is filename"<<filename<<endl;
        int fd = open(filename.c_str(),O_WRONLY|O_CREAT,0666);
        if(fd==-1){
        perror("wrong Open file");
        _exit(2);
    }   
        if (numofremain != -1)
            {
                // cout<<"*************************"<<endl;
                // cout<<"*************************"<<endl;
                // cout<<remain<<endl;
                // cout<<"*************************"<<endl;
                // cout<<"*************************"<<endl;
                int count2=write(fd,remain,numofremain);

                cout<<"##################"<<endl;
                cout<<count2<<endl;
                cout<<numofremain<<endl;
                cout<<"##################"<<endl;

                toreceivelen -= count2;
                if(count2==-1){
                perror("Write");
                _exit(3);
                }
            }
        while (toreceivelen > 0){
            memset(content, '\0', 1024);
            int count3=recv(sockfd, content, sizeof(content), 0);
            // cout<<"99999999999999999"<<endl;
            // cout<<count3<<endl;
            // cout<<content<<endl;
            // cout<<content[count3-1]<<endl;

            if (count3== -1) {
                perror("recvwrong");
                return 5;
            }
            toreceivelen -= count3;
            //cout<<"this is current receivedlen:"<<contentlen - toreceivelen<<endl;
            
            cout<<"this is current receivedlen:"<<toreceivelen<<endl;
            //cout<<"echo:"<<content<<endl;
            if (toreceivelen < 0) 
            {
                cout<<"************final bulk************"<<endl;
                cout<<content<<endl;
                cout<<"************final bulk************"<<endl;
                cout<<"final length: "<<toreceivelen + 1024<<endl;
                cout<<"******************************"<<endl;
                cout<<content[113]<<content[114]<<content[115]<<content[116]<<endl;
                cout<<"==============================="<<endl;

                int count1=write(fd,content,count3);
                if(count1==-1){
                perror("Write");
                _exit(3);}

            }
            else{
                int count1=write(fd,content,count3);
                if(count1==-1){
                perror("Write");
                _exit(3);}
            }
            
        }
    close(fd);    
}

    
    close(sockfd);
    return 0;
}

