#ifndef SERVANT_H
#define SERVANT_H

#include <vector>
#include <thread>

#include "network.h"
#include "Session.h"
#include "Resource.h"

// config defaults
#define DEFAULT_PORT 80
#define DEFAULT_ROOTDIR "./root"
#define DEFAULT_NAME "no one of consequence"

// http errors
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_INTERNAL_ERROR 500
#define HTTP_STATUS_NOT_IMPLEMENTED 501
#define HTTP_STATUS_VERSION_NOT_SUPPORTED 505

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
