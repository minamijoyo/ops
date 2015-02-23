//basenetwork.cpp
#include"basenetwork.h"
#include<vector>
#include<algorithm>//std::sort
#include<iostream>//�f�o�b�O�p

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
	
	//�m�[�h���A�����N���̎擾
	const node_param_vec_t node_param = p_topology_->get_node_param();
	const link_param_vec_t link_param = p_topology_->get_link_param();

	//�O���t�����p�̃G�b�W�z��ƃR�X�g�z��̐���
	for(itl_t itl=link_param.begin();itl!=link_param.end();++itl){
		edge_array.push_back(itl->pair_);
		cost_array.push_back(itl->cost_);
	}

	//�o�H�T���ȂǂɎg���O���t�̐���
	p_graph_=new graph_t(edge_array.begin(),edge_array.end(),
			cost_array.begin(),node_param.size());
			
}

//�m�[�h�ƃ����N�̑��݃R�l�N�V������ݒ肷��B
void CNetwork::set_connection(){
	typedef CTopology::node_param_vec_t node_param_vec_t;
	typedef CTopology::link_param_vec_t link_param_vec_t;
	typedef node_vec_t::size_type n_size_t;
	typedef link_vec_t::size_type l_size_t;
	typedef SNodeParam::link_index_vec_t::size_type l_index_size_t;

	const node_param_vec_t node_param = p_topology_->get_node_param();
	const link_param_vec_t link_param = p_topology_->get_link_param();

	//�m�[�h�̓��o�̓|�[�g�̐ݒ�
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
	//�����N�̗��[�m�[�h�̐ݒ�
	for(l_size_t l=0;l<link_.size();++l){
		link_[l]->set_node(get_node(link_param[l].pair_.first),
			get_node(link_param[l].pair_.second));
	}

}


/////////////////////////////////////////////////
//			class CRoutingTableSP
/////////////////////////////////////////////////
//�ŒZ�o�H�T��
void CRoutingTableSP::find_shortest_path(){
	typedef graph_traits<graph_t>::vertex_descriptor vertex_descriptor;
	typedef graph_traits<graph_t>::edge_descriptor edge_descriptor;

	graph_t& g= *p_graph_;
	std::vector<vertex_descriptor> p(v_num_);//�e���L�^����x�N�^
	const vertex_descriptor s = vertex(src_, g);//�T���J�n�_
	//�_�C�N�X�g���@�ɂ��ŒZ�o�H�T��
	//p[]�ɐe�m�[�h�Ad[]�ɋ������L�^�����
	dijkstra_shortest_paths(g, s, predecessor_map(&p[0]).distance_map(&d_vec_[0]));	
	
	graph_traits <graph_t>::vertex_iterator vi, vend;
	//�f�o�b�O�p

	//std::cout<<"node:"<<src_<<"\n\n";

	/*
	std::cout << "distances and parents:" << std::endl;
	

	for (tie(vi, vend) = vertices(g); vi != vend; ++vi) {
		std::cout << "distance(" << *vi << ") = " << d[*vi] << ", ";
		std::cout << "parent(" << *vi << ") = " << p[*vi] << std::endl;
	}
*/
	
	//�ŒZ�H�̐e��H��Ȃ���|�[�g�ԍ����L�^
	//std::vector< std::vector<vertex_descriptor> > route_n(v_num_);//�f�o�b�O�p�ɐe�o�H���L�^
	graph_traits< graph_t >::vertex_iterator vbegin;
	tie(vbegin, vend) = vertices(g);
	vertex_descriptor t=0;//t�͌o�H��ݒ肷�鈶��m�[�h
	vi=vbegin;
	for (; vi != vend; ++t,++vi) {//�S�Ă̈���m�[�h�ɂ��ă��[�v
		vertex_descriptor j=t;//j�̓\�[�Xs����^�[�Q�b�gt�܂ł̒T�����̃m�[�h
		while(j!=s){
			//�e�m�[�h���擾
			vertex_descriptor parent=p[j];
			//�e�m�[�h�̏o�̓����N�𒲂ׂ�
			graph_traits<graph_t>::out_edge_iterator out_i, out_begin,out_end;
			tie(out_begin, out_end) = out_edges(parent, g);
			port_t port=0;
			out_i=out_begin;
			for (;out_i != out_end; ++port,++out_i){
				//�e�m�[�h�̏o�̓����N���猻�݂�j�ɂȂ����Ă��郊���N��T��
				if(j==target(*out_i,g)){
					//�o�H�ɏo�̓|�[�g��ǉ�
					sp_table_[t].push(port);
/*
					//�����R�X�g�̌o�H�𕪎U���đI������悤�ɁA�킸���ɃR�X�g�𑝉�������B
					SLinkParam::link_cost_t cost=get(edge_weight,g,*out_i);
					cost+=0.0001;
					put(edge_weight,g,*out_i,cost);
*/
					break;
				}
			}
			//�e�m�[�h�ֈړ�
			j=parent;
			//route_n[t].push_back(parent);//�f�o�b�O�p�ɐe�m�[�h���L�^
		}
		//�f�o�b�O�o��
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
		//�f�o�b�O�o��
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
//��֌o�H�̍\�z
//��֌o�H�I���|���V�[�F�ŒZ�o�H��1�z�b�v�ڂ��قȂ�o�H�̒��ōŒZ�̂���
//�o�H���ЂƂ������݂��Ȃ��ꍇ�͑�֌o�H�ƍŒZ�o�H�͓������Ȃ�
void CRoutingTableSPAP::find_alternate_path(){
	graph_t& g= *p_graph_;

	//���ׂĂ̈���ɂ���
	for(size_t dis=0;dis<v_num_;dis++){
		if(src_!=dis){//���悪�������g������
			//1�z�b�v�ڂ̃����N��I��
			const port_t first_port= path(dis).top();
			graph_traits<graph_t>::out_edge_iterator out_i, out_begin,out_end;
			tie(out_begin, out_end) = out_edges(src_, g);
			port_t port=0;
			out_i=out_begin;
			while(first_port!=port){
				++port;
				++out_i;
			}
			//�I�񂵂��������N�̏d�݂��ꎞ�I�ɑ���
			const SLinkParam::link_cost_t cost=get(edge_weight,g,*out_i);
			SLinkParam::link_cost_t temp_cost= LINK_COST_INF ;
			put(edge_weight,g,*out_i,temp_cost);

			//�_�C�N�X�g���@�ɂ��ŒZ�o�H�̒T��
			CRoutingTableSP sp(p_network_,src_);
			ap_table_[dis]=sp.path(dis);

			//�ꎞ�I�ɑ����������d�݂����ɖ߂�
			put(edge_weight,g,*out_i,cost);
		}
	}
	
}

/////////////////////////////////////////////////
//			class CRoutingTableAPall
/////////////////////////////////////////////////
//��֌o�H�̍\�z
//���ׂĂ̏o�̓����N���父��ւ̌o�H��T��
void CRoutingTableAPall::find_all_alternate_path(){
	graph_t& g= *p_graph_;

	//���ׂĂ̈���ɂ���
	for(size_t dis=0;dis<v_num_;dis++){
		if(src_!=dis){//���悪�������g������
			const port_t shortest_port= path(dis).top();
			//���ׂĂ̏o�̓����N�ɂ���
			graph_traits<graph_t>::out_edge_iterator out_i, out_begin,out_end;
			tie(out_begin, out_end) = out_edges(src_, g);
			port_t port=0;
			out_i=out_begin;
			for(;out_i!=out_end;++port,++out_i){
				if(port==shortest_port) continue;//�ŒZ�o�H�����O

				//1�z�b�v��̃m�[�h���擾
				node_index_t next_hop_node=get(vertex_index,g,target(*out_i,g));
				//�t�߂�֎~�̂��߂Ɏ��z�b�v�ւ̃����N�̏d�݂��ꎞ�I�ɑ���
				const link_cost_t cost=get(edge_weight,g,*out_i);
				link_cost_t temp_cost= LINK_COST_INF ;
				put(edge_weight,g,*out_i,temp_cost);

				//1�z�b�v�悩��_�C�N�X�g���@�ɂ��ŒZ�o�H�̒T��
				CRoutingTableSP sp(p_network_,next_hop_node);

				link_cost_t al_cost=sp.distance_to(dis)+cost;
				if(al_cost < LINK_COST_INF ){//���B�s�\�łȂ����
					path_t alternate=sp.path(dis);
					alternate.push(port);//���݂�port��ǉ������src_����̌o�H�ɂȂ�
					ap_table_[dis].push_back(path_cost_pair_t(alternate,al_cost));
				}
				//�ꎞ�I�ɑ����������d�݂����ɖ߂�
				put(edge_weight,g,*out_i,cost);
			}
			//��֌o�H���R�X�g�̒Ⴂ���Ƀ\�[�g���ĕێ�
			std::sort(ap_table_[dis].begin(),ap_table_[dis].end(),FPathCostPairLess());

			/*
			//�f�o�b�O�p
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


