#ifndef _EVENTFWD_H__
#define _EVENTFWD_H__
//eventfwd.h
//�C�x���g�̑O���錾

namespace opssimu{
//�l�b�g���[�N�֌W
class CEventNetworkPacketNew;
class CEventNetworkPacketDeleteOk;
class CEventNetworkPacketDeleteLoss;

//�m�[�h�֌W
class CEventNodePacketNew;
class CEventNodePacketDeleteOk;
class CEventNodePacketDeleteLoss;
class CEventNodeArrival;
class CEventNodeService;
class CEventNodeDeparture;

//�����N�֌W
class CEventLinkArrival;
class CEventLinkDeparture;

//�V�~�����[�V�����֌W
class CEventOpsSimuStart;
class CEventOpsSimuEnd;
class CEventDebug1;

}//end of namespace opssimu

//end of eventfwd.h
#endif //_EVENTFWD_H__

