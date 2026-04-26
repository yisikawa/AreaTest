#define _CRT_SECURE_NO_WARNINGS
#include "EffectSystem.h"
#include "Area.h"
#include "Render.h"
#include <stdio.h>

#define SAFE_RELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; }

//======================================================================
// CEffectModel
//======================================================================
CEffectModel::CEffectModel()
    : m_texNo(0), m_ModelType(0), m_ModelTotal(0), m_ModelNo(0),
      m_VertexSize(0), m_pTexture(nullptr), m_lpVB(nullptr), m_lpIB(nullptr)
{
    m_NumIndex = m_NumVertices = m_NumFaces = m_VBSize = m_IBSize = m_FVF = 0;
}

CEffectModel::~CEffectModel()
{
    m_pTexture = nullptr;
    SAFE_RELEASE(m_lpVB);
    SAFE_RELEASE(m_lpIB);
}

//----------------------------------------------------------------------
// 0x1F エフェクトモデル読み込み
//----------------------------------------------------------------------
HRESULT CEffectModel::LoadEffectModel(char* pFile)
{
    m_ModelType = 0x1f;
    m_ModelTotal = 0;
    m_ModelNo    = 0;
    m_type.assign(pFile, 4);

    DAT1FH* pcp = (DAT1FH*)(pFile + 16);
    EFFECTVERTEX* pSrcVert = (EFFECTVERTEX*)(pFile + 16 + sizeof(DAT1FH));
    m_Name.assign(pcp->name, 16);

    int NumFaces    = pcp->numFace & 0xff;
    int NumVertices = NumFaces * 3;
    m_NumVertices   = NumVertices;
    m_NumFaces      = NumFaces;
    m_NumIndex      = NumFaces * 3;
    m_VBSize        = sizeof(EFFECTVERTEX) * NumVertices;
    m_IBSize        = m_NumIndex * sizeof(WORD);

    // CPU バッファ確保・頂点コピー
    m_cpuVB.resize(m_VBSize);
    memcpy(m_cpuVB.data(), pSrcVert, m_VBSize);

    // インデックス生成 (連番三角形リスト)
    m_cpuIB.resize(m_IBSize);
    WORD* pIdx = reinterpret_cast<WORD*>(m_cpuIB.data());
    for (WORD i = 0; i < (WORD)m_NumIndex; i++) pIdx[i] = i;

    // DX11 バッファ生成
    HRESULT hr;
    hr = CreateBuffer11(&m_lpVB, m_VBSize, D3D11_BIND_VERTEX_BUFFER, m_cpuVB.data());
    if (FAILED(hr)) return hr;
    hr = CreateBuffer11(&m_lpIB, m_IBSize, D3D11_BIND_INDEX_BUFFER, m_cpuIB.data());
    return hr;
}

//----------------------------------------------------------------------
// 0x21 画像関係モデル読み込み
//----------------------------------------------------------------------
HRESULT CEffectModel::LoadEffectModel2(char* pFile)
{
    m_ModelType = 0x21;
    m_ModelNo   = 0;
    m_type.assign(pFile, 4);

    DAT21H* pcp = (DAT21H*)(pFile + 16);
    m_Name.assign(pcp->name, 16);

    int NumFaces    = (pcp->S2 & 0xff) * 2;
    int NumVertices = NumFaces * 3;
    m_NumVertices   = NumVertices;
    m_NumFaces      = NumFaces;
    m_ModelTotal    = NumFaces / 2;
    m_NumIndex      = NumFaces * 3;
    m_VBSize        = sizeof(EFFECTVERTEX) * NumVertices;
    m_IBSize        = m_NumIndex * sizeof(WORD);

    // CPU バッファ確保
    m_cpuVB.resize(m_VBSize, 0);

    // 頂点展開
    EFFECTVERTEX* pDst = reinterpret_cast<EFFECTVERTEX*>(m_cpuVB.data());
    const char* pSrc   = pFile + 16 + sizeof(DAT21H);
    for (int i = 0; i < NumVertices; i++, pDst++)
    {
        if (i != 0 && (i % 6) == 0) pSrc += sizeof(DWORD);
        const EFFECT2VERTEX* pV = (const EFFECT2VERTEX*)pSrc;
        pSrc += sizeof(EFFECT2VERTEX);
        pDst->x  = pV->x; pDst->y  = pV->y; pDst->z  = pV->z;
        pDst->hx = 0.f;   pDst->hy = 0.f;   pDst->hz = -1.f;
        pDst->color = pV->color;
        pDst->u  = pV->u; pDst->v  = pV->v;
    }

    // インデックス生成
    m_cpuIB.resize(m_IBSize);
    WORD* pIdx = reinterpret_cast<WORD*>(m_cpuIB.data());
    for (WORD i = 0; i < (WORD)m_NumIndex; i++) pIdx[i] = i;

    // DX11 バッファ生成
    HRESULT hr;
    hr = CreateBuffer11(&m_lpVB, m_VBSize, D3D11_BIND_VERTEX_BUFFER, m_cpuVB.data());
    if (FAILED(hr)) return hr;
    hr = CreateBuffer11(&m_lpIB, m_IBSize, D3D11_BIND_INDEX_BUFFER, m_cpuIB.data());
    return hr;
}

//======================================================================
// CKeyFrame
//======================================================================
CKeyFrame::CKeyFrame() : m_isLoop(TRUE), m_startTime(0), m_duration(0) {}
CKeyFrame::~CKeyFrame() {}

void CKeyFrame::outputValue(HWND listObj)
{
    char buf[256];
    auto addStr = [&](const char* s) { SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)s); };
    for (size_t i = 0; i < m_keys.size(); i++) {
        sprintf(buf, "[%2d] Key (%5.5f) Value(%5.5f)", (int)i, m_keys[i], m_values[i]);
        addStr(buf);
    }
}

void CKeyFrame::GetKeyFrame(char* pBuf)
{
    char*  ptr = pBuf + 16;
    int    num = 0;
    float  key = 0.f, val = 0.f;
    m_type.assign(pBuf, 4);
    while (key < 1.0f) {
        key = *((float*)ptr); ptr += 4;
        val = *((float*)ptr); ptr += 4;
        num++;
    }
    CreateKey(num);
    ptr = pBuf + 16;
    for (int i = 0; i < num; i++) {
        key = *((float*)ptr); ptr += 4;
        val = *((float*)ptr); ptr += 4;
        SetKeyValue(i, key, val);
    }
    SetLoopFlag(true);
    SetStartTime(0);
}

void CKeyFrame::CreateKey(int numKey)
{
    m_keys.assign(numKey, 0.f);
    m_values.assign(numKey, 0.f);
}

void CKeyFrame::SetKeyValue(int index, float key, float value)
{
    if (index >= 0 && index < (int)m_keys.size()) {
        m_keys[index]   = key;
        m_values[index] = value;
    }
}

void CKeyFrame::SetDuration(DWORD duration) { m_duration  = duration; }
void CKeyFrame::SetLoopFlag(DWORD isLoop)   { m_isLoop    = isLoop;   }
void CKeyFrame::SetStartTime(DWORD start)   { m_startTime = start;    }

BOOL CKeyFrame::GetValue(DWORD time, float* pValue, BOOL* pIsEnd)
{
    if (!pValue || !pIsEnd || m_keys.empty()) return FALSE;

    DWORD endTime = m_startTime + m_duration;
    if (!m_isLoop) {
        if (time < m_startTime) { *pValue = m_values.front(); *pIsEnd = TRUE; return TRUE; }
        if (time > endTime)     { *pValue = m_values.back();  *pIsEnd = TRUE; return TRUE; }
    }

    float fraction  = GetFraction(time);
    int   beginIndex = GetBeginIndex(fraction);
    int   endIndex   = beginIndex + 1;

    if (beginIndex < 0)
        *pValue = m_values.front();
    else if (beginIndex >= (int)m_keys.size() - 1)
        *pValue = m_values.back();
    else {
        float dv   = m_values[endIndex] - m_values[beginIndex];
        float dt   = m_keys[endIndex]   - m_keys[beginIndex];
        float past = fraction - m_keys[beginIndex];
        *pValue    = (dv / dt) * past + m_values[beginIndex];
    }
    *pIsEnd = FALSE;
    return TRUE;
}

float CKeyFrame::GetFraction(DWORD time)
{
    if (m_duration == 0) return 1.f;
    DWORD diffTime;
    if (time > m_startTime) {
        diffTime = (time - m_startTime) % m_duration;
    } else {
        diffTime = (m_startTime - time) % m_duration;
        return 1.f - (float)diffTime / (float)m_duration;
    }
    return (float)diffTime / (float)m_duration;
}

int CKeyFrame::GetBeginIndex(float fraction)
{
    if (m_keys.empty() || fraction < m_keys[0]) return -1;
    int index = 0;
    for (size_t i = 0; i < m_keys.size(); i++) {
        if (m_keys[i] <= fraction) index = (int)i;
        else break;
    }
    return index;
}

//======================================================================
// CEffect  (Phase6: DirectXMath)
//======================================================================
CEffect::CEffect()  { InitData(); }
CEffect::~CEffect() {}

void CEffect::InitData(void)
{
    XMFLOAT3 z3 = { 0.f, 0.f, 0.f };
    XMFLOAT3 o3 = { 1.f, 1.f, 1.f };

    for (int i = 0; i < 128; i++) param[i] = false;
    StoreM(m_mRootTransform, XMMatrixIdentity());
    m_uv  = m_p01 = m_p02 = m_p03 = m_r06 =
    m_r09 = m_r0A = m_r0B = m_r0C =
    m_h1F = m_r1F = z3;
    m_s0F = m_s10 = m_s11 = m_s12 = m_s13 = m_s1F = o3;
    m_name.clear(); m_target.clear();
    m_no = m_1fdiv = 0;
    m_ModelType = 0;
    m_08dist = 0.f;
    m_color  = { 0.f, 0.f, 0.f, 0.f };
    m_Rd = m_Gr = m_Bl = m_Al = nullptr;
    m_kfpx = m_kfpy = m_kfpz = nullptr;
    m_kfrx = m_kfry = m_kfrz = nullptr;
    m_kfsx = m_kfsy = m_kfsz = nullptr;
    m_kfu  = m_kfv  = nullptr;
    m_pAreaMesh    = nullptr;
    m_pEffectModel = nullptr;
}

void CEffect::Set1F(int no)
{
    if (m_no <= 0) return;
    const float PAI = 3.14159265f;
    float angl = PAI * 2.f / (float)(m_no + 1) * no;

    XMFLOAT3 vec   = { 1.f, 0.f, 0.f };
    XMFLOAT3 tempv = {
        m_r1F.x + (float)rand() / 32767.f * m_r1F.y,
        m_h1F.x + (float)rand() / 32767.f * m_h1F.y,
        0.f
    };

    XMMATRIX rotY = XMMatrixRotationY(angl);
    StoreV(tempv, XMVector3TransformNormal(LoadV(tempv), rotY));
    StoreV(vec,   XMVector3TransformNormal(LoadV(vec),   rotY));

    m_p01.x += tempv.x; m_p01.y += tempv.y; m_p01.z += tempv.z;
    vec.x *= m_08dist;  vec.y *= m_08dist;   vec.z *= m_08dist;
}

#define LIFE_BASE 30

void CEffect::GetEffectMatrix(char* pBuff, CKeyFrame* pKeyFrame)
{
    CKeyFrame* pKFrame;
    bool       Sflg = false;
    char*      pAdr;
    char       type;
    int        num, dat1Adr, dat2Adr, dat3Adr, dat4Adr;

    m_name.assign(pBuff, 4);
    pAdr    = pBuff;
    dat1Adr = *((int*)(pAdr + 0x80));
    dat2Adr = *((int*)(pAdr + 0x84));
    dat3Adr = *((int*)(pAdr + 0x88));
    dat4Adr = *((int*)(pAdr + 0x8c));
    m_no       = *((unsigned char*)(pAdr + 0x78));
    m_interval = *((unsigned short*)(pAdr + 0x76));

    pAdr = pBuff + dat3Adr;
    while (pAdr < (pBuff + dat4Adr)) {
        type = *pAdr;
        num  = (int)*(pAdr + 1); num &= 0x0f;
        if (type == 0) break;
        switch (type) {
        case 0x27: m_uv.x = *((float*)(pAdr + 4)); pAdr += 4 * num; break;
        case 0x28: m_uv.y = *((float*)(pAdr + 4)); pAdr += 4 * num; break;
        default:   pAdr += 4 * num; break;
        }
    }

    pAdr = pBuff + dat2Adr;
    WORD  mtype;
    DWORD col;
    while (pAdr < (pBuff + dat3Adr)) {
        type = *pAdr;
        num  = (int)*(pAdr + 1); num &= 0x0f;
        switch (type) {
        case 0x00:
        {
            XMMATRIX xm = LoadM(m_mRootTransform);
            xm = XMMatrixMultiply(XMMatrixScaling(m_s0F.x, m_s0F.y, m_s0F.z), xm);
            xm = XMMatrixMultiply(XMMatrixRotationZ(m_r09.z), xm);
            xm = XMMatrixMultiply(XMMatrixRotationY(m_r09.y), xm);
            xm = XMMatrixMultiply(XMMatrixRotationX(m_r09.x), xm);
            xm = XMMatrixMultiply(XMMatrixTranslation(m_p01.x, m_p01.y, m_p01.z), xm);
            StoreM(m_mRootTransform, xm);
            return;
        }
        case 0x01:
            param[0x01] = true;
            m_target.assign(pAdr + 12, 4);
            m_kind1 = *((WORD*)(pAdr + 4));
            m_kind2 = *((WORD*)(pAdr + 6));
            m_p01.x = *((float*)(pAdr + 20));
            m_p01.y = *((float*)(pAdr + 24));
            m_p01.z = *((float*)(pAdr + 28));
            mtype   = *((WORD*)(pAdr + 32));
            if      ((mtype >> 8) == 0x0b) m_ModelType = 0x1f;
            else if ((mtype >> 8) == 0x0E) m_ModelType = 0x21;
            else if ((mtype & 0xff) == 0x3d) m_ModelType = 0x3d;
            else m_ModelType = 0xff;
            m_lifeTime = *((WORD*)(pAdr + 34));
            pAdr += 4 * num; break;
        case 0x02: param[0x02]=true; m_p02.x=*((float*)(pAdr+4)); m_p02.y=*((float*)(pAdr+8)); m_p02.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x03: param[0x03]=true; m_p03.x=*((float*)(pAdr+4)); m_p03.y=*((float*)(pAdr+8)); m_p03.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x06: param[0x06]=true; m_r06.x=*((float*)(pAdr+4)); m_r06.y=*((float*)(pAdr+8)); m_r06.z=0.f; pAdr+=4*num; break;
        case 0x08: param[0x08]=true; m_08dist=*((float*)(pAdr+4)); pAdr+=4*num; break;
        case 0x09: param[0x09]=true; m_r09.x=*((float*)(pAdr+4)); m_r09.y=*((float*)(pAdr+8)); m_r09.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x0a: param[0x0a]=true; m_r0A.x=*((float*)(pAdr+4)); m_r0A.y=*((float*)(pAdr+8)); m_r0A.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x0b: param[0x0b]=true; m_r0B.x=*((float*)(pAdr+4)); m_r0B.y=*((float*)(pAdr+8)); m_r0B.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x0c: param[0x0c]=true; m_r0C.x=*((float*)(pAdr+4)); m_r0C.y=*((float*)(pAdr+8)); m_r0C.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x0f: param[0x0f]=true; m_s0F.x=*((float*)(pAdr+4)); m_s0F.y=*((float*)(pAdr+8)); m_s0F.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x10: param[0x10]=true; m_s10.x=*((float*)(pAdr+4)); m_s10.y=*((float*)(pAdr+8)); m_s10.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x11: param[0x11]=true; m_s11.x=*((float*)(pAdr+4)); m_s11.y=*((float*)(pAdr+4)); m_s11.z=*((float*)(pAdr+4)); pAdr+=4*num; break;
        case 0x12: param[0x12]=true; Sflg=true; m_s12.x=*((float*)(pAdr+4)); m_s12.y=*((float*)(pAdr+8)); m_s12.z=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x13: param[0x13]=true; Sflg=true; m_s13.x+=*((float*)(pAdr+4)); m_s13.y+=*((float*)(pAdr+8)); m_s13.z+=*((float*)(pAdr+12)); pAdr+=4*num; break;
        case 0x16:
            param[0x16] = true;
            col = *((int*)(pAdr + 4));
            m_color.x = (float)((col >>  0) & 0xff) / 255.f;
            m_color.y = (float)((col >>  8) & 0xff) / 255.f;
            m_color.z = (float)((col >> 16) & 0xff) / 255.f;
            m_color.w = (float)((col >> 24) & 0xff) / 255.f;
            pAdr += 4 * num; break;
        case 0x1E: pAdr+=4*num; break;
        case 0x1F:
            param[0x1F]=true;
            m_r1F.y=*((float*)(pAdr+4)); m_r1F.x=*((float*)(pAdr+8));
            m_s1F.x=*((float*)(pAdr+12)); m_s1F.y=*((float*)(pAdr+16)); m_s1F.z=*((float*)(pAdr+20));
            m_h1F.x=*((float*)(pAdr+24)); m_h1F.y=*((float*)(pAdr+28));
            m_1fdiv=*((int*)(pAdr+44));
            pAdr+=4*num; break;
        // キーフレーム参照 (0x21〜0x2F, 0x60〜0x62)
        #define KF_CASE(ID, MEMBER) \
        case ID: param[ID]=true; MEMBER=nullptr; pKFrame=pKeyFrame; \
            while(pKFrame){if(!memcmp(pAdr+8,pKFrame->m_type.c_str(),4)){MEMBER=pKFrame;break;} pKFrame=(CKeyFrame*)pKFrame->Next;} \
            pAdr+=4*num; break;
        KF_CASE(0x21, m_kfpx) KF_CASE(0x22, m_kfpy) KF_CASE(0x23, m_kfpz)
        KF_CASE(0x24, m_kfrx) KF_CASE(0x25, m_kfry) KF_CASE(0x26, m_kfrz)
        KF_CASE(0x27, m_kfsx)
        case 0x28:
            param[0x28]=true; m_kfsy=nullptr; pKFrame=pKeyFrame;
            while(pKFrame){if(!memcmp(pAdr+8,pKFrame->m_type.c_str(),4)){m_kfsy=pKFrame;pKFrame->SetDuration(m_lifeTime);break;} pKFrame=(CKeyFrame*)pKFrame->Next;}
            pAdr+=4*num; break;
        KF_CASE(0x29, m_kfsz) KF_CASE(0x2D, m_Al) KF_CASE(0x2E, m_kfu) KF_CASE(0x2F, m_kfv)
        case 0x30: pAdr+=4*num; break;
        KF_CASE(0x60, m_Rd) KF_CASE(0x61, m_Gr) KF_CASE(0x62, m_Bl)
        #undef KF_CASE
        default: pAdr+=4*num; break;
        }
    }
}

void CEffect::outputProp(HWND listObj)
{
    char buf[256];
    auto addStr = [&](const char* s){ SendMessage(listObj, LB_ADDSTRING, 0, (LPARAM)s); };
    sprintf(buf,"[00] ID (%s) ターゲット(%s) U(%5.5f) V(%5.5f)",
        m_name.c_str(), m_target.c_str(), m_uv.x, m_uv.y);
    addStr(buf);
    for (int i = 0; i < 128; i++) {
        if (!param[i]) continue;
        switch (i) {
        case 0x01: sprintf(buf,"[%02x] mtype(%02x) kd1(%04x) kd2(%04x) LfT(%04x)",i,m_ModelType,m_kind1,m_kind2,m_lifeTime); addStr(buf);
                   sprintf(buf,"[%02x] ｵﾌｾｯﾄ(%5.5f,%5.5f,%5.5f)",i,m_p01.x,m_p01.y,m_p01.z); addStr(buf); break;
        case 0x02: sprintf(buf,"[%02x] ｵﾌｾｯﾄ2(%5.5f,%5.5f,%5.5f)",i,m_p02.x,m_p02.y,m_p02.z); addStr(buf); break;
        case 0x03: sprintf(buf,"[%02x] ｵﾌｾｯﾄ3(%5.5f,%5.5f,%5.5f)",i,m_p03.x,m_p03.y,m_p03.z); addStr(buf); break;
        case 0x06: sprintf(buf,"[%02x] 2D回転(%5.5f,%5.5f,%5.5f)",i,m_r06.x,m_r06.y,m_r06.z); addStr(buf); break;
        case 0x08: sprintf(buf,"[%02x] dist(%5.5f)",i,m_08dist); addStr(buf); break;
        case 0x09: sprintf(buf,"[%02x] 回転(%5.5f,%5.5f,%5.5f)",i,m_r09.x,m_r09.y,m_r09.z); addStr(buf); break;
        case 0x0a: sprintf(buf,"[%02x] 回転2(%5.5f,%5.5f,%5.5f)",i,m_r0A.x,m_r0A.y,m_r0A.z); addStr(buf); break;
        case 0x0b: sprintf(buf,"[%02x] 回転3(%5.5f,%5.5f,%5.5f)",i,m_r0B.x,m_r0B.y,m_r0B.z); addStr(buf); break;
        case 0x0c: sprintf(buf,"[%02x] 回転4(%5.5f,%5.5f,%5.5f)",i,m_r0C.x,m_r0C.y,m_r0C.z); addStr(buf); break;
        case 0x0f: sprintf(buf,"[%02x] ｽｹｰﾙ(%5.5f,%5.5f,%5.5f)",i,m_s0F.x,m_s0F.y,m_s0F.z); addStr(buf); break;
        case 0x10: sprintf(buf,"[%02x] ｽｹｰﾙ2(%5.5f,%5.5f,%5.5f)",i,m_s10.x,m_s10.y,m_s10.z); addStr(buf); break;
        case 0x11: sprintf(buf,"[%02x] ｽｹｰﾙ3(%5.5f,%5.5f,%5.5f)",i,m_s11.x,m_s11.y,m_s11.z); addStr(buf); break;
        case 0x12: sprintf(buf,"[%02x] ｽｹｰﾙ4(%5.5f,%5.5f,%5.5f)",i,m_s12.x,m_s12.y,m_s12.z); addStr(buf); break;
        case 0x13: sprintf(buf,"[%02x] ｽｹｰﾙ5(%5.5f,%5.5f,%5.5f)",i,m_s13.x,m_s13.y,m_s13.z); addStr(buf); break;
        case 0x16: sprintf(buf,"[%02x] ｶﾗｰ(%3d,%3d,%3d,%3d)",i,(int)(m_color.x*255),(int)(m_color.y*255),(int)(m_color.z*255),(int)(m_color.w*255)); addStr(buf); break;
        case 0x1F: sprintf(buf,"[%02x] r(%5.5f,%5.5f) s(%5.5f,%5.5f,%5.5f) h(%5.5f,%5.5f) div%d",i,m_r1F.x,m_r1F.y,m_s1F.x,m_s1F.y,m_s1F.z,m_h1F.x,m_h1F.y,m_1fdiv); addStr(buf); break;
        case 0x21: if(m_kfpx) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) px",i,m_kfpx->m_type.c_str()); addStr(buf); } break;
        case 0x22: if(m_kfpy) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) py",i,m_kfpy->m_type.c_str()); addStr(buf); } break;
        case 0x23: if(m_kfpz) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) pz",i,m_kfpz->m_type.c_str()); addStr(buf); } break;
        case 0x24: if(m_kfrx) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) rx",i,m_kfrx->m_type.c_str()); addStr(buf); } break;
        case 0x25: if(m_kfry) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) ry",i,m_kfry->m_type.c_str()); addStr(buf); } break;
        case 0x26: if(m_kfrz) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) rz",i,m_kfrz->m_type.c_str()); addStr(buf); } break;
        case 0x27: if(m_kfsx) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) sx",i,m_kfsx->m_type.c_str()); addStr(buf); } break;
        case 0x28: if(m_kfsy) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) sy",i,m_kfsy->m_type.c_str()); addStr(buf); } break;
        case 0x29: if(m_kfsz) { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) sz",i,m_kfsz->m_type.c_str()); addStr(buf); } break;
        case 0x2D: if(m_Al)   { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) alph",i,m_Al->m_type.c_str()); addStr(buf); } break;
        case 0x2E: if(m_kfu)  { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) u",i,m_kfu->m_type.c_str()); addStr(buf); } break;
        case 0x2F: if(m_kfv)  { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) v",i,m_kfv->m_type.c_str()); addStr(buf); } break;
        case 0x60: if(m_Rd)   { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) Rd",i,m_Rd->m_type.c_str()); addStr(buf); } break;
        case 0x61: if(m_Gr)   { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) Gr",i,m_Gr->m_type.c_str()); addStr(buf); } break;
        case 0x62: if(m_Bl)   { sprintf(buf,"[%02x] ｷｰﾌﾚｰﾑ(%s) Bl",i,m_Bl->m_type.c_str()); addStr(buf); } break;
        default: break;
        }
    }
}
