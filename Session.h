#ifndef SESSION_H
#define SESSION_H

class SessionError{
public:
	SessionError(std::string d):desc(d){}
	virtual const char *what()const{return desc.c_str();}

private:
	const std::string desc;
};

// error reading HTTP request
class SessionErrorClosed:public SessionError{
public:
	SessionErrorClosed():SessionError("error reading HTTP request"){}
};

// malformed http request
class SessionErrorMalformed:public SessionError{
public:
	SessionErrorMalformed():SessionError("malformed http request"){}
};

// operation (e.g. GET, POST) not supported
class SessionErrorNotSupported:public SessionError{
public:
	SessionErrorNotSupported():SessionError("operation not supported"){}
};

// HTTP version not supported
class SessionErrorVersion:public SessionError{
public:
	SessionErrorVersion():SessionError("http version not supported"){}
};

// resource not found (404)
class SessionErrorNotFound:public SessionError{
public:
	SessionErrorNotFound(const std::string &target)
	:SessionError(std::string("resource: \"")+target+"\" not found"){}
};

#define HTTP_KEEPALIVE 10
#define EXIT_REQUEST "session terminated: exit requested"

class Session{
public:
	Session(int,unsigned);
	Session(const Session&)=delete;
	Session(Session&&)=delete;
	Session &operator=(const Session&)=delete;
	static void entry(int,unsigned);

private:
	void serve();
	void get_http_request(std::string&);
	void send_error_generic(int);
	void send_error_not_found();
	void log(const std::string&)const;
	static void check_http_request(const std::string&);
	static void construct_response_header(int,int,std::string&);
	static void get_status_code(int,std::string&);
	static void get_target_resource(const std::string&,std::string&);

	net::tcp sock;
	const int sid; // session id
	int entry_time;
};

#endif // SESSION_H
