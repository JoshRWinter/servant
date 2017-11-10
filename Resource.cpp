#include <string.h>
#include <ctype.h>
#include <algorithm>

#include "Servant.h"

#undef min
#undef max

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

	// figure out the content type
	content_type=Resource::get_type(fname);

	init_file();
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
	if(!strcmp(content_type,"text/html")){
		const int max=html_file.length();
		const int retrieve=std::min(max,size);
		memcpy(buf,html_file.c_str(),retrieve);

		// delete the chars just read from the string
		html_file.erase(0,retrieve);
		return retrieve;
	}
	else{
		rsrc.read(buf,size);
		return rsrc.gcount();
	}
}

int Resource::size()const{
	return fsize;
}

const char *Resource::type()const{
	return content_type;
}

// if the resource is html file, process it for server side includes
// otherwise just open the stream
void Resource::init_file(){
	// html files are processed differently
	if(!strcmp(content_type,"text/html")){
		std::ifstream f(fname,std::ifstream::ate); // opening at the end
		if(!f)
			throw SessionErrorInternal(std::string("couldn't open \"")+fname+"\" ("+content_type+")");

		// figure out length of file
		const int len=f.tellg();
		f.seekg(0);

		// read file
		int read=0;
		while(read!=len){
			const int get_size=4096;
			char block[get_size+1];
			f.read(block,get_size);
			const int got=f.gcount();

			read+=got;
			block[got]=0;
			html_file+=block;
		}

		// process string
		// inserts server side includes
		Resource::html(html_file);
		fsize=html_file.length();
	}
	else{ // not an html file
		// if it made it this far, <fname> must be safe
		rsrc.open(fname,std::ifstream::binary|std::ifstream::ate); // opening at the end
		if(!rsrc)
			throw SessionErrorInternal(std::string("couldn't open \"")+fname+" in read mode");

		// figure out length of file
		fsize=rsrc.tellg();
		// rewind
		rsrc.seekg(0);
	}
}

// fill in server side includes
// example syntax: "####include.html"
void Resource::html(std::string &stream){
	int pos=-1;
	while((pos=stream.find("####",pos+1))!=std::string::npos){
		// make sure it's on a line of its own
		// walk backwards to find a newline, ignoring whitespace
		try{
			char c=0;
			bool giveup=false;
			int current=pos-1;
			while(c!='\n'&&current>=0){
				c=stream.at(current);
				if(!isspace(c)){
					// includes need to be on there own line
					giveup=true;
					break;
				}

				--current;
			}
			if(giveup)
				continue;
		}catch(const std::out_of_range &e){
			throw SessionErrorInternal("out_of_range error when processing include line");
		}

		// try to figure out the filename
		std::string include_name;
		try{
			char c=0;
			int current=pos+4; // skip the "####"
			while(current<stream.length()&&!isspace(c=stream.at(current)))
				++current;
			include_name=stream.substr(pos+4,current-(pos+4));
		}catch(const std::out_of_range &e){
			throw SessionErrorInternal("out_of_range error when processing include line");
		}

		// delete the include from the stream
		try{
			stream.erase(pos,4+include_name.length());
		}catch(const std::out_of_range &e){
			throw SessionErrorInternal("out_of_range error when processing include line");
		}

		// make sure file is not a directory
		if(is_directory(include_name))
			throw SessionErrorInternal("error when including file: directories cannot be included");

		// try to read file
		std::string include_text;
		try{
			Resource rc(include_name);
			const int len=rc.size();
			int read=0;
			while(read!=len){
				const int get_size=1024;
				char block[get_size+1];

				const int got=rc.get(block,get_size);
				block[got]=0;
				include_text+=block;
				read+=got;
			}
		}catch(const SessionError &se){
			throw SessionErrorInternal(std::string("error when including file: ")+se.what());
		}

		// insert the include text
		stream.insert(pos,include_text);
	}
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
