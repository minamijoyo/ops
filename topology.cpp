//topology.cpp
#include<iostream>
#include<fstream>

#include"topology.h"

using namespace opsnetwork;

/////////////////////////////////////////////////
//			class CTopology
/////////////////////////////////////////////////
CTopology::CTopology(const std::string& filename):filename_(filename),state_(false){
	//�g�|���W���t�@�C������ǂݍ���
	if(state_=read()){
		//�m�[�h�̓��o�̓����N�ԍ���ݒ�
		set_node_port();
	}
}

//�s�ǂݔ�΂��}�N��
#define MACRO_SKEP_LINE() std::getline(ifs,line,'\n')
#define MACRO_SKEP_TO(str) do{std::getline(ifs,line,'\n');}while(line!= (str) )

//�g�|���W���t�@�C������ǂݍ���
bool CTopology::read(){
	std::ifstream ifs(filename_.c_str());
	if(!ifs.is_open()){//�t�@�C���I�[�v�����s
		std::cerr<<"error:CTopology::read():"<<filename_.c_str()
			<<"is not able to open."<<std::endl;
		return false;
	}else{//�t�@�C���I�[�v������
		std::string line;
		//�m�[�h��
		MACRO_SKEP_TO("%node_size");
		ifs>>node_size_;
		//�����N��
		MACRO_SKEP_TO("%link_size");
		ifs>>link_size_;

		//�m�[�h�ݒ�
		MACRO_SKEP_TO("%node_setting");
		MACRO_SKEP_LINE();//1�s�ǂݔ�΂�
		for(unsigned int i=0;i<node_size_;i++){
			SNodeParam::node_index_t index;
			ifs>>index;
			node_.push_back(SNodeParam(index));
		}
		//�����N�ݒ�
		MACRO_SKEP_TO("%link_setting");
		MACRO_SKEP_LINE();//1�s�ǂݔ�΂�
		for(unsigned int i=0;i<link_size_;i++){
			SLinkParam::link_index_t index;
			SNodeParam::node_index_t from,to;
			SLinkParam::link_cost_t cost;
			SLinkParam::link_speed_t speed;
			SLinkParam::link_dis_t distance;
			ifs>>index>>from>>to>>cost>>speed>>distance;
			speed=40;//�����I��40Gbps�Œ�
			cost+=100;//�����N�R�X�g���z�b�v���x�[�X�ɂ��邽�߂�100���Z
			typedef SLinkParam::node_pair_t node_pair_t;
			link_.push_back(SLinkParam(index,node_pair_t(from,to),cost,speed,distance));
		}
		//�t�@�C�������
		ifs.close();
	}
	return true;
	

/*
	//�e�X�g�p�ɂƂ肠�����K���Ƀg�|���W�����B
	SNodeParam v[]={SNodeParam(0),SNodeParam(1),SNodeParam(2),SNodeParam(3)};
	typedef SLinkParam::node_pair_t node_pair_t;
	SLinkParam e[]={SLinkParam(0,node_pair_t(0,1),1),
					SLinkParam(1,node_pair_t(1,2),1),
					SLinkParam(2,node_pair_t(2,3),1),
					SLinkParam(3,node_pair_t(3,0),1),
					SLinkParam(4,node_pair_t(1,0),1),
					SLinkParam(5,node_pair_t(2,1),1),
					SLinkParam(6,node_pair_t(3,2),1),
					SLinkParam(7,node_pair_t(0,3),1)};
	node_.insert(node_.end(),v,v+sizeof(v)/sizeof(v[0]));
	link_.insert(link_.end(),e,e+sizeof(e)/sizeof(e[0]));
	return true;
*/
	
	
}

#undef MACRO_SKEP_LINE
#undef MACRO_SKEP_TO

//�����N�̗��[�m�[�h��񂩂�m�[�h�̓��o�̓����N�ԍ���ݒ�
void CTopology::set_node_port(){
	typedef link_param_vec_t::iterator it_t;
	for(it_t it=link_.begin();it!=link_.end();++it){
		node_index_t from=it->pair_.first;
		node_index_t to=it->pair_.second;
		node_[from].out_link_.push_back(it->index_);
		node_[to].in_link_.push_back(it->index_);
	}
}
/*
/////////////////////////////////////////////////
//			class CTopologyReader
/////////////////////////////////////////////////
//�g�|���W�t�@�C���̓ǂݍ���
bool CTopologyReader::read(CTopology& topology,const std::string& filename){
	std::ifstream ifs(filename.c_str());
	if(!ifs.is_open()){
		std::cerr<<"error:CTopologyReader::read():"<<filename.c_str()
			<<"is not able to open."<<std::endl;
		return false;
	}else{
		std::string line;
		while(std::getline(ifs,line)=="%node_size");
		int node_size;
		ifs>>node_size;
		ifs.close();
	}
	return true;
		
}
*/
//end of topology.cpp


