#pragma once

#include <windows.h>

//======================================================================
// リスト用基底クラス
//======================================================================
typedef class CListBase
{
	friend class CList;

protected:
	class CList		*pParentList;
	long			ReferenceCount;

public:
	CListBase		*Prev;
	CListBase		*Next;

	CListBase();
	virtual ~CListBase();

	virtual long Release( void );
	virtual void AddRef( void );
}
CListBase, *LPCListBase;


//==========================================================================
// リスト管理クラス
//==========================================================================
typedef class CList
{
protected:
	LPCListBase		ListTop;
	LPCListBase		ListEnd;

public:
	unsigned long	Count;
	CList();
	~CList();

	void Init( void );
	LPCListBase Top( void );
	LPCListBase End( void );
	void InsertTop( LPCListBase t );
	void InsertEnd( LPCListBase t );
	void InsertPrev( LPCListBase pTarget, LPCListBase pIt );
	void InsertNext( LPCListBase pTarget, LPCListBase pIt );
	void Erase( LPCListBase t );
	void Release( void );
	long Size( void );
	LPCListBase Data( long no );
}
CList, *LPCList;
