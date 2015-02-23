#ifndef _OPSNETWORK_H__
#define _OPSNETWORK_H__
//opsnetwork.h
//OPS�l�b�g���[�N�̒�`
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
//CStatable�̃O���[�v�����p�t���O
struct EStatableLabel{
	enum{NETWORK=1,NODE,LINK,PK_MGR};//NONE=0,ALL=0xFFFF��CStatManager�ŗ\�񂳂�Ă���̂Ŏg��Ȃ�
};

//�O���錾
class INodeMaker;
/////////////////////////////////////////////////
//			class COpsNetwork
/////////////////////////////////////////////////
class COpsNetwork:public CNetwork{
public:
	//�l�b�g���[�N�̐ݒ�p�����[�^���܂Ƃ߂��\����
	struct SNetworkSetting{
		simuframe::simutime_t end_time_;//�V�~�����[�V�����J�n����
		double lambda_;//�p�P�b�g�̓�����lambda
		const INodeMaker* p_node_maker_;//�m�[�h�����C���^�[�t�F�C�X
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
	simuframe::ISimu* const p_simu_;//�V�~�����[�V����
	CPacketManager* p_pk_manager_;//�p�P�b�g�}�l�[�W��
	simuframe::CEventManager* p_event_manager_;//�C�x���g�}�l�[�W��
	simuframe::CStatManager* p_stat_manager_;//���v�}�l�[�W��
	opssimu::CRandMtExp<simuframe::simutime_t>* p_rnd_exp_;//�p�P�b�g�̓������z

	const SNetworkSetting* const p_setting_;//�l�b�g���[�N�̐ݒ�p�����[�^
public:
	COpsNetwork(simuframe::ISimu* const p_simu,CTopology* const p_topology,
		const SNetworkSetting* const p_setting);
	virtual ~COpsNetwork();

public:
	virtual void onPacketNew(const CEvent* const p_event);//�p�P�b�g��������
	virtual void onPacketDeleteOk(const CEvent* const p_event);//�p�P�b�g�폜����
	virtual void onPacketDeleteLoss(const CEvent* const p_event);//�p�P�b�g�p������
	virtual void onStat(const CEvent* const p_event);//���v����
	simuframe::ISimu* const get_simu() const { return p_simu_; }

	const SNetworkSetting* const setting() const{ return p_setting_; }


};

/////////////////////////////////////////////////
//			class CPacketBuf
/////////////////////////////////////////////////
//�p�P�b�g�T�C�Y���Ǘ��ł���p�P�b�g�o�b�t�@
class CPacketBuf{
public:
	typedef std::deque<CPacket* > buf_t;
	typedef buf_t::iterator iterator;
private:
	typedef CPacket::pk_size_t pk_size_t;

	buf_t buf_;//�o�b�t�@
	const pk_size_t pk_size_max_;//�ő�e��
	const pk_size_t pk_size_th_;//臒l
	pk_size_t pk_size_;//���݂̃p�P�b�g���̍��v
public:
	CPacketBuf(const pk_size_t pk_size_max,const double threshold):pk_size_max_(pk_size_max),
		pk_size_th_(static_cast<pk_size_t>(
			(pk_size_max-CONST_PK_SIZE)*threshold+CONST_PK_SIZE //�v�C��(���L�Q��)
		)),pk_size_(0){}
	
	//�o�b�t�@�̐擪�𕔕����o�̓C���^�[�t�F�C�X�ƈÖقɉ��肵�Ă��邽�߁A
	//�ϒ��p�P�b�g�̏ꍇ�A������臒l�������Ȃ�
	//�ϒ��Ή����邽�߂ɂ̓o�b�t�@�Əo�̓C���^�[�t�F�C�X��_���I�ɕ�������K�v����

	//�p�P�b�g�̒ǉ�
	void push_back(CPacket* const p_packet){
		pk_size_+=p_packet->size();//�T�C�Y���X�V
		buf_.push_back(p_packet);
	}
	//�p�P�b�g�̍폜
	void pop_front(){
		pk_size_-=buf_.front()->size();//�T�C�Y���X�V
		buf_.pop_front();
	}
	//�擪�̃p�P�b�g��Ԃ�
	CPacket* const front() const{ return buf_.front(); }

	bool empty() const { return buf_.empty(); }
	const pk_size_t pk_size() const{ return pk_size_; }

	//bool insert_ok(const CPacket* const p_packet)const{
	//	return (pk_size_+p_packet->size() <= pk_size_max_);
	//}

	//�ő�T�C�Y�ɑ΂��ăp�P�b�g��ǉ��\���`�F�b�N
	bool insert_ok_max(const CPacket* const p_packet)const{
		return (pk_size_+p_packet->size() <= pk_size_max_);
	}

	//臒l�ɑ΂��ăp�P�b�g��ǉ��\���`�F�b�N
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
	outbuf_t outbuf_;//�o�̓o�b�t�@
	
	//���v�p�\����
	struct SNodeStat{
		typedef CPacket::pk_id_t pk_id_t;
		pk_id_t pk_new_;//���m�[�h�����M���̃p�P�b�g��
		pk_id_t pk_delete_ok_;//���m�[�h����M���̃p�P�b�g��
		pk_id_t pk_delete_loss_;//�p�������p�P�b�g��
		pk_id_t pk_service_;//���p�����p�P�b�g��(���m�[�h�����M���̏ꍇ���܂�)
		SNodeStat():pk_new_(0),pk_delete_ok_(0),pk_delete_loss_(0),pk_service_(0){}
	};
	SNodeStat stat_;
private:
	double ttl_margin_;
public:
	COpsNode(CNetwork* const p_network,const SNodeParam& param);
	virtual ~COpsNode();

	//�C�x���g�n���h��
	virtual void onPacketNew(const CEvent* const p_event);//�p�P�b�g��������
	virtual void onPacketDeleteOk(const CEvent* const p_event);//�p�P�b�g�폜����
	virtual void onPacketDeleteLoss(const CEvent* const p_event);//�p�P�b�g�p������

	virtual void onArrival(const CEvent* const p_event);//��������
	virtual void onService(const CEvent* const p_event);//�T�[�r�X����
	virtual void onDeparture(const CEvent* const p_event);//�ދ�����
	virtual void onStat(const CEvent* const p_event);//���v����

	//protected�ɂ���������DefImpl����R�[�������
	void onOutbufPush(const CEvent* const p_event,CPacket* const p_packet,const port_t out_port);

protected:
	virtual void onRouting(const CEvent* const p_event);//���[�`���O����(�T�[�r�X��������R�[�������)
	
	
};

/////////////////////////////////////////////////
//			class COpsNodeRef
/////////////////////////////////////////////////
//���˂����������m�[�h
class COpsNodeRef:public COpsNode{
	typedef CPacket::pk_id_t pk_id_t;
protected:
	port_t ref_port_;//�O��̔��ː�
	pk_id_t ref_pk_id_;//���˃p�P�b�gID(�l�b�g���[�N�S�̂ł͂Ȃ��m�[�h���ɊǗ�)

public:
	COpsNodeRef(CNetwork* const p_network,const SNodeParam& param)
		:COpsNode(p_network,param),ref_port_(0),ref_pk_id_(0){}
	virtual ~COpsNodeRef(){}
protected:
	virtual void onRouting(const CEvent* const p_event);//���[�`���O����(�T�[�r�X��������R�[�������)
	
	virtual void onRoutingRefNONE(const CEvent* const p_event,CPacketRef* const p_packet);//�ʏ�p�P�b�g�̏���
	virtual void onRoutingRefFWD(const CEvent* const p_event,CPacketRef* const p_packet);//���˃p�P�b�g�̏���
	virtual void onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet);//���˂���Ԃ��Ă����p�P�b�g�̏���

private:
	void send_ref_pk_common(const CEvent* const p_event,CPacketRef* const p_packet,
									 const port_t out_port);//���˃p�P�b�g���M�̋��ʃ��[�`��
	const port_t select_ref_port(const CPacket* const p_packet,const port_t out_port);//���ː�̑I��


};

/////////////////////////////////////////////////
//			COpsNodeDef
/////////////////////////////////////////////////
//�I������������m�[�h
class COpsNodeDef:public COpsNode{
public:
	//�I�񏈗��̎�����public�ȃ��[�J���N���X�Ƃ��ĕ���(COpsNodeRefDef�łЂ��`�p���������Ȃ��̂�)
	class COpsNodeDefImpl{
		const IRoutingTable* const p_routing_table_;//���[�`���O�e�[�u��
		outbuf_t& outbuf_;//�o�b�t�@�ւ̎Q��
		simuframe::CEventManager* const p_event_manager_;
	public:
		COpsNodeDefImpl(const IRoutingTable* const p_routing_table,outbuf_t& outbuf,
			simuframe::CEventManager* const p_event_manager)
			:p_routing_table_(p_routing_table),outbuf_(outbuf),p_event_manager_(p_event_manager){}
		//�I�񃋁[�`���O����
		void onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet,
			COpsNode* const p_this_node);
	};

private:
	COpsNodeDefImpl def_impl_;//���������̃C���X�^���X
public:
	COpsNodeDef(CNetwork* const p_network,const SNodeParam& param)
		:COpsNode(p_network,param),def_impl_(p_routing_table_,outbuf_,p_event_manager_){}
	virtual ~COpsNodeDef(){}
protected:
	virtual void onRouting(const CEvent* const p_event);//���[�`���O����(�T�[�r�X��������R�[�������)
private:
	void onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet){ 
		def_impl_.onDefRouting(p_event,p_packet,this);//�I������ɈϏ����邾��
	}
};

/////////////////////////////////////////////////
//			COpsNodeRefDef
/////////////////////////////////////////////////
//��ĕ����F���ˁE�I����s���m�[�h
class COpsNodeRefDef:public COpsNodeRef{
private:
	COpsNodeDef::COpsNodeDefImpl def_impl_;//�I������̃��[�J���N���X�𗬗p
public:
	COpsNodeRefDef(CNetwork* const p_network,const SNodeParam& param)
		:COpsNodeRef(p_network,param),def_impl_(p_routing_table_,outbuf_,p_event_manager_){}
	virtual ~COpsNodeRefDef(){}
protected:
	virtual void onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet);//���˂���Ԃ��Ă����p�P�b�g�̏���
private:
	void onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet){ 
		def_impl_.onDefRouting(p_event,p_packet,this);//�I������ɈϏ����邾��
	}
};

/////////////////////////////////////////////////
//			class COpsLink
/////////////////////////////////////////////////
class COpsLink:public CLink{
	typedef std::deque<CPacket* > pkbuf_t;
	
	const SLinkParam param_;
	simuframe::CEventManager* p_event_manager_;
	pkbuf_t pkbuf_;//�����N���̃p�P�b�g��ێ����邽�߂̃o�b�t�@

	//���v�p�\����
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
	//�C�x���g�n���h��
	virtual void onArrival(const CEvent* const p_event);//��������
	virtual void onDeparture(const CEvent* const p_event);//�ދ�����
	virtual void onStat(const CEvent* const p_event);//���v����

	void inform_packet_loss(){ stat_.pk_loss_++; }//�p�P�b�g���p�����ꂽ���Ƃ�ʒm����i���v�p�Ɏg����j
};

//Node����
/////////////////////////////////////////////////
//			class INodeMaker
/////////////////////////////////////////////////
//�m�[�h�𐶐�����C���^�[�t�F�C�X
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

