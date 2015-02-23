#ifndef _MTRANDOM_H__
#define _MTRANDOM_H__
//mtrandom.h
#include<cmath>
#include<limits>
#include<boost/random.hpp>

#ifdef max
#undef max
#endif

namespace opssimu{
typedef unsigned int seed_t;
/////////////////////////////////////////////////
//			CRandMtUni
/////////////////////////////////////////////////
//乱数生成エンジン：メルセンヌツイスタ法
//分布：一様分布
template<typename T_return=unsigned int>
class CRandMtUni{
	boost::mt19937 rand_;
	const T_return range_;
public:
	CRandMtUni(const seed_t seed,const T_return range)
		:rand_(static_cast<const boost::uint32_t>(seed)),range_(range){}
	const T_return operator()(){ return static_cast<const T_return>(rand_()%range_); }
};

template<>
class CRandMtUni<double>{
	boost::mt19937 rand_;
	const double uint32max_;
public:
	CRandMtUni(const seed_t seed)
		:rand_(static_cast<const boost::uint32_t>(seed)),
		uint32max_(static_cast<const double>(std::numeric_limits<unsigned int>::max())){}
	const double operator()(){ return rand_()/uint32max_; }
};

/////////////////////////////////////////////////
//			CRandMtExp
/////////////////////////////////////////////////
//乱数生成エンジン：メルセンヌツイスタ法
//分布：指数分布
template<typename T_return>
class CRandMtExp{
	boost::mt19937 rand_;
	const double lambda_;
	const double uint32max_;
	//enum{uint32max=std::numeric_limits<boost::uint32_t>::max()};
public:
	CRandMtExp(const seed_t seed,const double lambda)
		:rand_(static_cast<const boost::uint32_t>(seed)),lambda_(lambda),
		uint32max_(static_cast<const double>(std::numeric_limits<unsigned int>::max())){}
		
		const T_return operator()(){ return static_cast<const T_return>(
			-1.0/lambda_* (::log(1.0-rand_()/uint32max_))); }
};

}//end of opssimu
//end of mtrandom.h
#endif //_MTRANDOM_H__

