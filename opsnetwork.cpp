//opsnetwork.cpp
#include"opsnetwork.h"
#include"event.h"

//デバッグ用
#include<iostream>

using namespace opsnetwork;
using namespace opssimu;
using namespace simuframe;

/////////////////////////////////////////////////
//			class COpsNetwork
/////////////////////////////////////////////////

COpsNetwork::COpsNetwork(simuframe::ISimu* const p_simu,CTopology* const p_topology
						 ,const SNetworkSetting* const p_setting)
		:CNetwork(p_topology),p_simu_(p_simu),p_setting_(p_setting)
{
	typedef CTopology::node_param_vec_t node_param_vec_t;
	typedef CTopology::link_param_vec_t link_param_vec_t;
	typedef CTopology::node_param_vec_t::const_iterator itn_t;
	typedef CTopology::link_param_vec_t::const_iterator itl_t;

	//ノード･リンク情報の取得
	const node_param_vec_t node_param = p_topology_->get_node_param();
	const link_param_vec_t link_param = p_topology_->get_link_param();

	//ノード生成
	for(itn_t itn=node_param.begin();itn!=node_param.end();++itn){
		node_.push_back(p_setting_->p_node_maker_->node_new(this,*itn));
	}
	//リンク生成
	for(itl_t itl=link_param.begin();itl!=link_param.end();++itl){
		link_.push_back(new COpsLink(this,*itl));
	}
	//ノード･リンク間の相互ポインタの設定
	set_connection();
	
	//パケットマネージャの生成
	p_pk_manager_=new CPacketManager(static_cast<node_index_t>(node_param.size()),p_setting_->seed_);

	//乱数ジェネレータの作成(パケットの到着分布)
	p_rnd_exp_=new opssimu::CRandMtExp<simuframe::simutime_t>(p_setting_->seed_,p_setting_->lambda_);//seed,指数分布(lambda)

	//イベントマネージャの取得
	p_event_manager_=simuframe::CEventManager::getInstance();

	//統計マネージャの生成
	const simutime_t interval=10000;
	p_stat_manager_=new simuframe::CStatManager(p_event_manager_,interval);
	//統計をとるオブジェクトの登録
	p_stat_manager_->add(this,EStatableLabel::NETWORK);
	p_stat_manager_->add(p_pk_manager_,EStatableLabel::PK_MGR);
	for(node_vec_t::iterator itn=node_.begin();itn!=node_.end();++itn){
		p_stat_manager_->add(*itn,EStatableLabel::NODE);
	}
	for(link_vec_t::iterator itl=link_.begin();itl!=link_.end();++itl){
		p_stat_manager_->add(*itl,EStatableLabel::LINK);
	}

	//シミュレーション開始・終了イベントの生成
	p_event_manager_->setEvent(new CEventOpsSimuStart(0,this));
	p_event_manager_->setEvent(new CEventOpsSimuEnd(p_setting_->end_time_,this));

	//最初の統計イベントの生成
	const simutime_t transit_time=10000;//統計開始までの過渡期間
	p_event_manager_->setEvent(new CEventStat(transit_time,p_stat_manager_,CStatManager::NONE));
}

COpsNetwork::~COpsNetwork(){
	//統計マネージャの削除
	delete p_stat_manager_;

	//乱数ジェネレータの削除
	delete p_rnd_exp_;

	//パケットマネージャの削除
	delete p_pk_manager_;

	typedef node_vec_t::iterator itn_t;
	typedef link_vec_t::iterator itl_t;
	
	//リンク削除
	for(itl_t itl=link_.begin();itl!=link_.end();++itl)
		delete *itl;

	//ノード削除
	for(itn_t itn=node_.begin();itn!=node_.end();++itn)
		delete *itn;	
}


//パケット生成イベント
void COpsNetwork::onPacketNew(const CEvent* const p_event){
	CPacket* p_packet=p_pk_manager_->packet_new();
    //std::cout<<"debug:COpsNetwork::opPacketNew():"<<(void*)p_packet<<std::endl;
	//ノードにパケットを投げる
	const simutime_t now=p_event->time();
	p_event_manager_->setEvent(new CEventNodePacketNew(
		now,static_cast<COpsNode*>(node_[p_packet->src()]),p_packet));
	//次のパケット生成イベントを生成する
	p_event_manager_->setEvent(new CEventNetworkPacketNew(now+(*p_rnd_exp_)(),this));
}

//パケット削除イベント
void COpsNetwork::onPacketDeleteOk(const CEvent* const p_event){
	p_pk_manager_->packet_delete_ok(
			(static_cast<const CEventNetworkPacketDeleteOk* const>(p_event))->packet()
		);
}

//パケット廃棄イベント
void COpsNetwork::onPacketDeleteLoss(const CEvent* const p_event){
	p_pk_manager_->packet_delete_loss(
			(static_cast<const CEventNetworkPacketDeleteLoss* const>(p_event))->packet()
		);
}

//統計処理イベント
void COpsNetwork::onStat(const CEvent* const p_event){
}

/////////////////////////////////////////////////
//			class COpsNode
/////////////////////////////////////////////////
//コンストラクタ
COpsNode::COpsNode(CNetwork* const p_network,const SNodeParam& param)
	:CNode(p_network,param.index_),param_(param),
	outbuf_(param.out_link_.size(),
			CPacketBuf((static_cast<COpsNetwork* const>(p_network)->setting())->buf_sz_,
					(static_cast<COpsNetwork* const>(p_network)->setting())->buf_th_)
	){
	//イベントマネージャの取得
	p_event_manager_=simuframe::CEventManager::getInstance();

	//ルーチングテーブルの生成
	p_routing_table_= new CRoutingTableAPall(p_network_,index_);

	//TTLマージンの設定
	ttl_margin_=(static_cast<COpsNetwork* const>(p_network_)->setting())->ttl_margin_;
	/*
	//デバッグ用経路ホップ長を表示
	for(int i=0;i<28;i++)
		std::cout<<p_routing_table_->path(i).size()<<" ";
	std::cout<<std::endl;
	*/
}


//デストラクタ
COpsNode::~COpsNode(){
	//出力バッファのパケットを捨てる
	typedef outbuf_t::iterator oit_t;
	typedef pkbuf_t::iterator pit_t;
	for(oit_t oit=outbuf_.begin();oit!=outbuf_.end();++oit){
		for(pit_t pit=oit->begin();pit!=oit->end();++pit){
			delete (*pit);
		}
	}
	//ルーチングテーブルの削除
	delete p_routing_table_;
}

//パケット生成イベント
void COpsNode::onPacketNew(const CEvent* const p_event){
	stat_.pk_new_++;
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodePacketNew* const>(p_event))->packet()
		);

	//経路の設定
	const IRoutingTable::path_t& path=p_routing_table_->path(p_packet->dis());
	p_packet->set_path(path);
	p_packet->set_sp_hop(static_cast<CPacket::hop_t>(path.size()));
	//p_packet->set_ttl(static_cast<CPacket::ttl_t>(path.size()*ttl_margin_));マージンを倍率で設定
	p_packet->set_ttl(static_cast<CPacket::ttl_t>(path.size()+ttl_margin_));//マージンをホップ数で入力
	//p_packet->set_ttl(9);//TTLを固定

	//ノード到着イベント
	p_event_manager_->setEvent(new CEventNodeArrival(p_event->time(),this,p_packet,0/*ポート番号はダミー*/));

	
}
//パケット削除イベント
void COpsNode::onPacketDeleteOk(const CEvent* const p_event){
	stat_.pk_delete_ok_++;
	p_event_manager_->setEvent(new CEventNetworkPacketDeleteOk(
			p_event->time(),
			static_cast<COpsNetwork* const>(p_network_),
			(static_cast<const CEventNodePacketDeleteOk* const>(p_event))->packet()
			)
		);
}
//パケット廃棄イベント
void COpsNode::onPacketDeleteLoss(const CEvent* const p_event){
	stat_.pk_delete_loss_++;
	COpsLink* const p_link=static_cast<COpsLink* const>(
			(static_cast<const CEventNodePacketDeleteLoss* const>(p_event))->link()
		);
	p_link->inform_packet_loss();
	p_event_manager_->setEvent(new CEventNetworkPacketDeleteLoss(
			p_event->time(),
			static_cast<COpsNetwork* const>(p_network_),
			(static_cast<const CEventNodePacketDeleteLoss* const>(p_event))->packet()
			)
		);
}

//パケット到着イベント
void COpsNode::onArrival(const CEvent* const p_event){
	//入力バッファを考慮する場合はここに追加

	//とりあえずサービスイベントを生成するだけ
	p_event_manager_->setEvent(new CEventNodeService(
			p_event->time(),
			this,
			(static_cast<const CEventNodeArrival* const>(p_event))->packet(),
			(static_cast<const CEventNodeArrival* const>(p_event))->in_port()
		));
}

//パケットサービスイベントの共通処理ルーチン
//ルーチングポリシーの変更はonRouting()をカスタマイズ
void COpsNode::onService(const CEvent* const p_event){
	stat_.pk_service_++;
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodeService* const>(p_event))->packet()
		);

	if(!p_packet->empty()){//パケットが送信元ノードor中継ノードに到着した
		//TTLのチェック(ホップ数制限)
		if(p_packet->ttl()){//TTLが0でない場合
			//TTLの更新
			p_packet->dec_ttl_inc_hop();//1減らす
			//ルーチング処理
			onRouting(p_event);
		
		}else{//TTLが0の場合
			//パケットを廃棄
			p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[p_packet->top_port()]))
			);
		}
	}else{//パケットが宛先ノードに到着した
		/*
		//宛先チェック
		if(p_packet->dis()!=param_.index_){
			std::cerr<<"COpsNode::onService():distination check fault."
			<<"dis:"<<p_packet->dis()<<"node:"<<param_.index_<<std::endl;
		}
		*/
		//パケットを削除
		p_event_manager_->setEvent(
			new CEventNodePacketDeleteOk(p_event->time(),this,p_packet)
		);
	}
}

//ルーチング処理
//デフォルトは固定経路。ルーチングポリシーは派生クラスでカスタマイズ可能。
void COpsNode::onRouting(const CEvent* const p_event){
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodeService* const>(p_event))->packet()
		);
	//出力ポートを取得
	const port_t out_port=p_packet->top_port();

	//バッファ容量のチェック
	//固定経路の場合はバッファ閾値を無視してバッファ最大サイズまで追加可能。
	if(outbuf_[out_port].insert_ok_max(p_packet)){//バッファに追加可能の場合
		p_packet->pop_port();//先頭のラベルを捨てる
		onOutbufPush(p_event,p_packet,out_port);//バッファに追加
	}else{//バッファ溢れの場合廃棄
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[out_port]))
			);
	}
}

//出力バッファにパケットを追加する
//CPacketBuf::insert_ok()は保障されていることを前提とする
void COpsNode::onOutbufPush(const CEvent* const p_event,CPacket* const p_packet,const port_t out_port){
	if(!outbuf_[out_port].empty()){//出力バッファが空でない場合
		outbuf_[out_port].push_back(p_packet);//パケットを出力バッファに突っ込む
	
	}else{//出力バッファが空の場合
		outbuf_[out_port].push_back(p_packet);//パケットを出力バッファに突っ込む

		//パケット退去イベントの生成
		//パケットをリンクに出力するのに要する時間の計算
		//出力リンク速度を取得
		//const double link_speed=40000;//とりあえず設定ファイル無視で40Gbps=40000bit/us 
		//const simutime_t output_time=p_packet->size()/link_speed;
		simutime_t output_time=p_packet->size()/
			static_cast<COpsLink*>(out_link_[out_port])->speed();
		p_event_manager_->setEvent(
			new CEventNodeDeparture(p_event->time()+output_time,this,p_packet,out_port)
		);
	}
}

//パケット退去イベント
void COpsNode::onDeparture(const CEvent* const p_event){
	//出力ポートを取得
	const port_t out_port=(static_cast<const CEventNodeDeparture* const>(p_event))->out_port();
	//出力バッファからパケットを取り出す
	CPacket* const p_packet=outbuf_[out_port].front();
	outbuf_[out_port].pop_front();
	//出力リンクに投げる
	p_event_manager_->setEvent(
			new CEventLinkArrival(p_event->time(),out_link_[out_port],p_packet)
		);
	//出力バッファのチェック
	if(!outbuf_[out_port].empty()){//出力バッファが空でない
		//次パケットの退去イベントの生成
		//パケットをリンクに出力するのに要する時間の計算
		//出力リンク速度を取得
		CPacket* const p_next_packet=outbuf_[out_port].front();		
		//const double link_speed=40000;//とりあえず設定ファイル無視で40Gbps=40000bit/us
		//const simutime_t output_time=p_next_packet->size()/link_speed;
		simutime_t output_time=p_packet->size()/
			static_cast<COpsLink*>(out_link_[out_port])->speed();
		p_event_manager_->setEvent(
			new CEventNodeDeparture(p_event->time()+output_time,this,p_next_packet,out_port)
		);
	}//else{}出力バッファが空の場合は何もしない

}
//統計イベント
void COpsNode::onStat(const CEvent* const p_event){}

/////////////////////////////////////////////////
//			class COpsNodeRef
/////////////////////////////////////////////////
//ルーチング処理
void COpsNodeRef::onRouting(const CEvent* const p_event){
	CPacketRef* const p_packet=static_cast<CPacketRef* const>(
		(static_cast<const CEventNodeService* const>(p_event))->packet());

	switch(p_packet->ref_flg() ){
		case CPacketRef::NONE:
			onRoutingRefNONE(p_event,p_packet);//通常パケットの処理
			break;
		case CPacketRef::REF_FWD:
			onRoutingRefFWD(p_event,p_packet);//反射パケットの処理
			break;
		case CPacketRef::REF_RET:
			onRoutingRefRET(p_event,p_packet);//反射から返ってきたパケットの処理
			break;
		default:
			std::cerr<<"unknown ref_flg"<<std::endl;
			break;
	}
}

//通常パケットの処理
//反射元ノードでの処理
void COpsNodeRef::onRoutingRefNONE(const CEvent* const p_event,CPacketRef* const p_packet){
	//出力ポートを取得
	const port_t out_port=p_packet->top_port();

	//バッファ容量のチェック
	//通常パケットは閾値を超えて追加しない
	if(outbuf_[out_port].insert_ok_th(p_packet)){//バッファに追加可能の場合
		p_packet->pop_port();//先頭のラベルを捨てる
		onOutbufPush(p_event,p_packet,out_port);//バッファに追加
	}else{//バッファ溢れの場合

		//TTLのチェック
		//TTLが2より小さい場合は反射しない
		//TTLをdecするタイミングに注意。TTL=2の場合はok。
		if(p_packet->ttl()<2){
			//閾値を超えて主経路に追加を試みる
			if(outbuf_[out_port].insert_ok_max(p_packet)){//追加可能の場合
				p_packet->pop_port();//先頭のラベルを捨てる
				onOutbufPush(p_event,p_packet,out_port);//バッファに追加
			}else{//追加できない場合
				//廃棄
				p_event_manager_->setEvent(
						new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
						static_cast<COpsLink*>(out_link_[out_port]))
					);
			}

		}else{//TTLが2以上の場合
			//反射
			send_ref_pk_common(p_event,p_packet,out_port);
		}
	}

}
//反射パケットの処理
//反射先ノードでの処理
void COpsNodeRef::onRoutingRefFWD(const CEvent* const p_event,CPacketRef* const p_packet){
	//送り返す先を選択
	//上りと下りリンクはトポロジファイル上で連続しているので、同じポート番号となるになることが
	//暗黙のうちに仮定されている
	const port_t return_port=(static_cast<const CEventNodeService* const>(p_event))->in_port();
	
	//バッファ容量のチェック
	//閾値を超えて追加可能。
	if(outbuf_[return_port].insert_ok_max(p_packet)){//バッファに追加可能の場合
		//反射フラグの更新
		p_packet->set_ref_flg(CPacketRef::REF_RET);
		//バッファに追加
		onOutbufPush(p_event,p_packet,return_port);
	}else{
		//バッファ溢れの場合
		//廃棄
		p_event_manager_->setEvent(
					new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
					static_cast<COpsLink*>(out_link_[return_port]))
			);
	}
}
//反射から返ってきたパケットの処理
//反射元ノードでの処理
void COpsNodeRef::onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet){
	//出力ポートを取得
	const port_t out_port=p_packet->top_port();

	//反射フラグをNONEに戻す
	p_packet->set_ref_flg(CPacketRef::NONE);

	//バッファ容量のチェック
	//閾値を超えて追加可能。
	if(outbuf_[out_port].insert_ok_max(p_packet)){//バッファに追加可能の場合
		p_packet->pop_port();//先頭のラベルを捨てる
		onOutbufPush(p_event,p_packet,out_port);//バッファに追加
	}else{//バッファ溢れの場合
		
		if(p_packet->ttl()<2){//TTLのチェック
			//廃棄
			p_event_manager_->setEvent(
					new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
					static_cast<COpsLink*>(out_link_[out_port]))
			);
		
		}else{
			//再反射
			send_ref_pk_common(p_event,p_packet,out_port);
		}
	}
}

//反射パケット送信の共通ルーチン
void COpsNodeRef::send_ref_pk_common(const CEvent* const p_event,CPacketRef* const p_packet,
									 const port_t out_port){
	//反射先の選択
	const port_t target=select_ref_port(p_packet,out_port);
	
	if(target!=out_port){//反射先が見つかった
		//反射フラグの設定
		p_packet->set_ref_flg(CPacketRef::REF_FWD);
		p_packet->set_ref_id(ref_pk_id_++);
		p_packet->set_group_flg(1);//統計用に印を付ける
		p_packet->inc_count();//反射回数記録
		//出力バッファに追加
		onOutbufPush(p_event,p_packet,target);
	}else{
		//反射先が見つからなかった
		//廃棄
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[out_port]))
			);
	}
}

//反射先の選択
//反射先が見つからない場合はout_portを返す
const CNode::port_t COpsNodeRef::select_ref_port(const CPacket* const p_packet,const port_t out_port){
	const port_t end=ref_port_;//ref_port_は前回の反射先。ループチェック用にendに記録しておく。

	//ラウンドロビン
	ref_port_= (ref_port_< (out_link_num_-1))? ref_port_+1 : 0;//1増やす(out_link_num_は出力ポートの数)
	//余裕のあるバッファを見つけるまでループ
	//反射パケットは閾値を超えて追加しない
	while(!outbuf_[ref_port_].insert_ok_th(p_packet)){
		if(end==ref_port_){
			return out_port;//全てのポートを調べたが見つからなかった
		}
		ref_port_= (ref_port_< (out_link_num_-1))? ref_port_+1 : 0;
	}
	return ref_port_;
}

/////////////////////////////////////////////////
//			COpsNodeDef
/////////////////////////////////////////////////
//ルーチング処理
void COpsNodeDef::onRouting(const CEvent* const p_event){
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodeService* const>(p_event))->packet()
		);
	//出力ポートを取得
	const port_t out_port=p_packet->top_port();

	//バッファ容量のチェック
	//通常パケットは閾値を超えて追加しない。
	if(outbuf_[out_port].insert_ok_th(p_packet)){//バッファに追加可能の場合
		p_packet->pop_port();//先頭のラベルを捨てる
		onOutbufPush(p_event,p_packet,out_port);//バッファに追加
	}else{//バッファ溢れの場合
		//迂回
		onDefRouting(p_event,p_packet);	
	}
}

/*
//迂回処理(CRoutingTableSPAP版)
void COpsNodeDef::onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet){
	//迂回経路の設定
	p_packet->set_path(
		static_cast<const CRoutingTableSPAP*>(p_routing_table_)->alternate_path(p_packet->dis())
		);
	//迂回出力ポートを取得
	const port_t def_port=p_packet->top_port();

	//バッファ容量のチェック
	if(outbuf_[def_port].insert_ok(p_packet)){//バッファに追加可能の場合
		p_packet->pop_port();//先頭のラベルを捨てる
		onOutbufPush(p_event,p_packet,def_port);//バッファに追加
	}else{//バッファ溢れの場合
		//廃棄
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet)
			);
	}
}
*/

//迂回処理(CRoutingTableAPall版)
void COpsNodeDef::COpsNodeDefImpl::onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet,
												COpsNode* const p_this_node){
	typedef CRoutingTableAPall::path_all_t path_all_t;
	//迂回経路の候補リストを取得(迂回コストが低い順にソートされている)
	const path_all_t& alternate=
		static_cast<const CRoutingTableAPall*>(p_routing_table_)->alternate_path_all(p_packet->dis());
	//すべての候補について
	for(path_all_t::const_iterator it=alternate.begin();it!=alternate.end();++it){
		//it->firstは迂回経路を表す

		//迂回経路のホップ数がTTL以内であるかをチェック
		//TTLがdecされるタイミングに注意。
		if(p_packet->ttl() < ((it->first).size()-1)){//TTLが足りない場合
			//迂回路選択をbreakして主経路への追加を試みる
			break;

		}else{//TTLチェックokの場合
			const port_t def_port=(it->first).top();//迂回先ポート
			//迂回ポートのバッファ容量のチェック
			//閾値を超えて追加しない。
			if(outbuf_[def_port].insert_ok_th(p_packet)){//バッファに追加可能の場合
				//迂回経路の設定
				p_packet->set_path(it->first);
				p_packet->set_group_flg(2);//統計用にマークをつける
				//送信
				p_packet->pop_port();//先頭のラベルを捨てる
				p_this_node->onOutbufPush(p_event,p_packet,def_port);//バッファに追加
				return;
			}
		}
	}
	//ここに来るのは迂回先が見つからなかった場合or迂回経路のTTLが足りない場合
	//閾値を超えて主経路に追加を試みる
	const port_t out_port=p_packet->top_port();//出力ポートを取得
	if((outbuf_[out_port].insert_ok_max(p_packet))&&
		!(p_packet->ttl() < p_packet->path_size()-1)){//バッファに追加可能&&残りホップOKの場合
		p_packet->pop_port();//先頭のラベルを捨てる
		p_this_node->onOutbufPush(p_event,p_packet,out_port);//バッファに追加
	}else{//バッファ溢れor主経路の残りホップ足りない場合
		//廃棄
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),p_this_node,p_packet,
				static_cast<COpsLink*>(p_this_node->get_out_link(out_port))
				)
			);
	}
	return;
}


/////////////////////////////////////////////////
//			COpsNodeRefDef
/////////////////////////////////////////////////
//反射から返ってきたパケットの処理
//反射元ノードでの処理
void COpsNodeRefDef::onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet){

	//出力ポートを取得
	const port_t out_port=p_packet->top_port();

	//反射フラグをNONEに戻す
	p_packet->set_ref_flg(CPacketRef::NONE);

	//主経路のTTLをチェック。残りホップ数より小さければ廃棄。
	if(p_packet->ttl()<p_packet->path_size()-1){
		//残りホップ数に届かない場合
		//廃棄
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[out_port]))
		);
	
	}else{
		//TTLチェックokの場合
		//主経路のバッファ容量のチェック
		//主経路には閾値を超えて追加しない。
		if(outbuf_[out_port].insert_ok_th(p_packet)){//バッファに追加可能の場合
			p_packet->pop_port();//先頭のラベルを捨てる
			onOutbufPush(p_event,p_packet,out_port);//バッファに追加
		}else{//バッファ溢れの場合
			//迂回
			onDefRouting(p_event,p_packet);
		}
		/*
		//帰ってきたら主経路の状態によらず迂回
		onDefRouting(p_event,p_packet);
		*/
	}
}


/////////////////////////////////////////////////
//			class COpsLink
/////////////////////////////////////////////////
//コンストラクタ
COpsLink::COpsLink(CNetwork* const p_network,const SLinkParam& param)
	:CLink(p_network,param.index_),param_(param){
	//イベントマネージャの取得
	p_event_manager_=simuframe::CEventManager::getInstance();
}

//デストラクタ
COpsLink::~COpsLink(){
	//バッファのパケットを捨てる
	typedef pkbuf_t::iterator pit_t;
	for(pit_t pit=pkbuf_.begin();pit!=pkbuf_.end();++pit){
		delete (*pit);
	}

	//デバッグ用
	std::cout<<param_.index_<<","<<"load:"<<load()<<std::endl;
	std::cout<<param_.index_<<","<<"link_loss_ratio:"<<loss_ratio()<<std::endl;
}

//パケット到着イベント
void COpsLink::onArrival(const CEvent* const p_event){
	
	//パケットをバッファに入れる
	CPacket* const p_packet=(static_cast<const CEventLinkArrival* const>(p_event))->packet();
	pkbuf_.push_back(p_packet);
	//退去イベントの生成
	const double propagation_speed=0.2;//ファイバ中の光の伝播速度20万km/s=0.2km/us 
	const simutime_t delay_time=param_.dis_/propagation_speed;//us
	p_event_manager_->setEvent(
		new CEventLinkDeparture(p_event->time()+delay_time,this,p_packet)
	);
}
//パケット退去イベント
void COpsLink::onDeparture(const CEvent* const p_event){
	stat_.pk_count_++;
	const simutime_t now=p_event->time();
	CPacket* const p_packet=pkbuf_.front();

	stat_.pk_size_sum_+=p_packet->size();

	//次のノードに投げる
	p_event_manager_->setEvent(
		new CEventNodeArrival(now,p_to_,p_packet,to_port_)
	);
	pkbuf_.pop_front();
}
//統計イベント
void COpsLink::onStat(const CEvent* const p_event){}

//パケット交換の仕組み
/*
COpsNetwork::onPacketNew
ネットワークがパケットを生成してノードに割り振る

COpsNode::onPacketNew
ノードで経路をヘッダに設定。COpsNode::onArrivalを生成

COpsNode::onPacketDeleteOk
転送成功数カウントしてCopsNetwork::onPacketDeleteOkを生成。

CopsNetwork::onPacketDeleteOk
メモリ解放をパケットマネージャに依頼。

COpsNode::onArrival
入力バッファを考慮する場合はここで処理。
今のところはなんもなし。到着数をカウントするぐらい。

COpsNode::onService
宛先が自分自身(目的地に届いた)の場合、COpsNode::onPacketDeleteOkを生成。
出力ポートを取得。
パケットをバッファに突っ込む。
このとき、バッファが空の場合は、パケットサイズからサービス時間を求めて、
onDepartureイベントを生成。

反射制御をする場合
パケットをバッファに突っ込む前に、バッファ状況を確認。
一定値以上の場合は、反射先ノードを選択する。(選択法は未定)
反射先ノードが決定したらパケットの反射フラグ1をtrueにして、
反射先ノードへの出力バッファに突っ込む。バッファが空ならonDepartureを生成。

反射パケットを受け取った場合の処理
反射フラグ1がtrueのとき、反射フラグ2をtrueにして、元のノードに送り返す。
このために各リンクは対になる上りリンクと下りリンクを把握しておく必要がある。



COpsNode::onDeparture
先頭のパケットを取り出してCOpsLink::onArrivalイベントを生成。
出力バッファが空でない場合は、バッファの先頭パケットのonDepartureイベントを生成。

COpsLink::onArrival
パケットをバッファに突っ込む。
リンク距離、パケットサイズから出力時間を計算してonDepartureイベントを生成。

COpsLink::onDeparture
バッファからパケット取り出して、COpsNode::onArrivalを生成。



*/



//end of opsnetwork.cpp


