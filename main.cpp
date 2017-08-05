#include <iostream>
#include <atomic>
#include <unistd.h>
#include <signal.h>

#include "Servant.h"

std::atomic<bool> running;

int main(int argc,char **argv){
	running.store(true);

	// signal handlers
	signal(SIGINT,handler);
	signal(SIGTERM,handler);
	signal(SIGPIPE,handler);

	// figure out cmd options
	config cfg;
	cmdline(cfg,argc,argv);

	// new unnamed scope
	{
		// initialize the server
		Servant servant(cfg.port);
		if(!servant){
			std::cout<<"error: could not bind to port "<<cfg.port<<std::endl;
			return 1;
		}

		// drop root priviledges if requested
		if(cfg.uid!=0){
			if(!drop_root(cfg.uid)){
				return 1;
			}
		}

		// chdir to <cfg.rootdir>
		if(!working_dir(cfg.root)){
			std::cout<<"error: could not find directory \""<<cfg.root<<"\""<<std::endl;
			return 1;
		}

		while(running.load()){
			servant.accept();

			usleep(100);
		}
	}

	std::cout<<"exiting..."<<std::endl;

	return 0;
}

// parse cmdline params
void cmdline(config &cfg,int argc,char **argv){
	// defaults
	cfg.port=DEFAULT_PORT;
	cfg.root=DEFAULT_ROOTDIR;
	cfg.uid=0;

	opterr=1;
	int c;
	while((c=getopt(argc,argv,"p:r:u:h"))!=-1){
		switch(c){
		case 'p': // port (-p)
			if(1!=sscanf(optarg,"%d",&cfg.port))
				usage(argv[0]);
			break;
		case 'r': // root dir (-r)
			cfg.root=optarg;
			break;
		case 'u':
			if(1!=sscanf(optarg,"%u",&cfg.uid))
				usage(argv[0]);
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			break;
		}
	}
}

void usage(const char *name){
	std::cout<<"usage: "<<name<<" [-u uid] [-p port] [-r rootdir] [-h]"<<std::endl;
	std::cout<<"- port: the port the server should listen on (default="<<DEFAULT_PORT<<")"<<std::endl;
	std::cout<<"- rootdir: the http root from which files will be served (default="<<DEFAULT_ROOTDIR<<")"<<std::endl;
	std::cout<<"- uid: if specified, servant will try to setuid() and setgid() to this value after binding to <port> (default=0"<<std::endl;

	exit(EXIT_SUCCESS);
}

// try to setuid and setgid to config::uid
bool drop_root(unsigned uid){
	if(setgid(uid)){
		std::cout<<"error: setgid("<<uid<<") errored (errno="<<errno<<")"<<std::endl;
		return false;
	}

	if(setuid(uid)){
		std::cout<<"error: setuid("<<uid<<") errored (errno="<<errno<<""<<std::endl;
		return false;
	}

	return true;
}

bool working_dir(const std::string &dir){
	return chdir(dir.c_str())==0;
}

void handler(int sig){
	switch(sig){
	case SIGINT:
	case SIGTERM:
		std::cout<<std::endl;
		running.store(false);
		break;
	case SIGPIPE:
		break;
	}
}
