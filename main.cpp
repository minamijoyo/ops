//main.cpp

#include<iostream>
#include<stdlib.h>
#include"opssimu.h"

#include<boost/timer.hpp>//�V�~�����[�V�������ԑ���p

//import namespace
using simuframe::ISimu;
using simuframe::CSimu;
using simuframe::ISimuProperty;
using opssimu::COpsSimuProperty;

typedef opssimu::COpsSimuProperty::SSimuSetting setting_t;

//�R�}���h���C�������̉��
bool cmd_param(int argc,char *argv[],setting_t* p_setting){
	//�����̐��`�F�b�N
	if(argc!=9){
		std::cerr<<"usage: ./ops file time node lambda ttl_margin buf_th buf_sz seed"<<std::endl;
		return false;
	}
	//�������V�~�����[�V�����ݒ�\���̂ɓǂݍ���
	p_setting->filename_=argv[1];//�g�|���W�t�@�C����
	p_setting->end_time_=atof(argv[2]);//�V�~�����[�V�����I������
	p_setting->node_type_=atoi(argv[3]);//�m�[�h�^�C�v
	p_setting->lambda_=atof(argv[4])/10;//�p�P�b�g������
	//p_setting->ttl_margin_=atof(argv[5])/100.0;//TTL�}�[�W����{���œ���
	p_setting->ttl_margin_=atof(argv[5]);//TTL�}�[�W�����z�b�v���œ���
	p_setting->buf_th_=atof(argv[6])/100;//�o�b�t�@臒l
	p_setting->buf_sz_=atoi(argv[7]);//�o�b�t�@�T�C�Y
	p_setting->seed_=atoi(argv[8]);//seed
	for(int i=0;i<argc;i++)
		std::cout<<argv[i]<<" ";
	std::cout<<std::endl;
	return true;
}


/////////////////////////////////////////////////
//			�G���g���|�C���g
/////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	boost::timer time;//�V�~�����[�V�������ԑ���p

	setting_t setting;
	//�R�}���h���C�������̉��
	if(!cmd_param(argc,argv,&setting)) return 1;
	
	//�p�����[�^�̐ݒ�
	ISimuProperty* property=new COpsSimuProperty(setting);
	ISimu* sim=new CSimu(property);//�V�~�����[�V�����̐���

	if(sim!=NULL){
		if(sim->init()){//������
			sim->start();//�V�~�����[�V�����J�n
		}else{
			std::cerr<<"Error:simulation initilization."<<std::endl;
		}
	}else{
		std::cerr<<"Error:new CSimu"<<std::endl;
	}

	//��Еt��
	delete sim;
	delete property;
	
	std::cout<<"simulation time:"<<time.elapsed()<<"sec.\n\n\n"<<std::endl;
	return 0;
}


//end of main.cpp

