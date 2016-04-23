#ifndef HTTP_MESSAGE
#define HTTP_MESSAGE
#include <string>
#include <map>
#include <iostream>
#include <map>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <string.h>

using namespace std;

enum HttpVersion {
	HTTP_1_0 = 10, 
	HTTP_1_1 = 11,
	HTTP_OTHER = 1
};

enum HttpMethod {
	GET = 0,
	METHOD_OTHER = 1
};

enum ConnectionMethod {
	KEEPALIVE = 8,
	TEMP = 9,
	CM_OTHER = 10
};

class HttpMessage {
private:
	// int f_pointer; //incrementally scan fed buffer message
protected:
	HttpVersion h_version;
	map<string, string> headers;
	std::stringstream ss;
	string h_message;
	bool isReady; //used only to indicate the request line and all headers are ready for decoding
public:
	HttpMessage() {
		h_version = HTTP_1_0;
		isReady = false;
		// f_pointer = 0;
	}

	int feed(const char *buf, const char *&remain);
	int feed(const char *buf);
//getter
	HttpVersion getVersion();
	string getHeader(string key);
//setter
	void setHeader(string key, string value);
	int decodeFirstLine();
	const char* encode();
};

class HttpRequest: public HttpMessage {
private:
	HttpMethod h_method;
	string url;
public:
	HttpRequest() {
		h_method = GET;
		url = "";
	}
	bool consume();
	int decodeFirstLine();
	char* encode();
//getter
	HttpMethod getMethod();
	const char* getUrl();
//setter
	void setUrl(string input);
};

class HttpResponse: public HttpMessage {
private:
	string m_status;
	string m_statusDescription;
public:
	bool consume();
	int decodeFirstLine();
	char* encode();
//getter
	string getStatus();
	string getDesciption();
//setter
	void setStatus(string status);
	void setDescription(string description);
};

#endif