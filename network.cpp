#include "network.h"
#include <iostream>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <time.h>

#ifdef _WIN32
WSADATA wsa;
static auto resultorino=WSAStartup(MAKEWORD(1,1), &wsa);
#endif // _WIN32

net::tcp_server::tcp_server(unsigned short port){
	bind(port);
}

net::tcp_server::~tcp_server(){
	this->close();
}

// error
bool net::tcp_server::operator!()const{
	return scan==-1;
}

// BLOCKS and returns the connecting socket
// returns -1 on failure
int net::tcp_server::accept(){
	if(scan==-1)
		return -1;

	sockaddr_in6 connector_addr;
	socklen_t addr_len=sizeof(sockaddr_in6);
	int sock=::accept(scan,(sockaddr*)&connector_addr,&addr_len);

	return sock;
}

// cleanup
void net::tcp_server::close(){
	if(scan != -1){
#ifdef _WIN32
		::closesocket(scan);
#else
		::close(scan);
#endif
		scan=-1;
	}
}

// creates and binds a socket
// true on success
// false on failure (most common cause for failure: someone else is already bound to <port>)
bool net::tcp_server::bind(unsigned short port){
	sockaddr_in6 addr;
	memset(&addr,0,sizeof(sockaddr_in6));
	addr.sin6_family=AF_INET6;
	addr.sin6_port=htons(port);
	addr.sin6_addr=in6addr_any;

	// create the socket for scanning
	scan=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
	if(scan==-1)
		return false;

	// non blocking
#ifdef _WIN32
	u_long nonblock=1;
	ioctlsocket(scan, FIONBIO, &nonblock);
#else
	fcntl(scan,F_SETFL,fcntl(scan,F_GETFL,0)|O_NONBLOCK);
#endif // _WIN32

#ifdef _WIN32
	// make this a dual stack socket
	u_long optmode=0;
	setsockopt(scan, IPPROTO_IPV6, 27, (char*)&optmode, 40);
#else
	// make this socket reusable
	int reuse=1;
	setsockopt(scan,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
#endif

	// bind this socket to <port>
	if(-1==::bind(scan,(sockaddr*)&addr,sizeof(sockaddr_in6))){
		close();
		return false;
	}

	listen(scan,SOMAXCONN);
	return true;
}

/* ------------------------------------------- */
/* ------------------------------------------- */
/* ------------------------------------------- */
/* ------------------------------------------- */

// default constructor
net::tcp::tcp(){
	init();
}

// SOCKET TCP CLIENT
// initialize with already opened socket
net::tcp::tcp(int socket){
	sock=socket;
	ai=NULL;
	blocking=true;

	// figure out name
	char n[51]="N/A";
	sockaddr_in6 addr;
	socklen_t len=sizeof(addr);
	getpeername(sock,(sockaddr*)&addr,&len);
	getnameinfo((sockaddr*)&addr,len,n,sizeof(n),NULL,0,NI_NUMERICHOST);
	name=n;
}

// regular constructor
net::tcp::tcp(const std::string &address,unsigned short port){
	if(!target(address,port)){
		init();
	}
}

// move constructor: useful for passing ownership of the internal socket
// in a ownership handoff from <a> -> <b>, <a> cannot be used again.
net::tcp::tcp(tcp &&rhs){
	sock=rhs.sock;
	name=rhs.name;
	ai=rhs.ai;
	blocking=rhs.blocking;

	rhs.sock=-1;
	rhs.name="N/A";
	rhs.ai=NULL;
	rhs.blocking=true;
}

net::tcp::~tcp(){
	this->close();
}

bool net::tcp::operator!()const{
	return error();
}

net::tcp::operator bool()const{
	return !error();
}

// attempt to connect to <address> on <port>, fills <name> with canonical name of <address>, returns true on success
bool net::tcp::target(const std::string &address,unsigned short port){
	init();

	addrinfo hints;

	memset(&hints,0,sizeof(addrinfo));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=IPPROTO_TCP;

	// convert port to string
	char port_string[20];
	sprintf(port_string,"%hu",port);

	// resolve hostname
	if(0!=getaddrinfo(address.c_str(),port_string,&hints,&ai)){
		return false;
	}

	// get socket name
	char info[51];
	if(0!=getnameinfo(ai->ai_addr,ai->ai_addrlen,info,51,NULL,0,NI_NUMERICHOST)){
		return false;
	}
	name=info;

	// create the socket
	sock=socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
	if(sock==-1){
		return false;
	}

	return true;
}

// non blocking connect attempt
// run this in a loop
// returns true on success
bool net::tcp::connect(){
	if(sock==-1)
		return false;

	set_blocking(false);

	bool result=::connect(sock,ai->ai_addr,ai->ai_addrlen)==0;

	// back to blocking
	if(result)
		set_blocking(true);

	return result;
}

// timeout connect
bool net::tcp::connect(int seconds){
	if(sock==-1)
		return false;

	// non blocking
	set_blocking(false);

	bool result;

	const int MICRO_REST=100;
	const int start=time(NULL);
	do{
		result=::connect(sock,ai->ai_addr,ai->ai_addrlen)==0;
		if(result)
			break;

#ifdef _WIN32
		Sleep(MICRO_REST/100);
#else
		usleep(MICRO_REST);
#endif // _WIN32
	}while(time(NULL)-start<seconds);

	// back to blocking
	set_blocking(true);

	return result;
}

// blocking send
void net::tcp::send_block(const void *buffer,unsigned size){
	if(sock==-1)
		return;

	set_blocking(true);

	unsigned tr=0; // bytes transferred
	while(tr!=size){
		int result=::send(sock,((char*)buffer)+tr,size-tr,0);
		if(result<1){
			this->close();
			return;
		}
		else
			tr+=result;
	}
}

// blocking recv
void net::tcp::recv_block(void *buffer,unsigned size){
	if(sock==-1)
		return;

	set_blocking(true);

	unsigned tr=0; // bytes transferred
	while(tr!=size){
		int result=::recv(sock,((char*)buffer)+tr,size-tr,0);
		if(result<1){
			this->close();
			return;
		}
		else
			tr+=result;
	}
}

// nonblocking send
int net::tcp::send_nonblock(const void *buffer,unsigned size){
	if(sock==-1)
		return 0;

	set_blocking(false);

	int sent=::send(sock,(const char*)buffer,size,0);
	if(sent==-1){
#ifdef _WIN32
		if(WSAGetLastError()==WSAEWOULDBLOCK){
#else
		if(errno==EWOULDBLOCK){ // acceptable, will happen a lot
#endif // _WIN32
			sent=0;
		}
		else{
			this->close(); // error
			return 0;
		}
	}

	return sent;
}

// nonblocking recv
int net::tcp::recv_nonblock(void *buffer,unsigned size){
	if(sock==-1)
		return 0;

	set_blocking(false);

	int received=::recv(sock,(char*)buffer,size,0);
	if(received==-1){
#ifdef _WIN32
		if(WSAGetLastError()==WSAEWOULDBLOCK){
#else
		if(errno==EWOULDBLOCK){ // acceptable, will happen a lot
#endif // _WIN32
			received=0;
		}
		else{
			this->close();
			return 0;
		}
	}

	return received;
}

// check how many bytes are available on the socket
unsigned net::tcp::peek(){
	if(sock==-1)
		return 0;

#ifdef _WIN32
	u_long available=0;
	ioctlsocket(sock,FIONREAD,&available);
#else
	int available=0;
	ioctl(sock,FIONREAD,&available);
#endif // _WIN32

	if(available<0){
		this->close();
		return 0;
	}

	return (unsigned)available;
}

// error check
bool net::tcp::error()const{
	return sock==-1;
}

// getter for socket name
const std::string &net::tcp::get_name()const{
	return name;
}

// cleanup
void net::tcp::close(){
	if(sock!=-1){
#ifdef _WIN32
		::closesocket(sock);
#else
		::close(sock);
#endif // _WIN32
		sock=-1;
	}

	if(ai!=NULL){
		freeaddrinfo(ai);
		ai=NULL;
	}
}

// set blocking or non blocking
void net::tcp::set_blocking(bool block){
	if(block){
		if(!blocking){
#ifdef _WIN32
			u_long nonblock=0;
			ioctlsocket(sock, FIONBIO, &nonblock);
#else
			fcntl(sock,F_SETFL,fcntl(sock,F_GETFL,0)&~O_NONBLOCK); // set to blocking
#endif // _WIN32
			blocking=true;
		}
	}
	else if(!block){
		if(blocking){
#ifdef _WIN32
			u_long nonblock=1;
			ioctlsocket(sock, FIONBIO, &nonblock);
#else
			fcntl(sock,F_SETFL,fcntl(sock,F_GETFL,0)|O_NONBLOCK); // set to non blocking
#endif // _WIN32
			blocking=false;
		}
	}
}

void net::tcp::init(){
	sock=-1;
	name="N/A";
	ai=NULL;
	blocking=true;
}

/* ------------------------------------------- */
/* ------------------------------------------- */
/* ------------------------------------------- */
/* ------------------------------------------- */

// UDP
net::udp_server::udp_server(unsigned short port){
	sock=-1;
	bind(port);
}

// move constructor: useful for passing ownership of the internal socket
// in a ownership handoff from <a> -> <b>, <a> cannot be used again.
net::udp_server::udp_server(udp_server &&rhs){
	sock=rhs.sock;

	rhs.sock=-1;
}

net::udp_server::~udp_server(){
	this->close();
}

bool net::udp_server::operator!()const{
	return error();
}

// cleanup
void net::udp_server::close(){
	if(sock!=-1){
#ifdef _WIN32
		::closesocket(sock);
#else
		::close(sock);
#endif // _WIN32
		sock=-1;
	}
}

// blocking send
void net::udp_server::send(const void *buffer,int len,const udp_id &id){
	if(sock==-1)
		return;

	if(!id.initialized){
		this->close();
		return;
	}

	// no such thing as partial sends for sendto with udp
	int result=sendto(sock,(const char*)buffer,len,0,(sockaddr*)&id.storage,id.len);
	if(result!=len){
		this->close();
		return;
	}
}

// blocking recv
void net::udp_server::recv(void *buffer,int len,udp_id &id){
	if(sock==-1)
		return;

	// no partial receives
	int result=recvfrom(sock,(char*)buffer,len,0,(sockaddr*)&id.storage,&id.len);
	if(result!=len){
		this->close();
		return;
	}
	id.initialized=true;
}

// how many bytes are available on the socket
unsigned net::udp_server::peek(){
	if(sock==-1)
		return 0;

#ifdef _WIN32
	u_long available;
	ioctlsocket(sock,FIONREAD,&available);
#else
	int available=0;
	ioctl(sock,FIONREAD,&available);
#endif // _WIN32

	if(available<0){
		this->close();
		return 0;
	}

	return available;
}

// error check
bool net::udp_server::error()const{
	return sock==-1;
}

bool net::udp_server::bind(unsigned short port){
	addrinfo hints,*ai;

	memset(&hints,0,sizeof(addrinfo));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_flags=AI_PASSIVE;

	// convert port to string
	char port_string[20];
	sprintf(port_string,"%hu",port);

	// resolve hostname
	if(0!=getaddrinfo(NULL,port_string,&hints,&ai)){
		this->close();
		return false;
	}

	// create the socket
	sock=socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
	if(sock==-1){
		this->close();
		return false;
	}

#ifdef _WIN32
	// make this a dual stack socket
	u_long optmode=0;
	setsockopt(sock, IPPROTO_IPV6, 27, (char*)&optmode, 40);
#else
	// make this socket reusable
	int reuse=1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
#endif // _WIN32
	if(::bind(sock,ai->ai_addr,ai->ai_addrlen)){
		this->close();
		return false;
	}

	freeaddrinfo(ai);

	return true;
}

/* ------------------------------------------- */
/* ------------------------------------------- */
/* ------------------------------------------- */
/* ------------------------------------------- */

// udp client
net::udp::udp(const std::string &address,unsigned short port){
	ai=NULL;
	sock=-1;

	target(address,port);
}

// move constructor: useful for passing ownership of the internal socket
// in a ownership handoff from <a> -> <b>, <a> cannot be used again.
net::udp::udp(udp &&rhs){
	sock=rhs.sock;
	ai=rhs.ai;

	rhs.sock=-1;
	rhs.ai=NULL;
}

net::udp::~udp(){
	this->close();
}

bool net::udp::operator!()const{
	return error();
}

void net::udp::send(const void *buffer,unsigned len){
	if(sock==-1)
		return;

	// no such thing as a partial send for udp with sendto
	ssize_t result=sendto(sock,(const char*)buffer,len,0,(sockaddr*)ai->ai_addr,ai->ai_addrlen);
	if(result!=len){
		this->close();
		return;
	}
}

void net::udp::recv(void *buffer,unsigned len){
	if(sock==-1)
		return;

	// ignored
	sockaddr_storage src_addr;
	socklen_t src_len=sizeof(sockaddr_storage);

	// no such thing as a partial send for udp with sendto
	ssize_t result=recvfrom(sock,(char*)buffer,len,0,(sockaddr*)&src_addr,&src_len);
	if(result!=len){
		this->close();
		return;
	}
}

unsigned net::udp::peek(){
	if(sock==-1)
		return 0;

#ifdef _WIN32
	u_long avail=0;
	ioctlsocket(sock,FIONREAD,&avail);
#else
	int avail=0;
	ioctl(sock,FIONREAD,&avail);
#endif // _WIN32

	if(avail<0){
		this->close();
		return 0;
	}

	return avail;
}

bool net::udp::error()const{
	return sock==-1;
}

void net::udp::close(){
	if(sock!=-1){
#ifdef _WIN32
		::closesocket(sock);
#else
		::close(sock);
#endif // _WIN32
		sock=-1;
	}

	if(ai!=NULL){
		freeaddrinfo(ai);
		ai=NULL;
	}
}

// remember it is auto bound no need to explicitly bind
bool net::udp::target(const std::string &address,unsigned short port){
	addrinfo hints;

	memset(&hints,0,sizeof(addrinfo));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_protocol=0;

	// convert port to string
	char port_string[20];
	sprintf(port_string,"%hu",port);

	// resolve hostname
	if(0!=getaddrinfo(address.c_str(),port_string,&hints,&ai)){
		return false;
	}

	// create the socket
	sock=socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
	if(sock==-1){
		return false;
	}

	return true;
}
