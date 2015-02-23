#ifndef _HISTOGRAM_H__
#define _HISTOGRAM_H__
//histogram.h
#include<boost/array.hpp>
#include<numeric>

/////////////////////////////////////////////////
//			CHistogram
/////////////////////////////////////////////////
//�q�X�g�O�����̊Ǘ�
//�e�퓝�v�ʂ͕p�x�����Ɍv�Z����̂Ő��x�������邱�Ƃɒ���
template<typename T,size_t N> 
class CHistogram{
public:
	typedef boost::array<unsigned int,N> container_t;
	typedef typename container_t::iterator iterator;
	typedef typename container_t::const_iterator const_iterator;
private:
	container_t data_;

	T lower_;
	T upper_;
	double step_;
	
public:
	CHistogram(const T& lower,const T& upper){
		init(lower,upper);
	}
	CHistogram(){}//�z�񏉊����p�Ƀf�t�H���g�R���X�g���N�^(�K���蓮��init���ĂԂ���)
	iterator begin(){ return data_.begin(); }
	iterator end(){ return data_.end(); }
	const_iterator begin()const{ return data_.begin(); }
	const_iterator end()const{ return data_.end(); }
	T operator[](size_t index){ return data_[index]; }
	const T operator[](size_t index)const{ return data_[index]; }
	void init(const T lower, const T upper){
		lower_=lower;
		upper_=upper;
		step_=static_cast<double>(upper-lower)/N;
		std::fill(data_.begin(),data_.end(),0);
	}

	void push(const T& value){//�f�[�^�ǉ����x���d�����A���v�����͕p�x�����Ɍ�ŕK�v�ɂȂ��Ă���v�Z����B
		if(value < lower_){
			data_[0]++;
		}else if( upper_ <= value){
			data_[N-1]++;
		}else{
			data_[static_cast<unsigned int>((value-lower_)/step_)]++;
		}
	}
	//�p�x�̍��v
	unsigned int count()const{ return std::accumulate(data_.begin(),data_.end(),0); }
	//�l�I�ɍ��v
	double sum()const{
		double total=0;
		for(size_t i=0;i<N;i++){
			total+= data_[i]*(lower_+step_*i);
		}
		return total;
	}
	//����
	double average()const{ return sum()/count(); }
	//���U
	double variance()const{
		double total=0;
		double av=average();
		for(size_t i=0;i<N;i++){
			double tmp=data_[i]*(lower_+step_*i-av);
			total+= tmp*tmp;
		}
		return total/count();
	}
	//�ő�l��Ԃ��B�ǉ�����upper_�𒴂����l��upper_-step_�ɃJ�E���g����Ă��邱�Ƃɒ��ӁB
	//upper_�𒴂����f�[�^�͂��ׂāAupper_-step_�Ƃ݂Ȃ����B
	const T max_element()const{
		for(size_t i=N-1;i>0;i--){//��납�猩�Ă͂��߂�0�ȊO�̕p�x��T��
			if(data_[i]){
				return lower_+i*step_;//��������
			}
		}
		//������Ȃ������ꍇ�Ƃ肠����lower_��Ԃ�
		return lower_;
	}

	//�p�x�̗ݐρ���ratio���͂��߂Ē����鋫�E�̒l��Ԃ�
	const T percent_bound(double ratio)const{
		double bound_count=ratio*count();
		unsigned int now_count=0;
		for(size_t i=0;i<N;i++){
			now_count+=data_[i];
			if(now_count>bound_count){
				return lower_+i*step_;
			}
		}
		//������Ȃ������ꍇ�͂Ƃ肠����upper_��Ԃ�
		return upper_;
	}
	
};
//end of histogram.h
#endif

