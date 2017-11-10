#ifndef NETWORK_H
#define NETWORK_H

#include <string>
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

// tcp
class tcp_server{
public:
	tcp_server(unsigned short);
	tcp_server(const tcp_server&)=delete;
	~tcp_server();
	tcp_server &operator=(const tcp_server&)=delete;
	bool operator!()const;
	int accept();
	void close();

private:
	bool bind(unsigned short);

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
	bool operator!()const;
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

private:
	void set_blocking(bool);
	void init();

	int sock;
	std::string name;
	addrinfo *ai;
	bool blocking;
};

// udp
struct udp_id{
	udp_id():initialized(false),len(sizeof(sockaddr_storage)){}

	bool initialized;
	sockaddr_storage storage;
	socklen_t len;
};

class udp_server{
public:
	udp_server(unsigned short);
	udp_server(const udp_server&)=delete;
	udp_server(udp_server&&);
	~udp_server();
	udp_server &operator=(const udp_server&)=delete;
	bool operator!()const;
	void close();
	void send(const void*,int,const udp_id&);
	void recv(void*,int,udp_id&);
	unsigned peek();
	bool error()const;

private:
	bool bind(unsigned short);

	int sock;
};

class udp{
public:
	udp(const std::string&,unsigned short);
	udp(const udp&)=delete;
	udp(udp&&);
	~udp();
	udp &operator=(const udp&)=delete;
	bool operator!()const;
	void close();
	void send(const void*,unsigned);
	void recv(void*,unsigned);
	unsigned peek();
	bool error()const;

private:
	bool target(const std::string&,unsigned short);

	int sock;
	addrinfo *ai;
};

} // namespace socket

#endif // NETWORK_H
