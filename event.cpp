//event.cpp
#include<iostream>
#include"event.h"

using namespace opssimu;

//イベント処理の実装
void CEventDebug1::onEvent()const{
	std::cout<<"time="<<this->time()<<"\tid="<<this->id()<<"\tparam="<<this->param_<<std::endl;
}

//end of event.cpp


