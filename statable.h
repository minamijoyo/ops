#ifndef _STATABLE_H__
#define _STATABLE_H__
//statable.h
#include<functional>
namespace simuframe{

//�O���錾
class CEvent;

/////////////////////////////////////////////////
//			class CStatable
/////////////////////////////////////////////////
//���v���Ƃ�I�u�W�F�N�g�̃C���^�[�t�F�C�X
class CStatable{
public:
	typedef unsigned int group_t;
private:
	group_t group_;
public:
	CStatable(){}
	virtual ~CStatable(){}
	virtual void onStat(const CEvent* const p_event){}
//	virtual void onStat(const CEvent* const p_event) const{}

	const group_t group() const { return group_; }
	void set_group(const group_t group){ group_=group; }
};

//CStatable*��group���\�[�g�p�̔�r�t�@���N�^
struct FCStatableLess:public std::binary_function<const CStatable* ,const CStatable* ,bool>{
	bool operator()(const CStatable* lhs,const CStatable* rhs)const{
		return (lhs->group() < rhs->group());
	}
};

}//end of namespace simuframe
//end of statable.h
#endif //_STATABLE_H__


