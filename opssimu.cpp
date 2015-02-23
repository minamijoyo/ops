//opssimu.cpp
#include"opssimu.h"

//デバッグ用
#include<iostream>

using namespace simuframe;
using namespace opssimu;
using namespace opsnetwork;

/////////////////////////////////////////////////
//			class COpsSimuProperty
/////////////////////////////////////////////////
bool COpsSimuProperty::init(ISimu* const p_simu){
	//トポロジの読み込み
	p_topology_=new CTopology(setting_.filename_);
	//ネットワークの設定を作成
	p_node_maker_=new_node_maker(setting_.node_type_);//ノード生成インターフェイス
	p_network_setting_=new COpsNetwork::SNetworkSetting(
		setting_.end_time_,setting_.lambda_,p_node_maker_,setting_.ttl_margin_,setting_.buf_th_,
		setting_.buf_sz_,setting_.seed_);

	//初期化チェック
	if(p_topology_==0 || p_node_maker_==0 || p_network_setting_==0)
		return false;
	//ネットワークの構築
	p_network_=new COpsNetwork(p_simu,p_topology_,p_network_setting_);
	return (p_network_!=0);
}

//ノードの種類フラグからノード生成インターフェイスを作成
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
	//ネットワークの解体
	delete p_network_;
	delete p_network_setting_;
	delete p_node_maker_;
	delete p_topology_;
	
	return true;
}
//end of opssimu.cpp

