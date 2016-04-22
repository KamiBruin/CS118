#include "httpMessage.h"
//HttpMessage
HttpVersion HttpMessage::getVersion() {
	if(!isReady) {
		perror("message is not ready !!\n");
		return HTTP_OTHER;
	}
	return h_version;
}

void HttpMessage::setHeader(string key, string value) {
	if (headers.find(key) != headers.end()) {
		perror("header duplicate!\n");
		return;
	} else {
		headers[key] = value;
	}
}

string HttpMessage::getHeader(string key) {
	if(!isReady) {
		perror("message is not ready !!\n");
		return "";
	}
	if (headers.find(key) == headers.end()) {
		perror("header not exist!\n");
		return "";
	}
	return headers[key];
}

//todo: incrementally feed. Don't scan from the beginning every time
int HttpMessage::feed(const char *buf) {
	// cout<<"****************"<<endl;
	// cout<<"&&&&&&&&&&&&&&&&&&"<<endl;
	// cout<<buf<<endl;
	ss << buf;
	string message = ss.str();

	string mark;
	int len = message.length();
	int i = 0;
	for (; i < len - 3; i++) {
		// cout<<i<<endl;
		mark = message.substr(i, 4);
		// cout<<mark<<endl;
		if (mark == "\r\n\r\n") {
			h_message = message.substr(0, i+2);
			// cout<<h_message<<endl;
			isReady = true;
			return 1;
		} 
	}
	return 0;
}

int HttpMessage::feed(const char *buf, char *&remain) {
	// cout<<"****************"<<endl;
	// cout<<"&&&&&&&&&&&&&&&&&&"<<endl;
	// cout<<buf<<endl;
	ss << buf;
	string message = ss.str();
	// cout<<"*&*&*&*&*&*&*&*&"<<endl;
	// cout<<message<<endl;
	// cout<<"())()()()()()()()"<<endl;
	// cout<<buf<<endl;
	string mark;
	int len = message.length();
	int i = 0;
	for (; i < len - 3; i++) {
		// cout<<i<<endl;
		mark = message.substr(i, 4);
		// cout<<mark<<endl;
		if (mark == "\r\n\r\n") {
			h_message = message.substr(0, i+2);
			// cout<<h_message<<endl;
			isReady = true;
			string remain_str = message.substr(i + 4, len - i - 4);
			//must allocate space in heap to transfer remain_str out
			remain = new char[remain_str.length()+1];
			// cout<<"remain length"<<(int)remain_str.length()<<endl;
			// cout<<remain_str[remain_str.length()-1]<<endl;
			for(int k = 0; k < (int)remain_str.length(); k++) {
				remain[k] = remain_str[k];
			}
			remain[remain_str.length()] = '\0';
			// cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<endl;
			// cout<<remain<<endl;
			// cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<endl;
			// cout<<remain<<endl;
			if(remain_str.length() == 0) return -1;
			return remain_str.length();
		} 
	}
	return 0;
}


//HttpRequest
void HttpRequest::consume() {
	if(!isReady) {
		perror("message is not ready !!\n");
		return;	
	}
	int i = decodeFirstLine();
	// cout<<h_message<<endl;
	// cout<<i<<endl;
	int j = i;
	for (; i < (int)h_message.length();) {
		if (h_message.substr(i, 2) == "\r\n") {
			if(i - j == 0) return;
			string subheader = h_message.substr(j, i-j);
			for (int k = 0; k < (int)subheader.length(); k++) {
				if (subheader.substr(k, 2) == ": ") {
					string key = subheader.substr(0, k);
					string value = subheader.substr(k+2, subheader.length() - k - 2);
					headers[key] = value;
				}
			}
			i += 2;
			j = i;
		} else {
			i++;
		}
	}
	return;
}

int HttpRequest::decodeFirstLine() {
	if (!isReady) {
		perror("message is not ready !!\n");
		return -1;
	}
	int i = 0, j = 0;
	for (; i < (int)h_message.length(); i++) {
		if(h_message.substr(i, 2) == "\r\n") break;
	}
	int ret = i + 2;
	string firstLine = h_message.substr(0, i);
	j = 0;
	for (i = 0; i < (int)firstLine.length(); i++) {
		if(firstLine[i] == ' ' && j == 0) {
			string method_str = firstLine.substr(0, i);
			if(method_str == "GET") {
				h_method = GET;
			} else {
				h_method = METHOD_OTHER;
			}
			j = i + 1;
		} else if(firstLine[i] == ' ' && j != 0) {
			string url_str = firstLine.substr(j, i - j);
			url = url_str;
			j = i + 1;
		}
		
	}
	string http_version_str = firstLine.substr(j, i - j);
	if (http_version_str == "HTTP/1.0") {
		h_version = HTTP_1_0;
	} else if (http_version_str == "HTTP/1.1" ) {
		h_version = HTTP_1_1;
	}
	return ret;
}

char* HttpRequest::encode() {
	std::stringstream des;
	if (h_method == GET) {
		des << "GET ";
	}
	des << url << ' ';
	if (h_version == HTTP_1_0) {
		des << "HTTP/1.0";
	} else if (h_version == HTTP_1_1) {
		des << "HTTP/1.1";
	}
	des << "\r\n";
	map<string,string>::iterator it=headers.begin();
	for(;it!=headers.end();++it) {
		des << it->first << ": ";
		des << it->second << "\r\n";
	}
	des << "\r\n";

	string res = des.str();

	char* ret = new char[res.length()+1];
	strcpy(ret, res.c_str());

	return ret;
}

HttpMethod HttpRequest::getMethod() {
	if (!isReady) {
		perror("message is not ready !!\n");
		return METHOD_OTHER;
	}
	return h_method;
}

const char* HttpRequest::getUrl() {
	if (!isReady) {
		perror("message is not ready !!\n");
		return NULL;
	}
	return url.c_str();
}

void HttpRequest::setUrl(string input) {
	url = input;
	return;
}

//Http Response
void HttpResponse::consume() {
	if(!isReady) {
		perror("message is not ready !!\n");
		return;	
	}
	int i = decodeFirstLine();
	int j = i;
	for (; i < (int)h_message.length();) {
		if (h_message.substr(i, 2) == "\r\n") {
			if(i - j == 0) return;
			string subheader = h_message.substr(j, i-j);
			for (int k = 0; k < (int)subheader.length(); k++) {
				if (subheader.substr(k, 2) == ": ") {
					string key = subheader.substr(0, k);
					string value = subheader.substr(k+2, subheader.length() - k - 2);
					headers[key] = value;
				}
			}
			i += 2;
			j = i;
		} else {
			i++;
		}
	}
	return;
}

string HttpResponse::getStatus() {
	if (!isReady) {
		perror("message is not ready !!\n");
		return "";
	}
	return m_status;
}

void HttpResponse::setStatus(string status) {
	m_status = status;
}

string HttpResponse::getDesciption() {
	if (!isReady) {
		perror("message is not ready !!\n");
		return "";
	}
	return m_statusDescription;
}

void HttpResponse::setDescription(string description) {
	m_statusDescription = description;
}

int HttpResponse::decodeFirstLine() {
	if (!isReady) {
		perror("message is not ready !!\n");
		return -1;
	}
	int i = 0, j = 0;
	for (; i < (int)h_message.length(); i++) {
		if(h_message.substr(i, 2) == "\r\n") break;
	}
	int ret = i + 2;
	string firstLine = h_message.substr(0, i);
	j = 0;
	for (i = 0; i < (int)firstLine.length(); i++) {
		if(firstLine[i] == ' ' && j == 0) {
			string http_version_str = firstLine.substr(j, i - j);
			if (http_version_str == "HTTP/1.0") {
				h_version = HTTP_1_0;
			} else if (http_version_str == "HTTP_1_1" ) {
				h_version = HTTP_1_1;
			}
			j = i + 1;
		} else if(firstLine[i] == ' ' && j != 0) {
			m_status = firstLine.substr(j, i - j);
			j = i + 1;
		}
		
	}
	string status_description_str = firstLine.substr(j, i - j);
	m_statusDescription = status_description_str;
	
	return ret;
}

char* HttpResponse::encode() {
	std::stringstream des;
	if (h_version == HTTP_1_0) {
		des << "HTTP/1.0 ";
	} else if (h_version == HTTP_1_1) {
		des << "HTTP/1.1 ";
	}
	des << m_status << ' ';
	des << m_statusDescription;
	des << "\r\n";
	map<string,string>::iterator it=headers.begin();
	for(;it!=headers.end();++it) {
		des << it->first << ": ";
		des << it->second << "\r\n";
	}
	des << "\r\n";
	string res = des.str();

	char* ret = new char[res.length()+1];
	strcpy(ret, res.c_str());

	return ret;
}

