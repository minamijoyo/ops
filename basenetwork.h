#ifndef _BASENETWORK_H__
#define _BASENETWORK_H__
//basenetwork.h
//���l�b�g���[�N�̒�`�B�m�[�h�ƃ����N�̑��݃A�N�Z�X��񋟂���B
//�m�[�h�A�����N�Ŕ�������C�x���g����R�[�������C���^�[�t�F�C�X�錾�B
#include"topology.h"
#include"fixedstack.h"
#include"statable.h"
#include<boost/graph/graph_traits.hpp>
#include<boost/graph/adjacency_list.hpp>
#include<boost/noncopyable.hpp>

#include<utility>//std::pair

//�O���錾
namespace simuframe{
class CEvent;
}
using simuframe::CEvent;

namespace opsnetwork{

//�O���錾
class CNode;
class CLink;
class IRoutingTable;
/////////////////////////////////////////////////
//			class CNetwork
/////////////////////////////////////////////////
class CNetwork:public simuframe::CStatable,private boost::noncopyable{
public:
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
		boost::no_property, boost::property<boost::edge_weight_t, SLinkParam::link_cost_t> > graph_t;
	typedef std::vector<CNode*> node_vec_t;
	typedef std::vector<CLink*> link_vec_t;
	typedef SNodeParam::node_index_t node_index_t;
	typedef SLinkParam::link_index_t link_index_t;

private:
	graph_t* p_graph_;//�����Ōo�H�T���Ƃ��Ɏg���O���t

protected:
	CTopology* const p_topology_;//�g�|���W���
	node_vec_t node_;//�m�[�h�|�C���^�z��
	link_vec_t link_;//�����N�|�C���^�z��

	void set_connection();//�m�[�h�ƃ����N�̑��݃R�l�N�V������ݒ肷��
public:
	CNetwork(CTopology* const p_topology);
	virtual ~CNetwork(){ delete p_graph_; }

	//�m�[�h�ƃ����N�ւ̃A�N�Z�X���
	const CNode* get_node(const node_index_t index) const { return node_[index]; }
	const CLink* get_link(const link_index_t index) const { return link_[index]; }
	
	CNode* get_node(const node_index_t index){ return node_[index]; }
	CLink* get_link(const link_index_t index){ return link_[index]; }

	const graph_t* const get_graph() const { return p_graph_; }
	graph_t* const get_graph(){ return p_graph_; }
	
};

/////////////////////////////////////////////////
//			class CNode
/////////////////////////////////////////////////
class CNode:public simuframe::CStatable,private boost::noncopyable{
public:
	typedef CNetwork::link_vec_t link_vec_t;
	typedef CNetwork::node_index_t node_index_t;
	typedef unsigned int port_t;

protected:
	CNetwork* const p_network_;//�l�b�g���[�N
	const node_index_t index_;//�m�[�h�ԍ�
	size_t in_link_num_;//���̓����N�̐�
	size_t out_link_num_;//�o�̓����N�̐�

	link_vec_t in_link_;//���̓����N
	link_vec_t out_link_;//�o�̓����N

	const IRoutingTable* p_routing_table_;//���[�`���O�e�[�u�� 

public:
	CNode(CNetwork* const p_network,const node_index_t index):p_network_(p_network),index_(index),
		in_link_num_(0),out_link_num_(0){}
	virtual ~CNode(){}

	virtual void onArrival(const CEvent* const p_event)=0;//��������
	virtual void onService(const CEvent* const p_event)=0;//�T�[�r�X����
	virtual void onDeparture(const CEvent* const p_event)=0;//�ދ�����
	
	void add_in_link(CLink* const p_link){ in_link_.push_back(p_link); in_link_num_++;}//���̓����N�ǉ�
	void add_out_link(CLink* const p_link){ out_link_.push_back(p_link); out_link_num_++;}//�o�̓����N�ǉ�
	//���o�̓����N�̍폜�͍��̂Ƃ�����Ȃ��̂ŕ��u
	//remove_in_link(),remove_out_link()
	CLink* get_out_link(const port_t port){ return out_link_[port]; }
};

/////////////////////////////////////////////////
//			class CLink
/////////////////////////////////////////////////
class CLink:public simuframe::CStatable,private boost::noncopyable{
public:
	typedef CNetwork::link_index_t link_index_t;
protected:
	typedef CNode::port_t port_t;
	CNetwork* const p_network_;//�l�b�g���[�N�ւ̃|�C���^
	const link_index_t index_;//�����N�ԍ�
	CNode* p_from_;//�����N�̓��͒[�m�[�h
	CNode* p_to_;//�����N�̏o�͒[�m�[�h
	port_t from_port_;//�m�[�h�̏o�̓|�[�g���ԂɂȂ����Ă��邩
	port_t to_port_;//�m�[�h�̓��̓|�[�g���ԂɂȂ����Ă��邩

public:
	CLink(CNetwork* const p_network,const link_index_t index):p_network_(p_network),index_(index){}
	virtual ~CLink(){}

	virtual void onArrival(const CEvent* const p_event)=0;//��������
	virtual void onDeparture(const CEvent* const p_event)=0;//�ދ�����

	void set_node(CNode* const p_from,CNode* const p_to){ p_from_=p_from; p_to_=p_to; }//���[�m�[�h�̐ݒ�
	void set_from_port(const port_t port){ from_port_=port; }
	void set_to_port(const port_t port){ to_port_=port; }
};

/////////////////////////////////////////////////
//			class IRoutingTable
/////////////////////////////////////////////////
//���[�`���O�e�[�u���C���^�[�t�F�C�X
class IRoutingTable{
public:
	typedef CNode::port_t port_t;
	typedef CFixedStack<port_t,16> path_t;//�o�H���̍ő�l���Œ肵�Ă��邱�Ƃɒ��ӁI�I
	//typedef std::stack<port_t> path_t;
protected:
	typedef CNetwork::graph_t graph_t;
	typedef CNetwork::node_index_t node_index_t;

	typedef std::vector<path_t> table_t;
public:
	virtual ~IRoutingTable(){}

	virtual const path_t& path(const node_index_t dis) const=0;//dis�ւ̌o�H��Ԃ��C���^�[�t�F�C�X
};

/////////////////////////////////////////////////
//			class CRoutingTableSP
/////////////////////////////////////////////////
//�ŒZ�o�H�p
class CRoutingTableSP:public IRoutingTable{
public:
	typedef SLinkParam::link_cost_t link_cost_t;
protected:
	CNetwork* const p_network_;
	graph_t* const p_graph_;
	const node_index_t src_;//���M���m�[�h
	size_t v_num_;//�m�[�h�̐�
	table_t sp_table_;//�ŒZ�o�H�e�[�u��
private:
	typedef std::vector<link_cost_t> distance_vec_t;
	distance_vec_t d_vec_;//�����}�b�v
	
	
private:
	void find_shortest_path();//�ŒZ�o�H�e�[�u���̍\�z
public:
	CRoutingTableSP(CNetwork* const p_network,const node_index_t src)
		:p_network_(p_network),p_graph_(p_network->get_graph()),src_(src),
		v_num_(num_vertices(*p_graph_)),sp_table_(v_num_),d_vec_(v_num_){ find_shortest_path(); }
	virtual ~CRoutingTableSP(){}
	const path_t& path(const node_index_t dis) const{ return sp_table_[dis]; }
	const link_cost_t distance_to(const node_index_t dis)const{ return d_vec_[dis]; }
};

/////////////////////////////////////////////////
//			class CRoutingTableSPAP
/////////////////////////////////////////////////
//�ŒZ�o�H+�I��o�H�p
class CRoutingTableSPAP:public CRoutingTableSP{
	typedef table_t ap_table_t;
	ap_table_t ap_table_;//��֌o�H�e�[�u��
private:
	void find_alternate_path();//��֌o�H�e�[�u���̍\�z
public:
	CRoutingTableSPAP(CNetwork* const p_network,const node_index_t src)
		:CRoutingTableSP(p_network,src),ap_table_(v_num_){ find_alternate_path(); }
		~CRoutingTableSPAP(){}
	const path_t& alternate_path(const node_index_t dis) const{ return ap_table_[dis]; }
};

/////////////////////////////////////////////////
//			class CRoutingTableAPall
/////////////////////////////////////////////////
//�ŒZ�o�H+���ׂẲI��o�H���
class CRoutingTableAPall:public CRoutingTableSP{
public:
	typedef std::pair<path_t,link_cost_t> path_cost_pair_t;
	typedef std::vector<path_cost_pair_t> path_all_t;
private:
	typedef std::vector<path_all_t> ap_table_t;

	ap_table_t ap_table_;//�I��o�H�e�[�u��
private:
	void find_all_alternate_path();//�o�H�T��
private:

	//path_cost_pair���R�X�g�̒Ⴂ���Ƀ\�[�g���邽�߂̔�r�t�@���N�^
	struct FPathCostPairLess:public std::binary_function
		<const path_cost_pair_t& ,const path_cost_pair_t& ,bool>
	{
		bool operator()(const path_cost_pair_t& lhs,const path_cost_pair_t& rhs)const{
			return lhs.second < rhs.second;
		}
	};
public:
	CRoutingTableAPall(CNetwork* const p_network,const node_index_t src)
		:CRoutingTableSP(p_network,src),ap_table_(v_num_){ find_all_alternate_path(); }
		~CRoutingTableAPall(){}
	//�����p�ɂƂ肠���������C���^�[�t�F�C�X�ɂ��Ă���
	const path_all_t& alternate_path_all(const node_index_t dis) const{ return ap_table_[dis]; }

};



}//end of namespace opsnetwork
//end of basenetwork.h
#endif //_BASENETWORK_H__

