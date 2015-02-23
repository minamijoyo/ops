#ifndef _FIXEDSTACK_H__
#define _FIXEDSTACK_H__
//fixedstack.h

/////////////////////////////////////////////////
//			class CFixedStack
/////////////////////////////////////////////////
//å≈íËí∑ÉXÉ^ÉbÉN
//T_value:äiî[Ç∑ÇÈílÇÃå^
//T_size:äiî[Ç∑ÇÈóvëfêî
template<typename T_value,unsigned int T_size>
class CFixedStack{
	T_value vec_[T_size];
	unsigned int sp_;
public:
	CFixedStack():sp_(0){}
	~CFixedStack(){}

	bool empty() const { return sp_==0; }
	size_t size() const { return sp_; }
	void push(const T_value& x){ vec_[sp_++]=x;	}
	void pop(){	--sp_; }
	const T_value& top() const{	return vec_[sp_-1];	}
};

//end of fixedstack.h
#endif

