#ifndef _MEMPOOL_H__
#define _MEMPOOL_H__
//mempool.h
namespace simuframe{
/////////////////////////////////////////////////
//			CMemPool
/////////////////////////////////////////////////
//シングルスレッド用固定サイズメモリプール
template<class T>
class CMemPool{
public:
	CMemPool(size_t size=EXPANSION_SIZE);
	~CMemPool();

	//空きリストからT要素を確保する
	inline void* alloc(size_t size);
	//T要素を空きリストに返す
	inline void free(void* someElement);
private:
	//空きリストの次の要素
	CMemPool<T> *next;
	//空きリストが空の場合は、この量だけ拡大する
	enum{ EXPANSION_SIZE = 256 };
	//フリー要素を空きリストに追加する
	void expandTheFreeList(size_t howMany = EXPANSION_SIZE);
};

//コンストラクタ
template<class T>
CMemPool<T>::CMemPool(size_t size){
	expandTheFreeList(size);
}

//デストラクタ
template<class T>
CMemPool<T>::~CMemPool(){
	CMemPool<T> *nextPtr = next;
	while(nextPtr!=0){
		next= next->next;
		delete [] static_cast<char*>(static_cast<void*>(nextPtr));
		nextPtr=next;
	}
}

//メモリ割り当て
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

//メモリ開放
template<class T>
inline
void CMemPool<T>::free(void* doomed){
	CMemPool<T>* head=static_cast<CMemPool<T>* >(doomed);
	head->next=next;
	next=head;
}

//空きリストの拡大
template<class T>
void CMemPool<T>::expandTheFreeList(size_t howMany){
	//nextポインタを入れるのに十分な大きさのオブジェクトを確保しなければならない
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

