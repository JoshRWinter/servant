#include <string.h>

#include "Servant.h"

Resource::Resource(const std::string &target){
	fname=target;

	// if fname is root ("/"), then replace with current dir
	if(fname=="/"||fname=="\\")
		fname="./";

	// get rid of leading slashes
	try{
		while(fname.at(0)=='/'||fname.at(0)=='\\')
			fname.erase(fname.begin());
	}catch(const std::out_of_range &e){}

	// append /index.html if fname is a direcory
	if(is_directory(fname))
		fname+=(fname[fname.length()-1]=='/')?"index.html":"/index.html";

	// checks for ../ tomfoolery, also checks if file exists
	Resource::check_valid(fname);

	// if it made it this far, <fname> must be safe
	rsrc=std::move(std::ifstream(fname,std::ifstream::binary|std::ifstream::ate)); // opening at the end
	if(!rsrc)
		throw SessionErrorInternal(std::string("couldn't open \"")+fname+" in read mode");

	// figure out length of file
	fsize=rsrc.tellg();
	// rewind
	rsrc.seekg(0);

	// figure out the content type
	content_type=Resource::get_type(fname);
}

// move constructor, leaves original unusable
Resource::Resource(Resource &&rhs):fname(std::move(rhs.fname)),rsrc(std::move(rhs.rsrc)){
	fsize=rhs.fsize;
	content_type=rhs.content_type;
}

// file name
const std::string &Resource::name()const{
	return fname;
}

// retrieve a chunk of size <size>, advances the internal stream pointer
// returns bytes read
int Resource::get(char *buf,int size){
	rsrc.read(buf,size);
	return rsrc.gcount();
}

int Resource::size()const{
	return fsize;
}

const char *Resource::type()const{
	return content_type;
}

// check input file
void Resource::check_valid(const std::string &target){
	// canonicalize path
	std::string canon;
	if(!canonical_path(target,canon))
		throw SessionErrorNotFound(target);

	// get the current working directory
	std::string cwd;
	if(!get_working_dir(cwd))
		throw SessionErrorInternal("couldn't get cwd");

	// make sure that the cwd is the first bit of <target>
	if(canon.find(cwd)!=0)
		throw SessionErrorForbidden(target);
}

// get the content type given the filename
const char *Resource::get_type(const std::string &target){
	// get the file extension
	std::string ext;
	Resource::get_ext(target,ext);

	if(ext=="html")
		return "text/html";
	else if(ext=="css")
		return "text/css";
	else if(ext=="js")
		return "application/javascript";
	else if(ext=="txt")
		return "text/plain";
	else if(ext=="ico")
		return "image/x-icon";
	else if(ext=="jpg"||ext=="jpeg")
		return "image/jpeg";
	else if(ext=="png")
		return "image/png";
	else if(ext=="gif")
		return "image/gif";
	else if(ext=="webm")
		return "video/webm";
	else if(ext=="mp4")
		return "video/mp4";
	else
		return "application/octet-stream";
}

void Resource::get_ext(const std::string &target,std::string &ext){
	if(target.length()<2){
		ext="";
		return;
	}

	for(int i=target.length()-2;i>=0;--i){
		if(target[i]=='/'||target[i]=='\\'){
			ext="";
			return;
		}
		else if(target[i]=='.'){
			ext=target.substr(i+1);
			return;
		}
	}

	ext="";
	return;
}
