#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include "List.h"
#include "AreaMesh.h"

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

//=======================================
// CEffectModel エフェクトメッシュクラス
//=======================================
typedef class CEffectModel : public CListBase
{

protected:

public:
	int						m_texNo;
	int						m_ModelType;
	DWORD					m_ModelTotal;
	DWORD					m_ModelNo;
	IDirect3DVertexDeclaration9	*m_VertexFormat;
	unsigned long			m_VertexSize;
	CTexture				*m_pTexture;
	unsigned long			m_NumVertices, m_NumFaces, m_NumIndex, m_VBSize, m_IBSize, m_FVF;
	LPDIRECT3DVERTEXBUFFER9 m_lpVB;
	LPDIRECT3DINDEXBUFFER9	m_lpIB;
	std::string			m_Name;
	std::string			m_type;
	CEffectModel();
	virtual		~CEffectModel();
	virtual		HRESULT		LoadEffectModel(char *pFile);
	virtual		HRESULT		LoadEffectModel2(char *pFile);
}
CEffectModel, *LPCEffectModel;

//=============================================================
// KeyFrame 線形補間によるキーフレームアニメーションクラス
//=============================================================
typedef class CKeyFrame : public CListBase
{
private:
	std::vector<float> m_keys;		// キーを格納する配列
	std::vector<float> m_values;	// キーに対応する値を格納する配列
	BOOL	m_isLoop;		// アニメーションをループするかどうかのフラグ
	DWORD	m_startTime;	// アニメーションの開始時刻
	DWORD	m_duration;		// アニメーションの長さ
public:
	std::string			m_type;
	CKeyFrame();	// デフォルトコンストラクタ
	~CKeyFrame();	// デストラクタ
public:
	void outputValue(HWND listObj);
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
	bool			param[128];
	int				m_bPos;			// 所属　01:腰,11:腰,21:頭上手,31:首,41:前方,51,61:頭上,71:胸,81:右足,91:左足,a1:右手,b1:左手
	// ボーン番号?
	D3DXMATRIX		m_mRootTransform;
	int				m_ModelType;	// 1f エフェクトモデル　21 画像関係　3D 音
	int				m_no,			// 一度に発生させる量
		m_1fdiv;		// 0x1Fデーターでdiv+1/360で発生
	int				m_interval;		// 発生間隔
	int				m_subID;		// ターゲット処理方法
	std::string			m_class;   // エフェクトタイプ effe,fefs,ligh等
	std::string			m_name;		// エフェクトID
	std::string			m_target;
	CAreaMesh		*m_pAreaMesh;	// エリアメッシュポインタ
	CEffectModel	*m_pEffectModel;	// エフェクトモデル 0x1F 0x21
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
	virtual void	outputProp(HWND listObj);
}
CEffect, *LPCEffect;
