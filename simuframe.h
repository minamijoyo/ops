#ifndef _SIMUFRAME_H__
#define _SIMUFRAME_H__
//simuframe.h

#include<queue>
#include<vector>
#include<functional>
#include<algorithm>
//#include<boost/pool/pool.hpp>
#include<boost/noncopyable.hpp>

#include"statable.h"


//�f�o�b�O�p
#include<iostream>
#define DEBUG_EVENT_TRACE 0

namespace simuframe{

//typedef

//64bit�����^
/*
#ifdef WIN32
typedef unsigned __int64 uint64;//VC�p
#else
typedef unsigned long long uint64;//gcc�p
#endif
typedef uint64 event_id_t;
typedef uint64 simutime_t;
*/
//32bit�A�[�L�e�N�`���ł�64bit��������double�̕������������Ƃ����
//��������bit��(���Ɉˑ����邪52bit���x)�𒴂����傫�����K�v�ɂȂ�ꍇ��
//64bit�̐����^���g������

typedef double event_id_t;//�C�x���gID�^
typedef double simutime_t;//�V�~�����[�V�������Ԍ^

//�O���錾
class ISimu;//�V�~�����[�V�����C���^�[�t�F�C�X

/////////////////////////////////////////////////
//			class CEvent
/////////////////////////////////////////////////
//�C�x���g���N���X
class CEvent{
	const event_id_t id_;//ID
//	static boost::pool<> event_pool_;//�������v�[��
	static event_id_t counter_;//�C�x���gID�̒ʂ��ԍ�
protected:
	const simutime_t time_;
public:
	CEvent(const simutime_t time):id_(counter_++),time_(time){
//        std::cout<<"debug:CEvent:CEvent()"<<std::endl;
}
	virtual ~CEvent(){
//        std::cout<<"debug:CEvent:~CEvent()"<<std::endl;
};
	virtual void onEvent() const =0;//�C�x���g�n���h���̃C���^�[�t�F�C�X
	const event_id_t id() const { return id_; }
	const simutime_t time() const { return time_; }
	
	static void reset_id(){ counter_=0; }
	//�������m�ۋy�ъJ��
/*	void* operator new(size_t t){ 
//        std::cout<<"debug:CEvent:new():start"<<std::endl;
        return event_pool_.malloc();
//        std::cout<<"debug:CEvent:new():end"<<std::endl;
        //return p;
    }
	void operator delete(void* p);//�R���p�C����inline�œK���o�O�̂��ߖ����I��outline
*/
};

//CEvent*�̗D�揇�ʔ�r�t�@���N�^
//�������������قǗD�揇�ʂ������B�����ꍇ��id�����������B
//���Ӓl���D��x���������Ɣ��f�����͎̂������傫���Ƃ��ł���̂ŕ����ɒ��ӁB
struct FCEventPriLess:public std::binary_function<const CEvent* ,const CEvent* ,bool>{
	bool operator()(const CEvent* lhs,const CEvent* rhs)const{
		return (
				(lhs->time() > rhs->time())
				|| ( (lhs->time() == rhs->time()) && (lhs->id() > rhs->id()) )
			);
	}
};


/////////////////////////////////////////////////
//			class CEventMaxsize
/////////////////////////////////////////////////
//�C�x���g�h���N���X���Œ蒷�������̈�Ƀ}�b�v���邽�߂̍ő�T�C�Y���`����_�~�[
//�T�C�Y���`���邾���ŃI�u�W�F�N�g���\�z����邱�Ƃ͂Ȃ��B
template<int T_size= 32>
class CEventMaxsize:public CEvent{
	CEventMaxsize():CEvent(0){}//�I�u�W�F�N�g�����֎~
protected:
	char work[T_size];//�_�~�[�̈�
public:
	//void onEvent() const����`���Ȃ�
};

/////////////////////////////////////////////////
//			class CEventManager
/////////////////////////////////////////////////
//�C�x���g�}�l�[�W��
class CEventManager:private boost::noncopyable{
	typedef std::priority_queue<
		const CEvent* ,
		std::vector<const CEvent* >,
		FCEventPriLess> table_t;//�C�x���g�e�[�u���̓q�[�v

	table_t table_;//�D�揇�ʕt�e�[�u��

	static CEventManager* p_instance_;//�C���X�^���X���ǂ�����ł��擾�ł���悤��static
	CEventManager(){};

public:

	~CEventManager();
	inline const CEvent* const getNextEvent();//���C�x���g�̎擾
	inline void setEvent(const CEvent* const p_event);//�C�x���g���e�[�u���ɒǉ�
	bool is_empty() const { return table_.empty(); }

	inline static CEventManager* getInstance();//�C�x���g�}�l�[�W���̃C���X�^���X���擾
};

//���̃C�x���g��Ԃ�
inline
const CEvent* const CEventManager::getNextEvent(){
	const CEvent* const p_event=table_.top();
	table_.pop();
	return p_event;
}

//�C�x���g���e�[�u���ɒǉ�����
//��{��time�D��œ�������id�D��
inline
void CEventManager::setEvent(const CEvent* const p_event){
#if DEBUG_EVENT_TRACE
    std::cout<<"CEventManager::setEvent():"<<(typeid(*p_event)).name()<<std::endl;
#endif
	table_.push(p_event);//�D�揇�ʂ͔�r�֐����I�[�o�[���[�h
}

//�ǂ�����ł��C���X�^���X���擾�ł���悤�ɂ��邽�߂�static�֐�
inline
CEventManager* CEventManager::getInstance(){
	if(p_instance_){
		return p_instance_;//2��ڈȍ~�C���X�^���X�̃|�C���^��Ԃ�
	}else{
		//����̂�private�R���X�g���N�^���N������static�t�B�[���h��������
		return (p_instance_=new CEventManager());
	}
}


/////////////////////////////////////////////////
//			class CStatManager
/////////////////////////////////////////////////
class CStatManager:private boost::noncopyable{
public:
	typedef CStatable::group_t group_t;
	enum { NONE=0,ALL=0xFFFF };//���ׂẴO���[�v��\���t���O
private:
	CEventManager* const p_event_manager_;
	const simutime_t interval_;//���̓��v�C�x���g�܂ł̊Ԋu
	typedef std::vector<CStatable* > table_t;//���v���Ƃ�I�u�W�F�N�g�̃e�[�u��
	table_t table_;
private:

public:
	CStatManager(CEventManager* const p_event_manager,const simutime_t interval)
		:p_event_manager_(p_event_manager),interval_(interval){}
	~CStatManager(){ table_.clear();}
	void add(CStatable* const p_obj,const group_t group);
	void onStat(const CEvent* const p_event);
};
/////////////////////////////////////////////////
//			class CEventStat
/////////////////////////////////////////////////
class CEventStat:public CEvent{
	typedef CStatable::group_t group_t;

	CStatManager* const p_stat_manager_;
	const group_t group_;
public:
	CEventStat(const simutime_t time,CStatManager* const p_stat_manager,const group_t group)
		:CEvent(time),p_stat_manager_(p_stat_manager),group_(group){}
	~CEventStat(){}
	void onEvent() const;
	const group_t group() const{ return group_;}
};

inline void CEventStat::onEvent() const{
	//���v�C�x���g�͓��v�}�l�[�W���̓��v�����n���h�����Ă�
	//���v�}�l�[�W���͓o�^����Ă���I�u�W�F�N�g�̓��v�����n���h�������ꂼ��Ă�
	p_stat_manager_->onStat(this);
}

/////////////////////////////////////////////////
//			class ISimuSetting
/////////////////////////////////////////////////
class ISimuSetting{
public:
	virtual ~ISimuSetting(){};
	virtual bool is_valid() const =0;
};


/////////////////////////////////////////////////
//			class ISimuProperty
/////////////////////////////////////////////////
//�V�~�����[�V�����ŗL�̏��������s�����߂̃C���^�[�t�F�C�X
class ISimuProperty:private boost::noncopyable{
public:
	virtual ~ISimuProperty(){}
	virtual bool init(ISimu* const p_simu)=0;
	virtual bool clean_up()=0;
};

/////////////////////////////////////////////////
//			class ISimu
/////////////////////////////////////////////////
//�V�~�����[�V�����C���^�[�t�F�C�X
class ISimu:private boost::noncopyable{
public:
	virtual ~ISimu(){};
	virtual bool init()=0;
	virtual bool start()=0;
	virtual bool end()=0;
};

/////////////////////////////////////////////////
//			class CSimu
/////////////////////////////////////////////////
//�V�~�����[�V�����̃��C���t���[�����[�N
//�V�~�����[�V�����̏������y�уC�x���g�̃R�[���o�b�N���s��
//�Ǝ��̃V�~�����[�V�������`����ꍇ��CSimu����h�������A
//ISimuProperty����h�������N���X�ŏ������y�эŏ��̃C�x���g�𐶐����������A
//����ISimuProperty�C���^�[�t�F�C�X��CSimu�̃R���X�g���N�^�ɓn��
class CSimu:public ISimu{
	ISimuProperty* const p_property_;
	bool run_flg_;
	CEventManager* p_event_manager_;
	
	static simutime_t time_;
public:
	CSimu(ISimuProperty* const p_property):p_property_(p_property),run_flg_(false){}
	virtual ~CSimu(){ clean_up(); }

	bool init();
	bool start();
	bool end();
	bool clean_up();

	static const simutime_t get_time(){ return time_; }
};


}//namespace simuframe

//end of simuframe.h
#endif //_SIMUFRAME_H__


