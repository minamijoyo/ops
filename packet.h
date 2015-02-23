#ifndef _PACKET_H__
#define _PACKET_H__
//packet.h
//�p�P�b�g�̒�`

#include<vector>
#include<deque>
//#include<boost/pool/pool.hpp>
#include<boost/noncopyable.hpp>
#include"simuframe.h"
#include"topology.h"
#include"mtrandom.h"
#include"fixedstack.h"
#include"histogram.h"

//�f�o�b�O�p
#include<iostream>

//���v�p�Ƀp�P�b�g�̃O���[�v��
#define PACKET_GROUP_MAX 4

//���v�p�̃z�b�v���̏��
#define PACKET_HOP_MAX 16
//���v�p�̌n�����ԕ��z�̕�����
#define PACKET_WATING_DIST_MAX 250
//�p�P�b�g�T�C�Y�Œ�12000bit=1500Byte
//�σT�C�Y�p�P�b�g�𐶐�����ꍇ�́ACPacketManager::packet_new()��ύX
//CPacketBuf::insert_ok_th()���Œ�p�P�b�g���Öقɉ��肵�Ă���̂ŗv�C��
#define CONST_PK_SIZE 12000

namespace opsnetwork{

/////////////////////////////////////////////////
//			CPacket
/////////////////////////////////////////////////
class CPacket{
//	static boost::pool<> packet_pool_;//�p�P�b�g�p�������Ǘ�
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
	const simutime_t time_;//��������
	const pk_size_t size_;//�T�C�Y
	const node_index_t src_;//���M��
	const node_index_t dis_;//����
	ttl_t ttl_;//1�z�b�v�i�ޖ��Ɍ��炳���
	hop_t hop_;//1�z�b�v�i�ޓx�ɑ��₳���
	hop_t sp_hop_;
	group_t group_;//���v�p�ɓ���̏����̃p�P�b�g�Ɉ��t�������Ƃ��Ɏg��
public:
	CPacket(const pk_id_t id,const simutime_t time,const pk_size_t size,
		const node_index_t src,const node_index_t dis)
		:id_(id),time_(time),size_(size),src_(src),dis_(dis),ttl_(255),hop_(0),group_(0){
        //std::cout<<"debug:CPacket:CPacket()"<<std::endl;
    }
	virtual ~CPacket(){
        //std::cout<<"debug:CPacket:~CPacket()"<<std::endl;
}
	
	//�������m�ۋy�ъJ��
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

    void operator delete(void* p);//�R���p�C����inline�œK���o�O�̂��ߖ����I��outline
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
	void set_group_flg(const group_t group){ group_ |= group; }//�r�b�g�a

};


/////////////////////////////////////////////////
//			CPacketPortStack
/////////////////////////////////////////////////
//�o�̓|�[�g�X�^�b�N��p�����\�[�X���[�`���O�p�p�P�b�g
class CPacketPortStack:public CPacket{
public:
	typedef unsigned int port_t;//�o�̓|�[�g�ԍ�
protected:
	typedef CFixedStack<port_t,PACKET_HOP_MAX> path_t;//�q�[�v�Ƀ�������v�����Ȃ��T�C�Y�Œ�X�^�b�N

	path_t path_;//�o�H��̏o�̓|�[�g�̃X�^�b�N	
public:
	CPacketPortStack(const pk_id_t id,const simutime_t time,const pk_size_t size,
		const node_index_t src,const node_index_t dis)
		:CPacket(id,time,size,src,dis){}
	virtual ~CPacketPortStack(){}
	
	bool empty() const { return path_.empty(); }
	void push_port(const port_t port){ path_.push(port); }//�|�[�g��push
	void pop_port(){ path_.pop(); }//�|�[�g��pop
	const port_t top_port(){ return path_.top(); }//�擪�̃|�[�g��Ԃ�
	const size_t path_size() const { return path_.size(); }//�c��z�b�v����Ԃ�

	void set_path(const path_t& path){ path_=path; }
	
};

/////////////////////////////////////////////////
//			CPacketRef
/////////////////////////////////////////////////
class CPacketRef:public CPacketPortStack{
public:
	enum ERefFlg{ NONE=0,REF_FWD=1,REF_RET=3 };//���˃t���O
private:
	ERefFlg ref_flg_;//���ˏ�ԃt���O
	pk_id_t ref_id_;//���˃p�P�b�g���ʎq
	int ref_count_;//���ˉ�
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
//�p�P�b�g�̏����t�]���`�F�b�N����
class CMisorderingChecker:private boost::noncopyable{
	typedef CPacket::pk_id_t pk_id_t;
	typedef CPacket::simutime_t simutime_t;
	typedef CPacket::node_index_t node_index_t;
	typedef std::vector<std::vector<pk_id_t> > id_matrix_t;
	typedef std::vector<std::vector<simutime_t> > time_matrix_t;
	typedef CHistogram<simutime_t,1000> jitter_record_t;
private:
	const node_index_t node_num_;//�m�[�h��
	id_matrix_t id_matrix_;//�Ō�Ɏ�M�����p�P�b�gID��src-dis�y�A�P�ʂŕێ�
	time_matrix_t time_matrix_;//�Ō�Ɏ�M����������src-dis�y�A�P�ʂŕێ�
	jitter_record_t jitter_record_;//�����t�]�����ꍇ�̃W�b�^�[�����ׂċL�^
	pk_id_t pk_total_;//���͂��ꂽ�S�p�P�b�g��
	pk_id_t pk_misordering_;//�����t�]�p�P�b�g��
public:
	CMisorderingChecker(const node_index_t node_num);
	~CMisorderingChecker();

	void check(const CPacket* const p_packet);
	double misordering_ratio()const{ return static_cast<double>(pk_misordering_)/pk_total_; }
};


/////////////////////////////////////////////////
//			CPacketManager
/////////////////////////////////////////////////
//�p�P�b�g�̐���new�A�폜delete�A�p��loss�̊Ǘ�
class CPacketManager:public simuframe::CStatable,private boost::noncopyable{
public:
	typedef CPacket::pk_id_t pk_id_t;
	typedef CPacket::pk_size_t pk_size_t;
	typedef CPacket::simutime_t simutime_t;
	typedef CPacket::node_index_t node_index_t;
	typedef opssimu::seed_t seed_t;
private:
	const node_index_t node_num_;
	const seed_t seed_;//�����̎�
	
	//���v�f�[�^�p�̍\����
	struct CPkMgrStat{
		pk_id_t pk_new_;//���������p�P�b�g��
		pk_id_t pk_delete_ok_;//�������`�������p�P�b�g��
		pk_id_t pk_delete_loss_;//�p�������p�P�b�g��
		pk_id_t pk_delete_;//�`������or�p�������p�P�b�g��=pk_delete_ok+pk_delete_loss
		
		pk_id_t group_pk_delete_ok_[PACKET_GROUP_MAX];//����̃p�P�b�g�̓`��������
		pk_id_t group_pk_delete_loss_[PACKET_GROUP_MAX];
		pk_id_t group_pk_delete_[PACKET_GROUP_MAX];

		simutime_t last_new_time_;//�Ō�Ƀp�P�b�g��new��������
		double arrival_time_av_;//���ϓ����Ԋu
		double wating_time_av_;//���όn���؍ݎ���
		double wating_time_sum_;//�n���؍ݎ��Ԃ̍��v
		double group_wating_time_sum_[PACKET_GROUP_MAX];//����̃p�P�b�g�̌n�����Ԃ̍��v
		CHistogram<simutime_t,PACKET_WATING_DIST_MAX> group_wating_time_dist_[PACKET_GROUP_MAX];//�n�����Ԃ̕��z
		
		double hop_sum_;
		double group_hop_sum_[PACKET_GROUP_MAX];//�z�b�v�����v
		double group_hop_dist_[PACKET_GROUP_MAX][PACKET_HOP_MAX];//�z�b�v���̕��z
		double group_def_hop_dist_[PACKET_GROUP_MAX][PACKET_HOP_MAX];//�I��z�b�v���̕��z
		double group_ref_hop_dist_[PACKET_GROUP_MAX][PACKET_HOP_MAX];//���˃z�b�v���̕��z

		double pk_size_sum_ok_;//�`�������p�P�b�g�T�C�Y�̍��v
		double pk_size_sum_;//�폜�p�P�b�g�T�C�Y�̍��v=�`���v���p�P�b�g�T�C�Y(ok+loss)

		std::vector<double> loss_ratio_vec_;
		CMisorderingChecker misordering_checker_;//�����t�]�`�F�b�N
		
		//�R���X�g���N�^
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
		//�p����
		double loss_ratio()const{ 
				return (pk_delete_==0)? 0 : ((double)pk_delete_loss_/pk_delete_);
		}
		double group_loss_ratio(int i) const{
			return (pk_delete_==0)? 0 : ((double)group_pk_delete_loss_[i]/pk_delete_);
		}
		double group_delete_ratio(int i) const{
			return (pk_delete_==0)? 0 : ((double)group_pk_delete_[i]/pk_delete_);
		}

		//�z�b�v��
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

		//�X���[�v�b�g
		//double throughput()const { return (pk_size_sum_ok_/simuframe::CSimu::get_time())/1000.0; }//Gbps
		//double throughput()const { return pk_size_sum_ok_/pk_size_sum_; }//�`���v���Ő��K�� 
		//���όn���؍ݎ���
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

	opssimu::CRandMtUni<double>* p_rnd_size_;//�p�P�b�g���̕��z����邽�߂̗���(�p�P�b�g����2��ތŒ�)
	opssimu::CRandMtUni<node_index_t>* p_rnd_node_;//���M���ƈ���̕��z
private:
	inline void pk_delete_common(CPacket* p_packet);//�p�P�b�g�폜�̋��ʃ��[�`��
public:
	CPacketManager(const node_index_t node_num,const seed_t seed);
	~CPacketManager();

	inline CPacket* packet_new();//�p�P�b�g����
	inline void packet_delete_ok(CPacket* p_packet);//�������`�����ꂽ�p�P�b�g�폜(�p�P�b�g�p���Ƌ�ʂ���)
	inline void packet_delete_loss(CPacket* p_packet);//�p�P�b�g�p��

	virtual void onStat(const simuframe::CEvent* const p_event);//���v����
};

inline
CPacket* CPacketManager::packet_new(){
    //std::cout<<"debug:packet_new():"<<stat_.pk_new_<<std::endl;
	//���M���ƈ��������
	node_index_t src=(*p_rnd_node_)();
	node_index_t dis=(*p_rnd_node_)();
	while(src==dis){//���悪�������g�ɂȂ�Ȃ��悤��
		dis=(*p_rnd_node_)();
	}

	//�p�P�b�g����
	return new CPacketRef(++(stat_.pk_new_),//id(0�Ԃ͎g�p���Ȃ��B�����t�]�̏����l�Ɏg���̂ŁB)
						simuframe::CSimu::get_time(),//����
						//(( (*p_rnd_size_)() < 0.6)? (64*8) : (1500*8)),//�p�P�b�g��
						CONST_PK_SIZE,//�p�P�b�g���Œ�
						src,//���M��
						dis//����
					); 
}

//�������`�����ꂽ�p�P�b�g�폜(�p�P�b�g�p���Ƌ�ʂ���)
inline
void CPacketManager::packet_delete_ok(CPacket* p_packet){
    //std::cout<<"debug:packet_delete_ok():"<<p_packet->id()<<std::endl;
	
	//�`�������p�P�b�g��
	stat_.pk_delete_ok_++; 
	stat_.group_pk_delete_ok_[p_packet->group()]++;
	//stat_.pk_size_sum_ok_+=p_packet->size();//�]���������v�p�P�b�g�T�C�Y�̍X�V(�蔲���I�[�o�[�t���[���邩��)

	//�n�����Ԃ̍��v�X�V
	const simutime_t wating_time=(simuframe::CSimu::get_time()-p_packet->time());
	stat_.wating_time_sum_+= wating_time;
	stat_.group_wating_time_sum_[p_packet->group()] += wating_time;
	stat_.group_wating_time_dist_[p_packet->group()].push(wating_time);

	/*���o�[�W����
	//���όn���؍ݎ��Ԃ̍X�V
	//����=���v����/�p�P�b�g��
	//�Ōv�Z����ƍ��v���Ԃ����̂������傫���Ȃ��Ă��܂��̂ŁA�������X�V����悤�Ɍv�Z�B
	stat_.wating_time_av_+=
		(simuframe::CSimu::get_time()-p_packet->time()-stat_.wating_time_av_)/stat_.pk_delete_;
	stat_.wating_time_sum_+=simuframe::CSimu::get_time()-p_packet->time();
	*/

	//hop��
	stat_.hop_sum_+=p_packet->hop();
	const int additional_hop=p_packet->hop()-p_packet->sp_hop();
	stat_.group_hop_sum_[p_packet->group()]+=(additional_hop);//�������L�^
	stat_.group_hop_dist_[p_packet->group()][additional_hop]++;//�����̕��z���L�^
	int ref_count= (static_cast<CPacketRef*>(p_packet))->ref_count();//���ˉ�
	stat_.group_ref_hop_dist_[p_packet->group()][ref_count*2]++;//���ˑ����̕��z���L�^
	stat_.group_def_hop_dist_[p_packet->group()][(additional_hop - ref_count*2)]++;//�I�񑝕��̕��z���L�^

	//�����t�]�̃`�F�b�N
	//stat_.misordering_checker_.check(p_packet);
	
	//delete���ʃ��[�`��
	pk_delete_common(p_packet);
}

//�p�P�b�g�p��
inline
void CPacketManager::packet_delete_loss(CPacket* p_packet){
	//std::cout<<"debug:packet_delete_loss():"<<p_packet->id()<<std::endl;
	stat_.pk_delete_loss_++;
	stat_.group_pk_delete_loss_[p_packet->group()]++;

	pk_delete_common(p_packet);//delete���ʃ��[�`��
}

//�p�P�b�g�폜���ʃ��[�`��(pk_delete_ok() or pk_delete_loss()����Ă΂��)
inline
void CPacketManager::pk_delete_common(CPacket* p_packet){
	//std::cout<<"debug:packet_delete_common():"<<p_packet->id()<<":"<<(void*)p_packet<<std::endl;
	stat_.pk_delete_++;
	stat_.group_pk_delete_[p_packet->group()]++;
	//stat_.pk_size_sum_+=p_packet->size();//�폜���v�p�P�b�g�T�C�Y�̍X�V(�蔲���I�[�o�[�t���[���邩��)
	
	delete p_packet;
}

//�������v�[���ɗp����ő�T�C�Y�p�P�b�g�̒�`�B
//�Ƃ肠�����܂��p�P�b�g��1�����Ȃ��̂�typedef�̂�
typedef CPacketRef CPacketMaxsize;


}//end of namespace opsnetwork
//end of packet.h
#endif //_PACKET_H__

