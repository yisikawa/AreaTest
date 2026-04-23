#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <list>
#include "List.h"

// CArea クラスの前方宣言 (CAreaMesh でポインタとして使用されるため)
class CArea;

//======================================================================
// TYPE DEFINE (CTexture等で使われる可能性のある構造体)
//======================================================================
typedef struct _D3DTEXVERTEX
{
	D3DXVECTOR3	v, n;	//座標,法線ベクトル
	DWORD color;     //色
	float tu, tv;     // UV座標
} D3DTEXVERTEX;

//======================================================================
// マテリアルクラス
//======================================================================
typedef class CTexture : public CListBase
{
protected:
	IDirect3DTexture9	*m_pTexture;

public:
	std::string			m_TexName;
	CTexture();
	virtual ~CTexture();
	virtual void		SetTexName(const char* name) { m_TexName = name; }
	virtual const char*	GetTexName() { return m_TexName.c_str(); }
	virtual void SetTexture( IDirect3DTexture9 *pTex );
	virtual IDirect3DTexture9 *GetTexture( void );
}
CTexture, LPCTexture;

//======================================================================
// ストリームタイプクラス
//======================================================================
typedef class CStream
{
protected:
	CTexture			*m_pTexture;			// texture pointer
	bool				m_StencilFlag;			// 
	short				m_AlphaFlag;
	D3DPRIMITIVETYPE	m_PrimitiveType;
	unsigned long		m_IndexStart;
	unsigned long		m_FaceCount;

public:
	int					m_TexNo;
	unsigned long		m_vertNum;
	unsigned long		m_faceNum;

	CStream();
	virtual ~CStream();
	virtual	void SetpTexture(CTexture* pTexture ) { m_pTexture = pTexture; }
	virtual	CTexture* GetpTexture( void ) { return m_pTexture; }
	virtual	void SetStencilFlag( bool StencilFlag ) { m_StencilFlag = StencilFlag; }
	virtual	short GetStencilFlag( void ) { return m_StencilFlag; }
	virtual	void SetAlphaFlag( short AlphaFlag ) { m_AlphaFlag = AlphaFlag; }
	virtual	short GetAlphaFlag( void ) { return m_AlphaFlag; }
	virtual void SetMeshAttr( D3DPRIMITIVETYPE PrimitiveType,unsigned long index_start, unsigned long face_count );
	virtual D3DPRIMITIVETYPE GetPrimitiveType( void );	
	virtual unsigned long GetIndexStart( void );
	virtual unsigned long GetFaceCount( void );
}
CStream, LPCStream;

//======================================================================
// エリアメッシュクラス
//======================================================================
typedef class CAreaMesh : public CListBase
{
	friend class CArea;

protected:
	std::list<CStream>		m_LStreams;
	D3DXVECTOR3				m_BoxLow, m_BoxHigh;
	unsigned long			m_NumVertices, m_NumFaces, m_NumIndex, m_VBSize, m_IBSize, m_FVF;
	LPDIRECT3DVERTEXBUFFER9 m_lpVB;
	LPDIRECT3DINDEXBUFFER9	m_lpIB;
	std::string			m_AreaName;
	std::string			m_AreaType;

public:
	CAreaMesh();
	virtual		~CAreaMesh();
	virtual		void		SetBoxLow(D3DXVECTOR3 BoxLow) { m_BoxLow = BoxLow; }
	virtual		D3DXVECTOR3	GetBoxLow(void) { return m_BoxLow; }
	virtual		void		SetBoxHigh(D3DXVECTOR3 BoxHigh) { m_BoxHigh = BoxHigh; }
	virtual		D3DXVECTOR3	GetBoxHigh(void) { return m_BoxHigh; }
	virtual		void		SetNumVertices(unsigned long NumVertices) { m_NumVertices = NumVertices; }
	virtual		unsigned long	GetNumVertices(void) { return m_NumVertices; }
	virtual		void		SetNumFaces(unsigned long NumFaces) { m_NumFaces = NumFaces; }
	virtual		unsigned long	GetNumFaces(void) { return m_NumFaces; }
	virtual		void		SetlpVB(LPDIRECT3DVERTEXBUFFER9 lpVB) { m_lpVB = lpVB; }
	virtual		LPDIRECT3DVERTEXBUFFER9	GetlpVB(void) { return m_lpVB; }
	virtual		void		SetlpIB(LPDIRECT3DINDEXBUFFER9 lpIB) { m_lpIB = lpIB; }
	virtual		LPDIRECT3DINDEXBUFFER9	GetlpIB(void) { return m_lpIB; }
	virtual		void		SetAreaName(const char *pAreaName) { m_AreaName = pAreaName; }
	virtual		const char*		GetAreaName(void) { return m_AreaName.c_str(); }
	virtual		void		SetAreaType(const char *pAreaType) { m_AreaType = pAreaType; }
	virtual		const char*		GetAreaType(void) { return m_AreaType.c_str(); }
	virtual		HRESULT		LoadAreaMesh(char *pFile, CArea *pArea, unsigned long FVF);
	virtual     int			countTextures(void);
}
CAreaMesh, *LPCAreaMesh;
