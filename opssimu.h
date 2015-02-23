#ifndef _OPSSIMU_H__
#define _OPSSIMU_H__
//opssimu.h
#include<string>

#include"simuframe.h"
#include"opsnetwork.h"

namespace opssimu{
using simuframe::ISimu;
using simuframe::ISimuProperty;

/////////////////////////////////////////////////
//			enum ENodeType
/////////////////////////////////////////////////
//�m�[�h�̎�ނ�\���t���O
struct ENodeType{
	enum{ FIX=0,REF,DEF,RDF };//�Œ�A���ˁA�I��A���
};

/////////////////////////////////////////////////
//			class COpsSimuProperty
/////////////////////////////////////////////////
//�V�~�����[�V�����ŗL�̏�����
class COpsSimuProperty:public ISimuProperty{
public:
	struct SSimuSetting{
		std::string filename_;
		simuframe::simutime_t end_time_;
		int node_type_;
		double lambda_;
		double ttl_margin_;
		double buf_th_;
		unsigned int buf_sz_;
		unsigned int seed_;

		SSimuSetting(){}
		SSimuSetting(const std::string& filename,const simuframe::simutime_t end_time,
			const int node_type,const double lambda,const double ttl_margin,const double buf_th,
			const unsigned int buf_sz,const unsigned int seed){}
	};
private:
	SSimuSetting setting_;

	opsnetwork::CTopology* p_topology_;
	opsnetwork::COpsNetwork* p_network_;
	opsnetwork::INodeMaker* p_node_maker_;
	opsnetwork::COpsNetwork::SNetworkSetting* p_network_setting_;

public:
	COpsSimuProperty(const SSimuSetting& setting):setting_(setting),p_topology_(0),p_network_(0)
		,p_node_maker_(0),p_network_setting_(0){}
	~COpsSimuProperty(){}
	bool init(ISimu* const p_simu);//CSimu::init()����R�[�������
	bool clean_up();//CSimu::clean=up()����R�[�������
private:
	opsnetwork::INodeMaker* new_node_maker(const int type);//�m�[�h�����C���^�[�t�F�C�X
};

}//end of namespace opssimu
//end of opssimu.h
#endif //_OPSSIMU_H__

