#ifndef _OPSNETWORK_H__
#define _OPSNETWORK_H__
//opsnetwork.h
//OPSネットワークの定義
#include<vector>
#include<deque>

#include"topology.h"
#include"basenetwork.h"
#include"packet.h"
#include"mtrandom.h"



using simuframe::CEvent;

namespace opsnetwork{

/////////////////////////////////////////////////
//			struct EStatableLabel
/////////////////////////////////////////////////
//CStatableのグループ分け用フラグ
struct EStatableLabel{
	enum{NETWORK=1,NODE,LINK,PK_MGR};//NONE=0,ALL=0xFFFFはCStatManagerで予約されているので使わない
};

//前方宣言
class INodeMaker;
/////////////////////////////////////////////////
//			class COpsNetwork
/////////////////////////////////////////////////
class COpsNetwork:public CNetwork{
public:
	//ネットワークの設定パラメータをまとめた構造体
	struct SNetworkSetting{
		simuframe::simutime_t end_time_;//シミュレーション開始時間
		double lambda_;//パケットの到着率lambda
		const INodeMaker* p_node_maker_;//ノード生成インターフェイス
		double ttl_margin_;
		double buf_th_;
		unsigned int buf_sz_;
		unsigned int seed_;
		SNetworkSetting(simuframe::simutime_t end_time,double lambda,
			const INodeMaker* p_node_maker,double ttl_margin,double buf_th,unsigned int buf_sz,
			unsigned int seed)
			:end_time_(end_time),lambda_(lambda),p_node_maker_(p_node_maker),
			ttl_margin_(ttl_margin),buf_th_(buf_th),buf_sz_(buf_sz),seed_(seed){}
	};

private:
	simuframe::ISimu* const p_simu_;//シミュレーション
	CPacketManager* p_pk_manager_;//パケットマネージャ
	simuframe::CEventManager* p_event_manager_;//イベントマネージャ
	simuframe::CStatManager* p_stat_manager_;//統計マネージャ
	opssimu::CRandMtExp<simuframe::simutime_t>* p_rnd_exp_;//パケットの到着分布

	const SNetworkSetting* const p_setting_;//ネットワークの設定パラメータ
public:
	COpsNetwork(simuframe::ISimu* const p_simu,CTopology* const p_topology,
		const SNetworkSetting* const p_setting);
	virtual ~COpsNetwork();

public:
	virtual void onPacketNew(const CEvent* const p_event);//パケット生成処理
	virtual void onPacketDeleteOk(const CEvent* const p_event);//パケット削除処理
	virtual void onPacketDeleteLoss(const CEvent* const p_event);//パケット廃棄処理
	virtual void onStat(const CEvent* const p_event);//統計処理
	simuframe::ISimu* const get_simu() const { return p_simu_; }

	const SNetworkSetting* const setting() const{ return p_setting_; }


};

/////////////////////////////////////////////////
//			class CPacketBuf
/////////////////////////////////////////////////
//パケットサイズを管理できるパケットバッファ
class CPacketBuf{
public:
	typedef std::deque<CPacket* > buf_t;
	typedef buf_t::iterator iterator;
private:
	typedef CPacket::pk_size_t pk_size_t;

	buf_t buf_;//バッファ
	const pk_size_t pk_size_max_;//最大容量
	const pk_size_t pk_size_th_;//閾値
	pk_size_t pk_size_;//現在のパケット長の合計
public:
	CPacketBuf(const pk_size_t pk_size_max,const double threshold):pk_size_max_(pk_size_max),
		pk_size_th_(static_cast<pk_size_t>(
			(pk_size_max-CONST_PK_SIZE)*threshold+CONST_PK_SIZE //要修正(下記参照)
		)),pk_size_(0){}
	
	//バッファの先頭を部分を出力インターフェイスと暗黙に仮定しているため、
	//可変長パケットの場合、正しい閾値が得られない
	//可変長対応するためにはバッファと出力インターフェイスを論理的に分離する必要あり

	//パケットの追加
	void push_back(CPacket* const p_packet){
		pk_size_+=p_packet->size();//サイズを更新
		buf_.push_back(p_packet);
	}
	//パケットの削除
	void pop_front(){
		pk_size_-=buf_.front()->size();//サイズを更新
		buf_.pop_front();
	}
	//先頭のパケットを返す
	CPacket* const front() const{ return buf_.front(); }

	bool empty() const { return buf_.empty(); }
	const pk_size_t pk_size() const{ return pk_size_; }

	//bool insert_ok(const CPacket* const p_packet)const{
	//	return (pk_size_+p_packet->size() <= pk_size_max_);
	//}

	//最大サイズに対してパケットを追加可能かチェック
	bool insert_ok_max(const CPacket* const p_packet)const{
		return (pk_size_+p_packet->size() <= pk_size_max_);
	}

	//閾値に対してパケットを追加可能かチェック
	bool insert_ok_th(const CPacket* const p_packet)const{
		return (pk_size_+p_packet->size() <= pk_size_th_);
	}

	iterator begin(){ return buf_.begin(); }
	iterator end(){ return buf_.end(); }
};



/////////////////////////////////////////////////
//			class COpsNode
/////////////////////////////////////////////////
class COpsNode:public CNode{

protected:
	typedef CPacketBuf pkbuf_t;
	typedef std::vector<pkbuf_t> outbuf_t;
	typedef CPacket::pk_size_t pk_size_t;

	const SNodeParam param_;
	simuframe::CEventManager* p_event_manager_;
	outbuf_t outbuf_;//出力バッファ
	
	//統計用構造体
	struct SNodeStat{
		typedef CPacket::pk_id_t pk_id_t;
		pk_id_t pk_new_;//自ノードが送信元のパケット数
		pk_id_t pk_delete_ok_;//自ノードが受信元のパケット数
		pk_id_t pk_delete_loss_;//廃棄したパケット数
		pk_id_t pk_service_;//中継したパケット数(自ノードが送信元の場合も含む)
		SNodeStat():pk_new_(0),pk_delete_ok_(0),pk_delete_loss_(0),pk_service_(0){}
	};
	SNodeStat stat_;
private:
	double ttl_margin_;
public:
	COpsNode(CNetwork* const p_network,const SNodeParam& param);
	virtual ~COpsNode();

	//イベントハンドラ
	virtual void onPacketNew(const CEvent* const p_event);//パケット生成処理
	virtual void onPacketDeleteOk(const CEvent* const p_event);//パケット削除処理
	virtual void onPacketDeleteLoss(const CEvent* const p_event);//パケット廃棄処理

	virtual void onArrival(const CEvent* const p_event);//到着処理
	virtual void onService(const CEvent* const p_event);//サービス処理
	virtual void onDeparture(const CEvent* const p_event);//退去処理
	virtual void onStat(const CEvent* const p_event);//統計処理

	//protectedにしたいけどDefImplからコールされる
	void onOutbufPush(const CEvent* const p_event,CPacket* const p_packet,const port_t out_port);

protected:
	virtual void onRouting(const CEvent* const p_event);//ルーチング処理(サービス処理からコールされる)
	
	
};

/////////////////////////////////////////////////
//			class COpsNodeRef
/////////////////////////////////////////////////
//反射を実装したノード
class COpsNodeRef:public COpsNode{
	typedef CPacket::pk_id_t pk_id_t;
protected:
	port_t ref_port_;//前回の反射先
	pk_id_t ref_pk_id_;//反射パケットID(ネットワーク全体ではなくノード毎に管理)

public:
	COpsNodeRef(CNetwork* const p_network,const SNodeParam& param)
		:COpsNode(p_network,param),ref_port_(0),ref_pk_id_(0){}
	virtual ~COpsNodeRef(){}
protected:
	virtual void onRouting(const CEvent* const p_event);//ルーチング処理(サービス処理からコールされる)
	
	virtual void onRoutingRefNONE(const CEvent* const p_event,CPacketRef* const p_packet);//通常パケットの処理
	virtual void onRoutingRefFWD(const CEvent* const p_event,CPacketRef* const p_packet);//反射パケットの処理
	virtual void onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet);//反射から返ってきたパケットの処理

private:
	void send_ref_pk_common(const CEvent* const p_event,CPacketRef* const p_packet,
									 const port_t out_port);//反射パケット送信の共通ルーチン
	const port_t select_ref_port(const CPacket* const p_packet,const port_t out_port);//反射先の選択


};

/////////////////////////////////////////////////
//			COpsNodeDef
/////////////////////////////////////////////////
//迂回を実装したノード
class COpsNodeDef:public COpsNode{
public:
	//迂回処理の実装をpublicなローカルクラスとして分離(COpsNodeRefDefでひし形継承したくないので)
	class COpsNodeDefImpl{
		const IRoutingTable* const p_routing_table_;//ルーチングテーブル
		outbuf_t& outbuf_;//バッファへの参照
		simuframe::CEventManager* const p_event_manager_;
	public:
		COpsNodeDefImpl(const IRoutingTable* const p_routing_table,outbuf_t& outbuf,
			simuframe::CEventManager* const p_event_manager)
			:p_routing_table_(p_routing_table),outbuf_(outbuf),p_event_manager_(p_event_manager){}
		//迂回ルーチング実装
		void onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet,
			COpsNode* const p_this_node);
	};

private:
	COpsNodeDefImpl def_impl_;//内部実装のインスタンス
public:
	COpsNodeDef(CNetwork* const p_network,const SNodeParam& param)
		:COpsNode(p_network,param),def_impl_(p_routing_table_,outbuf_,p_event_manager_){}
	virtual ~COpsNodeDef(){}
protected:
	virtual void onRouting(const CEvent* const p_event);//ルーチング処理(サービス処理からコールされる)
private:
	void onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet){ 
		def_impl_.onDefRouting(p_event,p_packet,this);//迂回実装に委譲するだけ
	}
};

/////////////////////////////////////////////////
//			COpsNodeRefDef
/////////////////////////////////////////////////
//提案方式：反射・迂回を行うノード
class COpsNodeRefDef:public COpsNodeRef{
private:
	COpsNodeDef::COpsNodeDefImpl def_impl_;//迂回実装のローカルクラスを流用
public:
	COpsNodeRefDef(CNetwork* const p_network,const SNodeParam& param)
		:COpsNodeRef(p_network,param),def_impl_(p_routing_table_,outbuf_,p_event_manager_){}
	virtual ~COpsNodeRefDef(){}
protected:
	virtual void onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet);//反射から返ってきたパケットの処理
private:
	void onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet){ 
		def_impl_.onDefRouting(p_event,p_packet,this);//迂回実装に委譲するだけ
	}
};

/////////////////////////////////////////////////
//			class COpsLink
/////////////////////////////////////////////////
class COpsLink:public CLink{
	typedef std::deque<CPacket* > pkbuf_t;
	
	const SLinkParam param_;
	simuframe::CEventManager* p_event_manager_;
	pkbuf_t pkbuf_;//リンク中のパケットを保持するためのバッファ

	//統計用構造体
	struct SLinkStat{
		typedef CPacket::pk_id_t pk_id_t;
		typedef simuframe::simutime_t simutime_t;
		pk_id_t pk_count_;
		pk_id_t pk_loss_;
		double pk_size_sum_;

		SLinkStat():pk_count_(0),pk_loss_(0),pk_size_sum_(0){}

		double throughput()const { return (pk_size_sum_/simuframe::CSimu::get_time())/1000.0; }//Gbps
	};
	SLinkStat stat_;

private:
	double load() const{ return stat_.throughput()/(param_.speed_); }
	double loss_ratio() const{ return ((stat_.pk_count_==0)? 0 : (double)stat_.pk_loss_/(stat_.pk_count_+stat_.pk_loss_)); }
public:
	COpsLink(CNetwork* const p_network,const SLinkParam& param);
	virtual ~COpsLink();

	const SLinkParam::link_speed_t speed()const{ return param_.speed_*1000; }
	//イベントハンドラ
	virtual void onArrival(const CEvent* const p_event);//到着処理
	virtual void onDeparture(const CEvent* const p_event);//退去処理
	virtual void onStat(const CEvent* const p_event);//統計処理

	void inform_packet_loss(){ stat_.pk_loss_++; }//パケットが廃棄されたことを通知する（統計用に使われる）
};

//Node生成
/////////////////////////////////////////////////
//			class INodeMaker
/////////////////////////////////////////////////
//ノードを生成するインターフェイス
class INodeMaker{
public:
	virtual CNode* node_new(CNetwork* const p_network,const SNodeParam& param) const =0;
};

/////////////////////////////////////////////////
//			class COpsNodeMaker
/////////////////////////////////////////////////
class COpsNodeMaker:public INodeMaker{
public:
	CNode* node_new(CNetwork* const p_network,const SNodeParam& param) const{
		return new COpsNode(p_network,param);
	}
};

/////////////////////////////////////////////////
//			class COpsNodeRefMaker
/////////////////////////////////////////////////
class COpsNodeRefMaker:public INodeMaker{
public:
	CNode* node_new(CNetwork* const p_network,const SNodeParam& param) const{
		return new COpsNodeRef(p_network,param);
	}
};

/////////////////////////////////////////////////
//			class COpsNodeDefMaker
/////////////////////////////////////////////////
class COpsNodeDefMaker:public INodeMaker{
public:
	CNode* node_new(CNetwork* const p_network,const SNodeParam& param) const{
		return new COpsNodeDef(p_network,param);
	}
};

/////////////////////////////////////////////////
//			class COpsNodeRefDefMaker
/////////////////////////////////////////////////
class COpsNodeRefDefMaker:public INodeMaker{
public:
	CNode* node_new(CNetwork* const p_network,const SNodeParam& param) const{
		return new COpsNodeRefDef(p_network,param);
	}
};

}//end of namespace opsnetwork

//end of opsnetwork.h
#endif //_OPSNETWORK_H__

