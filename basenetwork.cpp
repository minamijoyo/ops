//basenetwork.cpp
#include"basenetwork.h"
#include<vector>
#include<algorithm>//std::sort
#include<iostream>//デバッグ用

#include <boost/config.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

using namespace opsnetwork;
using namespace boost;

#define LINK_COST_INF 10000000

/////////////////////////////////////////////////
//			class CNetwork
/////////////////////////////////////////////////
CNetwork::CNetwork(CTopology *const p_topology):p_graph_(0),p_topology_(p_topology){
	typedef std::vector<SLinkParam::node_pair_t> edge_array_t;
	typedef std::vector<SLinkParam::link_cost_t> cost_array_t;
	edge_array_t edge_array;
	cost_array_t cost_array;

	typedef CTopology::node_param_vec_t node_param_vec_t;
	typedef CTopology::link_param_vec_t link_param_vec_t;
	typedef CTopology::node_param_vec_t::const_iterator itn_t;
	typedef CTopology::link_param_vec_t::const_iterator itl_t;
	
	//ノード情報、リンク情報の取得
	const node_param_vec_t node_param = p_topology_->get_node_param();
	const link_param_vec_t link_param = p_topology_->get_link_param();

	//グラフ生成用のエッジ配列とコスト配列の生成
	for(itl_t itl=link_param.begin();itl!=link_param.end();++itl){
		edge_array.push_back(itl->pair_);
		cost_array.push_back(itl->cost_);
	}

	//経路探索などに使うグラフの生成
	p_graph_=new graph_t(edge_array.begin(),edge_array.end(),
			cost_array.begin(),node_param.size());
			
}

//ノードとリンクの相互コネクションを設定する。
void CNetwork::set_connection(){
	typedef CTopology::node_param_vec_t node_param_vec_t;
	typedef CTopology::link_param_vec_t link_param_vec_t;
	typedef node_vec_t::size_type n_size_t;
	typedef link_vec_t::size_type l_size_t;
	typedef SNodeParam::link_index_vec_t::size_type l_index_size_t;

	const node_param_vec_t node_param = p_topology_->get_node_param();
	const link_param_vec_t link_param = p_topology_->get_link_param();

	//ノードの入出力ポートの設定
	for(n_size_t n=0;n<node_.size();++n){
		for(l_index_size_t li=0;li<node_param[n].in_link_.size();++li){
			CLink* p_link=get_link(node_param[n].in_link_[li]);
			node_[n]->add_in_link(p_link);
			p_link->set_to_port(li);
		}
		for(l_index_size_t lo=0;lo<node_param[n].out_link_.size();++lo){
			CLink* p_link=get_link(node_param[n].out_link_[lo]);
			node_[n]->add_out_link(p_link);
			p_link->set_from_port(lo);
		}
	}
	//リンクの両端ノードの設定
	for(l_size_t l=0;l<link_.size();++l){
		link_[l]->set_node(get_node(link_param[l].pair_.first),
			get_node(link_param[l].pair_.second));
	}

}


/////////////////////////////////////////////////
//			class CRoutingTableSP
/////////////////////////////////////////////////
//最短経路探索
void CRoutingTableSP::find_shortest_path(){
	typedef graph_traits<graph_t>::vertex_descriptor vertex_descriptor;
	typedef graph_traits<graph_t>::edge_descriptor edge_descriptor;

	graph_t& g= *p_graph_;
	std::vector<vertex_descriptor> p(v_num_);//親を記録するベクタ
	const vertex_descriptor s = vertex(src_, g);//探索開始点
	//ダイクストラ法による最短経路探索
	//p[]に親ノード、d[]に距離が記録される
	dijkstra_shortest_paths(g, s, predecessor_map(&p[0]).distance_map(&d_vec_[0]));	
	
	graph_traits <graph_t>::vertex_iterator vi, vend;
	//デバッグ用

	//std::cout<<"node:"<<src_<<"\n\n";

	/*
	std::cout << "distances and parents:" << std::endl;
	

	for (tie(vi, vend) = vertices(g); vi != vend; ++vi) {
		std::cout << "distance(" << *vi << ") = " << d[*vi] << ", ";
		std::cout << "parent(" << *vi << ") = " << p[*vi] << std::endl;
	}
*/
	
	//最短路の親を辿りながらポート番号を記録
	//std::vector< std::vector<vertex_descriptor> > route_n(v_num_);//デバッグ用に親経路を記録
	graph_traits< graph_t >::vertex_iterator vbegin;
	tie(vbegin, vend) = vertices(g);
	vertex_descriptor t=0;//tは経路を設定する宛先ノード
	vi=vbegin;
	for (; vi != vend; ++t,++vi) {//全ての宛先ノードについてループ
		vertex_descriptor j=t;//jはソースsからターゲットtまでの探索中のノード
		while(j!=s){
			//親ノードを取得
			vertex_descriptor parent=p[j];
			//親ノードの出力リンクを調べる
			graph_traits<graph_t>::out_edge_iterator out_i, out_begin,out_end;
			tie(out_begin, out_end) = out_edges(parent, g);
			port_t port=0;
			out_i=out_begin;
			for (;out_i != out_end; ++port,++out_i){
				//親ノードの出力リンクから現在のjにつながっているリンクを探す
				if(j==target(*out_i,g)){
					//経路に出力ポートを追加
					sp_table_[t].push(port);
/*
					//同じコストの経路を分散して選択するように、わずかにコストを増加させる。
					SLinkParam::link_cost_t cost=get(edge_weight,g,*out_i);
					cost+=0.0001;
					put(edge_weight,g,*out_i,cost);
*/
					break;
				}
			}
			//親ノードへ移動
			j=parent;
			//route_n[t].push_back(parent);//デバッグ用に親ノードを記録
		}
		//デバッグ出力
		/*
		std::cout<<t<<"->"<<s<<" node:";
		for(std::vector<vertex_descriptor>::iterator vpi=route_n[t].begin();vpi!=route_n[t].end();++vpi){
			std::cout<<*vpi<<",";
		}
		std::cout<<std::endl;
*/
		/*
		std::cout<<t<<"->"<<s<<" port:";
		for(std::vector<port_t>::iterator pi=route[t].begin();pi!=route[t].end();++pi){
			std::cout<<*pi<<",";
		}
		std::cout<<std::endl;


*/
		/*
		//デバッグ出力
		graph_traits<graph_t>::edge_iterator e_i,e_end;
		tie(e_i,e_end)=edges(g);
		for(;e_i!=e_end;++e_i){
			std::cout<<get(edge_weight,g,*e_i)<<",\t";
		}
		std::cout<<std::endl;
		*/
	}
	

}


/////////////////////////////////////////////////
//			class CRoutingTableSPAP
/////////////////////////////////////////////////
//代替経路の構築
//代替経路選択ポリシー：最短経路と1ホップ目が異なる経路の中で最短のもの
//経路がひとつしか存在しない場合は代替経路と最短経路は等しくなる
void CRoutingTableSPAP::find_alternate_path(){
	graph_t& g= *p_graph_;

	//すべての宛先について
	for(size_t dis=0;dis<v_num_;dis++){
		if(src_!=dis){//宛先が自分自身を除く
			//1ホップ目のリンクを選択
			const port_t first_port= path(dis).top();
			graph_traits<graph_t>::out_edge_iterator out_i, out_begin,out_end;
			tie(out_begin, out_end) = out_edges(src_, g);
			port_t port=0;
			out_i=out_begin;
			while(first_port!=port){
				++port;
				++out_i;
			}
			//迂回したいリンクの重みを一時的に増加
			const SLinkParam::link_cost_t cost=get(edge_weight,g,*out_i);
			SLinkParam::link_cost_t temp_cost= LINK_COST_INF ;
			put(edge_weight,g,*out_i,temp_cost);

			//ダイクストラ法による最短経路の探索
			CRoutingTableSP sp(p_network_,src_);
			ap_table_[dis]=sp.path(dis);

			//一時的に増加させた重みを元に戻す
			put(edge_weight,g,*out_i,cost);
		}
	}
	
}

/////////////////////////////////////////////////
//			class CRoutingTableAPall
/////////////////////////////////////////////////
//代替経路の構築
//すべての出力リンクから宛先への経路を探す
void CRoutingTableAPall::find_all_alternate_path(){
	graph_t& g= *p_graph_;

	//すべての宛先について
	for(size_t dis=0;dis<v_num_;dis++){
		if(src_!=dis){//宛先が自分自身を除く
			const port_t shortest_port= path(dis).top();
			//すべての出力リンクについて
			graph_traits<graph_t>::out_edge_iterator out_i, out_begin,out_end;
			tie(out_begin, out_end) = out_edges(src_, g);
			port_t port=0;
			out_i=out_begin;
			for(;out_i!=out_end;++port,++out_i){
				if(port==shortest_port) continue;//最短経路を除外

				//1ホップ先のノードを取得
				node_index_t next_hop_node=get(vertex_index,g,target(*out_i,g));
				//逆戻り禁止のために次ホップへのリンクの重みを一時的に増加
				const link_cost_t cost=get(edge_weight,g,*out_i);
				link_cost_t temp_cost= LINK_COST_INF ;
				put(edge_weight,g,*out_i,temp_cost);

				//1ホップ先からダイクストラ法による最短経路の探索
				CRoutingTableSP sp(p_network_,next_hop_node);

				link_cost_t al_cost=sp.distance_to(dis)+cost;
				if(al_cost < LINK_COST_INF ){//到達不可能でなければ
					path_t alternate=sp.path(dis);
					alternate.push(port);//現在のportを追加するとsrc_からの経路になる
					ap_table_[dis].push_back(path_cost_pair_t(alternate,al_cost));
				}
				//一時的に増加させた重みを元に戻す
				put(edge_weight,g,*out_i,cost);
			}
			//代替経路をコストの低い順にソートして保持
			std::sort(ap_table_[dis].begin(),ap_table_[dis].end(),FPathCostPairLess());

			/*
			//デバッグ用
			std::cout<<"src_:"<<src_<<",dis:"<<dis<<std::endl;
			for(path_all_t::iterator it=ap_table_[dis].begin();it!=ap_table_[dis].end();++it){
				std::cout<<"cost:"<<it->second<<",path:";
				path_t temp=it->first;
				while(!temp.empty()){
					std::cout<<temp.top()<<",";
					temp.pop();
				}
				std::cout<<std::endl;
			}
			*/
		}
	}
}
//end of basenetwork.cpp


