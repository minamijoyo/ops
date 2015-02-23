//opssimu.cpp
#include"opssimu.h"

//�f�o�b�O�p
#include<iostream>

using namespace simuframe;
using namespace opssimu;
using namespace opsnetwork;

/////////////////////////////////////////////////
//			class COpsSimuProperty
/////////////////////////////////////////////////
bool COpsSimuProperty::init(ISimu* const p_simu){
	//�g�|���W�̓ǂݍ���
	p_topology_=new CTopology(setting_.filename_);
	//�l�b�g���[�N�̐ݒ���쐬
	p_node_maker_=new_node_maker(setting_.node_type_);//�m�[�h�����C���^�[�t�F�C�X
	p_network_setting_=new COpsNetwork::SNetworkSetting(
		setting_.end_time_,setting_.lambda_,p_node_maker_,setting_.ttl_margin_,setting_.buf_th_,
		setting_.buf_sz_,setting_.seed_);

	//�������`�F�b�N
	if(p_topology_==0 || p_node_maker_==0 || p_network_setting_==0)
		return false;
	//�l�b�g���[�N�̍\�z
	p_network_=new COpsNetwork(p_simu,p_topology_,p_network_setting_);
	return (p_network_!=0);
}

//�m�[�h�̎�ރt���O����m�[�h�����C���^�[�t�F�C�X���쐬
INodeMaker* COpsSimuProperty::new_node_maker(const int type){
	switch(type){
		case ENodeType::FIX:
			return new COpsNodeMaker();
			break;
		case ENodeType::REF:
			return new COpsNodeRefMaker();
			break;
		case ENodeType::DEF:
			return new COpsNodeDefMaker();
			break;
		case ENodeType::RDF:
			return new COpsNodeRefDefMaker();
			break;
		default:
			std::cerr<<"new_node_maker():unknown node type"<<std::endl;
			return 0;
			break;
	}
}

bool COpsSimuProperty::clean_up(){
	//�l�b�g���[�N�̉��
	delete p_network_;
	delete p_network_setting_;
	delete p_node_maker_;
	delete p_topology_;
	
	return true;
}
//end of opssimu.cpp

