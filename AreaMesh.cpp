#define _CRT_SECURE_NO_WARNINGS
#include "AreaMesh.h"
#include "Area.h" // For CArea and CTexture list

extern HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf );
extern HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage );
#define SAFE_RELEASE(p)		if ( (p) != NULL ) { (p)->Release(); (p) = NULL; }

//		コンストラクタ
CStream::CStream()
{
	m_pTexture		= NULL;
	m_PrimitiveType	= D3DPT_TRIANGLELIST;
	m_IndexStart	= 0;
	m_FaceCount		= 0;
	m_AlphaFlag		= 0;
	m_StencilFlag	= false;
}

//		デストラクタ
CStream::~CStream()
{
}

//		インデックスデータ設定
void CStream::SetMeshAttr( D3DPRIMITIVETYPE PrimitiveType, unsigned long index_start, unsigned long face_count )
{
	m_PrimitiveType	= PrimitiveType;
	m_IndexStart	= index_start;
	m_FaceCount		= face_count;
}

//		プリミティブタイプ取得
D3DPRIMITIVETYPE CStream::GetPrimitiveType( void )
{
	return m_PrimitiveType;
}

//		インデックス開始位置取得
unsigned long CStream::GetIndexStart( void )
{
	return m_IndexStart;
}

//		インデックス使用数取得
unsigned long CStream::GetFaceCount( void )
{
	return m_FaceCount;
}

//		コンストラクタ
CTexture::CTexture()
{
	m_pTexture		= NULL;
}

//		デストラクタ
CTexture::~CTexture()
{
	SAFE_RELEASE( m_pTexture );
}

//		テクスチャ設定
void CTexture::SetTexture( IDirect3DTexture9 *pTex )
{
	SAFE_RELEASE( m_pTexture );
	m_pTexture = pTex;
	if ( m_pTexture != NULL ) m_pTexture->AddRef();
}

//		テクスチャ取得
IDirect3DTexture9 *CTexture::GetTexture( void )
{
	return m_pTexture;
}

//		コンストラクタ
CAreaMesh::CAreaMesh()
{
	m_lpVB					= NULL;
	m_lpIB					= NULL;
	m_NumIndex = m_NumVertices = m_NumFaces = m_VBSize = m_IBSize = m_FVF = 0;
}

//		デストラクタ
CAreaMesh::~CAreaMesh()
{
	m_LStreams.clear();
}



//		Areaメッシュの生成
HRESULT CAreaMesh::LoadAreaMesh( char *pFile, CArea *pArea, unsigned long FVF )
{
#pragma pack(push,2)
	typedef struct _D3DTEXVERTEX
	{
		D3DXVECTOR3	v;	//座標
		D3DXVECTOR3	n;	//法線ベクトル
		DWORD		color;     //色
		float		tu,tv;     // UV座標
	} D3DTEXVERTEX;

	typedef	struct _AREAMESHBOX
	{
		float		x1,x2,y1,y2,z1,z2;
	} AREAMESHBOX;
#pragma pack(pop)

	HRESULT			hr						= D3D_OK; 
	int				CountIndex =0,NumFaces=0,NumVertices=0,NumIndex = 0;
	D3DTEXVERTEX	*pV,*pVertex;
	AREAMESHBOX		*pAreaMeshBox;
	D3DXVECTOR3		BoxXYZ;
	char			TexName[18];
	CTexture		*pTexture;
	WORD			*pIndex,*pIdx;
	int				NumElement;
	int				Level;
	char			*Pos;
	int				nVertex,nVerReal;
	int				nIndex,NextPos;
	short			AlphaFlag=0;
	unsigned char	aFlag,bFlag;

	// エリアデータの判定

	m_AreaType.assign(pFile, 4);
	aFlag = *pFile;bFlag = *(pFile+4);
	pFile += 16;
//	if( *(pFile+4)!=1 ) return -1;
	// 面、頂点のカウントの生成
	Pos		=	pFile + 0x020;
	Level		=	*(int*)Pos;
	if( Level > 64 ) {
		return -1;
	} else if( Level > 16 ) {
		Pos += Level*4;
	} else if( Level >=2 && Level <= 16 ) {
		Pos += 16*4;
	} else if( Level ==1 ) {
		Pos += 32;
	} else {
		return -1;
	}
	while( Level ){
		NumElement = *(int*)Pos;
		if( NumElement > 0xffff || NumElement<0 ) return -1;
		Pos += 32;
		for( int i=0; i<NumElement ; i++ ){
			nVertex     = (int)*(short*)(Pos+16);
			nVerReal	= (int)*(int*)(Pos+16);
			if( nVertex > 0xffff || nVertex<0 ) return -1;
			nIndex		= (*(int*)(Pos+16+4+nVertex*36));
			if( nIndex > 0xffff || nIndex < 0 ) return -1;
			NextPos = 16  +    4+ nVertex*36+    4+ nIndex*2;
			NextPos = 4*((NextPos+3)/4);
			NumVertices	+=	nVertex;
			NumFaces	+=	nIndex-2;
			NumIndex    +=  nIndex;
			Pos += NextPos;
		}
		Level--;
	}
	m_NumVertices = NumVertices;
	m_NumFaces = NumFaces;
	m_NumIndex = NumIndex;
	m_FVF = FVF;
	m_VBSize = sizeof(D3DTEXVERTEX)*NumVertices;
	m_IBSize = NumIndex*sizeof(WORD);
	hr = CreateIB( &m_lpIB, NumIndex*sizeof(WORD), D3DUSAGE_WRITEONLY );
	if FAILED( hr ) return hr;
	hr = CreateVB( &m_lpVB, NumVertices*sizeof(D3DTEXVERTEX), D3DUSAGE_WRITEONLY, FVF );
	if FAILED( hr ) return hr;
	if( FAILED( m_lpIB->Lock( 0,                 // バッファの最初からデータを格納する。
						m_IBSize, // ロードするデータのサイズ。
						(void**)&pIndex, // 返されるインデックス データ。
						D3DLOCK_DISCARD ) ) )            // デフォルト フラグをロックに送る。
		return E_FAIL;
	if( FAILED( m_lpVB->Lock( 0,                 // バッファの最初からデータを格納する。
						m_VBSize, // ロードするデータのサイズ。
						(void**)&pV, // 返されるインデックス データ。
						D3DLOCK_DISCARD ) ) )            // デフォルト フラグをロックに送る。
		return E_FAIL;
	// 面、頂点の格納
	D3DXVECTOR3	Low( 65535.f,65535.f,65535.f),High( -65535.f,-65535.f,-65535.f);
	CountIndex = NumFaces = NumVertices = 0;
	Pos		=	pFile + 0x20;
	Level	=	*(int*)Pos;
	if( Level > 64 ) {
		return -1;
	} else if( Level > 16 ) {
		Pos += Level*4;
	} else if( Level >=2 && Level <= 16 ) {
		Pos += 16*4;
	} else if( Level ==1 ) {
		pAreaMeshBox = (AREAMESHBOX*)(Pos+4);
		BoxXYZ.x = pAreaMeshBox->x1; BoxXYZ.y = pAreaMeshBox->y1; BoxXYZ.z = pAreaMeshBox->z1;
		SetBoxLow( BoxXYZ );
		BoxXYZ.x = pAreaMeshBox->x2; BoxXYZ.y = pAreaMeshBox->y2; BoxXYZ.z = pAreaMeshBox->z2;
		SetBoxHigh( BoxXYZ );
		if( pAreaMeshBox->x1>pAreaMeshBox->x2 ) 
			pAreaMeshBox->x1 = pAreaMeshBox->x1;
		if( pAreaMeshBox->z1>pAreaMeshBox->z2 ) 
			pAreaMeshBox->x1 = pAreaMeshBox->x1;
		if( pAreaMeshBox->y1>pAreaMeshBox->y2 ) 
			pAreaMeshBox->x1 = pAreaMeshBox->x1;
		Pos += 32; // 全体の　要素数:i4,下限xyz:f,上限xyz:f,謎:i4 
	} else {
		return -1;
	}
	while( Level ){
		NumElement = *(int*)Pos;
		if( NumElement > 0xffff || NumElement<0 ) return -1;
		pAreaMeshBox = (AREAMESHBOX*)(Pos+4);
		Pos += 32; // レベル毎のBox　要素数:i4,下限xyz:f,上限xyz:f,謎:i4
		for( int i=0; i<NumElement ; i++ ){
			strncpy(TexName,Pos,16);
			pTexture = (CTexture*)pArea->m_Textures.Top();
			int texNo = 0;
			while ( pTexture != NULL ) {
				if( !memcmp(TexName,pTexture->GetTexName(),16) ){
					break;
				}
				pTexture = (CTexture*)pTexture->Next;
				texNo++;
			}
			nVertex     = (int)*(short*)(Pos+16);
			nVerReal	= (int)*(int*)(Pos+16);
			AlphaFlag = (nVerReal>>28)&0x0f;
			if( nVertex > 0xffff || nVertex<0 ) return -1;
			nIndex		= (*(int*)(Pos+16+4+nVertex*36));
			if( nIndex > 0xffff || nIndex < 0 ) return -1;
			NextPos = 16  +    4+ nVertex*36+    4+ nIndex*2;
			NextPos = 4*((NextPos+3)/4);
			CStream		tStream;
			tStream.SetMeshAttr(D3DPT_TRIANGLESTRIP, CountIndex, nIndex-2 );
			tStream.SetpTexture(pTexture);
			tStream.m_TexNo = texNo;
			tStream.SetAlphaFlag( (short)AlphaFlag );
			pVertex = (D3DTEXVERTEX*)(Pos+16+4);
			tStream.SetAlphaFlag(AlphaFlag);
			tStream.m_vertNum = nVertex;
			tStream.m_faceNum = nIndex - 2;
			for( int j=0 ; j<nVertex ; j++ ) {
				Low.x = Low.x>pVertex[j].v.x?pVertex[j].v.x:Low.x;
				Low.y = Low.y>pVertex[j].v.y?pVertex[j].v.y:Low.y;
				Low.z = Low.z>pVertex[j].v.z?pVertex[j].v.z:Low.z;
				High.x = High.x<pVertex[j].v.x?pVertex[j].v.x:High.x;
				High.y = High.y<pVertex[j].v.y?pVertex[j].v.y:High.y;
				High.z = High.z<pVertex[j].v.z?pVertex[j].v.z:High.z;
			}
			if( aFlag==0x5f ) tStream.SetStencilFlag(true);
			pIdx = new WORD[nIndex]();
			memcpy((char*)pIdx,(Pos+16+4+nVertex*36+4),sizeof(WORD)*nIndex);
			for( int j=0 ; j<nIndex ; j++ ) {
				pIdx[j] += NumVertices;
			}
			memcpy((char*)pV,(char*)pVertex,sizeof(D3DTEXVERTEX)*nVertex);
			pV += nVertex;
			memcpy((char*)pIndex,(char*)pIdx,sizeof(WORD)*nIndex);
			pIndex += nIndex;
			delete[] pIdx; pIdx = NULL;
			m_LStreams.push_back( tStream );
			NumVertices += nVertex;
			NumFaces	+= nIndex -2;
			CountIndex	+= nIndex;
			Pos += NextPos;
		}
		Level--;
	}
	if( FAILED( hr = m_lpVB->Unlock() ) ) {
		return hr;
	}
	if( FAILED( hr = m_lpIB->Unlock() ) ) {
		return hr;
	}
	D3DXVECTOR3	TempLow  = GetBoxLow();
	D3DXVECTOR3	TempHigh  = GetBoxHigh();
	if( Low != TempLow )
		SetBoxLow( Low );
	if( High != TempHigh )
		SetBoxHigh( High );
	return hr;
}

int CAreaMesh::countTextures(void)
{
	int count = 0;
	std::list<CStream>::iterator it;
	for (it = m_LStreams.begin(); it != m_LStreams.end(); ++it) {
		if (it->GetpTexture() != NULL) {
			count++;
		}
	}
	return count;
}
