#ifndef SERVANT_H
#define SERVANT_H

#include "network.h"

#define DEFAULT_PORT 80
#define DEFAULT_ROOTDIR "./root"

struct config;
void handler(int);
void cmdline(config&,int,char**);
void usage(const char*);
bool drop_root(unsigned);
bool working_dir(const std::string&);

class Servant{
public:
	Servant(unsigned short);
	Servant(const Servant&)=delete;
	Servant(Servant&&)=delete;
	~Servant();
	bool operator!()const;
	void accept();

private:
	net::tcp_server sock;
};

struct config{
	unsigned short port;
	std::string root;
	unsigned uid;
};

#endif // SERVANT_H
