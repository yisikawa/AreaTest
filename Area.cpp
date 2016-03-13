// INCLUDE
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "Render.h"
#include "Area.h"
#include <list>

using namespace std;
// Function
BOOL GetFileNameFromDno(LPSTR filename,DWORD dwID);
DWORD	ConvertStr2Dno( char* DataName );
HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf );
HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage );
// DEFINE
#define SAFE_RELEASE(p)		if ( (p) != NULL ) { (p)->Release(); (p) = NULL; }
#define SAFE_DELETES(p)		if ( (p) != NULL ) { delete [] (p); (p) = NULL; }
#define SAFE_DELETE(p)		if ( (p) != NULL ) { delete (p); (p) = NULL; }
#define PAI					(3.1415926535897932384626433832795f)
#define PAI2				(3.1415926535897932384626433832795f*2.0f)
static const D3DXMATRIX matrixMirrorX(-1.0f,0,0,0,  0, 1.0f,0,0,  0,0, 1.0f,0,  0,0,0,1.0f);
static const D3DXMATRIX matrixMirrorY( 1.0f,0,0,0,  0,-1.0f,0,0,  0,0, 1.0f,0,  0,0,0,1.0f);
static const D3DXMATRIX matrixMirrorZ( 1.0f,0,0,0,  0, 1.0f,0,0,  0,0,-1.0f,0,  0,0,0,1.0f);
// グローバル
extern	BOOL		g_mIsUseSoftware;
extern	D3DLIGHT9	g_mLight,g_mLightbase;
extern	D3DXMATRIX	g_mProjection, g_mView;
extern	D3DXVECTOR3	g_mAt,g_mEye,g_mUp;
extern	float		g_mTime;
extern	float		g_mDispArea;
extern	float		g_mDispTree;
extern	D3DLIGHT9	g_mLight,g_mLightbase;
extern	D3DXVECTOR3	g_mEntry;
extern	int			g_mAreaBright;
extern	char		g_mWeather[];
extern	float		g_mLightDist;
extern	D3DXVECTOR3	g_mLightPosition;
extern	D3DXMATRIX	g_mViewLight;					// ライトから見た場合のビューマトリックス 
extern	char		g_className[];


int Trim(char *s) {
	int i;
	int count = 0;

	/* 空ポインタか? */
	if (s == NULL) { /* yes */
		return -1;
	}

	/* 文字列長を取得する */
	i = strlen(s);

	/* 末尾から順に空白でない位置を探す */
	while (--i >= 0 && s[i] == ' ') count++;

	/* 終端ナル文字を付加する */
	s[i + 1] = '\0';

	/* 先頭から順に空白でない位置を探す */
	i = 0;
	while (s[i] != '\0' && s[i] == ' ') i++;
	strcpy(s, &s[i]);

	return i + count;
}

char * // 文字列へのポインタ
strrstr
(
const char *string, // 検索対象文字列
const char *pattern // 検索する文字列
)
{
	// 文字列終端に達するまで検索を繰り返す。
	const char *last = NULL;
	{for (const char *p = string; NULL != (p = strstr(p, pattern)); ++p)
	{
		last = p;
		if ('\0' == *p)
			return (char *)last;
	}}
	return (char *)last;
}//strrstr


// 頂点フォーマット
D3DVERTEXELEMENT9 VSFormat[] = 
{
// Area Mesh Stream
    { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION, 0}, 
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,   0}, 
    { 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_COLOR,	0}, 
    { 0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD, 0}, 
    D3DDECL_END()
};


static char *pVertexShaders[3] =
{
//------1--エリア表示用-------------------------------------------------- 
	{
		"vs_1_1																		\n"
		"dcl_position0		v0														\n"
		"dcl_normal0		v1														\n"
		"dcl_color0			v2														\n"
		"dcl_texcoord0		v3														\n"
		"																			\n"
		"def	c[0],		0.005f, 0.002f, 500.f, 600.0f	; FOG用オフセット	    \n"
		"def	c[1],		0.0f, 2.0f, 1.0f, 2.0f	;	Phong定数					\n"
		"def	c[2],		0.3f, 0.3f, 0.3f, 0.3f	;	スペキュラー色				\n"
		"def	c[3],		1.0f, 0.0f, 3.0f, 765.01f	;	定数					\n"
		";--------------------------------------------------------------------------\n"
		"; 座標変換																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x3	r5,			v0,			c[10]										\n"
		"m3x3	r6,			v1,			c[10]										\n"
		"; Normalize																\n"
		"dp3	r6.w,		r6,			r6											\n"
		"rsq	r6.w,		r6.w													\n"
		"mul	r6,			r6,			r6.w										\n"
		";--------------------------------------------------------------------------\n"
		"; w = 1.0 に																\n"
		";--------------------------------------------------------------------------\n"
		"mov	r5.w,		v0.w													\n"
//		"mov	r5.w,		c[3].x													\n"
		"mov	r6.w,		c[3].x													\n"
		";--------------------------------------------------------------------------\n"
		"; 座標変換																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x4	oPos,		r5,		c[6]											\n"
		";--------------------------------------------------------------------------\n"
		"; メッシュのテクスチャー													\n"
		";--------------------------------------------------------------------------\n"
		"mov	oT0.xy,		v3.xy													\n"
		"																			\n"
		";--------------------------------------------------------------------------\n"
		"; FOG																		\n"
		";--------------------------------------------------------------------------\n"
		"add	r1,			c[5],	-r5												\n"
		"dp3	r1.w,		r1,			r1											\n"
		"rsq	r0.w,		r1.w													\n"
		"rcp	r0.w,		r0.w				; 視点からの距離					\n"
		"add	r0.w,		c[0].w,		-r0.w	; 距離　1000 - 距離					\n"
		"mul	oFog,		c[0].y,		r0.w	; 距離　X 0.004						\n"
		";--------------------------------------------------------------------------\n"
		"; ライト																	\n"
		";--------------------------------------------------------------------------\n"
		";視点から頂点の方向e														\n"
		"rsq	r1.w,		r1.w													\n"
		"mul	r1,			r1,			r1.w	; r1 = e = 正規化(視点-頂点）		\n"
		";Phong																		\n"
		"dp3	r2.w,		c[4],		r6		; (l.n)								\n"
		"dp3	r2.x,		c[4],		r1		; (l,e)								\n"
		"dp3	r2.y,		r6,			r1		; (n,e)								\n"
		"																			\n"
		"mul	r2.z,		r2.w,		r2.y			; r2.z = (l,n)(n,e)			\n"
		"mad	r2.z,		c[1].y,		r2.z,	-r2.x	; r2.z = 2(l,n)(n,e)-(l,e)	\n"
		"max	r2.z,		r2.z,		c[1].x			; 負の値をカット			\n"
		"mov	r2.w,		c[1].w						; 2乗のためのパラメータ		\n"
		"lit	r2.z,		r2.zzww						; r2.z = r2.z ^r2.w			\n"
		"mul	r3,			c[2],		r2.z			; ｽﾍﾟｷｭﾗｰの色をつける		\n"
		"																			\n"
		"dp4	r2.z,		c[4],		r6				; (l,n)+ambient				\n"
		"mad	oD0,		v2,			r2.z,	r3		; ランバート diffuse		\n"
		"mov	oD0.a,		v2.a						; alpha = model diffuse.a	\n"
	},
//------2--effect表示用---
	{
		"vs_1_1																		\n"
		"dcl_position0		v0														\n"
		"dcl_normal0		v1														\n"
		"dcl_color0			v2														\n"
		"dcl_texcoord0		v3														\n"
		"																			\n"
//		"def	c[0],		0.005f, 0.001f, 500.f, 1000.0f	; FOG用オフセット	    \n"
		"def	c[0],		0.005f, 0.002f, 500.f, 600.0f	; FOG用オフセット	    \n"
		";--------------------------------------------------------------------------\n"
		"; 座標変換																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x4	r0,			v0,		c[3]											\n"
//		"mov	r0.w,		v0.w													\n"
		"mov	oPos,		r0														\n"
		"																			\n"
		";--------------------------------------------------------------------------\n"
		"; FOG																	\n"
		";--------------------------------------------------------------------------\n"
		"dp3	r1.w,		r0,			r0											\n"
		"rsq	r0.w,		r1.w													\n"
		"mul	r0.w,		r0.w,		r1.w	; 視点からの距離					\n"
		"add	r0.w,		c[0].w,		-r0.w	; 距離　500 - 距離						\n"
		"mul	oFog,		c[0].y,		r0.w	; 距離　X 0.004						\n"
		";--------------------------------------------------------------------------\n"
		"; メッシュのテクスチャー													\n"
		";--------------------------------------------------------------------------\n"
		"mov	r1.xy,		v3.xy													\n"
		"add	oT0.xy,		r1.xy,		c[1].xy										\n"
		"																			\n"
		"mov	oD0,		c[2]						; ランバート diffuse		\n"
		"mov	oD0.a,		v2.a						; alpha = model diffuse.a	\n"
	},
//----3----Weather表示用---
	{
		"vs_1_1																		\n"
		"dcl_position0		v0														\n"
		"dcl_normal0		v1														\n"
		"dcl_color0			v2														\n"
		"dcl_texcoord0		v3														\n"
		"																			\n"
//		"def	c[0],		0.005f, 0.001f, 500.f, 1000.0f	; FOG用オフセット	    \n"
		"def	c[0],		0.005f, 0.002f, 500.f, 600.0f	; FOG用オフセット	    \n"
		";--------------------------------------------------------------------------\n"
		"; 座標変換																	\n"
		";--------------------------------------------------------------------------\n"
		"m4x4	r0,			v0,		c[2]											\n"
		"mov	oPos,		r0														\n"
		"																			\n"
		";--------------------------------------------------------------------------\n"
		"; FOG																	\n"
		";--------------------------------------------------------------------------\n"
		"dp3	r1.w,		r0,			r0											\n"
		"rsq	r0.w,		r1.w													\n"
		"mul	r0.w,		r0.w,		r1.w	; 視点からの距離					\n"
		"add	r0.w,		c[0].w,		-r0.w	; 距離　500 - 距離						\n"
		"mul	oFog,		c[0].y,		r0.w	; 距離　X 0.004						\n"
		";--------------------------------------------------------------------------\n"
		"; メッシュのテクスチャー													\n"
		";--------------------------------------------------------------------------\n"
		"mov	r1.xy,		v3.xy													\n"
		"add	oT0.xy,		r1.xy,		c[1].xy										\n"
		"																			\n"
		"mov	oD0,		v2							; alpha = model diffuse.a	\n"
	},
};


float Min4( float v1, float v2, float v3, float v4 )
{
	float val;

	val = min( v1, v2 );
	val = min( val, v3 );
	val = min( val, v4 );
	return val;
}

float Max4( float v1, float v2, float v3, float v4 )
{
	float val;

	val = max( v1, v2 );
	val = max( val, v3 );
	val = max( val, v4 );
	return val;
}

float Max5( float v1, float v2, float v3, float v4,float v5 )
{
	float val;

	val = max( v1, v2 );
	val = max( val, v3 );
	val = max( val, v4 );
	val = max( val, v5 );
	return val;
}

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

//									CEffectModel													//
//		コンストラクタ
CEffectModel::CEffectModel()
{
	m_ModelType = 0;
	m_ModelTotal = 0;
	m_ModelNo = 0;
	m_pTexture = NULL;
	m_lpVB = NULL;
	m_lpIB = NULL;
	m_NumIndex = m_NumVertices = m_NumFaces = m_VBSize = m_IBSize = m_FVF = 0;
}

//		デストラクタ
CEffectModel::~CEffectModel()
{
	m_pTexture = NULL;
	SAFE_RELEASE(m_lpVB);
	SAFE_RELEASE(m_lpIB);
}

const DWORD FVF_EFFECT = (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1);
const DWORD FVF_EFFECT2 = (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

#pragma pack(push,2)

typedef struct{
	DWORD	L;
	unsigned char	numImage;	// 使用するイメージ数。上のデータの後にこの個数分IDが続く
	unsigned char	numNonImage; // イメージを使わないデータの数。
	WORD			numFace, numFace1, numFace2, Flag;
	char				name[16];			//      名前
} DAT1FH;

typedef struct{
	WORD		S1, S2, S3, S4;
	char		name[16];			//      名前
	WORD		S5, S6;
} DAT21H;

typedef struct{
	char		name[16];			//      名前
	BYTE		Ver;				//0x00
	BYTE		nazo;				//0x01
	WORD		Type;				//0x02 &7f==0モデル 1=クロス
	WORD		Flip;				//0x04 0==OFF  ON
} DAT2AH;

typedef struct
{
	float	x, y, z;     //座標
	float	hx, hy, hz;  //法線ベクトル
	DWORD	color;
	float	u, v;
} EFFECTVERTEX;

typedef struct
{
	float	x, y, z;     //座標
	DWORD	color;
	float	u, v;
} EFFECT2VERTEX;
#pragma pack(pop)

//		エフェクトモデル読み込み
HRESULT CEffectModel::LoadEffectModel(char *pFile)
{
	HRESULT hr = D3D_OK;


	//==============================================================
	// メッシュの生成
	//==============================================================
	int				NumFaces, NumVertices;
	EFFECTVERTEX	*pVertex;

	m_ModelType = 0x1f;
	m_ModelTotal = 0;
	m_ModelNo = 0;
	memcpy(m_type, pFile, 4);
	DAT1FH		*pcp = (DAT1FH *)(pFile + 16);
	pVertex = (EFFECTVERTEX*)(pFile + 16 + sizeof(DAT1FH));
	memcpy(m_Name, pcp->name, 16); m_Name[16] = '\0';
	NumFaces = pcp->numFace & 0xff;
	NumVertices = NumFaces * 3;
	m_NumVertices = NumVertices;
	m_NumFaces = NumFaces;
	m_NumIndex = NumFaces * 3;
	m_FVF = FVF_EFFECT;
	m_VBSize = sizeof(EFFECTVERTEX)*NumVertices;
	m_IBSize = m_NumIndex*sizeof(WORD);
	hr = CreateIB(&m_lpIB, m_NumIndex*sizeof(WORD), 0);
	if FAILED(hr) return hr;
	hr = CreateVB(&m_lpVB, NumVertices*sizeof(EFFECTVERTEX), 0, FVF_EFFECT);
	if FAILED(hr) return hr;
	//==============================================================
	// Mesh Data inport
	//  Vertex Inport
	EFFECTVERTEX*	pV;
	if (FAILED(m_lpVB->Lock(0,                 // バッファの最初からデータを格納する。
		m_VBSize, // ロードするデータのサイズ。
		(void**)&pV, // 返されるインデックス データ。
		D3DLOCK_DISCARD)))            // デフォルト フラグをロックに送る。
		return E_FAIL;
	for (int i = 0; i<NumVertices; i++, pV++) {
		pV->x = pVertex[i].x; pV->y = pVertex[i].y; pV->z = pVertex[i].z;
		pV->hx = pVertex[i].hx; pV->hy = pVertex[i].hy; pV->hz = pVertex[i].hz;
		pV->color = pVertex[i].color;
		pV->u = pVertex[i].u; pV->v = pVertex[i].v;
	}
	if (FAILED(hr = m_lpVB->Unlock())) {
		return hr;
	}
	// Face Inport
	WORD*	pIndex;
	int		count;
	if (FAILED(m_lpIB->Lock(0,                 // バッファの最初からデータを格納する。
		m_IBSize, // ロードするデータのサイズ。
		(void**)&pIndex, // 返されるインデックス データ。
		D3DLOCK_DISCARD)))            // デフォルト フラグをロックに送る。
		return E_FAIL;

	count = 0;
	for (int i = 0; i<NumFaces; i++) {
		*pIndex++ = (WORD)count++;
		*pIndex++ = (WORD)count++;
		*pIndex++ = (WORD)count++;
	}
	if (FAILED(hr = m_lpIB->Unlock())) {
		return hr;
	}
	return hr;
}


//		画像関係モデル読み込み
HRESULT CEffectModel::LoadEffectModel2(char *pFile)
{
	HRESULT hr = D3D_OK;


	//==============================================================
	// メッシュの生成
	//==============================================================
	int				NumFaces, NumVertices;
	EFFECT2VERTEX	*pVertex;

	m_ModelType = 0x21;
	m_ModelNo = 0;
	memcpy(m_type, pFile, 4);
	DAT21H		*pcp = (DAT21H *)(pFile + 16);
	//	pVertex		=	(EFFECT2VERTEX*)(pFile+16+sizeof(DAT21H));
	memcpy(m_Name, pcp->name, 16); m_Name[16] = '\0';
	NumFaces = (pcp->S2 & 0xff) * 2;
	NumVertices = NumFaces * 3;
	m_NumVertices = NumVertices;
	m_NumFaces = NumFaces;
	m_ModelTotal = NumFaces / 2;
	m_NumIndex = NumFaces * 3;
	m_FVF = FVF_EFFECT;
	m_VBSize = sizeof(EFFECTVERTEX)*NumVertices;
	m_IBSize = m_NumIndex*sizeof(WORD);
	hr = CreateIB(&m_lpIB, m_NumIndex*sizeof(WORD), 0);
	if FAILED(hr) return hr;
	hr = CreateVB(&m_lpVB, NumVertices*sizeof(EFFECTVERTEX), 0, FVF_EFFECT);
	if FAILED(hr) return hr;
	//==============================================================
	// Mesh Data inport
	//  Vertex Inport
	EFFECTVERTEX*	pV;
	if (FAILED(m_lpVB->Lock(0,                 // バッファの最初からデータを格納する。
		m_VBSize, // ロードするデータのサイズ。
		(void**)&pV, // 返されるインデックス データ。
		D3DLOCK_DISCARD)))            // デフォルト フラグをロックに送る。
		return E_FAIL;
	char *pVer = (pFile + 16 + sizeof(DAT21H));
	for (int i = 0; i<NumVertices; i++, pV++) {
		if (i != 0 && (i % 6) == 0)
			pVer += sizeof(DWORD);
		pVertex = (EFFECT2VERTEX*)pVer;
		pVer += sizeof(EFFECT2VERTEX);
		pV->x = pVertex->x; pV->y = pVertex->y; pV->z = pVertex->z;
		pV->hx = 0.f; pV->hy = 0.f; pV->hz = -1.f;
		pV->color = pVertex->color;
		pV->u = pVertex->u; pV->v = pVertex->v;
	}
	if (FAILED(hr = m_lpVB->Unlock())) {
		return hr;
	}
	// Face Inport
	WORD*	pIndex;
	int		count;
	if (FAILED(m_lpIB->Lock(0,                 // バッファの最初からデータを格納する。
		m_IBSize, // ロードするデータのサイズ。
		(void**)&pIndex, // 返されるインデックス データ。
		D3DLOCK_DISCARD)))            // デフォルト フラグをロックに送る。
		return E_FAIL;

	count = 0;
	for (int i = 0; i<NumFaces; i++) {
		*pIndex++ = (WORD)count++;
		*pIndex++ = (WORD)count++;
		*pIndex++ = (WORD)count++;
	}
	if (FAILED(hr = m_lpIB->Unlock())) {
		return hr;
	}
	return hr;
}

// CKeyFrame
// デフォルトコンストラクタ
//-------------------------------------------------------------
CKeyFrame::CKeyFrame()
	: m_numKey(0), m_keys(0), m_values(0), m_isLoop(TRUE), m_startTime(0), m_duration(0)
{
}

//---------------------------------------------------------
// デストラクタ
//---------------------------------------------------------
CKeyFrame::~CKeyFrame()
{
	if (m_keys != 0) {
		delete[] m_keys;
	}

	if (m_values != 0) {
		delete[] m_values;
	}
}

void CKeyFrame::outputValue(HWND listObj) {
	char buf[256];

	for (int i = 0; i < m_numKey; i++) {
		sprintf(buf, "[%2d] Key (%5.5f) Value(%5.5f)",i, m_keys[i],m_values[i]);
		SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
	}
}

void CKeyFrame::GetKeyFrame(char *pBuf)
{
	char	*ptr;
	int		num = 0;
	float	key = 0.f, val = 0.f;

	memcpy(m_type, pBuf, 4);m_type[4]='\0';
	ptr = pBuf + 16;
	while (key<1.0) {
		key = *((float*)ptr); ptr += 4;
		val = *((float*)ptr); ptr += 4;
		num++;
	}
	CreateKey(num);
	ptr = pBuf + 16;
	for (int i = 0; i<num; i++) {
		key = *((float*)ptr); ptr += 4;
		val = *((float*)ptr); ptr += 4;
		SetKeyValue(i, key, val);
	}
	SetLoopFlag(true);
	SetStartTime(0);
}

//---------------------------------------------------------
// キーの生成
// 引数
//		numKey : キーの数
//---------------------------------------------------------
void CKeyFrame::CreateKey(int numKey)
{

	// キーフレーム数の設定
	m_numKey = numKey;

	// 古いデータを削除
	if (m_keys != 0) {
		delete[] m_keys;
	}

	if (m_values != 0) {
		delete[] m_values;
	}

	// 新しい配列の生成
	m_keys = new float[m_numKey];
	m_values = new float[m_numKey];

}

//---------------------------------------------------------
// キーとキーに対応する値の設定
// キーは小さい方から順番に設定すること
// 2度目以降同じインデックスを使用した場合は上書きされる
// 引数
//		index : インデックス, 0 <= index < createKeyで生成したキーの数
//		key   : キー, 0.0f <= key <= 1.0f
//		value : キーに対応する値
//---------------------------------------------------------
void CKeyFrame::SetKeyValue(int index, float key, float value)
{
	//インデックスが0より小さい、または現在のキーの数以上なら何もしない
	if ((index < 0) || (index >= m_numKey)) {
		return;
	}

	m_keys[index] = key;
	m_values[index] = value;
}

//---------------------------------------------------------
// アニメーションの長さ(キーが0から1まで変化するのにかかる時間)の設定
// 引数
//		duration : 時間(ミリ秒)
//---------------------------------------------------------
void CKeyFrame::SetDuration(DWORD duration)
{
	// アニメーション全体の継続時間をメンバ変数に保存
	m_duration = duration;
}

//---------------------------------------------------------
// アニメーションをループするかどうかの設定
// 引数
//		isLoop : TRUEならループする, FALSEならループしない
//---------------------------------------------------------
void CKeyFrame::SetLoopFlag(DWORD isLoop)
{
	m_isLoop = isLoop;
}

//---------------------------------------------------------
// アニメーション開始時刻の設定
// 引数
//		start : アニメーションの開始時刻(ミリ秒)
//---------------------------------------------------------
void CKeyFrame::SetStartTime(DWORD start)
{
	m_startTime = start;
}

//---------------------------------------------------------
// 時刻に対応する値を取得する
// 引数
//		time   : 時刻(ミリ秒)
//		pValue : 結果を受け取る変数へのポインタ
//		pIsEnd : アニメーション終了判定フラグを受け取る変数へのポインタ, 終了していたらTRUEそうれなければFALSE
// 戻り値
//		値の取得に成功したらTRUE, 失敗したらFALSE
//---------------------------------------------------------
BOOL CKeyFrame::GetValue(DWORD time, float* pValue, BOOL* pIsEnd)
{
	if (pValue == 0) {
		return FALSE;
	}

	if (pIsEnd == 0) {
		return FALSE;
	}

	if (m_numKey <= 0) {
		return FALSE;
	}

	//----------------------------------------
	//アニメーションが終了していたときの処理
	//----------------------------------------
	DWORD	endTime = m_startTime + m_duration;	//終了時刻

	if (!m_isLoop) {	// ループしない場合
		if (time < m_startTime) {
			// 時刻が開始時刻よりも小さければ最初の値を返す
			*pValue = m_values[0];
			*pIsEnd = TRUE;
			return TRUE;
		}
		else if (time > endTime) {
			// 時刻が終了時刻よりも大きければ最後の値を返す
			*pValue = m_values[m_numKey - 1];
			*pIsEnd = TRUE;
			return TRUE;
		}
	}

	//----------------------------------------
	//アニメーションの計算
	//----------------------------------------
	int		beginIndex;
	int		endIndex;
	float	fraction;
	float	slope;

	// 現在時刻を割合に変換
	fraction = GetFraction(time);
	// 現在時刻にもっとも近い直前のキーフレーム取得
	beginIndex = GetBeginIndex(fraction);
	// 現在時刻にもっとも近い直後のキーフレーム取得
	endIndex = beginIndex + 1;

	// 値の計算
	if (beginIndex < 0) {	// アニメーション開始前
		*pValue = m_values[0];	// 最初のキーフレームの値を返す
	}
	else if (beginIndex >= m_numKey - 1) {	// アニメーション終了後
		*pValue = m_values[m_numKey - 1];	// 最後のキーフレームの値を返す
	}
	else {
		// 傾き（1.0あたりの増減量）を求める
		float diffValue = m_values[endIndex] - m_values[beginIndex];// キーフレーム間の値の差
		float diffTime = m_keys[endIndex] - m_keys[beginIndex];		// キーフレーム間の継続時間
		slope = diffValue / diffTime;

		// 現在値を求める
		float fPastFromPrev = fraction - m_keys[beginIndex];	// 直前のキーフレームからの経過時間（現在時刻- 直前のキーフレームの時刻）
		*pValue = slope * fPastFromPrev + m_values[beginIndex]; // 現在値の計算
	}

	*pIsEnd = FALSE;

	return TRUE;
}

//---------------------------------------------------------
// 時刻を割合に変換する
// 引数
//		time   : 時刻(ミリ秒)
// 戻り値
//		割合
//---------------------------------------------------------
float CKeyFrame::GetFraction(DWORD time)
{
	DWORD	diffTime;
	float	fraction;

	// 時刻から割合を求める
	if (m_duration == 0) {
		fraction = 1.0f;
	}
	else {
		if (time > m_startTime) {
			//通常処理
			// 現在時刻からスタート時刻を引く
			diffTime = time - m_startTime;
			// ループ対応
			diffTime = diffTime % m_duration;
			//割合の計算
			fraction = (float)diffTime / (float)m_duration;
			//小数部分だけ使用
			fraction = fraction - (int)fraction;
		}
		else {
			//もし、現在時刻が、スタート時刻より小さいとき
			//タイマーが一周してしまったとき発生する
			diffTime = m_startTime - time;
			diffTime = diffTime % m_duration;
			fraction = (float)diffTime / (float)m_duration;
			fraction = 1.0f - (fraction - (int)fraction);
		}
	}

	return fraction;
}

//---------------------------------------------------------
// 割合に対応するインデックスを取得する
// 引数
//		fraction : 割合
// 戻り値
//		配列のインデックス, fractionが登録されているキーより小さければ-1
//---------------------------------------------------------
int CKeyFrame::GetBeginIndex(float fraction)
{
	if (fraction < m_keys[0])
		return -1;

	// 引数に指定された割合とキーフレームの割合を比較する
	int index = 0;
	for (int i = 0; i < m_numKey; i++) {
		// キーフレームより大きくなったら発見
		if (m_keys[i] <= fraction) {
			index = i;
		}
		else {
			break;
		}
	}

	return index;
}
//-------------------------------------------------------------
// デフォルトコンストラクタ
//-------------------------------------------------------------
CEffect::CEffect()
{
	InitData();
}

//---------------------------------------------------------
// デストラクタ
//---------------------------------------------------------
CEffect::~CEffect()
{
}

void CEffect::InitData(void)
{
	D3DXVECTOR3 tmp1 = { 0.f, 0.f, 0.f };
	D3DXVECTOR3 tmp2 = { 1.f, 1.f, 1.f };

	for (int i = 0; i < 128; i++){
		param[i] = false;
	}
	D3DXMatrixIdentity(&m_mRootTransform);
	m_uv = m_p01 = m_p02 = m_p03 = m_r06 = m_r09 = m_r0A = m_r0B = m_r0C = m_h1F = m_r1F = tmp1;
	m_s0F = m_s10 = m_s11 = m_s12 = m_s13 =  m_s1F = tmp2;
	m_name[0] = '\0'; m_target[0] = '\0';
	m_no = m_1fdiv = 0;
	m_ModelType = 0;
	m_08dist = 0.f;
	m_color.r = m_color.b = m_color.g = m_color.a = 0.f;
	m_Rd = m_Gr = m_Bl = m_Al = NULL;
	m_kfpx = m_kfpy = m_kfpz = NULL;
	m_kfrx = m_kfry = m_kfrz = NULL;
	m_kfsx = m_kfsy = m_kfsz = NULL;
	m_kfu = m_kfv = NULL;
	m_pAreaMesh = NULL;
	m_pEffectModel = NULL;
}

void CEffect::Set1F(int no)
{
	D3DXVECTOR3	vec, pos, tempv;
	D3DXMATRIX	tempm;
	float		angl;

	if (m_no <= 0) return;
	angl = PAI * 2 / (float)(m_no + 1)*no;

	vec.x = 1.f; vec.y = vec.z = 0.f;
	tempv.x = (float)m_r1F.x + (float)rand() / 32767.f*m_r1F.y; tempv.z = 0.f;
	tempv.y = (float)m_h1F.x + (float)rand() / 32767.f*m_h1F.y;
	D3DXMatrixRotationY(&tempm, angl);
	D3DXVec3TransformNormal(&tempv, &tempv, &tempm);
	D3DXVec3TransformNormal(&vec, &vec, &tempm);
	pos.x += tempv.x;
	pos.y += tempv.y;
	pos.z += tempv.z;
	vec.x *= m_08dist;
	vec.y *= m_08dist;
	vec.z *= m_08dist;
}

#define	LIFE_BASE 30

//		エフェクトマトリックスの読み込み
void CEffect::GetEffectMatrix(char *pBuff, CKeyFrame *pKeyFrame)
{
	CKeyFrame	*pKFrame;
	bool		Sflg = false;
	char			*pAdr;
	char			type;
	D3DXMATRIX	AreaMatrix, TempMatrix;
	int			num, dat1Adr, dat2Adr, dat3Adr, dat4Adr;

	memcpy(m_name, pBuff, 4); m_name[4] = '\0';
	pAdr = pBuff;
	dat1Adr = *((int*)(pAdr + 0x80));
	dat2Adr = *((int*)(pAdr + 0x84));
	dat3Adr = *((int*)(pAdr + 0x88));
	dat4Adr = *((int*)(pAdr + 0x8c));

	m_no = *((unsigned char*)(pAdr + 0x78));
	m_interval = *((unsigned short*)(pAdr + 0x76));

	pAdr = pBuff + dat3Adr;
	while (pAdr<(pBuff + dat4Adr)) {
		type = *pAdr;
		num = (int)*(pAdr + 1); num &= 0x0f;
		if (type == 0) break;
		switch (type) {
			case 0x27:
				m_uv.x = *((float*)(pAdr + 4));
				pAdr += 4 * num;
				break;
			case 0x28:
				m_uv.y = *((float*)(pAdr + 4));
				pAdr += 4 * num;
				break;
			default:
				pAdr += 4 * num;
				break;
		}
	}
	pAdr = pBuff + dat2Adr;
	WORD	mtype;
	DWORD	col;
	D3DXMATRIX pmat;
	while (pAdr<(pBuff + dat3Adr)) {
		type = *pAdr;
		num = (int)*(pAdr + 1); num &= 0x0f;
		switch (type) {
		case 0x00:
			D3DXMatrixScaling(&pmat, m_s0F.x, m_s0F.y, m_s0F.z);
			D3DXMatrixMultiply(&m_mRootTransform, &m_mRootTransform, &pmat);
			D3DXMatrixRotationZ(&pmat, m_r09.z);
			D3DXMatrixMultiply(&m_mRootTransform, &m_mRootTransform, &pmat);
			D3DXMatrixRotationY(&pmat, m_r09.y);
			D3DXMatrixMultiply(&m_mRootTransform, &m_mRootTransform, &pmat);
			D3DXMatrixRotationX(&pmat, m_r09.x);
			D3DXMatrixMultiply(&m_mRootTransform, &m_mRootTransform, &pmat);
			D3DXMatrixTranslation(&pmat, m_p01.x, m_p01.y, m_p01.z);
			D3DXMatrixMultiply(&m_mRootTransform, &m_mRootTransform, &pmat);
			return;
		case 0x01: // オフセット
			param[0x01] = true;
			memcpy(m_target, pAdr + 12, 4);m_target[4]='\0';
			m_kind1 = *((WORD*)(pAdr + 4));
			m_kind2 = *((WORD*)(pAdr + 6));
			m_p01.x = *((float*)(pAdr + 20));
			m_p01.y = *((float*)(pAdr + 24));
			m_p01.z = *((float*)(pAdr + 28));
			mtype = *((WORD*)(pAdr + 32));
			if ((mtype >> 8) == 0x0b)
				m_ModelType = 0x1f;
			else if ((mtype >> 8) == 0x0E)
				m_ModelType = 0x21;
			else if ((mtype & 0xff) == 0x3d)
				m_ModelType = 0x3d;
			else
				m_ModelType = 0xff;
			m_lifeTime = *((WORD*)(pAdr + 34));
//			m_lifeTime = (DWORD)((float)m_lifeTime*1000.f / (float)LIFE_BASE);
			pAdr += 4 * num;
			break;
		case 0x02:
			param[0x02] = true;
			m_p02.x = *((float*)(pAdr + 4));
			m_p02.y = *((float*)(pAdr + 8));
			m_p02.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x03:
			param[0x03] = true;
			m_p03.x = *((float*)(pAdr + 4));
			m_p03.y = *((float*)(pAdr + 8));
			m_p03.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x06:
			param[0x06] = true;
			m_r06.x = *((float*)(pAdr + 4));
			m_r06.y = *((float*)(pAdr + 8));
			m_r06.z = 0.f;
			pAdr += 4 * num;
			break;
		case 0x08:
			param[0x08] = true;
			m_08dist = *((float*)(pAdr + 4));
			pAdr += 4 * num;
			break;
		case 0x09://初期位置
			param[0x09] = true;
			m_r09.x = *((float*)(pAdr + 4));
			m_r09.y = *((float*)(pAdr + 8));
			m_r09.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x0a://回転
			param[0x0a] = true;
			m_r0A.x = *((float*)(pAdr + 4));
			m_r0A.y = *((float*)(pAdr + 8));
			m_r0A.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x0b:// 回転差分
			param[0x0b] = true;
			m_r0B.x = *((float*)(pAdr + 4));
			m_r0B.y = *((float*)(pAdr + 8));
			m_r0B.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x0c:// 回転差分　ゆらぎ？
			param[0x0c] = true;
			m_r0C.x = *((float*)(pAdr + 4));
			m_r0C.y = *((float*)(pAdr + 8));
			m_r0C.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x0f:// 初期スケール
			param[0x0f] = true;
			m_s0F.x = *((float*)(pAdr + 4));
			m_s0F.y = *((float*)(pAdr + 8));
			m_s0F.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x10:// スケール差分
			param[0x10] = true;
			m_s10.x = *((float*)(pAdr + 4));
			m_s10.y = *((float*)(pAdr + 8));
			m_s10.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x11://スケールの揺らぎ
			param[0x11] = true;
			m_s11.x = *((float*)(pAdr + 4));
			m_s11.y = *((float*)(pAdr + 4));
			m_s11.z = *((float*)(pAdr + 4));
			pAdr += 4 * num;
			break;
		case 0x12:// スケール差分
			param[0x12] = true;
			Sflg = true;
			m_s12.x = *((float*)(pAdr + 4));
			m_s12.y = *((float*)(pAdr + 8));
			m_s12.z = *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x13:// スケール差分
			param[0x13] = true;
			Sflg = true;
			m_s13.x += *((float*)(pAdr + 4));
			m_s13.y += *((float*)(pAdr + 8));
			m_s13.z += *((float*)(pAdr + 12));
			pAdr += 4 * num;
			break;
		case 0x16:// 初期カラー
			param[0x16] = true;
			col = *((int*)(pAdr + 4));
			m_color.r = (float)((col >> 0) & 0x00ff) / 255.f;
			m_color.g = (float)((col >> 8) & 0x00ff) / 255.f;
			m_color.b = (float)((col >> 16) & 0x00ff) / 255.f;
			m_color.a = (float)((col >> 24) & 0x00ff) / 255.f;
			pAdr += 4 * num;
			break;
		case 0x1E:
			pAdr += 4 * num;
			break;
		case 0x1F:
			param[0x1F] = true;
			m_r1F.y = *((float*)(pAdr + 4));
			m_r1F.x = *((float*)(pAdr + 8));
			m_s1F.x = *((float*)(pAdr + 12));
			m_s1F.y = *((float*)(pAdr + 16));
			m_s1F.z = *((float*)(pAdr + 20));
			m_h1F.x = *((float*)(pAdr + 24));
			m_h1F.y = *((float*)(pAdr + 28));
			m_1fdiv = *((int*)(pAdr + 44));
			pAdr += 4 * num;
			break;
		case 0x21:
			param[0x21] = true;
			m_kfpx = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfpx = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x22:
			param[0x22] = true;
			m_kfpy = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfpy = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x23:
			param[0x23] = true;
			m_kfpz = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfpz = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x24:
			param[0x24] = true;
			m_kfrx = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfrx = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x25:
			param[0x25] = true;
			m_kfry = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfry = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x26:
			param[0x26] = true;
			m_kfrz = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfrz = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x27:
			param[0x27] = true;
			m_kfsx = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfsx = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x28:
			param[0x28] = true;
			m_kfsy = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfsy = pKFrame;
					pKFrame->SetDuration(m_lifeTime);
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x29:
			param[0x29] = true;
			m_kfsz = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfsz = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x2D:
			param[0x2d] = true;
			m_Al = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_Al = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x2E:
			param[0x2e] = true;
			m_kfu = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfu = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x2F:
			param[0x2f] = true;
			m_kfv = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_kfv = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x30:
			pAdr += 4 * num;
			break;
		case 0x60:
			param[0x60] = true;
			m_Rd = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_Rd = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x61:
			param[0x61] = true;
			m_Gr = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_Gr = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		case 0x62:
			param[0x62] = true;
			m_Bl = NULL;
			pKFrame = pKeyFrame;
			while (pKFrame) {
				if (!memcmp(pAdr + 8, pKFrame->m_type, strlen(pKFrame->m_type))) {
					m_Bl = pKFrame;
					break;
				}
				pKFrame = (CKeyFrame*)pKFrame->Next;
			}
			pAdr += 4 * num;
			break;
		default:
			pAdr += 4 * num;
			break;
		}
	}
}


void CEffect::outputProp(HWND listObj) {
	char buf[256];

	sprintf(buf, "[00] ID (%s) ﾀｰｹﾞｯﾄ(%s) U(%5.5f) V(%5.5f)", m_name, m_target,m_uv.x,m_uv.y);
	SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
	for (int i = 0; i < 128; i++) {
		if (param[i] == false) continue;
		switch (i) {
			case 0x00:
				break;
			case 0x01: // オフセット
				sprintf(buf,"[%02x] mtype (%02x) kd1(%04x) kd2(%04x) LfT(%04x)", i, m_ModelType, m_kind1, m_kind2, m_lifeTime);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				sprintf(buf, "[%02x] ｵﾌｾｯﾄ (%5.5f,%5.5f,%5.5f)", i, m_p01.x, m_p01.y, m_p01.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x02:
				sprintf(buf, "[%02x] ｵﾌｾｯﾄ2 (%5.5f,%5.5f,%5.5f)", i, m_p02.x, m_p02.y, m_p02.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x03:
				sprintf(buf, "[%02x] ｵﾌｾｯﾄ3 (%5.5f,%5.5f,%5.5f)", i, m_p03.x, m_p03.y, m_p03.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x06:
				sprintf(buf, "[%02x] 2D回転 (%5.5f,%5.5f,%5.5f)", i, m_r06.x, m_r06.y, m_r06.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x08:
				sprintf(buf, "[%02x] dist (%5.5f)", i, m_08dist);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x09://初期位置
				sprintf(buf, "[%02x] 回転 (%5.5f,%5.5f,%5.5f)", i, m_r09.x, m_r09.y, m_r09.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x0a://回転
				sprintf(buf, "[%02x] 回転2 (%5.5f,%5.5f,%5.5f)", i, m_r0A.x, m_r0A.y, m_r0A.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x0b:// 回転差分
				sprintf(buf, "[%02x] 回転3 (%5.5f,%5.5f,%5.5f)", i, m_r0B.x, m_r0B.y, m_r0B.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x0c:// 回転差分
				sprintf(buf, "[%02x] 回転4 (%5.5f,%5.5f,%5.5f)", i, m_r0C.x, m_r0C.y, m_r0C.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x0f:// 初期スケール
				sprintf(buf, "[%02x] ｽｹｰﾙ (%5.5f,%5.5f,%5.5f)", i, m_s0F.x, m_s0F.y, m_s0F.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x10:// スケール差分
				sprintf(buf, "[%02x] ｽｹｰﾙ2 (%5.5f,%5.5f,%5.5f)", i, m_s10.x, m_s10.y, m_s10.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x11://スケールの揺らぎ
				sprintf(buf, "[%02x] ｽｹｰﾙ3 (%5.5f,%5.5f,%5.5f)", i, m_s11.x, m_s11.y, m_s11.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x12:// スケール差分
				sprintf(buf, "[%02x] ｽｹｰﾙ4 (%5.5f,%5.5f,%5.5f)", i, m_s12.x, m_s12.y, m_s12.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x13:// スケール差分
				sprintf(buf, "[%02x] ｽｹｰﾙ5 (%5.5f,%5.5f,%5.5f)", i, m_s13.x, m_s13.y, m_s13.z);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x16:// 初期カラー
				sprintf(buf, "[%02x] ｶﾗｰ ( %3d ,%3d ,%3d ,%3d )", i, 
					(int)(m_color.r*255.f),(int)(m_color.g*255.f),(int)(m_color.b*255.f), (int)(m_color.a*255.f));
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x1E:
				break;
			case 0x1F:
				sprintf(buf, "[%02x] r (%5.5f,%5.5f) s(%5.5f,%5.5f,%5.5f) h(%5.5f,%5.5f) div %d", i, m_r1F.x, m_r1F.y,
					m_s1F.x, m_s1F.y, m_s1F.z, m_h1F.x, m_h1F.y, m_1fdiv);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x21:
				if (m_kfpx == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) px", i, m_kfpx->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x22:
				if (m_kfpy == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) py", i, m_kfpy->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x23:
				if (m_kfpz == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) pz", i, m_kfpz->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x24:
				if (m_kfrx == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) rx", i, m_kfrx->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x25:
				if (m_kfry == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) ry", i, m_kfry->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x26:
				if (m_kfrz == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) rz", i, m_kfrz->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x27:
				if (m_kfsx == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) sx", i, m_kfsx->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x28:
				if (m_kfsy == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) sy", i, m_kfsy->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x29:
				if (m_kfsz == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) sz", i, m_kfsz->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x2D:
				if (m_Al == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) alph", i, m_Al->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x2E:
				if (m_kfu == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) u", i, m_kfu->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x2F:
				if (m_kfv == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) v", i, m_kfv->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x30:
				break;
			case 0x60:
				if (m_Rd == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) Rd", i, m_Rd->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x61:
				if (m_Gr == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) Gr", i, m_Gr->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			case 0x62:
				if (m_Bl == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) Bl", i, m_Bl->m_type);
				SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)buf);
				break;
			default:
				break;
		}
	}
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

	memcpy(m_AreaType, pFile, 4);
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
			pIdx = new WORD[nIndex];
			memcpy((char*)pIdx,(Pos+16+4+nVertex*36+4),sizeof(WORD)*nIndex);
			for( int j=0 ; j<nIndex ; j++ ) {
				pIdx[j] += NumVertices;
			}
			memcpy((char*)pV,(char*)pVertex,sizeof(D3DTEXVERTEX)*nVertex);
			pV += nVertex;
			memcpy((char*)pIndex,(char*)pIdx,sizeof(WORD)*nIndex);
			pIndex += nIndex;
			SAFE_DELETES( pIdx );
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

//		コンストラクタ
CArea::CArea()
{
	m_mArea				= 0;
	m_VertexFormat			= NULL;
	m_VertexSize			= 0;
	m_hVertexShader			= NULL;
	m_Textures.Init();
	D3DXMatrixIdentity( &m_mRootTransform );
	m_mRootTransform		*= matrixMirrorY;
	//m_mRootTransform *= matrixMirrorZ;
	//D3DXMatrixRotationZ(&m_mRootTransform,PAI);
	m_pObjInfo = NULL;
	m_nObj					= 0;

	m_AreaMeshs.Init();
	m_EffMeshs.Init();
}

//		デストラクタ
CArea::~CArea()
{
	m_Textures.Release();
	CAreaMesh *pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
	while( pAreaMesh ) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_AreaMeshs.Release();
	pAreaMesh = (CAreaMesh*)m_EffMeshs.Top();
	while (pAreaMesh) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_EffMeshs.Release();
	CTexture* pTexture = (CTexture*)m_Textures.Top();
	while ( pTexture != NULL ) {
		pTexture->~CTexture();
		pTexture = (CTexture*)pTexture->Next;
	}
	m_Textures.Release();
	SAFE_DELETES( m_pObjInfo );
}

//		データの初期化
void	CArea::InitData(void)
{
	m_Textures.Release();
	CAreaMesh *pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
	while( pAreaMesh ) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_AreaMeshs.Release();
	pAreaMesh = (CAreaMesh*)m_EffMeshs.Top();
	while (pAreaMesh) {
		pAreaMesh->~CAreaMesh();
		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
	}
	m_EffMeshs.Release();
	CTexture* pTexture = (CTexture*)m_Textures.Top();
	while ( pTexture != NULL ) {
		pTexture->~CTexture();
		pTexture = (CTexture*)pTexture->Next;
	}
	m_Textures.Release();
	SAFE_DELETES( m_pObjInfo );
	m_pObjInfo		= NULL;
	m_nObj			= 0;
}

//		テクスチャの読み込み
HRESULT CArea::LoadTextureFromFile( char *FileName  )
{
	HRESULT hr							= S_OK;

	// ファイルをメモリに取り込む
	char *pdat=NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		dwSize = GetFileSize(hFile,NULL);
	    pdat = new char[dwSize];
	    ReadFile(hFile,pdat,dwSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}
	// テクスチャの読み込み
	int			type,pos=0,next;
	while( pos<dwSize ) {
		next = *((int*)(pdat+pos+4));next>>=3;next&=0x7ffff0;
		if( next<16 ) break; 
		if( next+pos>dwSize ) break;
		type = *((int*)(pdat+pos+4));type &=0x7f;
		switch( type ) {
		case 0x20 :
			CTexture *pTexture;
			pTexture = new CTexture;
			m_Textures.InsertEnd( pTexture );
			// テクスチャ
			char	TexName[18];
			strncpy(TexName,pdat+pos+16+1,16);TexName[16]='\0';
			pTexture->SetTexName(TexName);
			LPDIRECT3DTEXTURE9 pTex;
			UINT xx,yy;
			xx = *(UINT*)(pdat+pos+33+4);
			yy = *(UINT*)(pdat+pos+33+8);
			D3DLOCKED_RECT rc;
			if( *(DWORD*)(pdat+pos+33+0x28) == 'DXT3' ){
				hr = GetDevice()->CreateTexture(xx,yy,0,0 ,D3DFMT_DXT3,D3DPOOL_MANAGED,&pTex,NULL);
				if( hr!=D3D_OK ) break;
				hr = pTex->LockRect(0,&rc,NULL,0);
				if(hr==D3D_OK){
				  CopyMemory(rc.pBits,pdat+pos+33+0x28+12,(xx/4) * (yy/4) * 16 );
				  pTex->UnlockRect(0);
				}
			} else if( *(DWORD*)(pdat+pos+33+0x28) == 'DXT1' ){
				hr = GetDevice()->CreateTexture(xx,yy,0,0,D3DFMT_DXT1,D3DPOOL_MANAGED,&pTex,NULL);
				if( hr!=D3D_OK ) break;
				hr = pTex->LockRect(0,&rc,NULL,0);
				if(hr==D3D_OK){
				  CopyMemory(rc.pBits,pdat+pos+33+0x28+12,(xx/4) * (yy/4) * 8  );
				  pTex->UnlockRect(0);
				}
			} else {
				hr = GetDevice()->CreateTexture(xx,yy,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&pTex,NULL);
				if( hr!=D3D_OK ) return NULL;
				hr = pTex->LockRect(0,&rc,NULL,0);
				if(hr==D3D_OK){
					for( DWORD jy=0; jy<yy; jy++ ){
						for( DWORD jx=0; jx<xx; jx++ ){
							DWORD *pp  = (DWORD *)rc.pBits;
							BYTE  *idx = (BYTE  *)(pdat+pos+33+0x28+0x400);
							DWORD *pal = (DWORD *)(pdat+pos+33+0x28);
							pp[(yy-jy-1)*xx+jx] = pal[idx[jy*xx+jx]];
						}
					}
				}
				pTex->UnlockRect(0);
			}
			pTexture->SetTexture( pTex );
			SAFE_RELEASE( pTex );
			break;
		}
		pos+=next;
	}
	// 終了
	SAFE_DELETES(pdat);
//	delete pdat;
	return hr;
}

//======================================================================
//
//		(0x19,0x05)キーフレーム,エフェクトモーションの読み込み
//
//	input
//		char *filename			: 読み込みファイル名（リソース名でも可
//
//	output
//		エラー文字列へのポインタ。
//		正常終了の場合はNULL。
//
//======================================================================
HRESULT CArea::LoadEffectFromFile(char *FileName)
{
	HRESULT hr = S_OK;

	//====================================================
	// ファイルをメモリに取り込む
	//====================================================
	char *pdat = NULL;
	int dwSize;
	CKeyFrame *pKeyFrame;
	CEffect *pEffect;
	unsigned long	cnt;

	HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE){
		dwSize = GetFileSize(hFile, NULL);
		pdat = new char[dwSize];
		ReadFile(hFile, pdat, dwSize, &cnt, NULL);
		CloseHandle(hFile);
		hr = 0;
	}
	else {
		return -1;
	}

	//====================================================
	// メッシュの読み込み
	//====================================================
	int					type, pos = 0, next;
	while (pos<dwSize) {
		next = *((int*)(pdat + pos + 4)); next >>= 3; next &= 0x7ffff0;
		if (next<16) break;
		if (next + pos>dwSize) break;
		type = *((int*)(pdat + pos + 4)); type &= 0x7f;
		switch (type) {
		case 0x19:
			pKeyFrame = new CKeyFrame;
			pKeyFrame->GetKeyFrame(pdat + pos);
			m_KeyFrames.InsertTop(pKeyFrame);
			break;
		}
		pos += next;
	}
	pos = 0;
	char className[6]; className[0] = '\0';
	while (pos<dwSize) {
		next = *((int*)(pdat + pos + 4)); next >>= 3; next &= 0x7ffff0;
		if (next<16) break;
		if (next + pos>dwSize) break;
		type = *((int*)(pdat + pos + 4)); type &= 0x7f;
		switch (type) {
		case 0x00:
			className[0] = '\0';
			break;
		case 0x01:
			memcpy(className, pdat + pos, 4); className[4] = '\0';
			break;
		case 0x05:
			pEffect = new CEffect;
			strcpy(pEffect->m_class, className);
			pEffect->InitData();
			pEffect->GetEffectMatrix(pdat + pos, (CKeyFrame*)m_KeyFrames.Top());
			m_Effects.InsertTop(pEffect);
			break;
		}
		pos += next;
	}
	pEffect = (CEffect*)m_Effects.Top();
	while (pEffect) {
		pEffect->m_pAreaMesh = NULL;
		CAreaMesh *pAreaMesh = (CAreaMesh *)m_EffMeshs.Top();
		while (pAreaMesh) {
			if (!memcmp(pEffect->m_target, pAreaMesh->m_AreaType, 4) 
	//			&& pEffect->m_ModelType == pEffectModel->m_ModelType
			) {
				pEffect->m_pAreaMesh = pAreaMesh;
				break;
			}
			pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
		}
		pEffect->m_pEffectModel = NULL;
		CEffectModel *pEffectModel = (CEffectModel*)m_EffectModels.Top();
		while (pEffectModel) {
			if (!memcmp(pEffect->m_target, pEffectModel->m_type, 4) &&
				pEffect->m_ModelType == pEffectModel->m_ModelType) {
				pEffect->m_pEffectModel = pEffectModel;
				break;
			}
			pEffectModel = (CEffectModel*)pEffectModel->Next;
		}
		pEffect = (CEffect*)pEffect->Next;
	}
	SAFE_DELETES(pdat);
	return hr;
}
//======================================================================
//
//		(0x1F)エフェクトモデルの読み込み
//
//	input
//		char *filename			: 読み込みファイル名（リソース名でも可
//		unsigned long FVF		: メッシュのFVF
//
//	output
//		エラー文字列へのポインタ。
//		正常終了の場合はNULL。
//
//======================================================================
HRESULT CArea::LoadEffectModelFromFile(char *FileName)
{
	CEffectModel *pEffectModel;
	CTexture	*pTexture;
	DAT2AH	*pHeader;
	HRESULT hr = S_OK;

	//====================================================
	// ファイルをメモリに取り込む
	//====================================================
	char *pdat = NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE){
		dwSize = GetFileSize(hFile, NULL);
		pdat = new char[dwSize];
		ReadFile(hFile, pdat, dwSize, &cnt, NULL);
		CloseHandle(hFile);
		hr = 0;
	}
	else {
		return -1;
	}
	//====================================================
	// メッシュの読み込み
	//====================================================
	int			type, pos = 0, next;
	while (pos<dwSize) {
		next = *((int*)(pdat + pos + 4)); next >>= 3; next &= 0x7ffff0;
		if (next<16) break;
		if (next + pos>dwSize) break;
		type = *((int*)(pdat + pos + 4)); type &= 0x7f;
		switch (type) {
		case 0x1F: // EffectModel
			pHeader = (DAT2AH*)(pdat + pos);
			pEffectModel = new CEffectModel;
			m_EffectModels.InsertTop(pEffectModel);
			pEffectModel->LoadEffectModel(pdat + pos);
			pTexture = (CTexture*)m_Textures.Top();
			int texno = 0;
			while (pTexture != NULL) {
				if (!memcmp(pEffectModel->m_Name, pTexture->m_TexName, 16)){
					pEffectModel->m_texNo = texno;
					pEffectModel->m_pTexture = pTexture;
					break;
				}
				texno++;
				pTexture = (CTexture*)pTexture->Next;
			}
		}
		pos += next;
	}
	// 終了
	SAFE_DELETES(pdat);
	return hr;
}

//======================================================================
//
//		（0x21）エフェクト2モデルの読み込み
//
//	input
//		char *filename			: 読み込みファイル名（リソース名でも可
//		unsigned long FVF		: メッシュのFVF
//
//	output
//		エラー文字列へのポインタ。
//		正常終了の場合はNULL。
//
//======================================================================
HRESULT CArea::LoadEffectModel2FromFile(char *FileName)
{
	DAT2AH	*pHeader;
	HRESULT hr = S_OK;

	//====================================================
	// ファイルをメモリに取り込む
	//====================================================
	char *pdat = NULL;
	int dwSize;
	unsigned long	cnt;
	HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE){
		dwSize = GetFileSize(hFile, NULL);
		pdat = new char[dwSize];
		ReadFile(hFile, pdat, dwSize, &cnt, NULL);
		CloseHandle(hFile);
		hr = 0;
	}
	else {
		return -1;
	}

	//====================================================
	// メッシュの読み込み
	//====================================================
	int			count = 0, type, pos = 0, next;
	CTexture		*pTexture;
	while (pos<dwSize) {
		next = *((int*)(pdat + pos + 4)); next >>= 3; next &= 0x7ffff0;
		if (next<16) break;
		if (next + pos>dwSize) break;
		type = *((int*)(pdat + pos + 4)); type &= 0x7f;
		switch (type) {
		case 0x21: // EffectModel
			pHeader = (DAT2AH*)(pdat + pos);
			CEffectModel *pEffectModel = new CEffectModel;
			m_EffectModels.InsertTop(pEffectModel);
			pEffectModel->LoadEffectModel2(pdat + pos);
			if (pEffectModel->m_ModelTotal <= 0) {
				m_EffectModels.Erase(pEffectModel);
				SAFE_DELETE(pEffectModel);
			}
			else {
				pTexture = (CTexture*)m_Textures.Top();
				int texno = 0;
				while (pTexture != NULL) {
					if (!memcmp(pEffectModel->m_Name, pTexture->m_TexName, 16)){
						pEffectModel->m_texNo = texno;
						pEffectModel->m_pTexture = pTexture;
						break;
					}
					texno++;
					pTexture = (CTexture*)pTexture->Next;
				}
			}
		}
		pos += next;
	}
	// 終了
	SAFE_DELETES(pdat);
	return hr;
}

//		MMBのデコードsub
void CArea::DecodeMMBSub(BYTE *p)
{
	if(p[6] == 0xFF && p[7] == 0xFF)	{
		int decode_length = (p[0] << 0) | (p[1] << 8) | (p[2] << 16);
		DWORD key1 = p[5] ^ 0xF0;
		DWORD key2 = key_table2[key1] ;

		DWORD decode_count = ((decode_length - 8) & ~0xf) / 2;

		DWORD *data1 = (DWORD *)(p + 8 + 0);
		DWORD *data2 = (DWORD *)(p + 8 + decode_count);
		for(DWORD pos = 0; pos < decode_count; pos += 8)
		{
			if(key2 & 1)
			{
				DWORD tmp;

				tmp = data1[0];
				data1[0] = data2[0];
				data2[0] = tmp;

				tmp = data1[1];
				data1[1] = data2[1];
				data2[1] = tmp;
			}
			key1 += 9;
			key2 += key1;
			data1 += 2;
			data2 += 2;
		}
	}
}

//		MMBのデコード
void CArea::DecodeMMB(BYTE*p)
{
	if(p[3] >= 5)
	{
		int decode_length = (p[0] << 0) | (p[1] << 8) | (p[2] << 16);
		DWORD key = key_table[p[5] ^ 0xF0];
		int key_counter = 0;

		for(int pos = 8; pos < decode_length; pos++)
		{
			DWORD x = ((key & 0xFF) << 8) | (key & 0xFF);
			key += ++key_counter;

			p[pos] ^= (x >> (key & 7));
			key += ++key_counter;
		}
	}
	DecodeMMBSub(p);
}

//		MZBのデコード
void CArea::DecodeMZB(BYTE* p)
{
	if (p[3] >= 0x1B)
	{
		int decode_length = (p[0] << 0) | (p[1] << 8) | (p[2] << 16);
		DWORD key = key_table[p[7] ^ 0xFF];
		int key_counter = 0;

		for (int pos = 8; pos < decode_length; )
		{
			int xor_length = ((key >> 4) & 7) + 16;

			if ((key & 1) && (pos + xor_length < decode_length))
			{
				for (int i = 0; i < xor_length; i++)
	 			{
					p[pos+i] ^= 0xFF;
				}
			}
			key += ++key_counter;
			pos += xor_length;
		}
		int node_count = (p[4] << 0) | (p[5] << 8) | (p[6] << 16);
		TEMPOBJINFO *node = (TEMPOBJINFO *)(p+32);
		for(int i = 0; i < node_count; i++)
		{
			for(int i = 0; i < 16; i++)
			{
				node->id[i] ^= 0x55;
			}
			node++;
		}
	}
}

//		頂点シェーダー作成
bool CArea::CreateVertexShader( void )
{
	HRESULT hr;
	ID3DXBuffer *pShader = NULL;

    // シェーディング用バーテックスシェーダー作成
    hr = GetDevice()->CreateVertexDeclaration( VSFormat, &m_VertexFormat );
    if( hr ) return false;
	// シェーダーのアセンブル
	hr = D3DXAssembleShader( pVertexShaders[0],strlen(pVertexShaders[0]),0,NULL,NULL,&pShader,NULL );
	if SUCCEEDED( hr ){
		// シェーダー生成
		hr = GetDevice()->CreateVertexShader((DWORD*)pShader->GetBufferPointer(),&m_hVertexShader );
		pShader->Release();
	}
	return SUCCEEDED( hr );
}


//		エリアメッシュの読み込み
HRESULT CArea::LoadAreaFromFile( char *FileName, unsigned long FVF )
{
	char			MeshName[20];
	D3DXMATRIX		AreaMatrix,TempMatrix;
	TEMPOBJINFO		*pTobj;
	CAreaMesh		*pAreaMesh;
	HRESULT			hr			=	S_OK;
	unsigned long	cnt;
	char			*pFileBuf	= NULL;
	int				mFileSize	=	0;
	D3DXVECTOR3		BL,BH,BM1,BM2;

	// ファイルをメモリに取り込む
	HANDLE hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if( hFile!=INVALID_HANDLE_VALUE ){
		mFileSize = GetFileSize(hFile,NULL);
	    pFileBuf = new char[mFileSize];
	    hr = ReadFile(hFile,pFileBuf,mFileSize,&cnt,NULL);
	    CloseHandle(hFile);
		hr = 0;
	} else {
		return -1;
	}
	// エリアメッシュのインフォメーション読み込み
	int			mzbcnt=0,i,type,pos=0,next;
	while( pos<mFileSize ) {
		next = *((int*)(pFileBuf+pos+4));next>>=3;next&=0x7ffff0;
		if( next<16 ) break; 
		if( next+pos>mFileSize ) break;
		type = *((int*)(pFileBuf+pos+4));type &=0x7f;
		switch( type ) {
		case 0x1c : // AreaMesh
			DecodeMZB((BYTE*)&pFileBuf[pos+16]);
			if( mzbcnt==0 ) {
				m_nObj = (*(int*)(pFileBuf+pos+4+16) )&0xffffff;
				m_pObjInfo = new OBJINFO[ m_nObj ];
				pTobj = (TEMPOBJINFO*)(pFileBuf+pos+32+16);
				for( i=0 ; i<m_nObj ; i++,pTobj++ ) {
					memcpy((char*)&m_pObjInfo[i],(char*)pTobj,sizeof(TEMPOBJINFO));
				}
			} else {
				int nObj = (*(int*)(pFileBuf+pos+4+16) )&0xffffff;
				OBJINFO *pObjInfo = new OBJINFO[ m_nObj + nObj];
				pTobj = (TEMPOBJINFO*)(pFileBuf+pos+32+16);
				memcpy((char*)pObjInfo,(char*)m_pObjInfo,sizeof(OBJINFO)*m_nObj);
				for( i=0 ; i<nObj ; i++,pTobj++ ) {
					memcpy((char*)(pObjInfo+m_nObj+i),(char*)pTobj,sizeof(TEMPOBJINFO));
				}
				SAFE_DELETES( m_pObjInfo );
				m_pObjInfo = pObjInfo;
				m_nObj += nObj;
			}
			mzbcnt++;
			break;
		}
		pos+=next;
	}
	// エリアメッシュのデータ読み込み
	pos=0;
	while( pos<mFileSize ) {
		next = *((int*)(pFileBuf+pos+4));next>>=3;next&=0x7ffff0;
		if( next<16 ) break; 
		if( next+pos>mFileSize ) break;
		type = *((int*)(pFileBuf+pos+4));type &=0x7f;
		switch( type ) {
		case 0x2e : // AreaMesh
			DecodeMMB((BYTE*)&pFileBuf[pos+16]);
			memcpy(MeshName,pFileBuf+pos+32,16);
			for( i=0 ; i<m_nObj ; i++ ) {
				if( !memcmp(MeshName,m_pObjInfo[i].mObj.id,16) ) break;
			}
			if (i >= m_nObj) {
				pAreaMesh = new CAreaMesh;
				if (pAreaMesh == NULL) return -1;
				memcpy(pAreaMesh->m_AreaName, pFileBuf + pos + 32, 16);
				pAreaMesh->LoadAreaMesh(pFileBuf + pos, this, FVF);
				if (pAreaMesh->GetlpVB() == NULL || pAreaMesh->GetlpIB() == NULL) {
					SAFE_DELETE(pAreaMesh);
				}
				else {
					//				m_LAreaMeshs.push_back(*pAreaMesh );
					m_EffMeshs.InsertEnd(pAreaMesh);
				}
			}
			else {
				pAreaMesh = new CAreaMesh;
				if (pAreaMesh == NULL) return -1;
				memcpy(pAreaMesh->m_AreaName, pFileBuf + pos + 32, 16);
				pAreaMesh->LoadAreaMesh(pFileBuf + pos, this, FVF);
				if (pAreaMesh->GetlpVB() == NULL || pAreaMesh->GetlpIB() == NULL) {
					SAFE_DELETE(pAreaMesh);
				}
				else {
					//				m_LAreaMeshs.push_back(*pAreaMesh );
					m_AreaMeshs.InsertEnd(pAreaMesh);
				}
			}
			break;
		}
		pos+=next;
	}
	for( i=0 ; i<m_nObj ; i++ ) {
		D3DXMatrixIdentity(&AreaMatrix);
		D3DXMatrixScaling(&TempMatrix,m_pObjInfo[i].mObj.fScaleX,m_pObjInfo[i].mObj.fScaleY,m_pObjInfo[i].mObj.fScaleZ );
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixRotationX(&TempMatrix,m_pObjInfo[i].mObj.fRotX);
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixRotationY(&TempMatrix,m_pObjInfo[i].mObj.fRotY);
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixRotationZ(&TempMatrix,m_pObjInfo[i].mObj.fRotZ);
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		D3DXMatrixTranslation(&TempMatrix,m_pObjInfo[i].mObj.fTransX,m_pObjInfo[i].mObj.fTransY,m_pObjInfo[i].mObj.fTransZ );
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&TempMatrix);
		m_pObjInfo[i].mMat = AreaMatrix;
		m_pObjInfo[i].pAreaMesh = NULL;
		pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
		while( pAreaMesh ) {
			if( !memcmp(pAreaMesh->m_AreaName,m_pObjInfo[i].mObj.id,16) ) {
				m_pObjInfo[i].pAreaMesh = pAreaMesh;				
				break;
			}
			pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
		}
		if( pAreaMesh==NULL ) continue;
		if( (m_pObjInfo[i].mObj.fe&0xfff0fff0)!=0 ) continue;
	}
	SAFE_DELETES( pFileBuf );
	return hr;
}


//
//		レンダリング
unsigned long CArea::Rendering( float PosX, float PosY, float PosZ )
{
	float			DispArea;
	CAreaMesh		*pAreaMesh;
	D3DXMATRIX		AreaMatrix;
	D3DXVECTOR3		mPlate;
	D3DXVECTOR3		BL,BL2,BL3,BL4,BH,BH2,BH3,BH4;
	unsigned long	count = 0;

	//---------------------------------------------------------
	// ピクセルシェーダー設定
	//---------------------------------------------------------
	GetDevice()->SetPixelShader( NULL );
	//mPlate = g_mAt - g_mEye;D3DXVec3Normalize(&mPlate,&mPlate);
	GetDevice()->SetRenderState( D3DRS_FOGENABLE,			TRUE );
	GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
	GetDevice()->SetRenderState( D3DRS_STENCILENABLE,		FALSE );
	GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );	
	GetDevice()->SetRenderState( D3DRS_ALPHAREF ,			0x00 );
	GetDevice()->SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_NOTEQUAL );
	GetDevice()->SetRenderState( D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA );
	GetDevice()->SetRenderState( D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA );
	GetDevice()->SetRenderState( D3DRS_SHADEMODE,			D3DSHADE_GOURAUD );

	//---------------------------------------------------------
	// シェーダー設定
	//---------------------------------------------------------
	GetDevice()->SetTransform(D3DTS_VIEW, &g_mView);
	GetDevice()->SetTransform(D3DTS_PROJECTION, &g_mProjection);
	GetDevice()->SetSoftwareVertexProcessing(g_mIsUseSoftware);
	//GetDevice()->LightEnable(0, FALSE);
	//GetDevice()->SetRenderState(D3DRS_LIGHTING, FALSE);   // 発電所を回す！
	// 変換行列
	D3DXMATRIX mTransform = g_mView * g_mProjection;
	D3DXMatrixTranspose(&mTransform, &mTransform);
	// ライトの方向を変換
	D3DXVECTOR4 LightDir(-g_mLight.Direction.x, -g_mLight.Direction.y, -g_mLight.Direction.z, 0.7f);

	//GetDevice()->SetVertexShader( NULL );
	GetDevice()->SetVertexShader(m_hVertexShader);
	GetDevice()->SetVertexDeclaration(m_VertexFormat);
	GetDevice()->SetVertexShaderConstantF(5, (float*)&g_mEye, 1);
	GetDevice()->SetVertexShaderConstantF(4, (float*)&LightDir, 1);
	GetDevice()->SetVertexShaderConstantF(6, (float*)&mTransform, 4);

	//------------------------------------------------------
	// 各Area 1のレンダリング
	//------------------------------------------------------
	for( int i=0 ; i<m_nObj ; i++) {
		if( (m_pObjInfo[i].mObj.fe&0xfff0ffff)==0 ) { 
			DispArea = g_mDispArea;
		} else {
			DispArea = g_mDispTree;
		}
		if( (pAreaMesh = m_pObjInfo[i].pAreaMesh)==NULL )
			continue;
		AreaMatrix = m_pObjInfo[i].mMat;
		D3DXMatrixMultiply(&AreaMatrix,&AreaMatrix,&m_mRootTransform);
		BL = pAreaMesh->GetBoxLow();
		BH = pAreaMesh->GetBoxHigh();
		BL2.x = BL.x; BL2.y = BL.y; BL2.z = BH.z;
		BL3.x = BH.x; BL3.y = BL.y; BL3.z = BH.z;
		BL4.x = BH.x; BL4.y = BL.y; BL4.z = BL.z;
		BH2.x = BH.x; BH2.y = BH.y; BH2.z = BL.z;
		BH3.x = BL.x; BH3.y = BH.y; BH3.z = BL.z;
		BH4.x = BL.x; BH4.y = BH.y; BH4.z = BH.z;
		D3DXVec3TransformCoord(&BH,&BH,&AreaMatrix );
		D3DXVec3TransformCoord(&BL,&BL,&AreaMatrix );
		D3DXVec3TransformCoord(&BL2,&BL2,&AreaMatrix );
		D3DXVec3TransformCoord(&BH2,&BH2,&AreaMatrix );
		if( Min4(BL.x,BH.x,BL2.x,BH2.x)>PosX+DispArea ) continue;
		if( Max4(BL.x,BH.x,BL2.x,BH2.x)<PosX-DispArea ) continue;
		if( Min4(BL.z,BH.z,BL2.z,BH2.z)>PosZ+DispArea ) continue;
		if( Max4(BL.z,BH.z,BL2.z,BH2.z)<PosZ-DispArea ) continue;
		
		//---------------------------------------------------------
		// 頂点バッファをデバイスに設定
		//---------------------------------------------------------
		GetDevice()->SetStreamSource( 0, pAreaMesh->GetlpVB(), 0,D3DXGetFVFVertexSize(pAreaMesh->m_FVF) );

		//---------------------------------------------------------
		// インデックスバッファをデバイスに設定
		//---------------------------------------------------------
		GetDevice()->SetIndices( pAreaMesh->m_lpIB );


		//---------------------------------------------------------
		// サーフェイスごとにレンダリング
		//---------------------------------------------------------
		// マトリックス設定
		D3DXMatrixTranspose( &AreaMatrix, &AreaMatrix );
		GetDevice()->SetVertexShaderConstantF( 10, (float*)&AreaMatrix, 4 );

		// レンダリング
		list<CStream>::iterator its = pAreaMesh->m_LStreams.begin();
		list<CStream>::iterator ite = pAreaMesh->m_LStreams.end();
		for( ; its != ite ; its++ ) {	
			if( its->GetAlphaFlag() & 0x02 || its->GetStencilFlag() ) {
				GetDevice()->SetRenderState( D3DRS_CULLMODE,		D3DCULL_NONE );
			} else {
				if( (m_pObjInfo[i].mObj.fScaleX*m_pObjInfo[i].mObj.fScaleZ)<0.f )
					GetDevice()->SetRenderState( D3DRS_CULLMODE,	D3DCULL_CW );
				else
					GetDevice()->SetRenderState( D3DRS_CULLMODE,	D3DCULL_CCW );
			}
			CTexture *pTexture = (CTexture*)its->GetpTexture();
			if( (its->GetAlphaFlag() & 0x08) || its->GetStencilFlag() ) {
				if( its->GetStencilFlag() ) {
					GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
					GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		TRUE );
				} else {
					GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE );
					GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );
				}
				GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			} else {
				GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
				GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );
				GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
			}
			//
			// c14 : マテリアル
			//
			if( pTexture != NULL ) {
				// デバイスにテクスチャ設定
				GetDevice()->SetTexture( 0, pTexture->GetTexture() );
			} else {
				GetDevice()->SetTexture( 0, NULL );
			}
			HRESULT hr = GetDevice()->DrawIndexedPrimitive(
					its->GetPrimitiveType(),
					0,
					0,
					pAreaMesh->GetNumVertices(),
					its->GetIndexStart(),
					its->GetFaceCount() );

		}
		count += pAreaMesh->GetNumFaces();

	}
	//---------------------------------------------------------
	// 後処理
	//-----------------------------------------------------------
	GetDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE,	FALSE );
	GetDevice()->SetRenderState( D3DRS_ALPHATESTENABLE,		FALSE );	
	GetDevice()->SetRenderState( D3DRS_FOGENABLE,			FALSE );
	//GetDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	return count;
}

//		MAPデータ読み込み
bool CArea::LoadMAP()
{
	unsigned long FVF = (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1);
	char FileName[512],FileName2[512];

	// データ初期化
	InitData();

	if( (GetArea()&0xeffff) == 0 ) return true;

	//  ファイルのロード
	GetFileNameFromDno(FileName,GetArea());
	GetFileNameFromDno(FileName2,ConvertStr2Dno("0-0"));
	//  ファイルのロード
	HRESULT hr;
	//　テクスチャ、メッシュのロード
	hr = LoadTextureFromFile( FileName2 );
	if( hr ) return false;
	hr = LoadTextureFromFile( FileName );
	if( hr ) return false;
	hr = LoadAreaFromFile( FileName, FVF );
	if( hr ) return false;
	InitEffectModel();
	LoadEffectModelFromFile(FileName);
	LoadEffectModelFromFile(FileName2);
	LoadEffectModel2FromFile(FileName);
	LoadEffectModel2FromFile(FileName2);
	InitEffect();
	LoadEffectFromFile(FileName);
	//InitSchedule();
	//LoadScheduleFromFile(FileName);
	return true;
}

//======================================================================
//		MQOセーブ		通常データをMQOフォーマットで出力します
//======================================================================
bool CArea::saveMQO(char *FPath, char *FName,float posX,float posY,float posZ)
{
	FILE			*fd;
	char			*ptr, path[256],texpath[256];
	D3DTEXVERTEX	*pV;
	D3DTEXVERTEX	mVertex;
	D3DXVECTOR3     mVer;
	WORD			*pIndex, *pI;
	int				cnt,nVer, nFace,idxmin,idxmax;
	int				i1, i2, i3, t1, t2, t3;
	CAreaMesh		*pAreaMesh;
	D3DXMATRIX		RootMatrix,AreaMatrix;
	float			DispArea;
	D3DXVECTOR3		BL, BL2, BL3, BL4, BH, BH2, BH3, BH4;

	D3DXMatrixRotationZ(&RootMatrix, PAI);
	if ((ptr = strrstr(FPath, FName))) *ptr = '\0';
	if ((ptr = strrstr(FName, ".mqo"))) *ptr = '\0';
	sprintf(path, "%s%s.mqo", FPath, FName);
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	if ((fd = fopen(path, "w")) == NULL) return false;
	fprintf(fd, "Metasequoia Document\nFormat Text Ver 1.0\n\nScene {\n");
	fprintf(fd, "	pos 0.0000 0.0000 5000.0000\n");
	fprintf(fd, "	lookat 0.0000 0.0000 0.0000\n");
	fprintf(fd, "	head 0.0000\n");
	fprintf(fd, "	pich 0.0000\n");
	fprintf(fd, "	ortho 1\n");
	fprintf(fd, "	zoom2 2.0000\n");
	fprintf(fd, "	amb 0.250 0.250 0.250\n}\nMaterial ");
	fprintf(fd, "%d {\n", m_Textures.Count);
	CTexture *pTexture = (CTexture*)m_Textures.Top();
	int texNo = 0;
	while (pTexture != NULL) {
		if (pTexture == NULL) continue;
		char	texName[256];
		strcpy(texName, pTexture->GetTexName()); Trim(texName);
		fprintf(fd, "    \"%s\" shader(3) dbls(1) col(1.000 1.000 1.000 1.000)", texName);
		fprintf(fd, " dif(1.000) amb(0.250) emi(0.250) spc(0.000) power(5.00) tex(\"%s.bmp\")\n", texName);
		sprintf(texpath, "%s%s.bmp", FPath, texName);
		D3DXSaveTextureToFile(texpath, D3DXIFF_BMP, pTexture->GetTexture(), NULL);
		pTexture = (CTexture*)pTexture->Next;
		texNo++;
	}
	fprintf(fd, "}\n");
	for (int num = 0; num<m_nObj; num++) {
		if ((m_pObjInfo[num].mObj.fe & 0xfff0ffff) == 0) {
			DispArea = g_mDispArea;
		}
		else {
			DispArea = g_mDispTree;
		}
		if ((pAreaMesh = m_pObjInfo[num].pAreaMesh) == NULL) continue;
		AreaMatrix = m_pObjInfo[num].mMat;
		D3DXMatrixMultiply(&AreaMatrix, &AreaMatrix, &RootMatrix);
		BL = pAreaMesh->GetBoxLow();
		BH = pAreaMesh->GetBoxHigh();
		BL2.x = BL.x; BL2.y = BL.y; BL2.z = BH.z;
		BL3.x = BH.x; BL3.y = BL.y; BL3.z = BH.z;
		BL4.x = BH.x; BL4.y = BL.y; BL4.z = BL.z;
		BH2.x = BH.x; BH2.y = BH.y; BH2.z = BL.z;
		BH3.x = BL.x; BH3.y = BH.y; BH3.z = BL.z;
		BH4.x = BL.x; BH4.y = BH.y; BH4.z = BH.z;
		D3DXVec3TransformCoord(&BH, &BH, &AreaMatrix);
		D3DXVec3TransformCoord(&BL, &BL, &AreaMatrix);
		D3DXVec3TransformCoord(&BL2, &BL2, &AreaMatrix);
		D3DXVec3TransformCoord(&BH2, &BH2, &AreaMatrix);
		if (Min4(BL.x, BH.x, BL2.x, BH2.x)>posX+DispArea) continue;
		if (Max4(BL.x, BH.x, BL2.x, BH2.x)<posX-DispArea) continue;
		if (Min4(BL.z, BH.z, BL2.z, BH2.z)>posZ+DispArea) continue;
		if (Max4(BL.z, BH.z, BL2.z, BH2.z)<posZ-DispArea) continue;
		ptr = pAreaMesh->GetAreaName();
		*(ptr + 17) = 0x0;
		for (int i = 0; i < 18; i++ ) {
			if (*ptr++ == 0x20) *ptr = 0x0;
		}
		// バーテックスバッファをデバイスに設定
		pAreaMesh->m_lpVB->Lock(0, pAreaMesh->m_VBSize, (void **)&pV, D3DLOCK_READONLY);
		// インデックスバッファをデバイスに設定
		pAreaMesh->m_lpIB->Lock(0, pAreaMesh->m_IBSize, (void **)&pIndex, D3DLOCK_READONLY);
		nVer = nFace = 0;
		// 
		list<CStream>::iterator its2 = pAreaMesh->m_LStreams.begin();
		list<CStream>::iterator ite2 = pAreaMesh->m_LStreams.end();
		for (int count = 0; its2 != ite2; its2++, count++) {
			fprintf(fd, "Object \"%s%02d\" {\n", pAreaMesh->GetAreaName(), count);
			fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 1.000 1.000 1.000\n   color_type 0\n");
			pI = pIndex + its2->GetIndexStart();
			idxmin = 65535; idxmax = 0;
			for (unsigned int i = 0; i < its2->GetFaceCount() + 2; i++) {
				i1 = *pI++;
				if (i1 > idxmax) idxmax = i1;
				if (i1 < idxmin) idxmin = i1;
			}
			fprintf(fd, "vertex %d {\n", idxmax-idxmin+1);
			for (unsigned long verCnt = idxmin; verCnt <= idxmax; verCnt++) {
				mVertex = *(pV + verCnt); mVer = mVertex.v;
				D3DXVec3TransformCoord(&mVer, &mVer, &AreaMatrix);
				fprintf(fd, "        %5.5f %5.5f %5.5f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
			}
			fprintf(fd, "\t}\n");
			if (its2->GetPrimitiveType() == D3DPT_TRIANGLESTRIP) {
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					i1 = *pI++ - idxmin; nVer++; i2 = *pI++ - idxmin; nVer++;
					while (nVer<its2->GetFaceCount() + 2) {
						i3 = *pI++ - idxmin; nVer++;
						if (i2 == i3) {
							pI++; nVer++;
							break;
						}
						nFace++;
						i1 = i2; i2 = i3;
					}
				}
				fprintf(fd, "face %d {\n", nFace);
				//fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					cnt = 0;
					i1 = (*pI++) - idxmin; nVer++;
					i2 = (*pI++) - idxmin; nVer++;
					while( nVer<its2->GetFaceCount()+2 )  {
						i3 = (*pI++) - idxmin; nVer++;
						if (i2 == i3) {
							pI++; nVer++; break;
						}
						if ( nVer % 2) {
							t1 = i3; t2 = i2; t3 = i1;
						}
						else {
							t1 = i1; t2 = i2; t3 = i3;
						}
						fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
							t1 , t2 , t3, its2->m_TexNo,
							(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
							(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
						cnt++; nFace++;
						i1 = i2; i2 = i3;
					}
				}
				//i1 = (*pI++) - idxmin; i2 = (*pI++) - idxmin;
				//for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
				//	i3 = (*pI++) - idxmin;
				//	if ((idxmin+i) % 2) {
				//		t1 = i1; t2 = i2; t3 = i3;
				//	}
				//	else {
				//		t1 = i3; t2 = i2; t3 = i1;
				//	}
				//	fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
				//		t1 , t2 , t3, count,
				//		(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
				//		(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				//	i1 = i2; i2 = i3;
				//}
			}
			else if (its2->GetPrimitiveType() == D3DPT_TRIANGLELIST) {
				fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
					i1 = (*pI++)-idxmin; i2 = (*pI++)-idxmin; i3 = (*pI++)-idxmin;
					t1 = i3; t2 = i2; t3 = i1;
					fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
						t1, t2, t3, its2->m_TexNo,
						(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
						(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				}
			}
			fprintf(fd, "\t}\n");
			fprintf(fd, "}\n");
		}
		//break;
		pAreaMesh->m_lpIB->Unlock();
		pAreaMesh->m_lpVB->Unlock();
	}
	fprintf(fd, "EOF");
	fclose(fd);
	return true;
}

//======================================================================
//		MQOセーブ エフェクトデータすべてをMQOフォーマットで出力します
//======================================================================
bool CArea::saveMQO2(char *FPath, char *FName, float posX, float posY, float posZ)
{
	FILE			*fd;
	char			*ptr, path[256], texpath[256];
	D3DTEXVERTEX	*pV;
	D3DTEXVERTEX	mVertex;
	D3DXVECTOR3     mVer;
	WORD			*pIndex, *pI;
	int				cnt, nVer, nFace, idxmin, idxmax;
	int				i1, i2, i3, t1, t2, t3;
	CEffect         *pEffect;
	CAreaMesh		*pAreaMesh;
	D3DXMATRIX		RootMatrix,AreaMatrix;
	D3DXVECTOR3		BL, BL2, BL3, BL4, BH, BH2, BH3, BH4;

	D3DXMatrixRotationZ(&RootMatrix, PAI);
	if ((ptr = strrstr(FPath, FName))) *ptr = '\0';
	if ((ptr = strrstr(FName, ".mqo"))) *ptr = '\0';
	sprintf(path, "%s%s.mqo", FPath, FName);
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	if ((fd = fopen(path, "w")) == NULL) return false;
	fprintf(fd, "Metasequoia Document\nFormat Text Ver 1.0\n\nScene {\n");
	fprintf(fd, "	pos 0.0000 0.0000 5000.0000\n");
	fprintf(fd, "	lookat 0.0000 0.0000 0.0000\n");
	fprintf(fd, "	head 0.0000\n");
	fprintf(fd, "	pich 0.0000\n");
	fprintf(fd, "	ortho 1\n");
	fprintf(fd, "	zoom2 2.0000\n");
	fprintf(fd, "	amb 0.250 0.250 0.250\n}\nMaterial ");
	fprintf(fd, "%d {\n", m_Textures.Count);
	CTexture *pTexture = (CTexture*)m_Textures.Top();
	int texNo = 0;
	while (pTexture != NULL) {
		if (pTexture == NULL) continue;
		char	texName[256];
		strcpy(texName, pTexture->GetTexName()); Trim(texName);
		fprintf(fd, "    \"%s\" shader(3) dbls(1) col(1.000 1.000 1.000 1.000)", texName);
		fprintf(fd, " dif(1.000) amb(0.250) emi(0.250) spc(0.000) power(5.00) tex(\"%s.bmp\")\n", texName);
		sprintf(texpath, "%s%s.bmp", FPath, texName);
		D3DXSaveTextureToFile(texpath, D3DXIFF_BMP, pTexture->GetTexture(), NULL);
		pTexture = (CTexture*)pTexture->Next;
		texNo++;
	}
	fprintf(fd, "}\n");
//	pAreaMesh = (CAreaMesh*)m_EffMeshs.Top();
//	while (pAreaMesh) {
	pEffect = (CEffect*)m_Effects.Top();
	while (pEffect) {
		if (memcmp(pEffect->m_class, g_className, 4)) {
			pEffect = (CEffect*)pEffect->Next;
			continue;
		}
		if ( (pAreaMesh = pEffect->m_pAreaMesh) == NULL) {
			pEffect = (CEffect*)pEffect->Next;
			continue;
		} else {
			//AreaMatrix = RootMatrix;
			//AreaMatrix *= pEffect->m_mRootTransform;
			AreaMatrix = pEffect->m_mRootTransform;
			AreaMatrix *= RootMatrix;
		}
		ptr = pAreaMesh->GetAreaName();
		*(ptr + 17) = 0x0;
		for (int i = 0; i < 18; i++) {
			if (*ptr++ == 0x20) *ptr = 0x0;
		}
		// バーテックスバッファをデバイスに設定
		pAreaMesh->m_lpVB->Lock(0, pAreaMesh->m_VBSize, (void **)&pV, D3DLOCK_READONLY);
		// インデックスバッファをデバイスに設定
		pAreaMesh->m_lpIB->Lock(0, pAreaMesh->m_IBSize, (void **)&pIndex, D3DLOCK_READONLY);
		nVer = nFace = 0;
		// 
		list<CStream>::iterator its2 = pAreaMesh->m_LStreams.begin();
		list<CStream>::iterator ite2 = pAreaMesh->m_LStreams.end();
		for (int count = 0; its2 != ite2; its2++, count++) {
			fprintf(fd, "Object \"%s%02d\" {\n", pAreaMesh->GetAreaName(), count);
			fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 1.000 1.000 1.000\n   color_type 0\n");
//			fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 0.898 0.498 0.698\n   color_type 0\n");
			pI = pIndex + its2->GetIndexStart();
			idxmin = 65535; idxmax = 0;
			for (unsigned int i = 0; i < its2->GetFaceCount() + 2; i++) {
				i1 = *pI++;
				if (i1 > idxmax) idxmax = i1;
				if (i1 < idxmin) idxmin = i1;
			}
			fprintf(fd, "vertex %d {\n", idxmax - idxmin + 1);
			for (unsigned long verCnt = idxmin; verCnt <= idxmax; verCnt++) {
				mVertex = *(pV + verCnt); mVer = mVertex.v;
				D3DXVec3TransformCoord(&mVer, &mVer, &AreaMatrix);
				fprintf(fd, "        %4.4f %4.4f %4.4f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
			}
			fprintf(fd, "\t}\n");
			if (its2->GetPrimitiveType() == D3DPT_TRIANGLESTRIP) {
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					i1 = (*pI++) - idxmin; nVer++; i2 = (*pI++) - idxmin; nVer++;
					while (nVer<its2->GetFaceCount() + 2) {
						i3 = (*pI++) - idxmin; nVer++;
						if (i2 == i3) {
							pI++; nVer++;
							break;
						}
						nFace++;
						i1 = i2; i2 = i3;
					}
				}
				fprintf(fd, "face %d {\n", nFace);
				//fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				nFace = 0;
				for (nVer = 0; nVer < its2->GetFaceCount() + 2;) {
					cnt = 0;
					i1 = (*pI++) - idxmin; nVer++;
					i2 = (*pI++) - idxmin; nVer++;
					while (nVer<its2->GetFaceCount() + 2)  {
						i3 = (*pI++) - idxmin; nVer++;
						if (i2 == i3) {
							pI++; nVer++; break;
						}
						if (nVer % 2) {
							t1 = i3; t2 = i2; t3 = i1;
						}
						else {
							t1 = i1; t2 = i2; t3 = i3;
						}
						fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
							t1, t2, t3, its2->m_TexNo,
							(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
							(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
						cnt++; nFace++;
						i1 = i2; i2 = i3;
					}
				}
				//i1 = (*pI++) - idxmin; i2 = (*pI++) - idxmin;
				//for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
				//	i3 = (*pI++) - idxmin;
				//	if ((idxmin+i) % 2) {
				//		t1 = i1; t2 = i2; t3 = i3;
				//	}
				//	else {
				//		t1 = i3; t2 = i2; t3 = i1;
				//	}
				//	fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
				//		t1 , t2 , t3, count,
				//		(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
				//		(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				//	i1 = i2; i2 = i3;
				//}
			}
			else if (its2->GetPrimitiveType() == D3DPT_TRIANGLELIST) {
				fprintf(fd, "face %d {\n", its2->GetFaceCount());
				pI = pIndex + its2->GetIndexStart();
				for (unsigned int i = 0; i<its2->GetFaceCount(); i++) {
					i1 = (*pI++) - idxmin; i2 = (*pI++) - idxmin; i3 = (*pI++) - idxmin;
					t1 = i3; t2 = i2; t3 = i1;
//					t1 = i1; t2 = i2; t3 = i3;
					fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
						t1, t2, t3, its2->m_TexNo,
						(pV + idxmin + t1)->tu, (pV + idxmin + t1)->tv, (pV + idxmin + t2)->tu,
						(pV + idxmin + t2)->tv, (pV + idxmin + t3)->tu, (pV + idxmin + t3)->tv);
				}
			}
			fprintf(fd, "\t}\n");
			fprintf(fd, "}\n");
		}
		//break;
		pAreaMesh->m_lpIB->Unlock();
		pAreaMesh->m_lpVB->Unlock();
//		pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
		pEffect = (CEffect*)pEffect->Next;
	}
	fprintf(fd, "EOF");
	fclose(fd);
	return true;
}
//======================================================================
//		MQOセーブ データをMQOフォーマットで出力します
//======================================================================
bool CArea::saveMQO3(char *FPath, char *FName){
	FILE			*fd;
	char			*ptr, path[256], texpath[256];
	EFFECTVERTEX	*pVertex, *pV;
	D3DXVECTOR3     mVer;
	CEffect         *pEffect;
	CEffectModel	*pEffMdl;
	WORD			*pIndex, *pI;
	int				i1, i2, i3, t1, t2, t3;
	D3DXMATRIX		RootMatrix, EffectMatrix;

	D3DXMatrixRotationZ(&RootMatrix, PAI);
	if ((ptr = strrstr(FPath, FName))) *ptr = '\0';
	if ((ptr = strrstr(FName, ".mqo"))) *ptr = '\0';
	sprintf(path, "%s%s.mqo", FPath, FName);
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	if ((fd = fopen(path, "w")) == NULL) return false;
	fprintf(fd, "Metasequoia Document\nFormat Text Ver 1.0\n\nScene {\n");
	fprintf(fd, "	pos 0.0000 0.0000 5000.0000\n");
	fprintf(fd, "	lookat 0.0000 0.0000 0.0000\n");
	fprintf(fd, "	head 0.0000\n");
	fprintf(fd, "	pich 0.0000\n");
	fprintf(fd, "	ortho 1\n");
	fprintf(fd, "	zoom2 2.0000\n");
	fprintf(fd, "	amb 0.250 0.250 0.250\n}\nMaterial ");
	fprintf(fd, "%d {\n", m_Textures.Count);
	CTexture *pTexture = (CTexture*)m_Textures.Top();
	char	texName[256];
	while (pTexture != NULL) {
		if (pTexture == NULL) continue;
		strcpy(texName, pTexture->m_TexName); Trim(texName);
		fprintf(fd, "    \"%s\" col(1.000 1.000 1.000 1.000)", texName);
		fprintf(fd, " dif(1.000) amb(0.250) emi(0.250) spc(0.000) power(5.00) tex(\"%s.bmp\")\n", texName);
		sprintf(texpath, "%s%s.bmp", FPath, texName);
		D3DXSaveTextureToFile(texpath, D3DXIFF_BMP, pTexture->GetTexture(), NULL);
		pTexture = (CTexture*)pTexture->Next;
	}
	fprintf(fd, "}\n");
	pEffect = (CEffect*)m_Effects.Top();
	while (pEffect) {
		if (memcmp(pEffect->m_class, g_className, 4)) {
			pEffect = (CEffect*)pEffect->Next;
			continue;
		}
		if ((pEffMdl = pEffect->m_pEffectModel) == NULL) {
			pEffect = (CEffect*)pEffect->Next;
			continue;
		} else {
			//AreaMatrix = RootMatrix;
			//AreaMatrix *= pEffect->m_mRootTransform;
			EffectMatrix = pEffect->m_mRootTransform;
			EffectMatrix *= RootMatrix;
		}
		char	objName[256];
		strcpy(objName, pEffMdl->m_Name); Trim(objName);
		fprintf(fd, "Object \"%s\" {\n", objName);
		fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 0.898 0.498 0.698\n   color_type 0\n");
		// バーテックスバッファをデバイスに設定
		pEffMdl->m_lpVB->Lock(0, pEffMdl->m_VBSize, (void **)&pVertex, D3DLOCK_READONLY);
		// インデックスバッファをデバイスに設定
		pEffMdl->m_lpIB->Lock(0, pEffMdl->m_IBSize, (void **)&pIndex, D3DLOCK_READONLY);
		switch (pEffMdl->m_ModelType) {
			case 0x21:
				// 頂点出力
				fprintf(fd, "vertex %d {\n", pEffMdl->m_NumVertices);
				pV = pVertex;
				for (int i = 0; i <= pEffMdl->m_NumVertices; i++, pV++) {
					mVer.x = pV->x; mVer.y = pV->y; mVer.z = pV->z;
					D3DXVec3TransformCoord(&mVer, &mVer, &EffectMatrix);
					fprintf(fd, "        %5.5f %5.5f %5.5f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
				}
				fprintf(fd, "\t}\n");
				// 面出力
				fprintf(fd, "face %d {\n", pEffMdl->m_NumFaces);
				pI = pIndex;
				for (int i = 0; i<pEffMdl->m_NumFaces; i++) {
					i1 = *pI++; i2 = *pI++; i3 = *pI++;
					t1 = i3; t2 = i2; t3 = i1;
					fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
						t1, t2, t3, pEffMdl->m_texNo,
						(pVertex + t1)->u, (pVertex + t1)->v, (pVertex + t2)->u,
						(pVertex + t2)->v, (pVertex + t3)->u, (pVertex + t3)->v);
				}
				fprintf(fd, "\t}\n");
				break;
			case 0x1f:
				// 頂点出力
				fprintf(fd, "vertex %d {\n", pEffMdl->m_NumVertices);
				pV = pVertex;
				for (int i = 0; i <= pEffMdl->m_NumVertices; i++, pV++) {
					mVer.x = pV->x; mVer.y = pV->y; mVer.z = pV->z;
					D3DXVec3TransformCoord(&mVer, &mVer, &EffectMatrix);
					fprintf(fd, "        %5.5f %5.5f %5.5f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
				}
				fprintf(fd, "\t}\n");
				// 面出力
				fprintf(fd, "face %d {\n", pEffMdl->m_NumFaces);
				pI = pIndex;
				for (int i = 0; i<pEffMdl->m_NumFaces; i++) {
					i1 = *pI++; i2 = *pI++; i3 = *pI++;
					t1 = i3; t2 = i2; t3 = i1;
					fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
						t1, t2, t3, pEffMdl->m_texNo,
						(pVertex + t1)->u, (pVertex + t1)->v, (pVertex + t2)->u,
						(pVertex + t2)->v, (pVertex + t3)->u, (pVertex + t3)->v);
				}
				fprintf(fd, "\t}\n");
				break;
		}
		fprintf(fd, "}\n");
		pEffMdl->m_lpIB->Unlock();
		pEffMdl->m_lpVB->Unlock();
		pEffect = (CEffect*)pEffect->Next;
	}
	fprintf(fd, "EOF");
	fclose(fd);
	return true;
}

//======================================================================
//
//		テクスチャカウント
//
//		ストリーム内のテクスチャデータをカウントする
//======================================================================
int CAreaMesh::countTextures(void)
{
	int numTex = 0;

	list<CStream>::iterator its = this->m_LStreams.begin();
	list<CStream>::iterator ite = this->m_LStreams.end();
	for (; its != ite; its++) {
		CTexture *pTexture = (CTexture*)its->GetpTexture();
		if (pTexture)
			numTex++;
	}
	return numTex;
}



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
