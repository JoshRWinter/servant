#include <iostream>
#include <atomic>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <thread>
#include <chrono>

#include "Servant.h"

extern std::atomic<bool> running;
static std::mutex stdout_lock; // locks the std::cout in Session::log

Session::Session(int sockfd,unsigned id):sock(sockfd),sid(id){
	entry_time=time(NULL);
}

// the entry point for the session (and this thread)
void Session::entry(Servant *parent,int sockfd,unsigned id){
	Session session(sockfd,id);
	session.log(std::string("session begin ")+session.sock.get_name());

	// serve the client
	try{
		session.serve();
	}catch(const SessionErrorNotFound &e){
		// file not found
		session.log(e.what());
		session.send_error_not_found();
	}catch(const SessionErrorForbidden &e){
		// forbidden file, treat as 404
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
	}catch(const SessionErrorInternal &e){
		// internal server error
		session.log(e.what());
		session.send_error_generic(HTTP_STATUS_INTERNAL_ERROR);
	}catch(SessionError &se){
		// generic catch-all
		session.log(se.what());
	}

	// let parent know it's done
	parent->complete();
	session.log("session end");
}

// loop and take http requests till the keepalive timer runs out
// each request resets the timer
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
			Resource rc(target);
			log(std::string("request resource \""+target+"\" (")+rc.type()+")");

			// send the file
			send_file(rc);
		}

		// check for exit request
		if(!running.load())
			throw SessionErrorExit();

		// check for error
		if(sock.error())
			return;

		// wait just a little bit
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
		const int received=recv(block,get_size);

		// check for socket error
		if(sock.error())
			throw SessionErrorClosed();

		// check for exit request
		if(!running.load())
			throw SessionErrorExit();

		block[received]=0;
		req+=block;

		// end of http request is denoted by CRLFCRLF
		end=req.find("\r\n\r\n")!=std::string::npos;
	}

	// reset the keepalive timeout
	entry_time=time(NULL);
}

// send a chunk of data
void Session::send(const char *buf,unsigned size){
	int sent=0;
	while(sent!=size){
		sent+=sock.send_nonblock(buf+sent,size-sent);

		if(sock.error())
			throw SessionErrorClosed();
		if(!running.load())
			throw SessionErrorExit();
	}
}

int Session::recv(char *buf,unsigned size){
	return sock.recv_nonblock(buf,size);
}

void Session::send_file(Resource &rc){
	const long long size=rc.size();

	// construct and send the header
	std::string header;
	Session::construct_response_header(HTTP_STATUS_OK,size,rc.type(),header);
	send(header.c_str(),header.length());

	// send the body
	int read=0; // bytes read from rc
	const int block_size=4096;
	while(read!=size){
		// read a block
		char block[block_size];
		const int got=rc.get(block,block_size);
		read+=got;

		// send the block
		send(block,got);

		if(!running.load())
			throw SessionErrorExit();
	}

	// convert bytes to string
	char bytes_string[25];
	sprintf(bytes_string,"%lld",size);

	log(std::string("sent ")+rc.name()+" ("+bytes_string+")");
}

// send a generic http response error (i.e. with no response body, just the header)
void Session::send_error_generic(int code){
	// get status code
	std::string status;
	Session::get_status_code(code,status);

	// construct body
	const std::string body=std::string("")+
		"<!Doctype html>\n"
		"<html>\n"
		"<head><title>"+status+"</title></head>\n"
		"<body>\n"
		"<h2>"+status+"</h2>\n"
		"</body>\n"
		"</html>\n"
	;

	// construct response header
	std::string header;
	Session::construct_response_header(code,body.length(),"text/html",header);

	// send response header
	send(header.c_str(),header.length());
	// send body
	send(body.c_str(),body.length());

	// convert bytes to string
	char bytes_string[25];
	sprintf(bytes_string,"%lld",body.length());
	// convert code to string
	char code_string[35];
	sprintf(code_string,"%d",code);
	log(std::string("sent generic ")+code_string+" page ("+bytes_string+")");
}

// send the 404page.html, or a default
void Session::send_error_not_found(){
	// try to send "/404page.html"
	try{
		Resource rc("/404page.html");
		send_file(rc);
	}catch(const SessionErrorNotFound &e){
		// no "/404page.html"
		send_error_generic(HTTP_STATUS_NOT_FOUND);
	}
}

void Session::log(const std::string &line)const{
	std::lock_guard<std::mutex> lock(stdout_lock);
	std::cout<<sid<<" - '"<<sock.get_name()<<"' -- "<<line<<std::endl;
}

// check http request for validity, throw appropriate exception
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
void Session::construct_response_header(int code,long long content_length,const std::string &type,std::string &header){
	char length_string[25];
	sprintf(length_string,"%lld",content_length);

	std::string status;
	get_status_code(code,status);

	header=std::string("HTTP/1.1 ")+status+"\r\n"+
	"Content-Length: "+length_string+"\r\n"+
	"Content-Type: "+type+"\r\n"+
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
	case HTTP_STATUS_INTERNAL_ERROR:
	default:
		status="500 Internal Server Error";
		break;
	}
}

// pick out the requested resource from <header> and put it in <target>
void Session::get_target_resource(const std::string &header,std::string &target){
	// find the first space in the header
	unsigned beginning=header.find(" ");
	if(beginning==std::string::npos)
		return;
	++beginning; // get past the space

	// consistency check
	if(beginning>=header.size()-1)
		return;

	// find the second space in the header
	const unsigned end=header.find(" ",beginning);
	if(end==std::string::npos)
		return;

	// get the bit in the middle
	target=header.substr(beginning,end-beginning);

	// to lower
	for(char &c:target)
		c=tolower(c);
}
