#ifndef _BASENETWORK_H__
#define _BASENETWORK_H__
//basenetwork.h
//基底ネットワークの定義。ノードとリンクの相互アクセスを提供する。
//ノード、リンクで発生するイベントからコールされるインターフェイス宣言。
#include"topology.h"
#include"fixedstack.h"
#include"statable.h"
#include<boost/graph/graph_traits.hpp>
#include<boost/graph/adjacency_list.hpp>
#include<boost/noncopyable.hpp>

#include<utility>//std::pair

//前方宣言
namespace simuframe{
class CEvent;
}
using simuframe::CEvent;

namespace opsnetwork{

//前方宣言
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
	graph_t* p_graph_;//内部で経路探索とかに使うグラフ

protected:
	CTopology* const p_topology_;//トポロジ情報
	node_vec_t node_;//ノードポインタ配列
	link_vec_t link_;//リンクポインタ配列

	void set_connection();//ノードとリンクの相互コネクションを設定する
public:
	CNetwork(CTopology* const p_topology);
	virtual ~CNetwork(){ delete p_graph_; }

	//ノードとリンクへのアクセスを提供
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
	CNetwork* const p_network_;//ネットワーク
	const node_index_t index_;//ノード番号
	size_t in_link_num_;//入力リンクの数
	size_t out_link_num_;//出力リンクの数

	link_vec_t in_link_;//入力リンク
	link_vec_t out_link_;//出力リンク

	const IRoutingTable* p_routing_table_;//ルーチングテーブル 

public:
	CNode(CNetwork* const p_network,const node_index_t index):p_network_(p_network),index_(index),
		in_link_num_(0),out_link_num_(0){}
	virtual ~CNode(){}

	virtual void onArrival(const CEvent* const p_event)=0;//到着処理
	virtual void onService(const CEvent* const p_event)=0;//サービス処理
	virtual void onDeparture(const CEvent* const p_event)=0;//退去処理
	
	void add_in_link(CLink* const p_link){ in_link_.push_back(p_link); in_link_num_++;}//入力リンク追加
	void add_out_link(CLink* const p_link){ out_link_.push_back(p_link); out_link_num_++;}//出力リンク追加
	//入出力リンクの削除は今のとこいらないので放置
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
	CNetwork* const p_network_;//ネットワークへのポインタ
	const link_index_t index_;//リンク番号
	CNode* p_from_;//リンクの入力端ノード
	CNode* p_to_;//リンクの出力端ノード
	port_t from_port_;//ノードの出力ポート何番につながっているか
	port_t to_port_;//ノードの入力ポート何番につながっているか

public:
	CLink(CNetwork* const p_network,const link_index_t index):p_network_(p_network),index_(index){}
	virtual ~CLink(){}

	virtual void onArrival(const CEvent* const p_event)=0;//到着処理
	virtual void onDeparture(const CEvent* const p_event)=0;//退去処理

	void set_node(CNode* const p_from,CNode* const p_to){ p_from_=p_from; p_to_=p_to; }//両端ノードの設定
	void set_from_port(const port_t port){ from_port_=port; }
	void set_to_port(const port_t port){ to_port_=port; }
};

/////////////////////////////////////////////////
//			class IRoutingTable
/////////////////////////////////////////////////
//ルーチングテーブルインターフェイス
class IRoutingTable{
public:
	typedef CNode::port_t port_t;
	typedef CFixedStack<port_t,16> path_t;//経路長の最大値を固定していることに注意！！
	//typedef std::stack<port_t> path_t;
protected:
	typedef CNetwork::graph_t graph_t;
	typedef CNetwork::node_index_t node_index_t;

	typedef std::vector<path_t> table_t;
public:
	virtual ~IRoutingTable(){}

	virtual const path_t& path(const node_index_t dis) const=0;//disへの経路を返すインターフェイス
};

/////////////////////////////////////////////////
//			class CRoutingTableSP
/////////////////////////////////////////////////
//最短経路用
class CRoutingTableSP:public IRoutingTable{
public:
	typedef SLinkParam::link_cost_t link_cost_t;
protected:
	CNetwork* const p_network_;
	graph_t* const p_graph_;
	const node_index_t src_;//送信元ノード
	size_t v_num_;//ノードの数
	table_t sp_table_;//最短経路テーブル
private:
	typedef std::vector<link_cost_t> distance_vec_t;
	distance_vec_t d_vec_;//距離マップ
	
	
private:
	void find_shortest_path();//最短経路テーブルの構築
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
//最短経路+迂回経路用
class CRoutingTableSPAP:public CRoutingTableSP{
	typedef table_t ap_table_t;
	ap_table_t ap_table_;//代替経路テーブル
private:
	void find_alternate_path();//代替経路テーブルの構築
public:
	CRoutingTableSPAP(CNetwork* const p_network,const node_index_t src)
		:CRoutingTableSP(p_network,src),ap_table_(v_num_){ find_alternate_path(); }
		~CRoutingTableSPAP(){}
	const path_t& alternate_path(const node_index_t dis) const{ return ap_table_[dis]; }
};

/////////////////////////////////////////////////
//			class CRoutingTableAPall
/////////////////////////////////////////////////
//最短経路+すべての迂回経路候補
class CRoutingTableAPall:public CRoutingTableSP{
public:
	typedef std::pair<path_t,link_cost_t> path_cost_pair_t;
	typedef std::vector<path_cost_pair_t> path_all_t;
private:
	typedef std::vector<path_all_t> ap_table_t;

	ap_table_t ap_table_;//迂回経路テーブル
private:
	void find_all_alternate_path();//経路探索
private:

	//path_cost_pairをコストの低い順にソートするための比較ファンクタ
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
	//実験用にとりあえず同じインターフェイスにしておく
	const path_all_t& alternate_path_all(const node_index_t dis) const{ return ap_table_[dis]; }

};



}//end of namespace opsnetwork
//end of basenetwork.h
#endif //_BASENETWORK_H__

