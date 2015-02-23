#ifndef _HISTOGRAM_H__
#define _HISTOGRAM_H__
//histogram.h
#include<boost/array.hpp>
#include<numeric>

/////////////////////////////////////////////////
//			CHistogram
/////////////////////////////////////////////////
//ヒストグラムの管理
//各種統計量は頻度を元に計算するので精度が落ちることに注意
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
	CHistogram(){}//配列初期化用にデフォルトコンストラクタ(必ず手動でinitを呼ぶこと)
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

	void push(const T& value){//データ追加速度を重視し、統計処理は頻度を元に後で必要になってから計算する。
		if(value < lower_){
			data_[0]++;
		}else if( upper_ <= value){
			data_[N-1]++;
		}else{
			data_[static_cast<unsigned int>((value-lower_)/step_)]++;
		}
	}
	//頻度の合計
	unsigned int count()const{ return std::accumulate(data_.begin(),data_.end(),0); }
	//値的に合計
	double sum()const{
		double total=0;
		for(size_t i=0;i<N;i++){
			total+= data_[i]*(lower_+step_*i);
		}
		return total;
	}
	//平均
	double average()const{ return sum()/count(); }
	//分散
	double variance()const{
		double total=0;
		double av=average();
		for(size_t i=0;i<N;i++){
			double tmp=data_[i]*(lower_+step_*i-av);
			total+= tmp*tmp;
		}
		return total/count();
	}
	//最大値を返す。追加時にupper_を超えた値はupper_-step_にカウントされていることに注意。
	//upper_を超えたデータはすべて、upper_-step_とみなされる。
	const T max_element()const{
		for(size_t i=N-1;i>0;i--){//後ろから見てはじめの0以外の頻度を探す
			if(data_[i]){
				return lower_+i*step_;//見つかった
			}
		}
		//見つからなかった場合とりあえずlower_を返す
		return lower_;
	}

	//頻度の累積％がratioをはじめて超える境界の値を返す
	const T percent_bound(double ratio)const{
		double bound_count=ratio*count();
		unsigned int now_count=0;
		for(size_t i=0;i<N;i++){
			now_count+=data_[i];
			if(now_count>bound_count){
				return lower_+i*step_;
			}
		}
		//見つからなかった場合はとりあえずupper_を返す
		return upper_;
	}
	
};
//end of histogram.h
#endif

