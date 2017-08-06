#include <iostream>
#include <atomic>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "Servant.h"

extern std::atomic<bool> running;

Session::Session(int sockfd,unsigned id):sock(sockfd),sid(id){
	entry_time=time(NULL);
}

// the entry point for the session (and this thread)
void Session::entry(int sockfd,unsigned id){
	Session session(sockfd,id);

	// serve the client
	try{
		session.serve();
	}catch(const SessionErrorNotFound &e){
		// file not found
		session.log(e.what());
		session.send_error_not_found();
	}catch(const SessionErrorMalformed &e){
		// malformed http request
		session.log(e.what());
		session.send_error_generic(HTTP_STATUS_BAD_REQUEST);
	}catch(const SessionErrorNotSupported &e){
		// http operation not implemented
		session.log(e.what());
		session.send_error_generic(HTTP_STATUS_NOT_IMPLEMENTED);
	}catch(const SessionErrorVersion &e){
		// http version not supported
		session.log(e.what());
		session.send_error_generic(HTTP_STATUS_VERSION_NOT_SUPPORTED);
	}catch(SessionError &se){
		// generic catch-all
		session.log(se.what());
	}

	session.log("session end");
}

void Session::serve(){
	while(time(NULL)-entry_time<HTTP_KEEPALIVE){
		// check if anything is on the socket
		if(sock.peek()>0){
			// get the http request
			std::string request;
			get_http_request(request);

			// make sure it's valid
			Session::check_http_request(request);

			// get the requested resource name from the request header
			std::string target;
			Session::get_target_resource(request,target);

			// initialize resource
			throw SessionErrorNotFound(target);
		}

		// check for exit request
		if(!running.load())
			throw SessionError(EXIT_REQUEST);

		// check for error
		if(sock.error())
			return;

		// wait just a little bit
		usleep(100);
	}
}

void Session::get_http_request(std::string &req){
	bool end=false;
	const int get_size=128; // try to recv how many characters at a time
	while(!end){
		// make sure HTTP_KEEPALIVE seconds hasn't elapsed yet
		if(time(NULL)-entry_time>HTTP_KEEPALIVE)
			throw SessionErrorClosed();

		// receive a bit of the HTTP request
		char block[get_size+1];
		const int received=sock.recv_nonblock(block,get_size);

		// check for socket error
		if(sock.error())
			throw SessionErrorClosed();

		// check for exit request
		if(!running.load())
			throw SessionError(EXIT_REQUEST);

		block[received]=0;
		req+=block;

		// end of http request is denoted by CRLFCRLF
		end=req.find("\r\n\r\n")!=std::string::npos;
	}

	// reset the keepalive timeout
	entry_time=time(NULL);
}

// send a generic http response error (i.e. with no response body, just the header)
void Session::send_error_generic(int code){
	// construct response header
	std::string header;
	Session::construct_response_header(code,0,header);

	// send
	sock.send_block(header.c_str(),header.length());
}

// send the 404page.html, or a default
void Session::send_error_not_found(){
	const char *const notfound=
		"<!Doctype html>\n"
		"<html>\n"
		"<head><title>404 Not Found</title></head>\n"
		"<body>\n"
		"<h2>The requested resource does not exist</h2>\n"
		"</body>\n"
		"</html>\n"
	;
	const int notfoundlen=strlen(notfound);

	std::string header;
	Session::construct_response_header(HTTP_STATUS_NOT_FOUND,notfoundlen,header);

	sock.send_block(header.c_str(),header.length());
	sock.send_block(notfound,notfoundlen);
}

void Session::log(const std::string &line)const{
	std::cout<<sid<<" - '"<<sock.get_name()<<"' -- "<<line<<std::endl;
}

void Session::check_http_request(const std::string &request){
	const int len=request.length();
	// make sure there's enough room at least for "GET "
	if(len<4)
		throw SessionErrorMalformed();

	// servant only supports GET request
	if(!(request[0]=='G'&&request[1]=='E'&&request[2]=='T'&&request[3]==' '))
		throw SessionErrorNotSupported();

	if(len<14) // shortest possible request: GET / HTTP/1.1
		throw SessionErrorMalformed();

	// check http version
	unsigned pos=request.find("\r\n");
	if(pos==std::string::npos||pos<3)
		throw SessionErrorMalformed();
	if(request[pos-3]!='1') // 3 back from the newline is the major http version
		throw SessionErrorVersion();
}

// given its parameters, construct the appropriate response header in <header>,
void Session::construct_response_header(int code,int content_length,std::string &header){
	char length_string[25];
	sprintf(length_string,"%u",content_length);

	std::string status;
	get_status_code(code,status);

	header=std::string("HTTP/1.1 ")+status+"\r\n"+
	"Content-Length: "+length_string+"\r\n"+
	"Server: "+DEFAULT_NAME+"\r\n\r\n";
}

// fill in the status code (e.g. "200 OK")
void Session::get_status_code(int code,std::string &status){
	switch(code){
	case HTTP_STATUS_OK:
		status="200 OK";
		break;
	case HTTP_STATUS_BAD_REQUEST:
		status="400 Bad Request";
		break;
	case HTTP_STATUS_NOT_FOUND:
		status="404 Not Found";
		break;
	case HTTP_STATUS_NOT_IMPLEMENTED:
		status="501 Not Implemented";
		break;
	case HTTP_STATUS_VERSION_NOT_SUPPORTED:
		status="505 HTTP Version Not Supported";
		break;
	default:
		status="500 Internal Server Error";
		break;
	}
}

// pick out the requested resource from <header> and put it in <target>
void Session::get_target_resource(const std::string &header,std::string &target){
	// find the first space in the header
	const unsigned beginning=header.find(" ")+1;
	if(beginning==std::string::npos)
		return;

	// consistency check
	if(beginning>=header.size()-1)
		return;

	// find the second space in the header
	const unsigned end=header.find(" ",beginning);
	if(end==std::string::npos)
		return;

	// get the bit in the middle
	target=header.substr(beginning,end-beginning);
}
