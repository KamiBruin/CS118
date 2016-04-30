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
#include <sys/select.h>
#include "httpMessage.h"

#define TRANS_UNIT 512

using namespace std;
string getipaddress(string hostname,string portnum)
{
    struct addrinfo hints;
    struct addrinfo* res;
    cerr<<"this is hostname in the sub:"<<hostname<<endl;
    cerr<<"this is hostname in the sub:"<<hostname.c_str()<<endl;
    cerr<<"this is portnum in the sub:"<<portnum<<endl;
    cerr<<"this is portnum in the sub:"<<portnum.c_str()<<endl;
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
    
    cerr << "IP addresses for " <<hostname<< ": " << std::endl;
    string ip= "";
    for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
        // convert address to IPv4 address
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
        
        // convert the IP to a string and print it:
        char ipstr[INET_ADDRSTRLEN] = {'\0'};
        inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
        cerr << "  " << ipstr << std::endl;
        ip=ipstr;
        // std::cout << "  " << ipstr << ":" << ntohs(ipv4->sin_port) << std::endl;
        
    }
    
    freeaddrinfo(res); // free the linked list
    
    return ip;
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "usage: showip hostname" << std::endl;
        return 1;
    }
    int filenum = 1; 

    vector <string> all_filerequest;
    string hostname;
    string portnum;
    string requestname;
    for (filenum = 1; filenum < argc ; filenum++ ) {
        string url = argv[filenum];
        int start = int(url.find("//"));
        int hostend = int(url.find(":",start));
        int portend = int(url.find("/",start + 2));
        int len = int(url.size());
       
        try {
            hostname= url.substr(start + 2, hostend - start - 2);
            portnum= url.substr(hostend + 1, portend - hostend - 1);
            requestname= url.substr(portend, len - portend);
        }
        catch(exception e)
        {
            
        }

        if (hostend == -1) {
            hostend = portend;
            portnum = "80";
            hostname = url.substr(start + 2, hostend - start - 2);
        }
        if (portend == -1 || portend == len - 1) {requestname = "/index.html";}  
        if (start == -1) {
            if (hostend == -1) hostname = url.substr(0,len);
            else  hostname = url.substr(0,hostend+ 1);
        }  
        // cout<<"this is hostname "<<hostname<<endl;
        // cout<<"this is portnum "<<portnum<<endl;
        // cout<<"this is request "<<requestname<<endl;
        // cout<<endl;
        all_filerequest.push_back(requestname);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(portnum.c_str()));     // short, network byte order
    string ipaddr = getipaddress(hostname,portnum);
    //cout<<"this is ip"<<ipaddr<<endl;
    serverAddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
    cerr<<"finish1"<<endl;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //connect to the server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        return 2;
    }
    cerr<<"finish2"<<endl;
    //to get the client address
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
        perror("getsockname");
        return 3;
    }
    cerr<<"finish3"<<endl;
    //to return the normal client address
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    //cout<<"finish4"<<endl;
    //cout<<ipstr<<endl;
    cerr << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;
    
    char buf[TRANS_UNIT] = {'\0'}; // buffer to feed httpMessage
    const char *remain = NULL; //start pointer of leftover real data from feed
    int numofremain = 0;//now it means length of header.******

    fd_set readFds;
    fd_set writeFds;
    fd_set errFds;
    fd_set watchFds;
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    FD_ZERO(&errFds);
    FD_ZERO(&watchFds);   

    int maxSockfd = sockfd;
    FD_SET(sockfd, &watchFds);
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    int file_to_recv = 0; //the file that we are receiving
    int request_to_send = all_filerequest.size();
    bool flag = false; //false: feed httpmessage; true:receive real data

    int toreceivelen = 0;
    int fd;
    char *content = new char[TRANS_UNIT]; //buffer to put real data
    int content_left_start = 0; //exceeded real data length
    int content_left_len = 0;

    HttpResponse httpResponse2;
    while (file_to_recv < (int)all_filerequest.size()) {
        int nReadyFds = 0;
        readFds = watchFds;
        writeFds = watchFds;
        errFds = watchFds;
        //wait for both write status and read status
        if ((nReadyFds = select(maxSockfd + 1, &readFds, &writeFds, &errFds, &tv)) == -1) {
          perror("select");
          return 4;
        }

        if (nReadyFds == 0) {
          cerr << "no data of header respnse is received for 3 seconds!" << std::endl;
        }
        else {
            for(int fdnum = 0; fdnum <= maxSockfd; fdnum++) {
            // get one socket for reading
                if (FD_ISSET(fdnum, &readFds)) {
                    if (fdnum == sockfd) {
                        if (flag == false) {
                            int count;
                            string requestname = all_filerequest[file_to_recv];
                            if(content_left_start > 0) {
                                numofremain = httpResponse2.feed(content + content_left_start, remain);
                                count = content_left_len;
                                cerr<<"Now We have some leftovers from last file: "<<endl;
                                cerr<<content + content_left_start<<endl;
                                content_left_start = 0;
                            } else {
                                memset(buf, '\0', strlen(buf));
                                count=recv(sockfd, buf, TRANS_UNIT, 0);
                                if (count== -1) {
                                    perror("recv");
                                    return 5;
                                }
                                cerr << buf << std::endl;
                                cerr <<"*****COUNT***** || "<<count<<endl;
                                numofremain = httpResponse2.feed(buf,remain);
                            }
                            //cout<<"this is hunger nor not:"<<st;
                            cerr<<"numofremain now: "<<numofremain<<endl;
                            
                            if (numofremain != 0) {
                                flag = true;
                                httpResponse2.clear();
                                httpResponse2.consume();
                                string status = httpResponse2.getStatus();
                                cerr<<"status"<<status<<endl;
                                if (status[0] != '2') {
                                    cerr<<"failed message"<<endl;
                                    return 0;
                                }
                                cerr<<"cong! retrevied message"<<endl;
                                cerr<<"this is statues:"<<httpResponse2.getStatus()<<endl;
                                int contentlen = atoi(httpResponse2.getHeader("Content-Length").c_str());
                                cerr<<"the whole content length should be:"<<contentlen<<endl;
                                toreceivelen = contentlen;
                                cerr<<"this is the first time of remain length:"<<numofremain<<endl;
                                int filenum = requestname.rfind('/');
                                string filename = requestname.substr(filenum + 1,requestname.size() - filenum - 1);
                                //cout<<"this is filename"<<filename<<endl;
                                fd = open(filename.c_str(),O_WRONLY|O_CREAT,0666);
                                if(fd==-1) {
                                    perror("wrong Open file");
                                    _exit(2);
                                }   
                                
                                if (numofremain != -1) {         
                                    toreceivelen = toreceivelen - count + numofremain;          
                                    int tmp = write(fd,remain,count - numofremain);
                                    cerr<<"TMP:"<<tmp<<endl;

                                    if(tmp==-1) {
                                        perror("Write");
                                        _exit(3);
                                    }

                                } 
                            }
                            // if(count==0) break;
                        } else { // flag == true, receive real data package
                                // get one socket for reading
                            memset(content, '\0', TRANS_UNIT);
                            int count3 = recv(sockfd, content, TRANS_UNIT, 0);
                            if (count3 == -1) {
                                perror("recvwrong");
                                return 5;
                            }
                            if (count3== 0) {
                                perror("no receive");
                                return 5;
                            }
                            toreceivelen -= count3;
                            //cout<<"this is current receivedlen:"<<contentlen - toreceivelen<<endl;
                            
                            cerr<<"this is current receivedlen:"<<toreceivelen<<endl;
                            //cout<<"echo:"<<content<<endl;
                            
                            if (toreceivelen >= 0) {
                                int count1 = write(fd,content,count3);
                                if(count1 == -1) {
                                    perror("Write");
                                    _exit(3);
                                }
                                if(toreceivelen == 0) {
                                    flag = false;
                                    content_left_start = 0;
                                    file_to_recv++;
                                    close(fd);
                                }
                            }
                            else { // some received data belongs to next file
                                int count1 = write(fd,content,toreceivelen + count3);
                                if(count1 == -1) {
                                    perror("Write");
                                    _exit(3);
                                }
                                content_left_start = toreceivelen + count3; //must larger than 0
                                content_left_len = -toreceivelen;
                                flag = false;
                                file_to_recv++;
                                close(fd);
                            }
                        }
                    }
                }
            // get one socket for writing
                if (FD_ISSET(fdnum, &writeFds)) {
                    if (fdnum == sockfd && request_to_send > 0) {
                        string requestname = all_filerequest[all_filerequest.size() - request_to_send];
                        HttpRequest httpRequest;
                        httpRequest.setUrl(requestname);
                        httpRequest.setHeader("User-Agent", "Wget/1.15 (linux-gnu)");
                        
                        httpRequest.setHeader("Connection", "Keep-Alive");
                        const char* test_request = httpRequest.encode();
                        cerr<<"this is test_request:\n"<<test_request;
                        //strcpy(test_request,"GET /index.html HTTP/1.0\nHost:www.baidu.com\n\n\n");
                        std::stringstream ss;

                        if (send(sockfd, test_request, strlen(test_request), 0) == -1) {
                            perror("send");
                            return 4;
                        }
                        request_to_send--;
                    }
                }
                if (FD_ISSET(fdnum, &errFds)) {
                    cerr <<"error has happened"<<endl;
                    break;
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
    // httpResponse2.consume();
    // string status = httpResponse2.getStatus();
    // cout<<"status"<<status<<endl;
    // if (status[0] != '2') {
    //     cout<<"failed message"<<endl;
    //     return 0;
    // }
    // else {
    //     cout<<"cong! retrevied message"<<endl;
    //     cout<<"this is statues:"<<httpResponse2.getStatus()<<endl;
    //     int contentlen = atoi(httpResponse2.getHeader("Content-Length").c_str());
    //     cout<<"the whole content length should be:"<<contentlen<<endl;
    //     char content[1024] = {0};
    //     int toreceivelen = contentlen;
    //     cout<<"this is the first time of remain length:"<<numofremain<<endl;
    //     int filenum = requestname.rfind('/');
    //     string filename = requestname.substr(filenum + 1,requestname.size() - filenum - 1);
    //     //cout<<"this is filename"<<filename<<endl;
    //     int fd = open(filename.c_str(),O_WRONLY|O_CREAT,0666);
    //     if(fd==-1) {
    //         perror("wrong Open file");
    //         _exit(2);
    //     }   
        
    //     int count2 = 0;
    //     while (count2 != count-numofremain) {
    //         int tmp = write(fd,remain,count-numofremain);

    //         if(tmp==-1) {
    //             perror("Write");
    //             _exit(3);
    //         }
    //         count2 += tmp;
    //         toreceivelen -= tmp;
    //         cout<<"##################"<<endl;
    //         cout<<count2<<endl;
    //         cout<<numofremain<<endl;
    //         cout<<"##################"<<endl;
    //     }

    //     while (1) {
    //         int nReadyFds = 0;
    //         readFds = watchFds;
    //         errFds = watchFds;
    //         if ((nReadyFds = select(maxSockfd + 1, &readFds, NULL, &errFds, &tv)) == -1) {
    //           perror("select");
    //           return 4;
    //         }

    //         if (nReadyFds == 0) {
    //             std::cout << "no data is received for 3 seconds!" << std::endl;
    //         }
    //         else {
    //             for(int fdnum = 0; fdnum <= maxSockfd; fdnum++) {
    //                 // get one socket for reading
    //                   if (FD_ISSET(fdnum, &readFds)) {
    //                         memset(content, '\0', 1024);
    //                         int count3 = recv(sockfd, content, sizeof(content), 0);
    //                         if (count3 == -1) {
    //                             perror("recvwrong");
    //                             return 5;
    //                         }
    //                         if (count3== 0) {
    //                             perror("no receive");
    //                             return 5;
    //                         }
    //                         toreceivelen -= count3;
    //                         //cout<<"this is current receivedlen:"<<contentlen - toreceivelen<<endl;
                            
    //                         cout<<"this is current receivedlen:"<<toreceivelen<<endl;
    //                         //cout<<"echo:"<<content<<endl;
                            
    //                         int count1 = write(fd,content,count3);
    //                         if(count1 == -1) {
    //                             perror("Write");
    //                             _exit(3);
    //                         }
    //                     }
                        
    //                   if (FD_ISSET(fdnum, &errFds)) {
    //                         cout <<"error has happened"<<endl;
    //                         break;
    //                     }
    //             }
    //             close(fd);    
    //         }
    //     }
  
    // }
    // close(sockfd);
    // return 0;
// }

