#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <string.h>
#ifdef _WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <sys/types.h>
#define ssize_t SSIZE_T
#else
#include <netinet/in.h>
#include <netdb.h>
#endif // WIN32

namespace net{

	std::string me();

#ifdef _WIN32
	const int WOULDBLOCK = WSAEWOULDBLOCK;
	const int CONNRESET = WSAECONNRESET;
#else
	const int WOULDBLOCK = EWOULDBLOCK;
	const int CONNRESET = ECONNRESET;
#endif // _WIN32

// tcp
class tcp_server{
public:
	tcp_server();
	tcp_server(unsigned short);
	tcp_server(const tcp_server&)=delete;
	~tcp_server();
	tcp_server &operator=(const tcp_server&)=delete;
	operator bool()const;
	bool bind(unsigned short);
	int accept(int = 0);
	void close();

private:
	int scan; // the socket for scanning
};

class tcp{
public:
	tcp();
	tcp(const std::string&,unsigned short);
	tcp(int);
	tcp(const tcp&)=delete;
	tcp(tcp&&);
	~tcp();
	tcp &operator=(const tcp&)=delete;
	tcp &operator=(tcp&&);
	operator bool()const;
	bool target(const std::string &address,unsigned short);
	bool connect();
	bool connect(int);
	void send_block(const void*,unsigned);
	void recv_block(void*,unsigned);
	int send_nonblock(const void*,unsigned);
	int recv_nonblock(void*,unsigned);
	unsigned peek();
	void close();
	bool error()const;
	const std::string &get_name()const;
	int release();

private:
	void set_blocking(bool);
	void init();
	bool writable();

	int sock;
	std::string name;
	addrinfo *ai;
	bool blocking;
};

// udp
struct udp_id{
	udp_id():initialized(false),len(sizeof(sockaddr_storage)){
		memset(&storage, 0, sizeof(storage));
	}

	bool initialized;
	sockaddr_storage storage;
	socklen_t len;
};

class udp_server{
public:
	udp_server();
	udp_server(unsigned short);
	udp_server(const udp_server&)=delete;
	udp_server(udp_server&&);
	~udp_server();
	udp_server &operator=(const udp_server&)=delete;
	operator bool()const;
	void close();
	void send(const void*,int,const udp_id&);
	int recv(void*,int,udp_id&);
	unsigned peek();
	bool error()const;

private:
	bool bind(unsigned short);

	int sock;
};

class udp{
public:
	udp();
	udp(const std::string&,unsigned short);
	udp(const udp&)=delete;
	udp(udp&&);
	~udp();
	udp &operator=(const udp&)=delete;
	udp &operator=(udp&&);
	bool target(const std::string&,unsigned short);
	operator bool()const;
	void close();
	void send(const void*,unsigned);
	int recv(void*,unsigned);
	unsigned peek();
	bool error()const;

private:
	int sock;
	addrinfo *ai;
};

} // namespace socket

#endif // NETWORK_H
