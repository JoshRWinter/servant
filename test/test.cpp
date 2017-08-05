#include <iostream>
#include <atomic>

#define private public // nice
#include "../Servant.h"

#define RED_TEXT  "\033[31;1m"
#define GREEN_TEXT "\033[32;1m"
#define RESET_TEXT "\033[0m"

std::atomic<bool> running; // Session needs this

bool http_validate_test(){
	running.store(true);
	Session session(-1,1);
	bool success=true;

	struct validate{
		const char *const request;
		const bool valid; // is request supposed to be valid
	};

	// test cases
	validate cases[]={
		{"POST / HTTP/1.1\r\n\r\n",false},
		{"",false},
		{"\r\n\r\n",false},
		{"GET /n\r\n",false},
		{"GET / HTTP/2.0\r\n\r\n",false},
		{"GET / HTTP/1.1\r\n\r\n",true},
		{"GET /folder/file/folder/folder/test.html HTTP/1.1\r\n\r\n",true},
		{"GOT /folder/file/folder/folder/test.html HTTP/1.1\r\n\r\n",false}
	};

	for(int i=0;i<sizeof(cases)/sizeof(validate);++i){
		bool threw=false;

		std::string what="(no exception was thrown)";
		try{
			session.check_http_request(cases[i].request);
		}catch(const SessionError &e){
			threw=true;
			what=e.what();
		}

		if(threw==cases[i].valid){
			std::cout<<RED_TEXT<<"test "<<i<<" failed\n\""<<what<<"\""<<RESET_TEXT<<std::endl;
			success=false;
		}
		else
			std::cout<<GREEN_TEXT<<"test "<<i<<" passed: "<<what<<RESET_TEXT<<std::endl;
	}

	return success;
}

int main(){
	bool success=http_validate_test();

	return success?0:1;
}
