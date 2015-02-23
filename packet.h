#ifndef _PACKET_H__
#define _PACKET_H__
//packet.h
//パケットの定義

#include<vector>
#include<deque>
//#include<boost/pool/pool.hpp>
#include<boost/noncopyable.hpp>
#include"simuframe.h"
#include"topology.h"
#include"mtrandom.h"
#include"fixedstack.h"
#include"histogram.h"

//デバッグ用
#include<iostream>

//統計用にパケットのグループ数
#define PACKET_GROUP_MAX 4

//統計用のホップ数の上限
#define PACKET_HOP_MAX 16
//統計用の系内時間分布の分割数
#define PACKET_WATING_DIST_MAX 250
//パケットサイズ固定12000bit=1500Byte
//可変サイズパケットを生成する場合は、CPacketManager::packet_new()を変更
//CPacketBuf::insert_ok_th()が固定パケットを暗黙に仮定しているので要修正
#define CONST_PK_SIZE 12000

namespace opsnetwork{

/////////////////////////////////////////////////
//			CPacket
/////////////////////////////////////////////////
class CPacket{
//	static boost::pool<> packet_pool_;//パケット用メモリ管理
public:
	typedef unsigned long pk_id_t;
	typedef simuframe::simutime_t simutime_t;
	typedef unsigned int pk_size_t;
	typedef CTopology::node_index_t node_index_t;
	typedef unsigned char ttl_t;
	typedef unsigned char hop_t;
	typedef unsigned char group_t;

protected:
	const pk_id_t id_;//ID
	const simutime_t time_;//生成時刻
	const pk_size_t size_;//サイズ
	const node_index_t src_;//送信元
	const node_index_t dis_;//宛先
	ttl_t ttl_;//1ホップ進む毎に減らされる
	hop_t hop_;//1ホップ進む度に増やされる
	hop_t sp_hop_;
	group_t group_;//統計用に特定の条件のパケットに印を付けたいときに使う
public:
	CPacket(const pk_id_t id,const simutime_t time,const pk_size_t size,
		const node_index_t src,const node_index_t dis)
		:id_(id),time_(time),size_(size),src_(src),dis_(dis),ttl_(255),hop_(0),group_(0){
        //std::cout<<"debug:CPacket:CPacket()"<<std::endl;
    }
	virtual ~CPacket(){
        //std::cout<<"debug:CPacket:~CPacket()"<<std::endl;
}
	
	//メモリ確保及び開放
/*	void* operator new(size_t t){
//        std::cout<<"debug:CPacket:new():start"<<std::endl;
        return packet_pool_.malloc();
//        std::cout<<"debug:CPacket:new():end"<<std::endl;
    }

	void operator delete(void* p){ 
        std::cout<<"debug:CPacket:delete():start"<<std::endl;
        packet_pool_.free(p); 
//        std::cout<<"debug:CPacket:delete():end"<<std::endl;
}

    void operator delete(void* p);//コンパイラのinline最適化バグのため明示的にoutline
*/
	const pk_id_t id() const { return id_; }
	const simutime_t time() const { return time_;}
	const pk_size_t size() const { return size_; }
	const node_index_t src() const { return src_; }
	const node_index_t dis() const { return dis_; }
	
	const ttl_t ttl() const { return ttl_; }
	const hop_t hop() const { return hop_; }
	
	void set_ttl(const ttl_t ttl){ ttl_=ttl; }
	void dec_ttl_inc_hop(){ ttl_--;hop_++; }
	
	void set_sp_hop(const hop_t sp_hop){ sp_hop_=sp_hop; }
	const hop_t sp_hop() const { return sp_hop_; }

	const group_t group() const { return group_; }
	//void set_group(const group_t group){ group_=group; }
	void set_group_flg(const group_t group){ group_ |= group; }//ビット和

};


/////////////////////////////////////////////////
//			CPacketPortStack
/////////////////////////////////////////////////
//出力ポートスタックを用いたソースルーチング用パケット
class CPacketPortStack:public CPacket{
public:
	typedef unsigned int port_t;//出力ポート番号
protected:
	typedef CFixedStack<port_t,PACKET_HOP_MAX> path_t;//ヒープにメモリを要求しないサイズ固定スタック

	path_t path_;//経路上の出力ポートのスタック	
public:
	CPacketPortStack(const pk_id_t id,const simutime_t time,const pk_size_t size,
		const node_index_t src,const node_index_t dis)
		:CPacket(id,time,size,src,dis){}
	virtual ~CPacketPortStack(){}
	
	bool empty() const { return path_.empty(); }
	void push_port(const port_t port){ path_.push(port); }//ポートをpush
	void pop_port(){ path_.pop(); }//ポートをpop
	const port_t top_port(){ return path_.top(); }//先頭のポートを返す
	const size_t path_size() const { return path_.size(); }//残りホップ数を返す

	void set_path(const path_t& path){ path_=path; }
	
};

/////////////////////////////////////////////////
//			CPacketRef
/////////////////////////////////////////////////
class CPacketRef:public CPacketPortStack{
public:
	enum ERefFlg{ NONE=0,REF_FWD=1,REF_RET=3 };//反射フラグ
private:
	ERefFlg ref_flg_;//反射状態フラグ
	pk_id_t ref_id_;//反射パケット識別子
	int ref_count_;//反射回数
public:
	CPacketRef(const pk_id_t id,const simutime_t time,const pk_size_t size,
		const node_index_t src,const node_index_t dis)
		:CPacketPortStack(id,time,size,src,dis),ref_flg_(NONE),ref_id_(0),ref_count_(0){
    //    std::cout<<"debug:CPacketRef:CPacketRef()"<<std::endl;
}
	~CPacketRef(){
    //std::cout<<"debug:CPacketRef:~CPacketRef()"<<std::endl;
}

	const ERefFlg ref_flg() const { return ref_flg_; }
	void set_ref_flg(const ERefFlg ref_flg){ ref_flg_=ref_flg; }
	const pk_id_t ref_id() const { return ref_id_; }
	void set_ref_id(const pk_id_t ref_id){ ref_id_=ref_id; }
	void inc_count(){ ref_count_++; }
	int ref_count() const { return ref_count_; }
};

/////////////////////////////////////////////////
//			CMisorderingChecker
/////////////////////////////////////////////////
//パケットの順序逆転をチェックする
class CMisorderingChecker:private boost::noncopyable{
	typedef CPacket::pk_id_t pk_id_t;
	typedef CPacket::simutime_t simutime_t;
	typedef CPacket::node_index_t node_index_t;
	typedef std::vector<std::vector<pk_id_t> > id_matrix_t;
	typedef std::vector<std::vector<simutime_t> > time_matrix_t;
	typedef CHistogram<simutime_t,1000> jitter_record_t;
private:
	const node_index_t node_num_;//ノード数
	id_matrix_t id_matrix_;//最後に受信したパケットIDをsrc-disペア単位で保持
	time_matrix_t time_matrix_;//最後に受信した時刻をsrc-disペア単位で保持
	jitter_record_t jitter_record_;//順序逆転した場合のジッターをすべて記録
	pk_id_t pk_total_;//入力された全パケット数
	pk_id_t pk_misordering_;//順序逆転パケット数
public:
	CMisorderingChecker(const node_index_t node_num);
	~CMisorderingChecker();

	void check(const CPacket* const p_packet);
	double misordering_ratio()const{ return static_cast<double>(pk_misordering_)/pk_total_; }
};


/////////////////////////////////////////////////
//			CPacketManager
/////////////////////////////////////////////////
//パケットの生成new、削除delete、廃棄lossの管理
class CPacketManager:public simuframe::CStatable,private boost::noncopyable{
public:
	typedef CPacket::pk_id_t pk_id_t;
	typedef CPacket::pk_size_t pk_size_t;
	typedef CPacket::simutime_t simutime_t;
	typedef CPacket::node_index_t node_index_t;
	typedef opssimu::seed_t seed_t;
private:
	const node_index_t node_num_;
	const seed_t seed_;//乱数の種
	
	//統計データ用の構造体
	struct CPkMgrStat{
		pk_id_t pk_new_;//生成したパケット数
		pk_id_t pk_delete_ok_;//正しく伝送したパケット数
		pk_id_t pk_delete_loss_;//廃棄したパケット数
		pk_id_t pk_delete_;//伝送完了or廃棄したパケット数=pk_delete_ok+pk_delete_loss
		
		pk_id_t group_pk_delete_ok_[PACKET_GROUP_MAX];//特定のパケットの伝送成功数
		pk_id_t group_pk_delete_loss_[PACKET_GROUP_MAX];
		pk_id_t group_pk_delete_[PACKET_GROUP_MAX];

		simutime_t last_new_time_;//最後にパケットをnewした時刻
		double arrival_time_av_;//平均到着間隔
		double wating_time_av_;//平均系内滞在時間
		double wating_time_sum_;//系内滞在時間の合計
		double group_wating_time_sum_[PACKET_GROUP_MAX];//特定のパケットの系内時間の合計
		CHistogram<simutime_t,PACKET_WATING_DIST_MAX> group_wating_time_dist_[PACKET_GROUP_MAX];//系内時間の分布
		
		double hop_sum_;
		double group_hop_sum_[PACKET_GROUP_MAX];//ホップ数合計
		double group_hop_dist_[PACKET_GROUP_MAX][PACKET_HOP_MAX];//ホップ数の分布
		double group_def_hop_dist_[PACKET_GROUP_MAX][PACKET_HOP_MAX];//迂回ホップ数の分布
		double group_ref_hop_dist_[PACKET_GROUP_MAX][PACKET_HOP_MAX];//反射ホップ数の分布

		double pk_size_sum_ok_;//伝送成功パケットサイズの合計
		double pk_size_sum_;//削除パケットサイズの合計=伝送要求パケットサイズ(ok+loss)

		std::vector<double> loss_ratio_vec_;
		CMisorderingChecker misordering_checker_;//順序逆転チェック
		
		//コンストラクタ
		CPkMgrStat(const node_index_t node_num):pk_new_(0),pk_delete_ok_(0),pk_delete_loss_(0),pk_delete_(0),
			last_new_time_(0),arrival_time_av_(0),wating_time_av_(0),wating_time_sum_(0),
			hop_sum_(0),pk_size_sum_ok_(0),pk_size_sum_(0),misordering_checker_(node_num){
				for(int i=0;i<PACKET_GROUP_MAX;i++){
					group_pk_delete_ok_[i]=0;
					group_pk_delete_loss_[i]=0;
					group_pk_delete_[i]=0;
					group_hop_sum_[i]=0;
					group_wating_time_sum_[i]=0;
					group_wating_time_dist_[i].init(0,100000);
					for(int j=0;j<PACKET_HOP_MAX;j++){
						group_hop_dist_[i][j]=0;
						group_def_hop_dist_[i][j]=0;
						group_ref_hop_dist_[i][j]=0;
					}
				}
			}
		//廃棄率
		double loss_ratio()const{ 
				return (pk_delete_==0)? 0 : ((double)pk_delete_loss_/pk_delete_);
		}
		double group_loss_ratio(int i) const{
			return (pk_delete_==0)? 0 : ((double)group_pk_delete_loss_[i]/pk_delete_);
		}
		double group_delete_ratio(int i) const{
			return (pk_delete_==0)? 0 : ((double)group_pk_delete_[i]/pk_delete_);
		}

		//ホップ数
		double hop_av()const{
			return (pk_delete_ok_==0)? 0 : (hop_sum_/pk_delete_ok_);
		}
		double group_hop_av(int i)const{
			return (group_pk_delete_ok_[i]==0)? 0 : (group_hop_sum_[i]/group_pk_delete_ok_[i]);
		}
		double group_hop_dist(int i, int j)const{
			return (group_pk_delete_ok_[i]==0)? 0 : (group_hop_dist_[i][j]/group_pk_delete_ok_[i]);
		}
		double group_def_hop_dist(int i, int j)const{
			return (group_pk_delete_ok_[i]==0)? 0 : (group_def_hop_dist_[i][j]/group_pk_delete_ok_[i]);
		}
		double group_ref_hop_dist(int i, int j)const{
			return (group_pk_delete_ok_[i]==0)? 0 : (group_ref_hop_dist_[i][j]/group_pk_delete_ok_[i]);
		}

		//スループット
		//double throughput()const { return (pk_size_sum_ok_/simuframe::CSimu::get_time())/1000.0; }//Gbps
		//double throughput()const { return pk_size_sum_ok_/pk_size_sum_; }//伝送要求で正規化 
		//平均系内滞在時間
		//double wating_time_av()const{ return wating_time_av_; }
		double wating_time_av()const{
			return (pk_delete_ok_==0)? 0 : (wating_time_sum_/pk_delete_ok_);
		}
		double group_wating_time_av(int i)const{
			return (group_pk_delete_ok_[i]==0)? 0 : (group_wating_time_sum_[i]/group_pk_delete_ok_[i]);
		}
		double group_wating_time_dist(int i,int j)const{
			return (group_pk_delete_ok_[i]==0)? 0 : (group_wating_time_dist_[i][j]/group_pk_delete_ok_[i]);
		}
	};
	CPkMgrStat stat_;

	opssimu::CRandMtUni<double>* p_rnd_size_;//パケット長の分布を作るための乱数(パケット長は2種類固定)
	opssimu::CRandMtUni<node_index_t>* p_rnd_node_;//送信元と宛先の分布
private:
	inline void pk_delete_common(CPacket* p_packet);//パケット削除の共通ルーチン
public:
	CPacketManager(const node_index_t node_num,const seed_t seed);
	~CPacketManager();

	inline CPacket* packet_new();//パケット生成
	inline void packet_delete_ok(CPacket* p_packet);//正しく伝送されたパケット削除(パケット廃棄と区別する)
	inline void packet_delete_loss(CPacket* p_packet);//パケット廃棄

	virtual void onStat(const simuframe::CEvent* const p_event);//統計処理
};

inline
CPacket* CPacketManager::packet_new(){
    //std::cout<<"debug:packet_new():"<<stat_.pk_new_<<std::endl;
	//送信元と宛先を決定
	node_index_t src=(*p_rnd_node_)();
	node_index_t dis=(*p_rnd_node_)();
	while(src==dis){//宛先が自分自身にならないように
		dis=(*p_rnd_node_)();
	}

	//パケット生成
	return new CPacketRef(++(stat_.pk_new_),//id(0番は使用しない。順序逆転の初期値に使うので。)
						simuframe::CSimu::get_time(),//時刻
						//(( (*p_rnd_size_)() < 0.6)? (64*8) : (1500*8)),//パケット長
						CONST_PK_SIZE,//パケット長固定
						src,//送信元
						dis//宛先
					); 
}

//正しく伝送されたパケット削除(パケット廃棄と区別する)
inline
void CPacketManager::packet_delete_ok(CPacket* p_packet){
    //std::cout<<"debug:packet_delete_ok():"<<p_packet->id()<<std::endl;
	
	//伝送成功パケット数
	stat_.pk_delete_ok_++; 
	stat_.group_pk_delete_ok_[p_packet->group()]++;
	//stat_.pk_size_sum_ok_+=p_packet->size();//転送完了合計パケットサイズの更新(手抜きオーバーフローするかも)

	//系内時間の合計更新
	const simutime_t wating_time=(simuframe::CSimu::get_time()-p_packet->time());
	stat_.wating_time_sum_+= wating_time;
	stat_.group_wating_time_sum_[p_packet->group()] += wating_time;
	stat_.group_wating_time_dist_[p_packet->group()].push(wating_time);

	/*旧バージョン
	//平均系内滞在時間の更新
	//平均=合計時間/パケット数
	//で計算すると合計時間がものすごく大きくなってしまうので、差分を更新するように計算。
	stat_.wating_time_av_+=
		(simuframe::CSimu::get_time()-p_packet->time()-stat_.wating_time_av_)/stat_.pk_delete_;
	stat_.wating_time_sum_+=simuframe::CSimu::get_time()-p_packet->time();
	*/

	//hop数
	stat_.hop_sum_+=p_packet->hop();
	const int additional_hop=p_packet->hop()-p_packet->sp_hop();
	stat_.group_hop_sum_[p_packet->group()]+=(additional_hop);//増分を記録
	stat_.group_hop_dist_[p_packet->group()][additional_hop]++;//増分の分布を記録
	int ref_count= (static_cast<CPacketRef*>(p_packet))->ref_count();//反射回数
	stat_.group_ref_hop_dist_[p_packet->group()][ref_count*2]++;//反射増分の分布を記録
	stat_.group_def_hop_dist_[p_packet->group()][(additional_hop - ref_count*2)]++;//迂回増分の分布を記録

	//順序逆転のチェック
	//stat_.misordering_checker_.check(p_packet);
	
	//delete共通ルーチン
	pk_delete_common(p_packet);
}

//パケット廃棄
inline
void CPacketManager::packet_delete_loss(CPacket* p_packet){
	//std::cout<<"debug:packet_delete_loss():"<<p_packet->id()<<std::endl;
	stat_.pk_delete_loss_++;
	stat_.group_pk_delete_loss_[p_packet->group()]++;

	pk_delete_common(p_packet);//delete共通ルーチン
}

//パケット削除共通ルーチン(pk_delete_ok() or pk_delete_loss()から呼ばれる)
inline
void CPacketManager::pk_delete_common(CPacket* p_packet){
	//std::cout<<"debug:packet_delete_common():"<<p_packet->id()<<":"<<(void*)p_packet<<std::endl;
	stat_.pk_delete_++;
	stat_.group_pk_delete_[p_packet->group()]++;
	//stat_.pk_size_sum_+=p_packet->size();//削除合計パケットサイズの更新(手抜きオーバーフローするかも)
	
	delete p_packet;
}

//メモリプールに用いる最大サイズパケットの定義。
//とりあえずまだパケットが1つしかないのでtypedefのみ
typedef CPacketRef CPacketMaxsize;


}//end of namespace opsnetwork
//end of packet.h
#endif //_PACKET_H__

