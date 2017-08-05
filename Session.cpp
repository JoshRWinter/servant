#include <iostream>
#include <atomic>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "Servant.h"

extern std::atomic<bool> running;

Session::Session(int sockfd,unsigned id):sock(sockfd),sid(id),entry_time(time(NULL)){
}

// the entry point for the session (and this thread)
void Session::entry(int sockfd,unsigned id){
	Session session(sockfd,id);

	// serve the client
	try{
		session.serve();
	}catch(SessionError &se){
		session.log(se.what());
	}
}

void Session::serve(){
	while(time(NULL)-entry_time<HTTP_KEEPALIVE){
		std::string request;
		// check if anything is on the socket
		if(sock.peek()>0){
			// get the http request
			get_http_request(request);
		}

		// make sure valid request
		if(request.length()>0){
			try{
				check_http_request(request);
			}catch(const SessionErrorMalformed &e){
				log(e.what());
				send_error_malformed();
			}catch(const SessionErrorNotSupported &e){
				log(e.what());
				send_error_not_supported();
			}catch(const SessionErrorVersion &e){
				log(e.what());
				send_error_version();
			}
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
		char block[get_size+1];
		const int received=sock.recv_nonblock(block,get_size);

		// check for socket error
		if(sock.error()){
			req=""; // reset
			return;
		}

		// check for exit request
		if(!running.load())
			throw SessionError(EXIT_REQUEST);

		block[received]=0;
		req+=block;

		// end of http request is denoted by CRLFCRLF
		end=req.find("\r\n\r\n")!=std::string::npos;
	}
}

void Session::check_http_request(const std::string &request)const{
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

void Session::send_error_malformed(){
	const char *const malformed=
	"HTTP/1.1 400 Bad Request\r\n"
	"Content-Length: 0\r\n\r\n";

	sock.send_block(malformed,strlen(malformed));
}

void Session::send_error_not_supported(){
	const char *const not_supported=
	"HTTP/1.1 501 Not Implemented\r\n"
	"Content-Length: 0\r\n\r\n";

	sock.send_block(not_supported,strlen(not_supported));
}

void Session::send_error_version(){
	const char *const version=
	"HTTP/1.1 505 HTTP Version Not Supported\r\n"
	"Content-Length: 0\r\n\r\n";

	sock.send_block(version,strlen(version));
}

void Session::log(const std::string &line)const{
	std::cout<<sid<<" - '"<<sock.get_name()<<"' -- "<<line<<std::endl;
}
