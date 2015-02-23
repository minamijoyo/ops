//simuframe.cpp

#include<algorithm>
#include<iostream>
#include"simuframe.h"

//デバッグ用
#if DEBUG_EVENT_TRACE
#include<typeinfo>
#endif

using namespace simuframe;
/*
//とりあえずテスト用。本来はopssimuには依存しない。
#include"event.h"
//とりあえずテスト用。opsnetworkには依存しない
#include"opsnetwork.h"
*/

//デバッグ用
#include<iostream>

//static member instance
//boost::pool<> CEvent::event_pool_(sizeof(CEventMaxsize<>));
event_id_t CEvent::counter_=0;
CEventManager* CEventManager::p_instance_=0;
simutime_t CSimu::time_=0;
/*
void CEvent::operator delete(void* p){
        //std::cout<<"debug:CEvent:delete():start"<<std::endl;
        event_pool_.free(p);
        //std::cout<<"debug:CEvent:delete():end"<<std::endl;
 }
 */
/////////////////////////////////////////////////
//			class CEventManager
/////////////////////////////////////////////////
//デストラクタ
CEventManager::~CEventManager(){
	//memory clean up

	//テーブル内のイベントを削除
	while(!table_.empty()){
		delete (getNextEvent());
	}
	//インスタンスポインタを初期状態に戻す
	p_instance_=0;
	//イベントIDを0に戻す
	CEvent::reset_id();
}


/////////////////////////////////////////////////
//			class CStatManager
/////////////////////////////////////////////////
//統計をとるオブジェクトをテーブルに追加
void CStatManager::add(CStatable* const p_obj,const group_t group){
	//groupフラグのセット	
	p_obj->set_group(group);
	//group順にソートして追加
	table_.insert(
		std::upper_bound(table_.begin(),table_.end(),p_obj,FCStatableLess()),
		p_obj
	);
}

//統計イベント用ハンドラ
void CStatManager::onStat(const CEvent* const p_event){
	//統計イベントを呼び出すグループを取得
	const group_t group=(static_cast<const CEventStat* const>(p_event))->group();
	
	if(group==CStatManager::ALL){
		//すべてをコールする場合
		//std::for_each(table_.begin(),table_.end(),boost::bind2nd(
		//	boost::mem_fun1_t<void,CStatable,const CEvent* const>(&(CStatable::onStat)),p_event));
		for(table_t::iterator it=table_.begin();it!=table_.end();++it){
			(*it)->onStat(p_event);
		}
	}else if(group!=CStatManager::NONE){
		//特定のgroupのみコールする場合
		CStatable dummy;//グループ探索用のダミー
		dummy.set_group(group);
		std::pair<table_t::iterator,table_t::iterator> range=
			std::equal_range(table_.begin(),table_.end(),(&dummy),FCStatableLess());
		//std::for_each(range.first,range.second,std::bind2nd(
		//	std::mem_fun1(&(CStatable::onStat)),p_event));
		for(;range.first!=range.second;++(range.first)){
			(*range.first)->onStat(p_event);
		}
	}else{
		//group==CStatManager::NONEの場合
		return;//何もせず、次のイベントも生成しない
	}

	//次の統計イベントを生成
	p_event_manager_->setEvent(
			new CEventStat(p_event->time()+interval_,this,group)
		);
	return;
}


/////////////////////////////////////////////////
//			class CSimu
/////////////////////////////////////////////////
//シミュレーション初期化時に呼ばれる
bool CSimu::init(){
	//initialize simulation

	//CEventManagerの初期化
	p_event_manager_=CEventManager::getInstance();
	if(p_event_manager_==NULL)
		return false;

	//シミュレーション固有の初期化
	return p_property_->init(this);
}

//シミュレーション開始
bool CSimu::start(){
	run_flg_=true;
	
	//simulation main loop
	while(run_flg_){
		if(!p_event_manager_->is_empty()){
            //std::cout<<"debug:CSimu::start():get"<<std::endl;
			const CEvent* const p_event=p_event_manager_->getNextEvent();//次イベント取得
			time_=p_event->time();//現在時刻の更新
#if DEBUG_EVENT_TRACE
            std::cout<<"debug:CSimu::start():start:"<<p_event->id()<<":"<<(typeid(*p_event)).name()<<std::endl;
#endif
			p_event->onEvent();//イベント処理
			
#if DEBUG_EVENT_TRACE
            std::cout<<"debug:CSimu::start():end:"<<p_event->id()<<":"<<(typeid(*p_event)).name()<<std::endl;
#endif
            delete p_event;
		}else{
			std::cerr<<"Error:event table is empty."<<std::endl;
			run_flg_=false;
		}
	}
	return true;
}

//シミュレーションを止める
//start()のmain_loopから抜けるだけでメモリの廃棄はデストラクタにまかせる。
bool CSimu::end(){
	run_flg_=false;
	return true;
};

bool CSimu::clean_up(){
	//シミュレーション固有の終了処理
	p_property_->clean_up();
	
	//イベントテーブルの削除
	delete p_event_manager_;
	
	//時刻のリセット
	time_=0;
	return true;
}

//end of simuframe.cpp


