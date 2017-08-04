#include <iostream>

#include "Servant.h"

Servant::Servant(unsigned short port):sock(port){
}

Servant::~Servant(){
}

bool Servant::operator!()const{
	return !sock;
}

void Servant::accept(){
}
