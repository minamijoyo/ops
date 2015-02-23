#ifndef _TOPOLOGY_H__
#define _TOPOLOGY_H__
//topology.h
//�m�[�h�̐ڑ��֌W��\���l�b�g���[�N�g�|���W����A�m�[�h�A�����N�̕ێ�������̒�`

#include<utility>
#include<string>
#include<vector>

namespace opsnetwork{

struct SNodeParam;
struct SLinkParam;

typedef unsigned int node_index_t;
typedef unsigned int link_index_t;

/////////////////////////////////////////////////
//			struct SNodeParam
/////////////////////////////////////////////////
//�m�[�h���̒�`
struct SNodeParam{
	typedef opsnetwork::node_index_t node_index_t;
	typedef opsnetwork::link_index_t link_index_t;
	typedef std::vector<link_index_t> link_index_vec_t;

	node_index_t index_;//�m�[�h�ԍ�

	link_index_vec_t in_link_;//���̓����N�ԍ�
	link_index_vec_t out_link_;//�o�̓����N�ԍ�

	SNodeParam(const node_index_t index)
		:index_(index){}
};

/////////////////////////////////////////////////
//			struct SLinkParam
/////////////////////////////////////////////////
//�����N���̒�`
struct SLinkParam{
	typedef opsnetwork::node_index_t node_index_t;
	typedef opsnetwork::link_index_t link_index_t;
	typedef std::pair<node_index_t,node_index_t> node_pair_t;
	
	typedef double link_speed_t;
	typedef double link_dis_t;
	typedef double link_cost_t;
	link_index_t index_;//�����N�ԍ�
	node_pair_t pair_;//���[�m�[�h(from,to)
	link_cost_t cost_;//�R�X�g(���x�Ƃ���������v�Z���Ă�����)
	link_speed_t speed_;//���x
	link_dis_t dis_;//����
	
	SLinkParam(const link_index_t index,const node_pair_t& pair,const link_cost_t cost,
		const link_speed_t speed=0,const link_dis_t dis=0)
		:index_(index),pair_(pair),cost_(cost),speed_(speed),dis_(dis){}
};

/////////////////////////////////////////////////
//			class CTopology
/////////////////////////////////////////////////
class CTopology{
public:
	typedef opsnetwork::node_index_t node_index_t;
	typedef opsnetwork::link_index_t link_index_t;
	typedef std::vector<SNodeParam> node_param_vec_t;
	typedef std::vector<SLinkParam> link_param_vec_t;
private:
	const std::string filename_;
	bool state_;

	size_t node_size_;//�m�[�h��
	size_t link_size_;//�����N��
	node_param_vec_t node_;//�m�[�h���
	link_param_vec_t link_;//�����N���
private:
	bool read();//�g�|���W����ǂݍ���
	void set_node_port();//�G�b�W��񂩂�m�[�h�̓��o�̓|�[�g��ݒ�
public:
	//�Ƃ肠�����t�@�C������ǂ܂��Ƀe�X�g�p�ɓ����œK���ɍ쐬
	CTopology(const std::string& filename);
	bool is_valid() const{ return state_; }

	const node_param_vec_t get_node_param() const { return node_; }
	const link_param_vec_t get_link_param() const { return link_; }

};



/*
/////////////////////////////////////////////////
//			class ITopologyReader
/////////////////////////////////////////////////
//�g�|���W���̓ǂݍ��݃C���^�[�t�F�C�X
class ITopologyReader{
public:
	virtual ~ITopologyReader(){}
	virtual bool read(CTopology& topology,const std::string& filename)=0;
};

/////////////////////////////////////////////////
//			class CTopologyReader
/////////////////////////////////////////////////
//�g�|���W���̓ǂݍ���
class CTopologyReader:public ITopologyReader{
public:
	bool read(CTopology& topology,const std::string& filename);
};
*/

}//end of namespace opsnetwork
//end of topology.h
#endif //_TOPOLOGY_H__

