//opsnetwork.cpp
#include"opsnetwork.h"
#include"event.h"

//�f�o�b�O�p
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

	//�m�[�h������N���̎擾
	const node_param_vec_t node_param = p_topology_->get_node_param();
	const link_param_vec_t link_param = p_topology_->get_link_param();

	//�m�[�h����
	for(itn_t itn=node_param.begin();itn!=node_param.end();++itn){
		node_.push_back(p_setting_->p_node_maker_->node_new(this,*itn));
	}
	//�����N����
	for(itl_t itl=link_param.begin();itl!=link_param.end();++itl){
		link_.push_back(new COpsLink(this,*itl));
	}
	//�m�[�h������N�Ԃ̑��݃|�C���^�̐ݒ�
	set_connection();
	
	//�p�P�b�g�}�l�[�W���̐���
	p_pk_manager_=new CPacketManager(static_cast<node_index_t>(node_param.size()),p_setting_->seed_);

	//�����W�F�l���[�^�̍쐬(�p�P�b�g�̓������z)
	p_rnd_exp_=new opssimu::CRandMtExp<simuframe::simutime_t>(p_setting_->seed_,p_setting_->lambda_);//seed,�w�����z(lambda)

	//�C�x���g�}�l�[�W���̎擾
	p_event_manager_=simuframe::CEventManager::getInstance();

	//���v�}�l�[�W���̐���
	const simutime_t interval=10000;
	p_stat_manager_=new simuframe::CStatManager(p_event_manager_,interval);
	//���v���Ƃ�I�u�W�F�N�g�̓o�^
	p_stat_manager_->add(this,EStatableLabel::NETWORK);
	p_stat_manager_->add(p_pk_manager_,EStatableLabel::PK_MGR);
	for(node_vec_t::iterator itn=node_.begin();itn!=node_.end();++itn){
		p_stat_manager_->add(*itn,EStatableLabel::NODE);
	}
	for(link_vec_t::iterator itl=link_.begin();itl!=link_.end();++itl){
		p_stat_manager_->add(*itl,EStatableLabel::LINK);
	}

	//�V�~�����[�V�����J�n�E�I���C�x���g�̐���
	p_event_manager_->setEvent(new CEventOpsSimuStart(0,this));
	p_event_manager_->setEvent(new CEventOpsSimuEnd(p_setting_->end_time_,this));

	//�ŏ��̓��v�C�x���g�̐���
	const simutime_t transit_time=10000;//���v�J�n�܂ł̉ߓn����
	p_event_manager_->setEvent(new CEventStat(transit_time,p_stat_manager_,CStatManager::NONE));
}

COpsNetwork::~COpsNetwork(){
	//���v�}�l�[�W���̍폜
	delete p_stat_manager_;

	//�����W�F�l���[�^�̍폜
	delete p_rnd_exp_;

	//�p�P�b�g�}�l�[�W���̍폜
	delete p_pk_manager_;

	typedef node_vec_t::iterator itn_t;
	typedef link_vec_t::iterator itl_t;
	
	//�����N�폜
	for(itl_t itl=link_.begin();itl!=link_.end();++itl)
		delete *itl;

	//�m�[�h�폜
	for(itn_t itn=node_.begin();itn!=node_.end();++itn)
		delete *itn;	
}


//�p�P�b�g�����C�x���g
void COpsNetwork::onPacketNew(const CEvent* const p_event){
	CPacket* p_packet=p_pk_manager_->packet_new();
    //std::cout<<"debug:COpsNetwork::opPacketNew():"<<(void*)p_packet<<std::endl;
	//�m�[�h�Ƀp�P�b�g�𓊂���
	const simutime_t now=p_event->time();
	p_event_manager_->setEvent(new CEventNodePacketNew(
		now,static_cast<COpsNode*>(node_[p_packet->src()]),p_packet));
	//���̃p�P�b�g�����C�x���g�𐶐�����
	p_event_manager_->setEvent(new CEventNetworkPacketNew(now+(*p_rnd_exp_)(),this));
}

//�p�P�b�g�폜�C�x���g
void COpsNetwork::onPacketDeleteOk(const CEvent* const p_event){
	p_pk_manager_->packet_delete_ok(
			(static_cast<const CEventNetworkPacketDeleteOk* const>(p_event))->packet()
		);
}

//�p�P�b�g�p���C�x���g
void COpsNetwork::onPacketDeleteLoss(const CEvent* const p_event){
	p_pk_manager_->packet_delete_loss(
			(static_cast<const CEventNetworkPacketDeleteLoss* const>(p_event))->packet()
		);
}

//���v�����C�x���g
void COpsNetwork::onStat(const CEvent* const p_event){
}

/////////////////////////////////////////////////
//			class COpsNode
/////////////////////////////////////////////////
//�R���X�g���N�^
COpsNode::COpsNode(CNetwork* const p_network,const SNodeParam& param)
	:CNode(p_network,param.index_),param_(param),
	outbuf_(param.out_link_.size(),
			CPacketBuf((static_cast<COpsNetwork* const>(p_network)->setting())->buf_sz_,
					(static_cast<COpsNetwork* const>(p_network)->setting())->buf_th_)
	){
	//�C�x���g�}�l�[�W���̎擾
	p_event_manager_=simuframe::CEventManager::getInstance();

	//���[�`���O�e�[�u���̐���
	p_routing_table_= new CRoutingTableAPall(p_network_,index_);

	//TTL�}�[�W���̐ݒ�
	ttl_margin_=(static_cast<COpsNetwork* const>(p_network_)->setting())->ttl_margin_;
	/*
	//�f�o�b�O�p�o�H�z�b�v����\��
	for(int i=0;i<28;i++)
		std::cout<<p_routing_table_->path(i).size()<<" ";
	std::cout<<std::endl;
	*/
}


//�f�X�g���N�^
COpsNode::~COpsNode(){
	//�o�̓o�b�t�@�̃p�P�b�g���̂Ă�
	typedef outbuf_t::iterator oit_t;
	typedef pkbuf_t::iterator pit_t;
	for(oit_t oit=outbuf_.begin();oit!=outbuf_.end();++oit){
		for(pit_t pit=oit->begin();pit!=oit->end();++pit){
			delete (*pit);
		}
	}
	//���[�`���O�e�[�u���̍폜
	delete p_routing_table_;
}

//�p�P�b�g�����C�x���g
void COpsNode::onPacketNew(const CEvent* const p_event){
	stat_.pk_new_++;
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodePacketNew* const>(p_event))->packet()
		);

	//�o�H�̐ݒ�
	const IRoutingTable::path_t& path=p_routing_table_->path(p_packet->dis());
	p_packet->set_path(path);
	p_packet->set_sp_hop(static_cast<CPacket::hop_t>(path.size()));
	//p_packet->set_ttl(static_cast<CPacket::ttl_t>(path.size()*ttl_margin_));�}�[�W����{���Őݒ�
	p_packet->set_ttl(static_cast<CPacket::ttl_t>(path.size()+ttl_margin_));//�}�[�W�����z�b�v���œ���
	//p_packet->set_ttl(9);//TTL���Œ�

	//�m�[�h�����C�x���g
	p_event_manager_->setEvent(new CEventNodeArrival(p_event->time(),this,p_packet,0/*�|�[�g�ԍ��̓_�~�[*/));

	
}
//�p�P�b�g�폜�C�x���g
void COpsNode::onPacketDeleteOk(const CEvent* const p_event){
	stat_.pk_delete_ok_++;
	p_event_manager_->setEvent(new CEventNetworkPacketDeleteOk(
			p_event->time(),
			static_cast<COpsNetwork* const>(p_network_),
			(static_cast<const CEventNodePacketDeleteOk* const>(p_event))->packet()
			)
		);
}
//�p�P�b�g�p���C�x���g
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

//�p�P�b�g�����C�x���g
void COpsNode::onArrival(const CEvent* const p_event){
	//���̓o�b�t�@���l������ꍇ�͂����ɒǉ�

	//�Ƃ肠�����T�[�r�X�C�x���g�𐶐����邾��
	p_event_manager_->setEvent(new CEventNodeService(
			p_event->time(),
			this,
			(static_cast<const CEventNodeArrival* const>(p_event))->packet(),
			(static_cast<const CEventNodeArrival* const>(p_event))->in_port()
		));
}

//�p�P�b�g�T�[�r�X�C�x���g�̋��ʏ������[�`��
//���[�`���O�|���V�[�̕ύX��onRouting()���J�X�^�}�C�Y
void COpsNode::onService(const CEvent* const p_event){
	stat_.pk_service_++;
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodeService* const>(p_event))->packet()
		);

	if(!p_packet->empty()){//�p�P�b�g�����M���m�[�hor���p�m�[�h�ɓ�������
		//TTL�̃`�F�b�N(�z�b�v������)
		if(p_packet->ttl()){//TTL��0�łȂ��ꍇ
			//TTL�̍X�V
			p_packet->dec_ttl_inc_hop();//1���炷
			//���[�`���O����
			onRouting(p_event);
		
		}else{//TTL��0�̏ꍇ
			//�p�P�b�g��p��
			p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[p_packet->top_port()]))
			);
		}
	}else{//�p�P�b�g������m�[�h�ɓ�������
		/*
		//����`�F�b�N
		if(p_packet->dis()!=param_.index_){
			std::cerr<<"COpsNode::onService():distination check fault."
			<<"dis:"<<p_packet->dis()<<"node:"<<param_.index_<<std::endl;
		}
		*/
		//�p�P�b�g���폜
		p_event_manager_->setEvent(
			new CEventNodePacketDeleteOk(p_event->time(),this,p_packet)
		);
	}
}

//���[�`���O����
//�f�t�H���g�͌Œ�o�H�B���[�`���O�|���V�[�͔h���N���X�ŃJ�X�^�}�C�Y�\�B
void COpsNode::onRouting(const CEvent* const p_event){
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodeService* const>(p_event))->packet()
		);
	//�o�̓|�[�g���擾
	const port_t out_port=p_packet->top_port();

	//�o�b�t�@�e�ʂ̃`�F�b�N
	//�Œ�o�H�̏ꍇ�̓o�b�t�@臒l�𖳎����ăo�b�t�@�ő�T�C�Y�܂Œǉ��\�B
	if(outbuf_[out_port].insert_ok_max(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
		p_packet->pop_port();//�擪�̃��x�����̂Ă�
		onOutbufPush(p_event,p_packet,out_port);//�o�b�t�@�ɒǉ�
	}else{//�o�b�t�@���̏ꍇ�p��
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[out_port]))
			);
	}
}

//�o�̓o�b�t�@�Ƀp�P�b�g��ǉ�����
//CPacketBuf::insert_ok()�͕ۏႳ��Ă��邱�Ƃ�O��Ƃ���
void COpsNode::onOutbufPush(const CEvent* const p_event,CPacket* const p_packet,const port_t out_port){
	if(!outbuf_[out_port].empty()){//�o�̓o�b�t�@����łȂ��ꍇ
		outbuf_[out_port].push_back(p_packet);//�p�P�b�g���o�̓o�b�t�@�ɓ˂�����
	
	}else{//�o�̓o�b�t�@����̏ꍇ
		outbuf_[out_port].push_back(p_packet);//�p�P�b�g���o�̓o�b�t�@�ɓ˂�����

		//�p�P�b�g�ދ��C�x���g�̐���
		//�p�P�b�g�������N�ɏo�͂���̂ɗv���鎞�Ԃ̌v�Z
		//�o�̓����N���x���擾
		//const double link_speed=40000;//�Ƃ肠�����ݒ�t�@�C��������40Gbps=40000bit/us 
		//const simutime_t output_time=p_packet->size()/link_speed;
		simutime_t output_time=p_packet->size()/
			static_cast<COpsLink*>(out_link_[out_port])->speed();
		p_event_manager_->setEvent(
			new CEventNodeDeparture(p_event->time()+output_time,this,p_packet,out_port)
		);
	}
}

//�p�P�b�g�ދ��C�x���g
void COpsNode::onDeparture(const CEvent* const p_event){
	//�o�̓|�[�g���擾
	const port_t out_port=(static_cast<const CEventNodeDeparture* const>(p_event))->out_port();
	//�o�̓o�b�t�@����p�P�b�g�����o��
	CPacket* const p_packet=outbuf_[out_port].front();
	outbuf_[out_port].pop_front();
	//�o�̓����N�ɓ�����
	p_event_manager_->setEvent(
			new CEventLinkArrival(p_event->time(),out_link_[out_port],p_packet)
		);
	//�o�̓o�b�t�@�̃`�F�b�N
	if(!outbuf_[out_port].empty()){//�o�̓o�b�t�@����łȂ�
		//���p�P�b�g�̑ދ��C�x���g�̐���
		//�p�P�b�g�������N�ɏo�͂���̂ɗv���鎞�Ԃ̌v�Z
		//�o�̓����N���x���擾
		CPacket* const p_next_packet=outbuf_[out_port].front();		
		//const double link_speed=40000;//�Ƃ肠�����ݒ�t�@�C��������40Gbps=40000bit/us
		//const simutime_t output_time=p_next_packet->size()/link_speed;
		simutime_t output_time=p_packet->size()/
			static_cast<COpsLink*>(out_link_[out_port])->speed();
		p_event_manager_->setEvent(
			new CEventNodeDeparture(p_event->time()+output_time,this,p_next_packet,out_port)
		);
	}//else{}�o�̓o�b�t�@����̏ꍇ�͉������Ȃ�

}
//���v�C�x���g
void COpsNode::onStat(const CEvent* const p_event){}

/////////////////////////////////////////////////
//			class COpsNodeRef
/////////////////////////////////////////////////
//���[�`���O����
void COpsNodeRef::onRouting(const CEvent* const p_event){
	CPacketRef* const p_packet=static_cast<CPacketRef* const>(
		(static_cast<const CEventNodeService* const>(p_event))->packet());

	switch(p_packet->ref_flg() ){
		case CPacketRef::NONE:
			onRoutingRefNONE(p_event,p_packet);//�ʏ�p�P�b�g�̏���
			break;
		case CPacketRef::REF_FWD:
			onRoutingRefFWD(p_event,p_packet);//���˃p�P�b�g�̏���
			break;
		case CPacketRef::REF_RET:
			onRoutingRefRET(p_event,p_packet);//���˂���Ԃ��Ă����p�P�b�g�̏���
			break;
		default:
			std::cerr<<"unknown ref_flg"<<std::endl;
			break;
	}
}

//�ʏ�p�P�b�g�̏���
//���ˌ��m�[�h�ł̏���
void COpsNodeRef::onRoutingRefNONE(const CEvent* const p_event,CPacketRef* const p_packet){
	//�o�̓|�[�g���擾
	const port_t out_port=p_packet->top_port();

	//�o�b�t�@�e�ʂ̃`�F�b�N
	//�ʏ�p�P�b�g��臒l�𒴂��Ēǉ����Ȃ�
	if(outbuf_[out_port].insert_ok_th(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
		p_packet->pop_port();//�擪�̃��x�����̂Ă�
		onOutbufPush(p_event,p_packet,out_port);//�o�b�t�@�ɒǉ�
	}else{//�o�b�t�@���̏ꍇ

		//TTL�̃`�F�b�N
		//TTL��2��菬�����ꍇ�͔��˂��Ȃ�
		//TTL��dec����^�C�~���O�ɒ��ӁBTTL=2�̏ꍇ��ok�B
		if(p_packet->ttl()<2){
			//臒l�𒴂��Ď�o�H�ɒǉ������݂�
			if(outbuf_[out_port].insert_ok_max(p_packet)){//�ǉ��\�̏ꍇ
				p_packet->pop_port();//�擪�̃��x�����̂Ă�
				onOutbufPush(p_event,p_packet,out_port);//�o�b�t�@�ɒǉ�
			}else{//�ǉ��ł��Ȃ��ꍇ
				//�p��
				p_event_manager_->setEvent(
						new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
						static_cast<COpsLink*>(out_link_[out_port]))
					);
			}

		}else{//TTL��2�ȏ�̏ꍇ
			//����
			send_ref_pk_common(p_event,p_packet,out_port);
		}
	}

}
//���˃p�P�b�g�̏���
//���ː�m�[�h�ł̏���
void COpsNodeRef::onRoutingRefFWD(const CEvent* const p_event,CPacketRef* const p_packet){
	//����Ԃ����I��
	//���Ɖ��胊���N�̓g�|���W�t�@�C����ŘA�����Ă���̂ŁA�����|�[�g�ԍ��ƂȂ�ɂȂ邱�Ƃ�
	//�Öق̂����ɉ��肳��Ă���
	const port_t return_port=(static_cast<const CEventNodeService* const>(p_event))->in_port();
	
	//�o�b�t�@�e�ʂ̃`�F�b�N
	//臒l�𒴂��Ēǉ��\�B
	if(outbuf_[return_port].insert_ok_max(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
		//���˃t���O�̍X�V
		p_packet->set_ref_flg(CPacketRef::REF_RET);
		//�o�b�t�@�ɒǉ�
		onOutbufPush(p_event,p_packet,return_port);
	}else{
		//�o�b�t�@���̏ꍇ
		//�p��
		p_event_manager_->setEvent(
					new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
					static_cast<COpsLink*>(out_link_[return_port]))
			);
	}
}
//���˂���Ԃ��Ă����p�P�b�g�̏���
//���ˌ��m�[�h�ł̏���
void COpsNodeRef::onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet){
	//�o�̓|�[�g���擾
	const port_t out_port=p_packet->top_port();

	//���˃t���O��NONE�ɖ߂�
	p_packet->set_ref_flg(CPacketRef::NONE);

	//�o�b�t�@�e�ʂ̃`�F�b�N
	//臒l�𒴂��Ēǉ��\�B
	if(outbuf_[out_port].insert_ok_max(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
		p_packet->pop_port();//�擪�̃��x�����̂Ă�
		onOutbufPush(p_event,p_packet,out_port);//�o�b�t�@�ɒǉ�
	}else{//�o�b�t�@���̏ꍇ
		
		if(p_packet->ttl()<2){//TTL�̃`�F�b�N
			//�p��
			p_event_manager_->setEvent(
					new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
					static_cast<COpsLink*>(out_link_[out_port]))
			);
		
		}else{
			//�Ĕ���
			send_ref_pk_common(p_event,p_packet,out_port);
		}
	}
}

//���˃p�P�b�g���M�̋��ʃ��[�`��
void COpsNodeRef::send_ref_pk_common(const CEvent* const p_event,CPacketRef* const p_packet,
									 const port_t out_port){
	//���ː�̑I��
	const port_t target=select_ref_port(p_packet,out_port);
	
	if(target!=out_port){//���ː悪��������
		//���˃t���O�̐ݒ�
		p_packet->set_ref_flg(CPacketRef::REF_FWD);
		p_packet->set_ref_id(ref_pk_id_++);
		p_packet->set_group_flg(1);//���v�p�Ɉ��t����
		p_packet->inc_count();//���ˉ񐔋L�^
		//�o�̓o�b�t�@�ɒǉ�
		onOutbufPush(p_event,p_packet,target);
	}else{
		//���ː悪������Ȃ�����
		//�p��
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[out_port]))
			);
	}
}

//���ː�̑I��
//���ː悪������Ȃ��ꍇ��out_port��Ԃ�
const CNode::port_t COpsNodeRef::select_ref_port(const CPacket* const p_packet,const port_t out_port){
	const port_t end=ref_port_;//ref_port_�͑O��̔��ː�B���[�v�`�F�b�N�p��end�ɋL�^���Ă����B

	//���E���h���r��
	ref_port_= (ref_port_< (out_link_num_-1))? ref_port_+1 : 0;//1���₷(out_link_num_�͏o�̓|�[�g�̐�)
	//�]�T�̂���o�b�t�@��������܂Ń��[�v
	//���˃p�P�b�g��臒l�𒴂��Ēǉ����Ȃ�
	while(!outbuf_[ref_port_].insert_ok_th(p_packet)){
		if(end==ref_port_){
			return out_port;//�S�Ẵ|�[�g�𒲂ׂ���������Ȃ�����
		}
		ref_port_= (ref_port_< (out_link_num_-1))? ref_port_+1 : 0;
	}
	return ref_port_;
}

/////////////////////////////////////////////////
//			COpsNodeDef
/////////////////////////////////////////////////
//���[�`���O����
void COpsNodeDef::onRouting(const CEvent* const p_event){
	CPacketPortStack* const p_packet=static_cast<CPacketPortStack* const>(
			(static_cast<const CEventNodeService* const>(p_event))->packet()
		);
	//�o�̓|�[�g���擾
	const port_t out_port=p_packet->top_port();

	//�o�b�t�@�e�ʂ̃`�F�b�N
	//�ʏ�p�P�b�g��臒l�𒴂��Ēǉ����Ȃ��B
	if(outbuf_[out_port].insert_ok_th(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
		p_packet->pop_port();//�擪�̃��x�����̂Ă�
		onOutbufPush(p_event,p_packet,out_port);//�o�b�t�@�ɒǉ�
	}else{//�o�b�t�@���̏ꍇ
		//�I��
		onDefRouting(p_event,p_packet);	
	}
}

/*
//�I�񏈗�(CRoutingTableSPAP��)
void COpsNodeDef::onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet){
	//�I��o�H�̐ݒ�
	p_packet->set_path(
		static_cast<const CRoutingTableSPAP*>(p_routing_table_)->alternate_path(p_packet->dis())
		);
	//�I��o�̓|�[�g���擾
	const port_t def_port=p_packet->top_port();

	//�o�b�t�@�e�ʂ̃`�F�b�N
	if(outbuf_[def_port].insert_ok(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
		p_packet->pop_port();//�擪�̃��x�����̂Ă�
		onOutbufPush(p_event,p_packet,def_port);//�o�b�t�@�ɒǉ�
	}else{//�o�b�t�@���̏ꍇ
		//�p��
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet)
			);
	}
}
*/

//�I�񏈗�(CRoutingTableAPall��)
void COpsNodeDef::COpsNodeDefImpl::onDefRouting(const CEvent* const p_event,CPacketPortStack* const p_packet,
												COpsNode* const p_this_node){
	typedef CRoutingTableAPall::path_all_t path_all_t;
	//�I��o�H�̌�⃊�X�g���擾(�I��R�X�g���Ⴂ���Ƀ\�[�g����Ă���)
	const path_all_t& alternate=
		static_cast<const CRoutingTableAPall*>(p_routing_table_)->alternate_path_all(p_packet->dis());
	//���ׂĂ̌��ɂ���
	for(path_all_t::const_iterator it=alternate.begin();it!=alternate.end();++it){
		//it->first�͉I��o�H��\��

		//�I��o�H�̃z�b�v����TTL�ȓ��ł��邩���`�F�b�N
		//TTL��dec�����^�C�~���O�ɒ��ӁB
		if(p_packet->ttl() < ((it->first).size()-1)){//TTL������Ȃ��ꍇ
			//�I��H�I����break���Ď�o�H�ւ̒ǉ������݂�
			break;

		}else{//TTL�`�F�b�Nok�̏ꍇ
			const port_t def_port=(it->first).top();//�I���|�[�g
			//�I��|�[�g�̃o�b�t�@�e�ʂ̃`�F�b�N
			//臒l�𒴂��Ēǉ����Ȃ��B
			if(outbuf_[def_port].insert_ok_th(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
				//�I��o�H�̐ݒ�
				p_packet->set_path(it->first);
				p_packet->set_group_flg(2);//���v�p�Ƀ}�[�N������
				//���M
				p_packet->pop_port();//�擪�̃��x�����̂Ă�
				p_this_node->onOutbufPush(p_event,p_packet,def_port);//�o�b�t�@�ɒǉ�
				return;
			}
		}
	}
	//�����ɗ���͉̂I��悪������Ȃ������ꍇor�I��o�H��TTL������Ȃ��ꍇ
	//臒l�𒴂��Ď�o�H�ɒǉ������݂�
	const port_t out_port=p_packet->top_port();//�o�̓|�[�g���擾
	if((outbuf_[out_port].insert_ok_max(p_packet))&&
		!(p_packet->ttl() < p_packet->path_size()-1)){//�o�b�t�@�ɒǉ��\&&�c��z�b�vOK�̏ꍇ
		p_packet->pop_port();//�擪�̃��x�����̂Ă�
		p_this_node->onOutbufPush(p_event,p_packet,out_port);//�o�b�t�@�ɒǉ�
	}else{//�o�b�t�@���or��o�H�̎c��z�b�v����Ȃ��ꍇ
		//�p��
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
//���˂���Ԃ��Ă����p�P�b�g�̏���
//���ˌ��m�[�h�ł̏���
void COpsNodeRefDef::onRoutingRefRET(const CEvent* const p_event,CPacketRef* const p_packet){

	//�o�̓|�[�g���擾
	const port_t out_port=p_packet->top_port();

	//���˃t���O��NONE�ɖ߂�
	p_packet->set_ref_flg(CPacketRef::NONE);

	//��o�H��TTL���`�F�b�N�B�c��z�b�v����菬������Δp���B
	if(p_packet->ttl()<p_packet->path_size()-1){
		//�c��z�b�v���ɓ͂��Ȃ��ꍇ
		//�p��
		p_event_manager_->setEvent(
				new CEventNodePacketDeleteLoss(p_event->time(),this,p_packet,
				static_cast<COpsLink*>(out_link_[out_port]))
		);
	
	}else{
		//TTL�`�F�b�Nok�̏ꍇ
		//��o�H�̃o�b�t�@�e�ʂ̃`�F�b�N
		//��o�H�ɂ�臒l�𒴂��Ēǉ����Ȃ��B
		if(outbuf_[out_port].insert_ok_th(p_packet)){//�o�b�t�@�ɒǉ��\�̏ꍇ
			p_packet->pop_port();//�擪�̃��x�����̂Ă�
			onOutbufPush(p_event,p_packet,out_port);//�o�b�t�@�ɒǉ�
		}else{//�o�b�t�@���̏ꍇ
			//�I��
			onDefRouting(p_event,p_packet);
		}
		/*
		//�A���Ă������o�H�̏�Ԃɂ�炸�I��
		onDefRouting(p_event,p_packet);
		*/
	}
}


/////////////////////////////////////////////////
//			class COpsLink
/////////////////////////////////////////////////
//�R���X�g���N�^
COpsLink::COpsLink(CNetwork* const p_network,const SLinkParam& param)
	:CLink(p_network,param.index_),param_(param){
	//�C�x���g�}�l�[�W���̎擾
	p_event_manager_=simuframe::CEventManager::getInstance();
}

//�f�X�g���N�^
COpsLink::~COpsLink(){
	//�o�b�t�@�̃p�P�b�g���̂Ă�
	typedef pkbuf_t::iterator pit_t;
	for(pit_t pit=pkbuf_.begin();pit!=pkbuf_.end();++pit){
		delete (*pit);
	}

	//�f�o�b�O�p
	std::cout<<param_.index_<<","<<"load:"<<load()<<std::endl;
	std::cout<<param_.index_<<","<<"link_loss_ratio:"<<loss_ratio()<<std::endl;
}

//�p�P�b�g�����C�x���g
void COpsLink::onArrival(const CEvent* const p_event){
	
	//�p�P�b�g���o�b�t�@�ɓ����
	CPacket* const p_packet=(static_cast<const CEventLinkArrival* const>(p_event))->packet();
	pkbuf_.push_back(p_packet);
	//�ދ��C�x���g�̐���
	const double propagation_speed=0.2;//�t�@�C�o���̌��̓`�d���x20��km/s=0.2km/us 
	const simutime_t delay_time=param_.dis_/propagation_speed;//us
	p_event_manager_->setEvent(
		new CEventLinkDeparture(p_event->time()+delay_time,this,p_packet)
	);
}
//�p�P�b�g�ދ��C�x���g
void COpsLink::onDeparture(const CEvent* const p_event){
	stat_.pk_count_++;
	const simutime_t now=p_event->time();
	CPacket* const p_packet=pkbuf_.front();

	stat_.pk_size_sum_+=p_packet->size();

	//���̃m�[�h�ɓ�����
	p_event_manager_->setEvent(
		new CEventNodeArrival(now,p_to_,p_packet,to_port_)
	);
	pkbuf_.pop_front();
}
//���v�C�x���g
void COpsLink::onStat(const CEvent* const p_event){}

//�p�P�b�g�����̎d�g��
/*
COpsNetwork::onPacketNew
�l�b�g���[�N���p�P�b�g�𐶐����ăm�[�h�Ɋ���U��

COpsNode::onPacketNew
�m�[�h�Ōo�H���w�b�_�ɐݒ�BCOpsNode::onArrival�𐶐�

COpsNode::onPacketDeleteOk
�]���������J�E���g����CopsNetwork::onPacketDeleteOk�𐶐��B

CopsNetwork::onPacketDeleteOk
������������p�P�b�g�}�l�[�W���Ɉ˗��B

COpsNode::onArrival
���̓o�b�t�@���l������ꍇ�͂����ŏ����B
���̂Ƃ���͂Ȃ���Ȃ��B���������J�E���g���邮�炢�B

COpsNode::onService
���悪�������g(�ړI�n�ɓ͂���)�̏ꍇ�ACOpsNode::onPacketDeleteOk�𐶐��B
�o�̓|�[�g���擾�B
�p�P�b�g���o�b�t�@�ɓ˂����ށB
���̂Ƃ��A�o�b�t�@����̏ꍇ�́A�p�P�b�g�T�C�Y����T�[�r�X���Ԃ����߂āA
onDeparture�C�x���g�𐶐��B

���ː��������ꍇ
�p�P�b�g���o�b�t�@�ɓ˂����ޑO�ɁA�o�b�t�@�󋵂��m�F�B
���l�ȏ�̏ꍇ�́A���ː�m�[�h��I������B(�I��@�͖���)
���ː�m�[�h�����肵����p�P�b�g�̔��˃t���O1��true�ɂ��āA
���ː�m�[�h�ւ̏o�̓o�b�t�@�ɓ˂����ށB�o�b�t�@����Ȃ�onDeparture�𐶐��B

���˃p�P�b�g���󂯎�����ꍇ�̏���
���˃t���O1��true�̂Ƃ��A���˃t���O2��true�ɂ��āA���̃m�[�h�ɑ���Ԃ��B
���̂��߂Ɋe�����N�͑΂ɂȂ��胊���N�Ɖ��胊���N��c�����Ă����K�v������B



COpsNode::onDeparture
�擪�̃p�P�b�g�����o����COpsLink::onArrival�C�x���g�𐶐��B
�o�̓o�b�t�@����łȂ��ꍇ�́A�o�b�t�@�̐擪�p�P�b�g��onDeparture�C�x���g�𐶐��B

COpsLink::onArrival
�p�P�b�g���o�b�t�@�ɓ˂����ށB
�����N�����A�p�P�b�g�T�C�Y����o�͎��Ԃ��v�Z����onDeparture�C�x���g�𐶐��B

COpsLink::onDeparture
�o�b�t�@����p�P�b�g���o���āACOpsNode::onArrival�𐶐��B



*/



//end of opsnetwork.cpp


