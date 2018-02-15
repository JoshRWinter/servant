// contains os specific functions, #ifdefs determine final implementation

#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <limits.h>
#include <stdlib.h>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif // _WIN32

extern std::atomic<bool> running;

// change working dir
bool working_dir(const std::string &dir){
#ifdef _WIN32
	return SetCurrentDirectory(dir.c_str())!=0;
#else
	return chdir(dir.c_str())==0;
#endif // _WIN32
}

// get working dir
static std::mutex working_dir_lock;
bool get_working_dir(std::string &dir){
	std::lock_guard<std::mutex> lock(working_dir_lock);
	char path[512];
#ifdef _WIN32
	bool result=GetCurrentDirectory(sizeof(path), path)!=0;
	if(result)
		dir=path;

	return result;
#else
	if(NULL==getcwd(path,sizeof(path)))
		return false;

	// make sure path doesn't start with "(unreachable)"
	if(path[0]!='/')
		return false;

	dir=path;
	return true;
#endif // _WIN32
}

// signal handler
#ifdef _WIN32
BOOL WINAPI handler(DWORD sig){
	if(sig==CTRL_C_EVENT){
		running.store(false);
		return TRUE;
	}

	return FALSE;
}
#else
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
#endif // _WIN32

void register_handlers(){
#ifdef _WIN32
	SetConsoleCtrlHandler(handler, TRUE);
#else
	// signal handlers
	signal(SIGINT,handler);
	signal(SIGTERM,handler);
	signal(SIGPIPE,handler);
#endif // _WIN32
}

// try to setuid and setgid to config::uid
// only relevant for linux
bool drop_root(unsigned uid){
#ifndef _WIN32
	if(setgid(uid)){
		std::cout<<"error: setgid("<<uid<<") errored (errno="<<errno<<")"<<std::endl;
		return false;
	}

	if(setuid(uid)){
		std::cout<<"error: setuid("<<uid<<") errored (errno="<<errno<<""<<std::endl;
		return false;
	}

#endif // _WIN32
	return true;
}

// canonical path
// return false if <target> doesn't exist
std::mutex canon_path_lock;
bool canonical_path(const std::string &target,std::string &canon){
	std::lock_guard<std::mutex> lock(canon_path_lock);
#ifdef _WIN32
	if(GetFileAttributes(target.c_str())==INVALID_FILE_ATTRIBUTES)
		return false;

	char path[MAX_PATH]="";
	bool result=GetFullPathName(target.c_str(), sizeof(path), path, NULL)!=0;
	canon=path;
	return result;
#else
	char *path=realpath(target.c_str(),NULL);
	if(path==NULL)
		return false;

	canon=path;
	free(path);

	return true;
#endif // _WIN32
}

bool is_directory(const std::string &target){
#ifdef _WIN32
	DWORD attributes=GetFileAttributes(target.c_str());
	if(attributes == INVALID_FILE_ATTRIBUTES)
		return false;

	return (attributes&FILE_ATTRIBUTE_DIRECTORY)==FILE_ATTRIBUTE_DIRECTORY;
#else
	struct stat s;
	if(stat(target.c_str(),&s))
		return false;

	return S_ISDIR(s.st_mode);
#endif // _WIN32
}

long long filesize(const std::string &fname){
#ifdef _WIN32
	HANDLE h = CreateFile(fname.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(h == INVALID_HANDLE_VALUE)
		return -1;

	LARGE_INTEGER li;
	GetFileSizeEx(h, &li);
	CloseHandle(h);

	return li.QuadPart;
#else
	struct stat s;
	if(0 != stat(fname.c_str(), &s))
		return -1;
	return s.st_size;
#endif // _WIN32
}
