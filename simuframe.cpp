//simuframe.cpp

#include<algorithm>
#include<iostream>
#include"simuframe.h"

//�f�o�b�O�p
#if DEBUG_EVENT_TRACE
#include<typeinfo>
#endif

using namespace simuframe;
/*
//�Ƃ肠�����e�X�g�p�B�{����opssimu�ɂ͈ˑ����Ȃ��B
#include"event.h"
//�Ƃ肠�����e�X�g�p�Bopsnetwork�ɂ͈ˑ����Ȃ�
#include"opsnetwork.h"
*/

//�f�o�b�O�p
#include<iostream>

//static member instance
//boost::pool<> CEvent::event_pool_(sizeof(CEventMaxsize<>));
event_id_t CEvent::counter_=0;
CEventManager* CEventManager::p_instance_=0;
simutime_t CSimu::time_=0;
/*
void CEvent::operator delete(void* p){
        //std::cout<<"debug:CEvent:delete():start"<<std::endl;
        event_pool_.free(p);
        //std::cout<<"debug:CEvent:delete():end"<<std::endl;
 }
 */
/////////////////////////////////////////////////
//			class CEventManager
/////////////////////////////////////////////////
//�f�X�g���N�^
CEventManager::~CEventManager(){
	//memory clean up

	//�e�[�u�����̃C�x���g���폜
	while(!table_.empty()){
		delete (getNextEvent());
	}
	//�C���X�^���X�|�C���^��������Ԃɖ߂�
	p_instance_=0;
	//�C�x���gID��0�ɖ߂�
	CEvent::reset_id();
}


/////////////////////////////////////////////////
//			class CStatManager
/////////////////////////////////////////////////
//���v���Ƃ�I�u�W�F�N�g���e�[�u���ɒǉ�
void CStatManager::add(CStatable* const p_obj,const group_t group){
	//group�t���O�̃Z�b�g	
	p_obj->set_group(group);
	//group���Ƀ\�[�g���Ēǉ�
	table_.insert(
		std::upper_bound(table_.begin(),table_.end(),p_obj,FCStatableLess()),
		p_obj
	);
}

//���v�C�x���g�p�n���h��
void CStatManager::onStat(const CEvent* const p_event){
	//���v�C�x���g���Ăяo���O���[�v���擾
	const group_t group=(static_cast<const CEventStat* const>(p_event))->group();
	
	if(group==CStatManager::ALL){
		//���ׂĂ��R�[������ꍇ
		//std::for_each(table_.begin(),table_.end(),boost::bind2nd(
		//	boost::mem_fun1_t<void,CStatable,const CEvent* const>(&(CStatable::onStat)),p_event));
		for(table_t::iterator it=table_.begin();it!=table_.end();++it){
			(*it)->onStat(p_event);
		}
	}else if(group!=CStatManager::NONE){
		//�����group�̂݃R�[������ꍇ
		CStatable dummy;//�O���[�v�T���p�̃_�~�[
		dummy.set_group(group);
		std::pair<table_t::iterator,table_t::iterator> range=
			std::equal_range(table_.begin(),table_.end(),(&dummy),FCStatableLess());
		//std::for_each(range.first,range.second,std::bind2nd(
		//	std::mem_fun1(&(CStatable::onStat)),p_event));
		for(;range.first!=range.second;++(range.first)){
			(*range.first)->onStat(p_event);
		}
	}else{
		//group==CStatManager::NONE�̏ꍇ
		return;//���������A���̃C�x���g���������Ȃ�
	}

	//���̓��v�C�x���g�𐶐�
	p_event_manager_->setEvent(
			new CEventStat(p_event->time()+interval_,this,group)
		);
	return;
}


/////////////////////////////////////////////////
//			class CSimu
/////////////////////////////////////////////////
//�V�~�����[�V�������������ɌĂ΂��
bool CSimu::init(){
	//initialize simulation

	//CEventManager�̏�����
	p_event_manager_=CEventManager::getInstance();
	if(p_event_manager_==NULL)
		return false;

	//�V�~�����[�V�����ŗL�̏�����
	return p_property_->init(this);
}

//�V�~�����[�V�����J�n
bool CSimu::start(){
	run_flg_=true;
	
	//simulation main loop
	while(run_flg_){
		if(!p_event_manager_->is_empty()){
            //std::cout<<"debug:CSimu::start():get"<<std::endl;
			const CEvent* const p_event=p_event_manager_->getNextEvent();//���C�x���g�擾
			time_=p_event->time();//���ݎ����̍X�V
#if DEBUG_EVENT_TRACE
            std::cout<<"debug:CSimu::start():start:"<<p_event->id()<<":"<<(typeid(*p_event)).name()<<std::endl;
#endif
			p_event->onEvent();//�C�x���g����
			
#if DEBUG_EVENT_TRACE
            std::cout<<"debug:CSimu::start():end:"<<p_event->id()<<":"<<(typeid(*p_event)).name()<<std::endl;
#endif
            delete p_event;
		}else{
			std::cerr<<"Error:event table is empty."<<std::endl;
			run_flg_=false;
		}
	}
	return true;
}

//�V�~�����[�V�������~�߂�
//start()��main_loop���甲���邾���Ń������̔p���̓f�X�g���N�^�ɂ܂�����B
bool CSimu::end(){
	run_flg_=false;
	return true;
};

bool CSimu::clean_up(){
	//�V�~�����[�V�����ŗL�̏I������
	p_property_->clean_up();
	
	//�C�x���g�e�[�u���̍폜
	delete p_event_manager_;
	
	//�����̃��Z�b�g
	time_=0;
	return true;
}

//end of simuframe.cpp


