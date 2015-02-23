#ifndef _SIMUFRAME_H__
#define _SIMUFRAME_H__
//simuframe.h

#include<queue>
#include<vector>
#include<functional>
#include<algorithm>
//#include<boost/pool/pool.hpp>
#include<boost/noncopyable.hpp>

#include"statable.h"


//デバッグ用
#include<iostream>
#define DEBUG_EVENT_TRACE 0

namespace simuframe{

//typedef

//64bit整数型
/*
#ifdef WIN32
typedef unsigned __int64 uint64;//VC用
#else
typedef unsigned long long uint64;//gcc用
#endif
typedef uint64 event_id_t;
typedef uint64 simutime_t;
*/
//32bitアーキテクチャでは64bit整数よりもdoubleの方が速かったという罠
//仮数部のbit長(環境に依存するが52bit程度)を超えた大きさが必要になる場合は
//64bitの整数型を使うこと

typedef double event_id_t;//イベントID型
typedef double simutime_t;//シミュレーション時間型

//前方宣言
class ISimu;//シミュレーションインターフェイス

/////////////////////////////////////////////////
//			class CEvent
/////////////////////////////////////////////////
//イベント基底クラス
class CEvent{
	const event_id_t id_;//ID
//	static boost::pool<> event_pool_;//メモリプール
	static event_id_t counter_;//イベントIDの通し番号
protected:
	const simutime_t time_;
public:
	CEvent(const simutime_t time):id_(counter_++),time_(time){
//        std::cout<<"debug:CEvent:CEvent()"<<std::endl;
}
	virtual ~CEvent(){
//        std::cout<<"debug:CEvent:~CEvent()"<<std::endl;
};
	virtual void onEvent() const =0;//イベントハンドラのインターフェイス
	const event_id_t id() const { return id_; }
	const simutime_t time() const { return time_; }
	
	static void reset_id(){ counter_=0; }
	//メモリ確保及び開放
/*	void* operator new(size_t t){ 
//        std::cout<<"debug:CEvent:new():start"<<std::endl;
        return event_pool_.malloc();
//        std::cout<<"debug:CEvent:new():end"<<std::endl;
        //return p;
    }
	void operator delete(void* p);//コンパイラのinline最適化バグのため明示的にoutline
*/
};

//CEvent*の優先順位比較ファンクタ
//時刻が小さいほど優先順位が高い。同じ場合はidが小さい方。
//左辺値が優先度が小さいと判断されるのは時刻が大きいときであるので符号に注意。
struct FCEventPriLess:public std::binary_function<const CEvent* ,const CEvent* ,bool>{
	bool operator()(const CEvent* lhs,const CEvent* rhs)const{
		return (
				(lhs->time() > rhs->time())
				|| ( (lhs->time() == rhs->time()) && (lhs->id() > rhs->id()) )
			);
	}
};


/////////////////////////////////////////////////
//			class CEventMaxsize
/////////////////////////////////////////////////
//イベント派生クラスを固定長メモリ領域にマップするための最大サイズを定義するダミー
//サイズを定義するだけでオブジェクトが構築されることはない。
template<int T_size= 32>
class CEventMaxsize:public CEvent{
	CEventMaxsize():CEvent(0){}//オブジェクト生成禁止
protected:
	char work[T_size];//ダミー領域
public:
	//void onEvent() constも定義しない
};

/////////////////////////////////////////////////
//			class CEventManager
/////////////////////////////////////////////////
//イベントマネージャ
class CEventManager:private boost::noncopyable{
	typedef std::priority_queue<
		const CEvent* ,
		std::vector<const CEvent* >,
		FCEventPriLess> table_t;//イベントテーブルはヒープ

	table_t table_;//優先順位付テーブル

	static CEventManager* p_instance_;//インスタンスがどこからでも取得できるようにstatic
	CEventManager(){};

public:

	~CEventManager();
	inline const CEvent* const getNextEvent();//次イベントの取得
	inline void setEvent(const CEvent* const p_event);//イベントをテーブルに追加
	bool is_empty() const { return table_.empty(); }

	inline static CEventManager* getInstance();//イベントマネージャのインスタンスを取得
};

//次のイベントを返す
inline
const CEvent* const CEventManager::getNextEvent(){
	const CEvent* const p_event=table_.top();
	table_.pop();
	return p_event;
}

//イベントをテーブルに追加する
//基本はtime優先で同じ時はid優先
inline
void CEventManager::setEvent(const CEvent* const p_event){
#if DEBUG_EVENT_TRACE
    std::cout<<"CEventManager::setEvent():"<<(typeid(*p_event)).name()<<std::endl;
#endif
	table_.push(p_event);//優先順位は比較関数をオーバーロード
}

//どこからでもインスタンスを取得できるようにするためのstatic関数
inline
CEventManager* CEventManager::getInstance(){
	if(p_instance_){
		return p_instance_;//2回目以降インスタンスのポインタを返す
	}else{
		//初回のみprivateコンストラクタを起動してstaticフィールドを初期化
		return (p_instance_=new CEventManager());
	}
}


/////////////////////////////////////////////////
//			class CStatManager
/////////////////////////////////////////////////
class CStatManager:private boost::noncopyable{
public:
	typedef CStatable::group_t group_t;
	enum { NONE=0,ALL=0xFFFF };//すべてのグループを表すフラグ
private:
	CEventManager* const p_event_manager_;
	const simutime_t interval_;//次の統計イベントまでの間隔
	typedef std::vector<CStatable* > table_t;//統計をとるオブジェクトのテーブル
	table_t table_;
private:

public:
	CStatManager(CEventManager* const p_event_manager,const simutime_t interval)
		:p_event_manager_(p_event_manager),interval_(interval){}
	~CStatManager(){ table_.clear();}
	void add(CStatable* const p_obj,const group_t group);
	void onStat(const CEvent* const p_event);
};
/////////////////////////////////////////////////
//			class CEventStat
/////////////////////////////////////////////////
class CEventStat:public CEvent{
	typedef CStatable::group_t group_t;

	CStatManager* const p_stat_manager_;
	const group_t group_;
public:
	CEventStat(const simutime_t time,CStatManager* const p_stat_manager,const group_t group)
		:CEvent(time),p_stat_manager_(p_stat_manager),group_(group){}
	~CEventStat(){}
	void onEvent() const;
	const group_t group() const{ return group_;}
};

inline void CEventStat::onEvent() const{
	//統計イベントは統計マネージャの統計処理ハンドラを呼ぶ
	//統計マネージャは登録されているオブジェクトの統計処理ハンドラをそれぞれ呼ぶ
	p_stat_manager_->onStat(this);
}

/////////////////////////////////////////////////
//			class ISimuSetting
/////////////////////////////////////////////////
class ISimuSetting{
public:
	virtual ~ISimuSetting(){};
	virtual bool is_valid() const =0;
};


/////////////////////////////////////////////////
//			class ISimuProperty
/////////////////////////////////////////////////
//シミュレーション固有の初期化を行うためのインターフェイス
class ISimuProperty:private boost::noncopyable{
public:
	virtual ~ISimuProperty(){}
	virtual bool init(ISimu* const p_simu)=0;
	virtual bool clean_up()=0;
};

/////////////////////////////////////////////////
//			class ISimu
/////////////////////////////////////////////////
//シミュレーションインターフェイス
class ISimu:private boost::noncopyable{
public:
	virtual ~ISimu(){};
	virtual bool init()=0;
	virtual bool start()=0;
	virtual bool end()=0;
};

/////////////////////////////////////////////////
//			class CSimu
/////////////////////////////////////////////////
//シミュレーションのメインフレームワーク
//シミュレーションの初期化及びイベントのコールバックを行う
//独自のシミュレーションを定義する場合はCSimuから派生せず、
//ISimuPropertyから派生したクラスで初期化及び最初のイベントを生成を実装し、
//このISimuPropertyインターフェイスをCSimuのコンストラクタに渡す
class CSimu:public ISimu{
	ISimuProperty* const p_property_;
	bool run_flg_;
	CEventManager* p_event_manager_;
	
	static simutime_t time_;
public:
	CSimu(ISimuProperty* const p_property):p_property_(p_property),run_flg_(false){}
	virtual ~CSimu(){ clean_up(); }

	bool init();
	bool start();
	bool end();
	bool clean_up();

	static const simutime_t get_time(){ return time_; }
};


}//namespace simuframe

//end of simuframe.h
#endif //_SIMUFRAME_H__


