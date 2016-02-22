
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <string>
#include <list>
#include <windows.h>
#define IDC_STATIC -1

using namespace std;
//======================================================================
// GLOBAL
//======================================================================
static BYTE key_table[0x100] =
{
	0xE2, 0xE5, 0x06, 0xA9, 0xED, 0x26, 0xF4, 0x42, 0x15, 0xF4, 0x81, 0x7F, 0xDE, 0x9A, 0xDE, 0xD0,
	0x1A, 0x98, 0x20, 0x91, 0x39, 0x49, 0x48, 0xA4, 0x0A, 0x9F, 0x40, 0x69, 0xEC, 0xBD, 0x81, 0x81,
	0x8D, 0xAD, 0x10, 0xB8, 0xC1, 0x88, 0x15, 0x05, 0x11, 0xB1, 0xAA, 0xF0, 0x0F, 0x1E, 0x34, 0xE6,
	0x81, 0xAA, 0xCD, 0xAC, 0x02, 0x84, 0x33, 0x0A, 0x19, 0x38, 0x9E, 0xE6, 0x73, 0x4A, 0x11, 0x5D,
	0xBF, 0x85, 0x77, 0x08, 0xCD, 0xD9, 0x96, 0x0D, 0x79, 0x78, 0xCC, 0x35, 0x06, 0x8E, 0xF9, 0xFE,
	0x66, 0xB9, 0x21, 0x03, 0x20, 0x29, 0x1E, 0x27, 0xCA, 0x86, 0x82, 0xE6, 0x45, 0x07, 0xDD, 0xA9,
	0xB6, 0xD5, 0xA2, 0x03, 0xEC, 0xAD, 0x62, 0x45, 0x2D, 0xCE, 0x79, 0xBD, 0x8F, 0x2D, 0x10, 0x18,
	0xE6, 0x0A, 0x6F, 0xAA, 0x6F, 0x46, 0x84, 0x32, 0x9F, 0x29, 0x2C, 0xC2, 0xF0, 0xEB, 0x18, 0x6F,
	0xF2, 0x3A, 0xDC, 0xEA, 0x7B, 0x0C, 0x81, 0x2D, 0xCC, 0xEB, 0xA1, 0x51, 0x77, 0x2C, 0xFB, 0x49,
	0xE8, 0x90, 0xF7, 0x90, 0xCE, 0x5C, 0x01, 0xF3, 0x5C, 0xF4, 0x41, 0xAB, 0x04, 0xE7, 0x16, 0xCC,
	0x3A, 0x05, 0x54, 0x55, 0xDC, 0xED, 0xA4, 0xD6, 0xBF, 0x3F, 0x9E, 0x08, 0x93, 0xB5, 0x63, 0x38,
	0x90, 0xF7, 0x5A, 0xF0, 0xA2, 0x5F, 0x56, 0xC8, 0x08, 0x70, 0xCB, 0x24, 0x16, 0xDD, 0xD2, 0x74,
	0x95, 0x3A, 0x1A, 0x2A, 0x74, 0xC4, 0x9D, 0xEB, 0xAF, 0x69, 0xAA, 0x51, 0x39, 0x65, 0x94, 0xA2,
	0x4B, 0x1F, 0x1A, 0x60, 0x52, 0x39, 0xE8, 0x23, 0xEE, 0x58, 0x39, 0x06, 0x3D, 0x22, 0x6A, 0x2D,
	0xD2, 0x91, 0x25, 0xA5, 0x2E, 0x71, 0x62, 0xA5, 0x0B, 0xC1, 0xE5, 0x6E, 0x43, 0x49, 0x7C, 0x58,
	0x46, 0x19, 0x9F, 0x45, 0x49, 0xC6, 0x40, 0x09, 0xA2, 0x99, 0x5B, 0x7B, 0x98, 0x7F, 0xA0, 0xD0,
};

static BYTE key_table2[0x100] =
{
	0xB8, 0xC5, 0xF7, 0x84, 0xE4, 0x5A, 0x23, 0x7B, 0xC8, 0x90, 0x1D, 0xF6, 0x5D, 0x09, 0x51, 0xC1,
	0x07, 0x24, 0xEF, 0x5B, 0x1D, 0x73, 0x90, 0x08, 0xA5, 0x70, 0x1C, 0x22, 0x5F, 0x6B, 0xEB, 0xB0,
	0x06, 0xC7, 0x2A, 0x3A, 0xD2, 0x66, 0x81, 0xDB, 0x41, 0x62, 0xF2, 0x97, 0x17, 0xFE, 0x05, 0xEF,
	0xA3, 0xDC, 0x22, 0xB3, 0x45, 0x70, 0x3E, 0x18, 0x2D, 0xB4, 0xBA, 0x0A, 0x65, 0x1D, 0x87, 0xC3,
	0x12, 0xCE, 0x8F, 0x9D, 0xF7, 0x0D, 0x50, 0x24, 0x3A, 0xF3, 0xCA, 0x70, 0x6B, 0x67, 0x9C, 0xB2,
	0xC2, 0x4D, 0x6A, 0x0C, 0xA8, 0xFA, 0x81, 0xA6, 0x79, 0xEB, 0xBE, 0xFE, 0x89, 0xB7, 0xAC, 0x7F,
	0x65, 0x43, 0xEC, 0x56, 0x5B, 0x35, 0xDA, 0x81, 0x3C, 0xAB, 0x6D, 0x28, 0x60, 0x2C, 0x5F, 0x31,
	0xEB, 0xDF, 0x8E, 0x0F, 0x4F, 0xFA, 0xA3, 0xDA, 0x12, 0x7E, 0xF1, 0xA5, 0xD2, 0x22, 0xA0, 0x0C,
	0x86, 0x8C, 0x0A, 0x0C, 0x06, 0xC7, 0x65, 0x18, 0xCE, 0xF2, 0xA3, 0x68, 0xFE, 0x35, 0x96, 0x95,
	0xA6, 0xFA, 0x58, 0x63, 0x41, 0x59, 0xEA, 0xDD, 0x7F, 0xD3, 0x1B, 0xA8, 0x48, 0x44, 0xAB, 0x91,
	0xFD, 0x13, 0xB1, 0x68, 0x01, 0xAC, 0x3A, 0x11, 0x78, 0x30, 0x33, 0xD8, 0x4E, 0x6A, 0x89, 0x05,
	0x7B, 0x06, 0x8E, 0xB0, 0x86, 0xFD, 0x9F, 0xD7, 0x48, 0x54, 0x04, 0xAE, 0xF3, 0x06, 0x17, 0x36,
	0x53, 0x3F, 0xA8, 0x11, 0x53, 0xCA, 0xA1, 0x95, 0xC2, 0xCD, 0xE6, 0x1F, 0x57, 0xB4, 0x7F, 0xAA,
	0xF3, 0x6B, 0xF9, 0xA0, 0x27, 0xD0, 0x09, 0xEF, 0xF6, 0x68, 0x73, 0x60, 0xDC, 0x50, 0x2A, 0x25,
	0x0F, 0x77, 0xB9, 0xB0, 0x04, 0x0B, 0xE1, 0xCC, 0x35, 0x31, 0x84, 0xE6, 0x22, 0xF9, 0xC2, 0xAB,
	0x95, 0x91, 0x61, 0xD9, 0x2B, 0xB9, 0x72, 0x4E, 0x10, 0x76, 0x31, 0x66, 0x0A, 0x0B, 0x2E, 0x83,
};

//======================================================================
// TYPE DEFINE
//======================================================================
class CTexture;
typedef struct _D3DTEXVERTEX
{
	D3DXVECTOR3	v, n;	//座標,法線ベクトル
	DWORD color;     //色
	float tu, tv;     // UV座標
} D3DTEXVERTEX;


//======================================================================
// リスト用基底クラス
//======================================================================
typedef class CListBase
{
	friend class CList;

protected:
	CList			*pParentList;
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
// マテリアルクラス
//======================================================================
typedef class CTexture : public CListBase
{
protected:
	IDirect3DTexture9	*m_pTexture;
	char				m_TexName[18];

public:
	CTexture();
	virtual ~CTexture();
	virtual void SetTexName( char *pTexName ) { strcpy(m_TexName,pTexName); }
	virtual char* GetTexName(void) { return m_TexName; }
	virtual void SetTexture( IDirect3DTexture9 *pTex );
	virtual IDirect3DTexture9 *GetTexture( void );
}
CTexture, LPCTexture;

//======================================================================
// エリアメッシュクラス
//======================================================================
typedef class CAreaMesh : public CListBase
{
	friend class CArea;

protected:
	list<CStream>			m_LStreams;
	D3DXVECTOR3				m_BoxLow, m_BoxHigh;
	unsigned long			m_NumVertices, m_NumFaces, m_NumIndex, m_VBSize, m_IBSize, m_FVF;
	LPDIRECT3DVERTEXBUFFER9 m_lpVB;
	LPDIRECT3DINDEXBUFFER9	m_lpIB;
	char					m_AreaName[18], m_AreaType[4];

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
	virtual		void		SetAreaName(char *pAreaName) { strcpy(m_AreaName, pAreaName); }
	virtual		char*		GetAreaName(void) { return m_AreaName; }
	virtual		HRESULT		LoadAreaMesh(char *pFile, CArea *pArea, unsigned long FVF);
	virtual     int			countTextures(void);
}
CAreaMesh, *LPCAreaMesh;

//=============================================================
// KeyFrame
// 線形補間によるキーフレームアニメーションクラス
//=============================================================
typedef class CKeyFrame : public CListBase
{
private:
	int		m_numKey;		// キーの数
	float*	m_keys;			// キーを格納する配列へのポインタ
	float*	m_values;		// キーに対応する値を格納する配列へのポインタ
	BOOL	m_isLoop;		// アニメーションをループするかどうかのフラグ
	DWORD	m_startTime;	// アニメーションの開始時刻
	DWORD	m_duration;		// アニメーションの長さ
public:
	char			m_type[4];
	CKeyFrame();	// デフォルトコンストラクタ
	~CKeyFrame();	// デストラクタ
public:

	void GetKeyFrame(char *pBuf);	// キーフレームの取得
	void CreateKey(int numKey);	// キーの生成
	void SetKeyValue(int index, float key, float value);	// キーとキーに対応する値の設定
	// キーは小さい方から順番に設定すること
	// 引数
	//		index : インデックス, 0 <= index < createKeyで生成したキーの数
	//		key   : キー, 0.0f <= key <= 1.0f
	//		value : キーに対応する値
	void SetDuration(DWORD duration);	// アニメーションの長さ(キーが0から1まで変化するのにかかる時間)の設定
	//		duration : 時間(ミリ秒)
	void SetLoopFlag(DWORD isLoop);	// アニメーションをループするかどうかの設定
	//		isLoop : TRUEならループする, FALSEならループしない
	//---------------------------------------------------------
	void SetStartTime(DWORD start);	// アニメーション開始時刻の設定
	//		start : アニメーションの開始時刻(ミリ秒)
	BOOL GetValue(DWORD time, float* pValue, BOOL* pIsEnd);	// 時刻に対応する値を取得する
	//		time   : 時刻(ミリ秒)
	//		pValue : 結果を受け取る変数へのポインタ
	//		pIsEnd : アニメーション終了判定フラグを受け取る変数へのポインタ, 終了していたらTRUEそうれなければFALSE
	// 戻り値
	//		値の取得に成功したらTRUE, 失敗したらFALSE
protected:
	float GetFraction(DWORD time);	// 時刻を割合に変換する
	//		time : 時刻(ミリ秒)
	// 戻り値
	//		割合
	//---------------------------------------------------------
	int GetBeginIndex(float fraction);	// 割合に対応するインデックスを取得する
	//		fraction : 割合
	// 戻り値
	//		配列のインデックス, fractionが登録されているキーより小さければ-1
}
CKeyFrame, *LPCKeyFrame;

//=============================================================
// Effect
//=============================================================
typedef class CEffect : public CListBase
{
public:
	int				m_bPos;			// 所属　01:腰,11:腰,21:頭上手,31:首,41:前方,51,61:頭上,71:胸,81:右足,91:左足,a1:右手,b1:左手
	// ボーン番号?
	D3DXMATRIX		m_mRootTransform;
	int				m_ModelType;	// 1f エフェクトモデル　21 画像関係　3D 音
	int				m_no,			// 一度に発生させる量
		m_1fdiv;		// 0x1Fデーターでdiv+1/360で発生
	int				m_interval;		// 発生間隔
	int				m_subID;		// ターゲット処理方法
	char			m_name[4],		// エフェクトID
		m_target[4];	//
	CAreaMesh		*m_pAreaMesh;	// エリアメッシュポインタ
	DWORD			m_lifeTime;		// 存続時間
	int				m_dir1;			// 01 ビルボード　40 XXX
	int				m_dir2;			// 40 ビルボードに近い
	D3DXVECTOR3		m_uv,			// texture uvの移動量
		m_r1F,			// 0x1Fデータの r,addr;
		m_h1F,			// 0x1Fデータの h,addh;
		m_s1F,			// 0x1Fデータの sx,sy,sz;
		m_r06,			// 0x06データの r,addr;
		m_p01,			// 0x01データの px,py,pz;
		m_p02,			// 0x02データの px,py,pz;
		m_p03,			// 0x03データの px,py,pz;
		m_r09,			// 0x09データの rx,ry,rz;
		m_r0A,			// 0x0Aデータの rx,ry,rz;
		m_r0B,			// 0x0Bデータの rx,ry,rz;
		m_r0C,			// 0x0Cデータの rx,ry,rz;
		m_s0F,			// 0x0Fデータの sx,sy,sz;
		m_s10,			// 0x10データの sx,sy,sz;
		m_s11,			// 0x11データの sx,sy,sz;
		m_s12,			// 0x12データの sx,sy,sz;
		m_s13;			// 0x13データの sx,sy,sz;
	D3DCOLORVALUE	m_color;		// 描画用カラー変数
	DWORD			m_kind1,		// 不明種別１
		m_kind2;		// 不明種別２
	float			m_08dist;		// 0x08の中心からの距離
	CList			m_EffectObj;	// エフェクト・オブジェクトのリスト
	CKeyFrame		*m_Rd,				// 赤のキーフレーム
		*m_Gr,				// 緑のキーフレーム
		*m_Bl,				// 青のキーフレーム
		*m_Al,				// アルファのキーフレーム
		*m_kfpx,			// 移動Xのキーフレーム
		*m_kfpy,			// 移動Yのキーフレーム
		*m_kfpz,			// 移動Zのキーフレーム
		*m_kfrx,			// 回転Xのキーフレーム
		*m_kfry,			// 回転Yのキーフレーム
		*m_kfrz,			// 回転Zのキーフレーム
		*m_kfsx,			// スケールXのキーフレーム
		*m_kfsy,			// スケールYのキーフレーム
		*m_kfsz,			// スケールZのキーフレーム
		*m_kfu,				// テクスチャuのキーフレーム
		*m_kfv;				// テクスチャvのキーフレーム
	CEffect();
	~CEffect();
	virtual	void	GetEffectMatrix(char *pBuf, CKeyFrame *pKeyFrame);
	virtual void	InitData(void);
	virtual void	Set1F(int no);
}
CEffect, *LPCEffect;


//======================================================================
// ベースデータクラス
//======================================================================

typedef struct
{
  char id[16];
  float fTransX,fTransY,fTransZ;
  float fRotX,fRotY,fRotZ;
  float fScaleX,fScaleY,fScaleZ;
  float fa,fb,fc,fd;
  long  fe,ff,fg,fh,fi,fj,fk,fl;
} TEMPOBJINFO;

typedef struct {
	D3DXVECTOR3		mPos[8];	//　不明
	DWORD			mInf[8];	// 不明
} WALLINFO;

typedef struct
{
	TEMPOBJINFO		mObj;
	D3DXMATRIX		mMat;
	CAreaMesh		*pAreaMesh;
	list<CAreaMesh>::iterator itrAreaMesh;
} OBJINFO;

//======================================================================
// エリアクラス
//======================================================================
typedef class CArea 
{
protected:
	IDirect3DVertexDeclaration9	*m_VertexFormat;
	unsigned long				m_VertexSize;
	IDirect3DVertexShader9		*m_hVertexShader;
	D3DXMATRIX					m_mRootTransform;
	CList						m_AreaMeshs,m_EffMeshs;
	CList						m_Effects;				// エフェクトリスト
	CList						m_Schedules;			// スケジュールリスト
	CList						m_KeyFrames;			// キーフレームリスト
	CList						m_EffectModels;			// エフェクトモデルリスト
	int							m_mArea;				// エリアファイル
	int							m_nObj;					// MMB個数　未使用
	OBJINFO						*m_pObjInfo;


public:
	CList						m_Textures;

	CArea();
	virtual ~CArea();
	virtual HRESULT			LoadTextureFromFile( char *filename );
	virtual	HRESULT			LoadEffectFromFile(char *FileName);
	virtual	void			DecodeMMB(BYTE* p);
	virtual	void			DecodeMMBSub(BYTE* p);
	virtual	void			DecodeMZB(BYTE* p);
	virtual HRESULT			LoadAreaFromFile( char *filename, unsigned long FVF );
	virtual	void			InitData(void);
	virtual	unsigned long	Rendering( float PosX, float PosY, float PosZ );
	virtual	bool			CreateVertexShader( void );
	virtual	bool			LoadMAP( void );
	virtual	int				GetArea(void) { return m_mArea; }
	virtual	void			SetArea( int mArea ) { m_mArea	=	mArea; }
	virtual bool	        saveMQO(char *FPath, char *FName,float posX,float posY,float posZ);
	virtual bool	        saveMQO2(char *FPath, char *FName, float posX, float posY, float posZ);
	bool InitKeyFrame(void) {
		CKeyFrame *pKeyFrame = (CKeyFrame*)m_KeyFrames.Top();
		while (pKeyFrame) {
			pKeyFrame->~CKeyFrame();
			pKeyFrame = (CKeyFrame*)pKeyFrame->Next;
		}
		m_KeyFrames.Release();
		return true;
	}
	virtual bool InitEffect(void) {
		CEffect *pEffect = (CEffect*)m_Effects.Top();
		while (pEffect) {
			pEffect->InitData();
			pEffect = (CEffect*)pEffect->Next;
		}
		m_Effects.Release();
		return true;
	}
}
CArea, *LPCArea;

//======================================================================
// MAPクラス
//======================================================================
typedef class CMAP : public CArea
{
protected:
public:
}
CMAP,*LPCMAP;
