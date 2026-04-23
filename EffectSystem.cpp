#define _CRT_SECURE_NO_WARNINGS
#include "EffectSystem.h"
#include "Area.h" // For CKeyFrame dependencies or others if needed
#include <stdio.h> // for sprintf

extern HRESULT CreateVB( LPDIRECT3DVERTEXBUFFER9 *lpVB, DWORD size, DWORD Usage, DWORD fvf );
extern HRESULT CreateIB( LPDIRECT3DINDEXBUFFER9 *lpIB, DWORD size, DWORD Usage );
#define SAFE_RELEASE(p)		if ( (p) != NULL ) { (p)->Release(); (p) = NULL; }

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
	m_type.assign(pFile, 4);
	DAT1FH		*pcp = (DAT1FH *)(pFile + 16);
	pVertex = (EFFECTVERTEX*)(pFile + 16 + sizeof(DAT1FH));
	m_Name.assign(pcp->name, 16);
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
	m_type.assign(pFile, 4);
	DAT21H		*pcp = (DAT21H *)(pFile + 16);
	//	pVertex		=	(EFFECT2VERTEX*)(pFile+16+sizeof(DAT21H));
	m_Name.assign(pcp->name, 16);
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
	: m_isLoop(TRUE), m_startTime(0), m_duration(0)
{
}

//---------------------------------------------------------
// デストラクタ
//---------------------------------------------------------
CKeyFrame::~CKeyFrame()
{
}

void CKeyFrame::outputValue(HWND listObj) {
	char buf[256];
	// ソースコードがUTF-8で、マニフェストによってANSI APIもUTF-8を受け付ける
	auto addStr = [&](const char* s) {
		SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)s);
	};

	for (size_t i = 0; i < m_keys.size(); i++) {
		sprintf(buf, "[%2d] Key (%5.5f) Value(%5.5f)", (int)i, m_keys[i], m_values[i]);
		addStr(buf);
	}
}

void CKeyFrame::GetKeyFrame(char *pBuf)
{
	char	*ptr;
	int		num = 0;
	float	key = 0.f, val = 0.f;

	m_type.assign(pBuf, 4);
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
	// 新しい配列のサイズを変更
	m_keys.assign(numKey, 0.0f);
	m_values.assign(numKey, 0.0f);
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
	//インデックスが範囲内なら設定
	if (index >= 0 && index < (int)m_keys.size()) {
		m_keys[index] = key;
		m_values[index] = value;
	}
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

	if (m_keys.empty()) {
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
			*pValue = m_values.back();
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
		*pValue = m_values.front();	// 最初のキーフレームの値を返す
	}
	else if (beginIndex >= (int)m_keys.size() - 1) {	// アニメーション終了後
		*pValue = m_values.back();	// 最後のキーフレームの値を返す
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
	if (m_keys.empty() || fraction < m_keys[0])
		return -1;

	// 引数に指定された割合とキーフレームの割合を比較する
	int index = 0;
	for (size_t i = 0; i < m_keys.size(); i++) {
		// キーフレームより大きくなったら発見
		if (m_keys[i] <= fraction) {
			index = (int)i;
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
	m_name.clear(); m_target.clear();
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
	angl = D3DX_PI * 2 / (float)(m_no + 1)*no; // Changed PAI to D3DX_PI or defined it below, Wait, let me replace PAI.

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

	m_name.assign(pBuff, 4);
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
			m_target.assign(pAdr + 12, 4);
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
				if (!memcmp(pAdr + 8, pKFrame->m_type.c_str(), 4)) {
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
	// ソースコードがUTF-8で、マニフェストによってANSI APIもUTF-8を受け付ける
	auto addStr = [&](const char* s) {
		SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)s);
	};

	sprintf(buf, "[00] ID (%s) ターゲット(%s) U(%5.5f) V(%5.5f)", m_name.c_str(), m_target.c_str(),m_uv.x,m_uv.y);
	addStr(buf);

	for (int i = 0; i < 128; i++) {
		if (param[i] == false) continue;
		switch (i) {
			case 0x00:
				break;
			case 0x01: // オフセット
				sprintf(buf,"[%02x] mtype (%02x) kd1(%04x) kd2(%04x) LfT(%04x)", i, m_ModelType, m_kind1, m_kind2, m_lifeTime);
				addStr(buf);
				sprintf(buf, "[%02x] ｵﾌｾｯﾄ (%5.5f,%5.5f,%5.5f)", i, m_p01.x, m_p01.y, m_p01.z);
				addStr(buf);
				break;
			case 0x02:
				sprintf(buf, "[%02x] ｵﾌｾｯﾄ2 (%5.5f,%5.5f,%5.5f)", i, m_p02.x, m_p02.y, m_p02.z);
				addStr(buf);
				break;
			case 0x03:
				sprintf(buf, "[%02x] ｵﾌｾｯﾄ3 (%5.5f,%5.5f,%5.5f)", i, m_p03.x, m_p03.y, m_p03.z);
				addStr(buf);
				break;
			case 0x06:
				sprintf(buf, "[%02x] 2D回転 (%5.5f,%5.5f,%5.5f)", i, m_r06.x, m_r06.y, m_r06.z);
				addStr(buf);
				break;
			case 0x08:
				sprintf(buf, "[%02x] dist (%5.5f)", i, m_08dist);
				addStr(buf);
				break;
			case 0x09://初期位置
				sprintf(buf, "[%02x] 回転 (%5.5f,%5.5f,%5.5f)", i, m_r09.x, m_r09.y, m_r09.z);
				addStr(buf);
				break;
			case 0x0a://回転
				sprintf(buf, "[%02x] 回転2 (%5.5f,%5.5f,%5.5f)", i, m_r0A.x, m_r0A.y, m_r0A.z);
				addStr(buf);
				break;
			case 0x0b:// 回転差分
				sprintf(buf, "[%02x] 回転3 (%5.5f,%5.5f,%5.5f)", i, m_r0B.x, m_r0B.y, m_r0B.z);
				addStr(buf);
				break;
			case 0x0c:// 回転差分
				sprintf(buf, "[%02x] 回転4 (%5.5f,%5.5f,%5.5f)", i, m_r0C.x, m_r0C.y, m_r0C.z);
				addStr(buf);
				break;
			case 0x0f:// 初期スケール
				sprintf(buf, "[%02x] ｽｹｰﾙ (%5.5f,%5.5f,%5.5f)", i, m_s0F.x, m_s0F.y, m_s0F.z);
				addStr(buf);
				break;
			case 0x10:// スケール差分
				sprintf(buf, "[%02x] ｽｹｰﾙ2 (%5.5f,%5.5f,%5.5f)", i, m_s10.x, m_s10.y, m_s10.z);
				addStr(buf);
				break;
			case 0x11://スケールの揺らぎ
				sprintf(buf, "[%02x] ｽｹｰﾙ3 (%5.5f,%5.5f,%5.5f)", i, m_s11.x, m_s11.y, m_s11.z);
				addStr(buf);
				break;
			case 0x12:// スケール差分
				sprintf(buf, "[%02x] ｽｹｰﾙ4 (%5.5f,%5.5f,%5.5f)", i, m_s12.x, m_s12.y, m_s12.z);
				addStr(buf);
				break;
			case 0x13:// スケール差分
				sprintf(buf, "[%02x] ｽｹｰﾙ5 (%5.5f,%5.5f,%5.5f)", i, m_s13.x, m_s13.y, m_s13.z);
				addStr(buf);
				break;
			case 0x16:// 初期カラー
				sprintf(buf, "[%02x] ｶﾗｰ ( %3d ,%3d ,%3d ,%3d )", i, 
					(int)(m_color.r*255.f),(int)(m_color.g*255.f),(int)(m_color.b*255.f), (int)(m_color.a*255.f));
				addStr(buf);
				break;
			case 0x1E:
				break;
			case 0x1F:
				sprintf(buf, "[%02x] r (%5.5f,%5.5f) s(%5.5f,%5.5f,%5.5f) h(%5.5f,%5.5f) div %d", i, m_r1F.x, m_r1F.y,
					m_s1F.x, m_s1F.y, m_s1F.z, m_h1F.x, m_h1F.y, m_1fdiv);
				addStr(buf);
				break;
			case 0x21:
				if (m_kfpx == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) px", i, m_kfpx->m_type.c_str());
				addStr(buf);
				break;
			case 0x22:
				if (m_kfpy == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) py", i, m_kfpy->m_type.c_str());
				addStr(buf);
				break;
			case 0x23:
				if (m_kfpz == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) pz", i, m_kfpz->m_type.c_str());
				addStr(buf);
				break;
			case 0x24:
				if (m_kfrx == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) rx", i, m_kfrx->m_type.c_str());
				addStr(buf);
				break;
			case 0x25:
				if (m_kfry == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) ry", i, m_kfry->m_type.c_str());
				addStr(buf);
				break;
			case 0x26:
				if (m_kfrz == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) rz", i, m_kfrz->m_type.c_str());
				addStr(buf);
				break;
			case 0x27:
				if (m_kfsx == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) sx", i, m_kfsx->m_type.c_str());
				addStr(buf);
				break;
			case 0x28:
				if (m_kfsy == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) sy", i, m_kfsy->m_type.c_str());
				addStr(buf);
				break;
			case 0x29:
				if (m_kfsz == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) sz", i, m_kfsz->m_type.c_str());
				addStr(buf);
				break;
			case 0x2D:
				if (m_Al == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) alph", i, m_Al->m_type.c_str());
				addStr(buf);
				break;
			case 0x2E:
				if (m_kfu == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) u", i, m_kfu->m_type.c_str());
				addStr(buf);
				break;
			case 0x2F:
				if (m_kfv == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) v", i, m_kfv->m_type.c_str());
				addStr(buf);
				break;
			case 0x30:
				break;
			case 0x60:
				if (m_Rd == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) Rd", i, m_Rd->m_type.c_str());
				addStr(buf);
				break;
			case 0x61:
				if (m_Gr == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) Gr", i, m_Gr->m_type.c_str());
				addStr(buf);
				break;
			case 0x62:
				if (m_Bl == NULL) break;
				sprintf(buf, "[%02x] ｷｰﾌﾚｰﾑ (%s) Bl", i, m_Bl->m_type.c_str());
				addStr(buf);
				break;
			default:
				break;
		}
	}
}
