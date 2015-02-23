#ifndef _MEMPOOL_H__
#define _MEMPOOL_H__
//mempool.h
namespace simuframe{
/////////////////////////////////////////////////
//			CMemPool
/////////////////////////////////////////////////
//�V���O���X���b�h�p�Œ�T�C�Y�������v�[��
template<class T>
class CMemPool{
public:
	CMemPool(size_t size=EXPANSION_SIZE);
	~CMemPool();

	//�󂫃��X�g����T�v�f���m�ۂ���
	inline void* alloc(size_t size);
	//T�v�f���󂫃��X�g�ɕԂ�
	inline void free(void* someElement);
private:
	//�󂫃��X�g�̎��̗v�f
	CMemPool<T> *next;
	//�󂫃��X�g����̏ꍇ�́A���̗ʂ����g�傷��
	enum{ EXPANSION_SIZE = 256 };
	//�t���[�v�f���󂫃��X�g�ɒǉ�����
	void expandTheFreeList(size_t howMany = EXPANSION_SIZE);
};

//�R���X�g���N�^
template<class T>
CMemPool<T>::CMemPool(size_t size){
	expandTheFreeList(size);
}

//�f�X�g���N�^
template<class T>
CMemPool<T>::~CMemPool(){
	CMemPool<T> *nextPtr = next;
	while(nextPtr!=0){
		next= next->next;
		delete [] static_cast<char*>(static_cast<void*>(nextPtr));
		nextPtr=next;
	}
}

//���������蓖��
template<class T>
inline
void* CMemPool<T>::alloc(size_t){
	if(!next){
		expandTheFreeList();
	}
	CMemPool<T>* head=next;
	next=head->next;

	return head;
}

//�������J��
template<class T>
inline
void CMemPool<T>::free(void* doomed){
	CMemPool<T>* head=static_cast<CMemPool<T>* >(doomed);
	head->next=next;
	next=head;
}

//�󂫃��X�g�̊g��
template<class T>
void CMemPool<T>::expandTheFreeList(size_t howMany){
	//next�|�C���^������̂ɏ\���ȑ傫���̃I�u�W�F�N�g���m�ۂ��Ȃ���΂Ȃ�Ȃ�
	size_t size=(sizeof(T) > sizeof(CMemPool<T>*)) ? sizeof(T):sizeof(CMemPool<T>*);

	CMemPool<T>* runner=static_cast<CMemPool<T>* >(static_cast<void*>(new char[size]));

	next=runner;
	for(size_t i=0;i<howMany;i++){
		runner->next=static_cast<CMemPool<T>* >(static_cast<void*>(new char[size]));
		runner=runner->next;
	}
	runner->next=0;
}

}//end of namespace simuframe
//end of mempool.h
#endif

