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

	sessions.push_back(std::thread(Session::entry,this,sock,++Servant::session_id));

	// get rid of completed threads
	std::lock_guard<std::mutex> lock(mut);
	for(std::vector<std::thread::id>::iterator it=completed.begin();it!=completed.end();){
		std::thread::id id=*it;

		// find id in sessions and delete it
		for(std::vector<std::thread>::iterator it2=sessions.begin();it2!=sessions.end();){
			std::thread &thread=*it2;

			if(thread.get_id()==id){
				thread.join();
				it2=sessions.erase(it2);
				break;
			}

			++it2;
		}

		// delete id from completed vector
		it=completed.erase(it);
	}
}

// called by Session::entry to notify Servant of completed session
void Servant::complete(){
	std::lock_guard<std::mutex> lock(mut);
	completed.push_back(std::this_thread::get_id());
}
