#ifndef _EVENT_H__
#define _EVENT_H__
//event.h

#include"simuframe.h"
#include"opsnetwork.h"

using simuframe::CEvent;
using simuframe::CEventManager;
using simuframe::event_id_t;
using simuframe::simutime_t;
using opsnetwork::COpsNetwork;
using opsnetwork::COpsNode;
using opsnetwork::COpsLink;
using opsnetwork::CNode;
using opsnetwork::CLink;
using opsnetwork::CPacket;

namespace opssimu{

//イベントの定義
//onEvent()で実装ハンドラをコールする

//ネットワーク関係
/////////////////////////////////////////////////
//			CEventNetworkPacketNew
/////////////////////////////////////////////////
class CEventNetworkPacketNew:public CEvent{
	COpsNetwork* const p_network_;
public:
	CEventNetworkPacketNew(const simutime_t time,COpsNetwork* const p_network)
		:CEvent(time),p_network_(p_network){}
	~CEventNetworkPacketNew(){}
	void onEvent() const{ p_network_->onPacketNew(this); }
};

/////////////////////////////////////////////////
//			CEventNetworkPacketDeleteOk
/////////////////////////////////////////////////
class CEventNetworkPacketDeleteOk:public CEvent{
	COpsNetwork* const p_network_;
	CPacket* const p_packet_;
public:
	CEventNetworkPacketDeleteOk(const simutime_t time,COpsNetwork* const p_network,CPacket* const p_packet)
		:CEvent(time),p_network_(p_network),p_packet_(p_packet){}
	~CEventNetworkPacketDeleteOk(){}
	void onEvent()const { p_network_->onPacketDeleteOk(this); }

	CPacket* const packet()const{ return p_packet_; }
};

/////////////////////////////////////////////////
//			CEventNetworkPacketDeleteLoss
/////////////////////////////////////////////////
class CEventNetworkPacketDeleteLoss:public CEvent{
	COpsNetwork* const p_network_;
	CPacket* const p_packet_;
public:
	CEventNetworkPacketDeleteLoss(const simutime_t time,COpsNetwork* const p_network,CPacket* const p_packet)
		:CEvent(time),p_network_(p_network),p_packet_(p_packet){}
	~CEventNetworkPacketDeleteLoss(){}
	void onEvent()const { p_network_->onPacketDeleteLoss(this); }

	CPacket* const packet()const{ return p_packet_; }
};

//ノード関係
/////////////////////////////////////////////////
//			CEventNodePacketNew
/////////////////////////////////////////////////
class CEventNodePacketNew:public CEvent{
	COpsNode* const p_node_;
	CPacket* const p_packet_;
public:
	CEventNodePacketNew(const simutime_t time,COpsNode* const p_node,CPacket* const p_packet)
		:CEvent(time),p_node_(p_node),p_packet_(p_packet){}
	~CEventNodePacketNew(){}
	void onEvent()const{ p_node_->onPacketNew(this); }

	CPacket* const packet()const{ return p_packet_; }
};

/////////////////////////////////////////////////
//			CEventNodePacketDeleteOk
/////////////////////////////////////////////////
class CEventNodePacketDeleteOk:public CEvent{
	COpsNode* const p_node_;
	CPacket* const p_packet_;
public:
	CEventNodePacketDeleteOk(const simutime_t time,COpsNode* const p_node,CPacket* const p_packet)
		:CEvent(time),p_node_(p_node),p_packet_(p_packet){}
	~CEventNodePacketDeleteOk(){}
	void onEvent()const { p_node_->onPacketDeleteOk(this); }

	CPacket* const packet()const{ return p_packet_; }
};

/////////////////////////////////////////////////
//			CEventNodePacketDeleteLoss
/////////////////////////////////////////////////
class CEventNodePacketDeleteLoss:public CEvent{
	COpsNode* const p_node_;
	CPacket* const p_packet_;
	COpsLink* const p_link_;
public:
	CEventNodePacketDeleteLoss(const simutime_t time,COpsNode* const p_node,CPacket* const p_packet,
		COpsLink* const p_link)
		:CEvent(time),p_node_(p_node),p_packet_(p_packet),p_link_(p_link){}
	~CEventNodePacketDeleteLoss(){}
	void onEvent()const { p_node_->onPacketDeleteLoss(this); }

	CPacket* const packet()const{ return p_packet_; }
	CLink* const link()const{ return p_link_; }
};

/////////////////////////////////////////////////
//			CEventNodeArrival
/////////////////////////////////////////////////
class CEventNodeArrival:public CEvent{
	CNode* const p_node_;
	CPacket* const p_packet_;
	const CNode::port_t in_port_;
public:
	CEventNodeArrival(const simutime_t time,CNode* const p_node,CPacket* const p_packet,
		const CNode::port_t in_port)
		:CEvent(time),p_node_(p_node),p_packet_(p_packet),in_port_(in_port){}
	~CEventNodeArrival(){}
	void onEvent()const { p_node_->onArrival(this); }

	CPacket* const packet()const{ return p_packet_; }
	const CNode::port_t in_port()const{ return in_port_; }
};

/////////////////////////////////////////////////
//			CEventNodeService
/////////////////////////////////////////////////
class CEventNodeService:public CEvent{
	CNode* const p_node_;
	CPacket* const p_packet_;
	const CNode::port_t in_port_;
public:
	CEventNodeService(const simutime_t time,CNode* const p_node,CPacket* const p_packet,
		const CNode::port_t in_port)
		:CEvent(time),p_node_(p_node),p_packet_(p_packet),in_port_(in_port){}
	~CEventNodeService(){}
	void onEvent()const{ p_node_->onService(this); }

	CPacket* const packet()const{ return p_packet_; }
	const CNode::port_t in_port()const{ return in_port_; }
};

/////////////////////////////////////////////////
//			CEventNodeDeparture
/////////////////////////////////////////////////
class CEventNodeDeparture:public CEvent{
	CNode* const p_node_;
	CPacket* const p_packet_;
	const CNode::port_t out_port_;
public:
	CEventNodeDeparture(const simutime_t time,CNode* const p_node,CPacket* const p_packet,
		const CNode::port_t out_port)
		:CEvent(time),p_node_(p_node),p_packet_(p_packet),out_port_(out_port){}
	~CEventNodeDeparture(){}
	void onEvent()const{ p_node_->onDeparture(this); }

	CPacket* const packet()const{ return p_packet_; }
	const CNode::port_t out_port()const{ return out_port_; }
};

//リンク関係
/////////////////////////////////////////////////
//			CEventLinkArrival
/////////////////////////////////////////////////
class CEventLinkArrival:public CEvent{
	CLink* const p_link_;
	CPacket* const p_packet_;
public:
	CEventLinkArrival(const simutime_t time,CLink* const p_link,CPacket* const p_packet)
		:CEvent(time),p_link_(p_link),p_packet_(p_packet){}
	~CEventLinkArrival(){}
	void onEvent()const { p_link_->onArrival(this); }

	CPacket* const packet()const{ return p_packet_; }
};

/////////////////////////////////////////////////
//			CEventLinkDeparture
/////////////////////////////////////////////////
class CEventLinkDeparture:public CEvent{
	CLink* const p_link_;
	CPacket* const p_packet_;
public:
	CEventLinkDeparture(const simutime_t time,CLink* const p_link,CPacket* const p_packet)
		:CEvent(time),p_link_(p_link),p_packet_(p_packet){}
	~CEventLinkDeparture(){}
	void onEvent()const{ p_link_->onDeparture(this); }

	CPacket* const packet()const{ return p_packet_; }
};



//シミュレーション関係
/////////////////////////////////////////////////
//			CEventOpsSimuStart
/////////////////////////////////////////////////
class CEventOpsSimuStart:public CEvent{
	COpsNetwork* const p_network_;
public:
	CEventOpsSimuStart(const simutime_t time,COpsNetwork* const p_network)
		:CEvent(time),p_network_(p_network){}
	~CEventOpsSimuStart(){
//        std::cout<<"debug:CEventOpsSimuStart:~CEventOpsSimuStart()"<<std::endl;
}
	void onEvent() const{//はじめのパケットを生成
		(CEventManager::getInstance())->setEvent(
			new CEventNetworkPacketNew(time_,p_network_));
	}
};

/////////////////////////////////////////////////
//			CEventOpsSimuEnd
/////////////////////////////////////////////////
class CEventOpsSimuEnd:public CEvent{
	COpsNetwork* const p_network_;
public:
	CEventOpsSimuEnd(const simutime_t time,COpsNetwork* const p_network)
		:CEvent(time),p_network_(p_network){}
	~CEventOpsSimuEnd(){}
	void onEvent()const{//ISimu::end()を呼ぶ
		(p_network_->get_simu())->end();
	}
};


/////////////////////////////////////////////////
//			CEventDebug1
/////////////////////////////////////////////////
//デバッグ用のイベント
class CEventDebug1:public CEvent{
	const int param_;
public:
	CEventDebug1(const simutime_t time,const int param):CEvent(time),param_(param){}
	~CEventDebug1(){}
	void onEvent()const;
};


}//endo of namespace opssimu
//end of event.h
#endif //_EVENT_H__

