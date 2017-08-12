#ifndef SERVANT_H
#define SERVANT_H

#include <vector>
#include <thread>
#include <mutex>

class Servant;
#include "network.h"
#include "os.h"
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
void cmdline(config&,int,char**);
void usage(const char*);

class Servant{
public:
	Servant(unsigned short);
	Servant(const Servant&)=delete;
	Servant(Servant&&)=delete;
	~Servant();
	Servant &operator=(const Session&)=delete;
	bool operator!()const;
	void accept();
	void complete();

private:
	void cleanup();

	std::vector<std::thread> sessions;
	std::vector<std::thread::id> completed; // array of thread ids that have completed
	net::tcp_server scan;
	static unsigned session_id;
	std::mutex mut; // used to protect Servant::completed
};

struct config{
	unsigned short port;
	std::string root;
	unsigned uid;
};

#endif // SERVANT_H
