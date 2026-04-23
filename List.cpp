#include "List.h"
#include <windows.h>

//		コンストラクタ
CListBase::CListBase()
{
	ReferenceCount = 1;
	Prev = Next = NULL;
	pParentList = NULL;
}

//		デストラクタ
CListBase::~CListBase()
{
	if ( pParentList != NULL )
	{
		pParentList->Erase( this );
	}
}

//		開放
long CListBase::Release( void )
{
	long ref = ReferenceCount - 1;

	// 参照がなくなったら破棄
	if ( --ReferenceCount == 0 ) delete this;

	return ref;
}

//		参照カウンタインクリメント
void CListBase::AddRef( void )
{
	ReferenceCount++;
}

//		コンストラクタ
CList::CList()
{
	Init();
}

//		デストラクタ
CList::~CList()
{
	Release();
}

//		初期化
void CList::Init( void )
{
	ListTop = NULL;
	ListEnd = NULL;
	Count = 0;
}

//		先頭取得
LPCListBase CList::Top( void )
{
	return ListTop;
}

//		終端取得
LPCListBase CList::End( void )
{
	return ListEnd;
}

//		リスト解体
void CList::Release( void )
{
	LPCListBase p = ListTop;
	while ( p != NULL )
	{
		// p の次を事前に取得（p が Release() 後解体されてる可能性高い）
		LPCListBase pp = p->Next;
		// 解体
		p->Release();
		// 次
		p = pp;
	}
	Init();
}

//		リストの先頭に挿入
void CList::InsertTop( LPCListBase p )
{
	// 他のリストに登録されてるときはそちらから切断
	if ( p->pParentList != NULL ) p->pParentList->Erase( p );
	p->pParentList = this;

	// 接続
	p->Prev = NULL;
	p->Next = ListTop;
	ListTop = p;
	if ( p->Next != NULL ) p->Next->Prev = p;
	if ( ListEnd == NULL ) ListEnd = p;
	Count++;
}
//		リストの終端に挿入
void CList::InsertEnd( LPCListBase p )
{
	// 他のリストに登録されてるときはそちらから切断
	if ( p->pParentList != NULL ) p->pParentList->Erase( p );
	p->pParentList = this;

	// 接続
	CListBase *pEnd = ListEnd;

	p->Prev = pEnd;
	p->Next = NULL;
	ListEnd = p;

	if ( pEnd == NULL )	{
		ListTop = p;
	} else {
		pEnd->Next = p;
	}
	Count++;
}

//		ターゲットの前にに挿入
void CList::InsertPrev( LPCListBase pTarget, LPCListBase pIt )
{
	if( pTarget == NULL ) 
		return;
	// 他のリストに登録されてるときはそちらから切断
	if ( pIt->pParentList != NULL ) pIt->pParentList->Erase( pIt );
	pIt->pParentList = this;

	// 接続
	pIt->Prev = pTarget->Prev;
	pIt->Next = pTarget;
	pTarget->Prev = pIt;
	if( ListTop == pTarget ) {
		ListTop = pIt;
		pIt->Prev = NULL;
	}
	Count++;
}

//		ターゲットの次に挿入
void CList::InsertNext( LPCListBase pTarget,LPCListBase pIt )
{
	if( pTarget == NULL ) 
		return;
	// 他のリストに登録されてるときはそちらから切断
	if ( pIt->pParentList != NULL ) pIt->pParentList->Erase( pIt );
	pIt->pParentList = this;

	// 接続
	pIt->Prev = pTarget;
	pIt->Next = pTarget->Next;
	pTarget->Next = pIt;
	if( ListEnd == pTarget ) 
		ListEnd = pIt;
	Count++;
}

//		リストから削除
void CList::Erase( LPCListBase p )
{
	if ( p->pParentList != NULL ) p->pParentList = NULL;

	BYTE flag = 0x00;
	if ( p->Prev == NULL ) flag |= 0x01;		// 前に何もないとき
	if ( p->Next == NULL ) flag |= 0x02;		// 後に何もないとき

	//	該当するデータの削除

	switch ( flag )
	{
	///////////////////////////////////// 前後に何かあるとき
	case 0x00:
		p->Prev->Next = p->Next;
		p->Next->Prev = p->Prev;
		break;
	///////////////////////////////////// 前に何もないとき
	case 0x01:
		ListTop = p->Next;
		ListTop->Prev = NULL;
		break;
	///////////////////////////////////// 後に何もないとき
	case 0x02:
		ListEnd = ListEnd->Prev;
		p->Prev->Next = NULL;
		break;
	///////////////////////////////////// 前後に何もないとき
	case 0x03:
		ListTop = NULL;
		ListEnd = NULL;
		break;
	}
	Count--;
}


//		特定のデータ取り出し
LPCListBase CList::Data( long no )
{
	LPCListBase p = ListTop;
	while ( (p != NULL) && no-- ) {
		p = p->Next;
	}
	return p;
}

//		サイズ取得
long CList::Size( void )
{
	return Count;
}
