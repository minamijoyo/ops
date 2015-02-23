//packet.cpp
#include"packet.h"

//デバッグ用
#include<iostream>
#include<fstream>
#include<algorithm>

using namespace opsnetwork;
/*
//static member instance
boost::pool<> CPacket::packet_pool_(sizeof(CPacketMaxsize));


void CPacket::operator delete(void* p){ 
//        std::cout<<"debug:CPacket:delete():start"<<std::endl;
        packet_pool_.free(p); 
//        std::cout<<"debug:CPacket:delete():end"<<std::endl;
}
*/
/////////////////////////////////////////////////
//			CMisorderingChecker
/////////////////////////////////////////////////
//コンストラクタ
CMisorderingChecker::CMisorderingChecker(const node_index_t node_num):node_num_(node_num),
	id_matrix_(node_num,std::vector<pk_id_t>(node_num,0)),
	time_matrix_(node_num,std::vector<simutime_t>(node_num,0)),
	jitter_record_(0,500),
	pk_total_(0),pk_misordering_(0){
}

//デストラクタ
CMisorderingChecker::~CMisorderingChecker(){
	//とりあえずファイルに出力
	//std::ofstream ofs("result/jitter.csv");
	//copy(jitter_record_.begin(),jitter_record_.end(),
	//	std::ostream_iterator<unsigned int>(ofs,"\n"));
	/*
	std::cout<<"pk_misordering:"<<pk_misordering_<<std::endl;
	std::cout<<"misordering_ratio:"<<misordering_ratio()<<std::endl;
	if(jitter_record_.count()){
		std::cout<<"jitter_av:"<<jitter_record_.average()<<std::endl;
		std::cout<<"jitter_var:"<<jitter_record_.variance()<<std::endl;
		std::cout<<"jitter_max:"<<jitter_record_.max_element()<<std::endl;
		std::cout<<"jitter_0.50:"<<jitter_record_.percent_bound(0.5)<<std::endl;
		std::cout<<"jitter_0.95:"<<jitter_record_.percent_bound(0.95)<<std::endl;
		std::cout<<"jitter_0.99:"<<jitter_record_.percent_bound(0.99)<<std::endl;
	}else{
		std::cout<<"jitter_av:"<<0<<std::endl;
		std::cout<<"jitter_var:"<<0<<std::endl;
		std::cout<<"jitter_max:"<<0<<std::endl;
		std::cout<<"jitter_0.50:"<<0<<std::endl;
		std::cout<<"jitter_0.95:"<<0<<std::endl;
		std::cout<<"jitter_0.99:"<<0<<std::endl;
	}
	*/
}

//順序逆転をチェックする
void CMisorderingChecker::check(const CPacket* const p_packet){
	pk_total_++;
	const node_index_t src=p_packet->src();
	const node_index_t dis=p_packet->dis();
	const pk_id_t id=p_packet->id();
	const simutime_t now_time=simuframe::CSimu::get_time();

	//チェック
	if(id_matrix_[src][dis] < id){//順序が正しい
		//更新
		id_matrix_[src][dis]=id;
		time_matrix_[src][dis]=now_time;
	}else{//順序逆転
		pk_misordering_++;
		//ジッターを記録しない
		//jitter_record_.push(now_time-time_matrix_[src][dis]);
	}
}

/////////////////////////////////////////////////
//			CPacketManager
/////////////////////////////////////////////////
CPacketManager::CPacketManager(const node_index_t node_num,const seed_t seed)
	:node_num_(node_num),seed_(seed),stat_(node_num),p_rnd_size_(0),p_rnd_node_(0){
	//乱数ジェネレータの作成
	p_rnd_size_=new opssimu::CRandMtUni<double>(seed_);//seed,一様分布[0,1]
	p_rnd_node_=new opssimu::CRandMtUni<node_index_t>(seed_,node_num_);//seed,一様分布[0,node_num_-1]
}

CPacketManager::~CPacketManager(){
	//乱数ジェネレータの削除
	delete p_rnd_node_;
	delete p_rnd_size_;

	//デバッグ用
	
	std::cout<<"total_pk_new:"<<stat_.pk_new_<<std::endl;
	std::cout<<"total_pk_delete_ok:"<<stat_.pk_delete_ok_<<std::endl;
	std::cout<<"total_pk_delete_loss:"<<stat_.pk_delete_loss_<<std::endl;
	std::cout<<"total_pk_loss_ratio:"<<stat_.loss_ratio()<<std::endl;
	//std::cout<<"throughput:"<<stat_.throughput()<<std::endl;
	//std::cout<<"arrival_time_av:"<<stat_.arrival_time_av_<<std::endl;
	std::cout<<"total_hop_av:"<<stat_.hop_av()<<std::endl;
	std::cout<<"total_wating_time_av:"<<stat_.wating_time_av()<<std::endl;
	//特定のパケットの統計
	for(int i=0;i<PACKET_GROUP_MAX;i++){
		std::cout<<i<<"group_pk_delete_count:"<<stat_.group_pk_delete_[i]<<std::endl;
		std::cout<<i<<"group_pk_loss_ratio:"<<stat_.group_loss_ratio(i)<<std::endl;
		std::cout<<i<<"group_pk_delete_ratio:"<<stat_.group_delete_ratio(i)<<std::endl;
		std::cout<<i<<"group_hop_av:"<<stat_.group_hop_av(i)<<std::endl;
		std::cout<<i<<"group_wating_time_av:"<<stat_.group_wating_time_av(i)<<std::endl;
		std::cout<<i<<"group_hop_dist:";
		for(int j=0;j<PACKET_HOP_MAX;j++)
			std::cout<<stat_.group_hop_dist(i,j)<<",";
		std::cout<<std::endl;

		std::cout<<i<<"group_ref_hop_dist:";
		for(int j=0;j<PACKET_HOP_MAX;j++)
			std::cout<<stat_.group_ref_hop_dist(i,j)<<",";
		std::cout<<std::endl;
		
		std::cout<<i<<"group_def_hop_dist:";
		for(int j=0;j<PACKET_HOP_MAX;j++)
			std::cout<<stat_.group_def_hop_dist(i,j)<<",";
		std::cout<<std::endl;
		
		std::cout<<i<<"group_wating_time_dist:";
		for(int k=0;k<PACKET_WATING_DIST_MAX;k++)
			std::cout<<stat_.group_wating_time_dist(i,k)<<",";
		std::cout<<std::endl;
	}

	/*
	std::ofstream ofs("result/pk_loss.csv");
	copy(stat_.loss_ratio_vec_.begin(),stat_.loss_ratio_vec_.end(),
		std::ostream_iterator<double>(ofs,"\n"));
	*/
}

//パケットの生成
/*デバッグ版
CPacket* CPacketManager::packet_new(){
	stat_.pk_new_++;
	const simutime_t now=simuframe::CSimu::get_time();
	//平均到着間隔の更新
	//平均=合計時間/パケット数
	//で計算すると合計時間がものすごく大きくなってしまうので、差分を更新するように計算。
	stat_.arrival_time_av_+=
		(now-stat_.last_new_time_-stat_.arrival_time_av_)/stat_.pk_new_;
	stat_.last_new_time_=now;

	//送信元と宛先を決定
	node_index_t src=(*p_rnd_node_)();
	node_index_t dis=(*p_rnd_node_)();
	while(src==dis){//宛先が自分自身にならないように
		dis=(*p_rnd_node_)();
	}

	//パケット長
	double a=(*p_rnd_size_)();
	const pk_size_t pk_size=( a < 0.6)? (64*8) : (1500*8);
	//パケット生成
	return new CPacketRef(stat_.pk_new_,//id
						now,//時刻
						pk_size,//パケット長
						src,//送信元
						dis//宛先
					); 
}
*/


//統計処理
void CPacketManager::onStat(const simuframe::CEvent* const p_event){
	stat_.loss_ratio_vec_.push_back(stat_.loss_ratio());
}

//end of packet.cpp

