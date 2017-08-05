#include "Servant.h"

unsigned Servant::session_id=0;

Servant::Servant(unsigned short port):scan(port){
}

Servant::~Servant(){
	// join all the sessions
	for(std::thread &t:sessions)
		t.join();
}

bool Servant::operator!()const{
	return !scan;
}

void Servant::accept(){
	int sock=scan.accept();

	if(sock==-1)
		return;

	sessions.push_back(std::thread(Session::entry,sock,Servant::session_id));
}
