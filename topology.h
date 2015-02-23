#ifndef _TOPOLOGY_H__
#define _TOPOLOGY_H__
//topology.h
//ノードの接続関係を表すネットワークトポロジ情報や、ノード、リンクの保持する情報の定義

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
//ノード情報の定義
struct SNodeParam{
	typedef opsnetwork::node_index_t node_index_t;
	typedef opsnetwork::link_index_t link_index_t;
	typedef std::vector<link_index_t> link_index_vec_t;

	node_index_t index_;//ノード番号

	link_index_vec_t in_link_;//入力リンク番号
	link_index_vec_t out_link_;//出力リンク番号

	SNodeParam(const node_index_t index)
		:index_(index){}
};

/////////////////////////////////////////////////
//			struct SLinkParam
/////////////////////////////////////////////////
//リンク情報の定義
struct SLinkParam{
	typedef opsnetwork::node_index_t node_index_t;
	typedef opsnetwork::link_index_t link_index_t;
	typedef std::pair<node_index_t,node_index_t> node_pair_t;
	
	typedef double link_speed_t;
	typedef double link_dis_t;
	typedef double link_cost_t;
	link_index_t index_;//リンク番号
	node_pair_t pair_;//両端ノード(from,to)
	link_cost_t cost_;//コスト(速度とか距離から計算してもいい)
	link_speed_t speed_;//速度
	link_dis_t dis_;//距離
	
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

	size_t node_size_;//ノード数
	size_t link_size_;//リンク数
	node_param_vec_t node_;//ノード情報
	link_param_vec_t link_;//リンク情報
private:
	bool read();//トポロジ情報を読み込む
	void set_node_port();//エッジ情報からノードの入出力ポートを設定
public:
	//とりあえずファイルから読まずにテスト用に内部で適当に作成
	CTopology(const std::string& filename);
	bool is_valid() const{ return state_; }

	const node_param_vec_t get_node_param() const { return node_; }
	const link_param_vec_t get_link_param() const { return link_; }

};



/*
/////////////////////////////////////////////////
//			class ITopologyReader
/////////////////////////////////////////////////
//トポロジ情報の読み込みインターフェイス
class ITopologyReader{
public:
	virtual ~ITopologyReader(){}
	virtual bool read(CTopology& topology,const std::string& filename)=0;
};

/////////////////////////////////////////////////
//			class CTopologyReader
/////////////////////////////////////////////////
//トポロジ情報の読み込み
class CTopologyReader:public ITopologyReader{
public:
	bool read(CTopology& topology,const std::string& filename);
};
*/

}//end of namespace opsnetwork
//end of topology.h
#endif //_TOPOLOGY_H__

