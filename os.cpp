// contains os specific functions, #ifdefs determine final implementation

#include <string>
#include <iostream>
#include <atomic>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

extern std::atomic<bool> running;

// change working dir
bool working_dir(const std::string &dir){
	return chdir(dir.c_str())==0;
}

// get working dir
bool get_working_dir(std::string &dir){
	char path[512];
	if(NULL==getcwd(path,sizeof(path)))
		return false;

	// make sure path doesn't start with "(unreachable)"
	if(path[0]!='/')
		return false;

	dir=path;
	return true;
}

// signal handler (only relevant for linux)
static void handler(int sig){
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

void register_handlers(){
	// signal handlers
	signal(SIGINT,handler);
	signal(SIGTERM,handler);
	signal(SIGPIPE,handler);
}

// try to setuid and setgid to config::uid
// only relevant for linux
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

// canonical path
bool canonical_path(const std::string &target,std::string &canon){
	char *path=realpath(target.c_str(),NULL);
	if(path==NULL)
		return false;

	canon=path;
	free(path);

	return true;
}

bool is_directory(const std::string &target){
	struct stat s;
	if(stat(target.c_str(),&s))
		return false;

	return S_ISDIR(s.st_mode);
}
