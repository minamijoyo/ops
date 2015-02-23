//main.cpp

#include<iostream>
#include<stdlib.h>
#include"opssimu.h"

#include<boost/timer.hpp>//シミュレーション時間測定用

//import namespace
using simuframe::ISimu;
using simuframe::CSimu;
using simuframe::ISimuProperty;
using opssimu::COpsSimuProperty;

typedef opssimu::COpsSimuProperty::SSimuSetting setting_t;

//コマンドライン引数の解析
bool cmd_param(int argc,char *argv[],setting_t* p_setting){
	//引数の数チェック
	if(argc!=9){
		std::cerr<<"usage: ./ops file time node lambda ttl_margin buf_th buf_sz seed"<<std::endl;
		return false;
	}
	//引数をシミュレーション設定構造体に読み込む
	p_setting->filename_=argv[1];//トポロジファイル名
	p_setting->end_time_=atof(argv[2]);//シミュレーション終了時刻
	p_setting->node_type_=atoi(argv[3]);//ノードタイプ
	p_setting->lambda_=atof(argv[4])/10;//パケット到着率
	//p_setting->ttl_margin_=atof(argv[5])/100.0;//TTLマージンを倍率で入力
	p_setting->ttl_margin_=atof(argv[5]);//TTLマージンをホップ数で入力
	p_setting->buf_th_=atof(argv[6])/100;//バッファ閾値
	p_setting->buf_sz_=atoi(argv[7]);//バッファサイズ
	p_setting->seed_=atoi(argv[8]);//seed
	for(int i=0;i<argc;i++)
		std::cout<<argv[i]<<" ";
	std::cout<<std::endl;
	return true;
}


/////////////////////////////////////////////////
//			エントリポイント
/////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	boost::timer time;//シミュレーション時間測定用

	setting_t setting;
	//コマンドライン引数の解析
	if(!cmd_param(argc,argv,&setting)) return 1;
	
	//パラメータの設定
	ISimuProperty* property=new COpsSimuProperty(setting);
	ISimu* sim=new CSimu(property);//シミュレーションの生成

	if(sim!=NULL){
		if(sim->init()){//初期化
			sim->start();//シミュレーション開始
		}else{
			std::cerr<<"Error:simulation initilization."<<std::endl;
		}
	}else{
		std::cerr<<"Error:new CSimu"<<std::endl;
	}

	//後片付け
	delete sim;
	delete property;
	
	std::cout<<"simulation time:"<<time.elapsed()<<"sec.\n\n\n"<<std::endl;
	return 0;
}


//end of main.cpp

