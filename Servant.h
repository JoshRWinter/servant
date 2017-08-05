#ifndef SERVANT_H
#define SERVANT_H

#include <vector>
#include <thread>

#include "network.h"
#include "Session.h"

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
	Servant &operator=(const Session&)=delete;
	bool operator!()const;
	void accept();

private:
	std::vector<std::thread> sessions;
	net::tcp_server scan;
	static unsigned session_id;
};

struct config{
	unsigned short port;
	std::string root;
	unsigned uid;
};

#endif // SERVANT_H
