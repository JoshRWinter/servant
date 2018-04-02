#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

#include "Servant.h"
#ifdef _WIN32
#include "getopt.h"
#else
#include <getopt.h>
#endif // _WIN32

std::atomic<bool> running;

int main(int argc,char **argv){
	running.store(true);

	// register signals
	register_handlers();

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

		// print status line
		std::cout<<"[document root: '"<<cfg.root<<"' -- port: '"<<cfg.port<<"' -- ready]"<<std::endl;

		while(running.load()){
			servant.accept();
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
			if(1!=sscanf(optarg,"%hu",&cfg.port))
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
