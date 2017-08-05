#ifndef SESSION_H
#define SESSION_H

class SessionError{
public:
	SessionError(std::string d):desc(d){}
	virtual const char *what()const{return desc.c_str();}

private:
	const std::string desc;
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
	void check_http_request(const std::string&)const;
	void send_error_malformed();
	void send_error_not_supported();
	void send_error_version();
	void log(const std::string&)const;

	net::tcp sock;
	const int sid; // session id
	const int entry_time;
};

#endif // SESSION_H
