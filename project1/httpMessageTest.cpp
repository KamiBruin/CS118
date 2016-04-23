#include "httpMessage.h"

int main()
{
	//client send request 
	HttpRequest httpRequest;
	httpRequest.setUrl("/index.html");
	httpRequest.setHeader("User-Agent", "Wget/1.15 (linux-gnu)");
	httpRequest.setHeader("Accept", "*/*");
	httpRequest.setHeader("Connection", "Keep-Alive");
	char* gen = httpRequest.encode();

	//client send wired data
	cout<<gen<<endl;

    //server receive request (until fed up: feed() return 1) and parse (consume)
    char *remain = NULL;
	HttpRequest httpRequest2;
	cout<<httpRequest2.feed("GET /index.html HTTP/1.0\r", remain)<<endl;
	cout<<httpRequest2.feed("\nAccept: */*\r\nConnection: Keep-Alive\r\nUser-Agent: Wget/1.15", remain)<<endl;
	cout<<httpRequest2.feed(" (linux-gnu)\r\n", remain)<<endl;
	cout<<httpRequest2.feed("\r\n", remain)<<endl;

	cout<<"consume result: "<<httpRequest2.consume()<<endl;

	cout<<httpRequest2.getUrl()<<endl;
	cout<<httpRequest2.getHeader("User-Agent")<<endl;
	cout<<httpRequest2.getHeader("Accept")<<endl;
	cout<<httpRequest2.getHeader("Connection")<<endl;

	//server response to request
	HttpResponse httpResponse;
	httpResponse.setStatus("200");
	httpResponse.setDescription("OK");
	httpResponse.setHeader("Content-Type", "text/html");
	httpResponse.setHeader("Server", "Apache/2.4.18 (FreeBSD)");
	httpResponse.setHeader("Content-Length","91");
	httpResponse.setHeader("Connection", "close");
	char* gen2 = httpResponse.encode();

	//server send wired data 
	cout<<"^^^^^^^^^^^^^^^^^^"<<endl;
	cout<<gen2;
	cout<<"^^^^^^^^^^^^^^^^^^"<<endl;
	char *remain2 = NULL;
	//client receive and parse wired data
	HttpResponse httpResponse2;
	cout<<httpResponse2.feed("HTTP/1.0 ", remain2)<<endl;
	cout<<httpResponse2.feed("200 OK\r\nConnection: clos", remain2)<<endl;
	cout<<httpResponse2.feed("e\r\nContent-Length: 91\r", remain2)<<endl;
	cout<<httpResponse2.feed("\nContent-Type: text/html\r\nServer: Apache/2.4.18 (FreeBSD)", remain2)<<endl;
	cout<<httpResponse2.feed("\r\n\r", remain2)<<endl;
	cout<<httpResponse2.feed("\nyayahehe", remain2)<<endl;
	// printf(remain2);
	cout<<remain2<<endl;
	cout<<"=============="<<endl;
	cout<<(remain2==NULL)<<endl;
	cout<<remain2<<endl;

	cout<<"consume result: "<<httpResponse2.consume()<<endl;
	cout<<httpResponse2.getStatus()<<endl;
	cout<<httpResponse2.getDesciption()<<endl;

	cout<<httpResponse2.getHeader("Content-Type")<<endl;
	cout<<httpResponse2.getHeader("Server")<<endl;
	cout<<httpResponse2.getHeader("Content-Length")<<endl;
	cout<<httpResponse2.getHeader("Connection")<<endl;
  return 1;
  // do your stuff here! or not if you don't want to.
}