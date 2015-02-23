//event.cpp
#include<iostream>
#include"event.h"

using namespace opssimu;

//ƒCƒxƒ“ƒgˆ—‚ÌÀ‘•
void CEventDebug1::onEvent()const{
	std::cout<<"time="<<this->time()<<"\tid="<<this->id()<<"\tparam="<<this->param_<<std::endl;
}

//end of event.cpp


