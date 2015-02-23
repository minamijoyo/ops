#ifndef _EVENTFWD_H__
#define _EVENTFWD_H__
//eventfwd.h
//イベントの前方宣言

namespace opssimu{
//ネットワーク関係
class CEventNetworkPacketNew;
class CEventNetworkPacketDeleteOk;
class CEventNetworkPacketDeleteLoss;

//ノード関係
class CEventNodePacketNew;
class CEventNodePacketDeleteOk;
class CEventNodePacketDeleteLoss;
class CEventNodeArrival;
class CEventNodeService;
class CEventNodeDeparture;

//リンク関係
class CEventLinkArrival;
class CEventLinkDeparture;

//シミュレーション関係
class CEventOpsSimuStart;
class CEventOpsSimuEnd;
class CEventDebug1;

}//end of namespace opssimu

//end of eventfwd.h
#endif //_EVENTFWD_H__

