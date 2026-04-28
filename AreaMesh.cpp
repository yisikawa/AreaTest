#define _CRT_SECURE_NO_WARNINGS
#include "AreaMesh.h"
#include "Area.h"
#include "Render.h"

#define SAFE_RELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; }

//======================================================================
// CStream
//======================================================================
CStream::CStream()
{
    m_pTexture    = NULL;
    m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    m_IndexStart  = 0;
    m_FaceCount   = 0;
    m_AlphaFlag   = 0;
    m_StencilFlag = false;
}

CStream::~CStream() {}

void CStream::SetMeshAttr(D3D11_PRIMITIVE_TOPOLOGY type,
                          unsigned long index_start,
                          unsigned long face_count)
{
    m_PrimitiveType = type;
    m_IndexStart    = index_start;
    m_FaceCount     = face_count;
}

D3D11_PRIMITIVE_TOPOLOGY CStream::GetPrimitiveType(void) { return m_PrimitiveType; }
unsigned long            CStream::GetIndexStart(void)    { return m_IndexStart; }
unsigned long            CStream::GetFaceCount(void)     { return m_FaceCount; }

//======================================================================
// CTexture  (Phase3: ID3D11ShaderResourceView)
//======================================================================
CTexture::CTexture()
    : m_pSRV(nullptr), m_texWidth(0), m_texHeight(0)
{}

CTexture::~CTexture()
{
    SAFE_RELEASE(m_pSRV);
}

void CTexture::SetTexture(ID3D11ShaderResourceView* pSRV)
{
    SAFE_RELEASE(m_pSRV);
    m_pSRV = pSRV;
    if (m_pSRV) m_pSRV->AddRef();
}

//======================================================================
// CAreaMesh
//======================================================================
CAreaMesh::CAreaMesh()
    : m_lpVB(nullptr), m_lpIB(nullptr)
{
    m_NumIndex = m_NumVertices = m_NumFaces = m_VBSize = m_IBSize = m_FVF = 0;
    m_BoxLow  = { 0.f, 0.f, 0.f };
    m_BoxHigh = { 0.f, 0.f, 0.f };
}

CAreaMesh::~CAreaMesh()
{
    m_LStreams.clear();
    SAFE_RELEASE(m_lpVB);
    SAFE_RELEASE(m_lpIB);
}

//======================================================================
//      エリアメッシュ読み込み  (Phase2+3: DX11 バッファ, CPU コピー)
//======================================================================
HRESULT CAreaMesh::LoadAreaMesh(char* pFile, CArea* pArea, unsigned long FVF)
{
#pragma pack(push, 2)
    typedef struct {
        XMFLOAT3 v, n;
        DWORD    color;
        float    tu, tv;
    } VTEX;

    typedef struct {
        float x1, x2, y1, y2, z1, z2;
    } AREAMESHBOX;
#pragma pack(pop)

    int   CountIndex = 0, NumFaces = 0, NumVertices = 0, NumIndex = 0;
    int   NumElement, Level;
    char* Pos;
    int   nVertex, nVerReal, nIndex, NextPos;
    short AlphaFlag = 0;
    unsigned char aFlag, bFlag;

    m_AreaType.assign(pFile, 4);
    aFlag = *(unsigned char*)pFile;
    bFlag = *(unsigned char*)(pFile + 4);
    pFile += 16;

    // ─── パス1: 頂点/インデックス数カウント ──────────────────────────────
    Pos   = pFile + 0x020;
    Level = *(int*)Pos;
    if      (Level > 64)              return -1;
    else if (Level > 16)              Pos += Level * 4;
    else if (Level >= 2 && Level <= 16) Pos += 16 * 4;
    else if (Level == 1)              Pos += 32;
    else                              return -1;

    while (Level)
    {
        NumElement = *(int*)Pos;
        if (NumElement > 0xffff || NumElement < 0) return -1;
        Pos += 32;
        for (int i = 0; i < NumElement; i++)
        {
            nVertex  = (int)*(short*)(Pos + 16);
            nVerReal = (int)*(int*)(Pos + 16);
            if (nVertex > 0xffff || nVertex < 0) return -1;
            nIndex   = *(int*)(Pos + 16 + 4 + nVertex * 36);
            if (nIndex > 0xffff || nIndex < 0) return -1;
            NextPos  = 16 + 4 + nVertex * 36 + 4 + nIndex * 2;
            NextPos  = 4 * ((NextPos + 3) / 4);
            NumVertices += nVertex;
            NumFaces    += nIndex - 2;
            NumIndex    += nIndex;
            Pos += NextPos;
        }
        Level--;
    }

    m_NumVertices = NumVertices;
    m_NumFaces    = NumFaces;
    m_NumIndex    = NumIndex;
    m_FVF         = FVF;
    m_VBSize      = sizeof(VTEX) * NumVertices;
    m_IBSize      = NumIndex * sizeof(WORD);

    // CPU バッファ確保
    m_cpuVB.resize(m_VBSize, 0);
    m_cpuIB.resize(m_IBSize, 0);
    VTEX* pV     = reinterpret_cast<VTEX*>(m_cpuVB.data());
    WORD* pIndex = reinterpret_cast<WORD*>(m_cpuIB.data());

    // ─── パス2: 頂点/インデックスデータ格納 ──────────────────────────────
    XMFLOAT3 Low  = {  65535.f,  65535.f,  65535.f };
    XMFLOAT3 High = { -65535.f, -65535.f, -65535.f };
    CountIndex = NumFaces = NumVertices = 0;

    Pos   = pFile + 0x20;
    Level = *(int*)Pos;
    if      (Level > 64)              return -1;
    else if (Level > 16)              Pos += Level * 4;
    else if (Level >= 2 && Level <= 16) Pos += 16 * 4;
    else if (Level == 1)
    {
        AREAMESHBOX* pBox = (AREAMESHBOX*)(Pos + 4);
        XMFLOAT3 lo = { pBox->x1, pBox->y1, pBox->z1 };
        XMFLOAT3 hi = { pBox->x2, pBox->y2, pBox->z2 };
        SetBoxLow(lo);
        SetBoxHigh(hi);
        Pos += 32;
    }
    else return -1;

    while (Level)
    {
        NumElement = *(int*)Pos;
        if (NumElement > 0xffff || NumElement < 0) return -1;
        Pos += 32;

        for (int i = 0; i < NumElement; i++)
        {
            char TexName[18];
            strncpy(TexName, Pos, 16); TexName[16] = '\0';
            CTexture* pTexture = (CTexture*)pArea->m_Textures.Top();
            int texNo = 0;
            while (pTexture != NULL) {
                if (!memcmp(TexName, pTexture->GetTexName(), 16)) break;
                pTexture = (CTexture*)pTexture->Next;
                texNo++;
            }

            nVertex  = (int)*(short*)(Pos + 16);
            nVerReal = (int)*(int*)(Pos + 16);
            AlphaFlag = (short)((nVerReal >> 28) & 0x0f);
            if (nVertex > 0xffff || nVertex <= 0) return -1;
            nIndex   = *(int*)(Pos + 16 + 4 + nVertex * 36);
            if (nIndex > 0xffff || nIndex <= 0) return -1;
            NextPos  = 16 + 4 + nVertex * 36 + 4 + nIndex * 2;
            NextPos  = 4 * ((NextPos + 3) / 4);

            CStream tStream;
            tStream.SetMeshAttr(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, CountIndex, nIndex - 2);
            tStream.SetpTexture(pTexture);
            tStream.m_TexNo     = texNo;
            tStream.SetAlphaFlag(AlphaFlag);
            tStream.m_vertNum   = nVertex;
            tStream.m_faceNum   = nIndex - 2;
            if (aFlag == 0x5f) tStream.SetStencilFlag(true);

            VTEX* pSrcVert = (VTEX*)(Pos + 16 + 4);
            for (int j = 0; j < nVertex; j++) {
                if (pSrcVert[j].v.x < Low.x)  Low.x  = pSrcVert[j].v.x;
                if (pSrcVert[j].v.y < Low.y)  Low.y  = pSrcVert[j].v.y;
                if (pSrcVert[j].v.z < Low.z)  Low.z  = pSrcVert[j].v.z;
                if (pSrcVert[j].v.x > High.x) High.x = pSrcVert[j].v.x;
                if (pSrcVert[j].v.y > High.y) High.y = pSrcVert[j].v.y;
                if (pSrcVert[j].v.z > High.z) High.z = pSrcVert[j].v.z;
            }

            // 頂点コピー
            memcpy(pV, pSrcVert, sizeof(VTEX) * nVertex);
            pV += nVertex;

            // インデックスコピー (グローバルオフセット加算)
            std::vector<WORD> tmpIdx(nIndex);
            memcpy(tmpIdx.data(), Pos + 16 + 4 + nVertex * 36 + 4, sizeof(WORD) * nIndex);
            for (int j = 0; j < nIndex; j++) tmpIdx[j] += (WORD)NumVertices;
            memcpy(pIndex, tmpIdx.data(), sizeof(WORD) * nIndex);
            pIndex += nIndex;

            m_LStreams.push_back(tStream);
            NumVertices += nVertex;
            NumFaces    += nIndex - 2;
            CountIndex  += nIndex;
            Pos += NextPos;
        }
        Level--;
    }

    // AABB 更新
    XMFLOAT3 TempLow  = GetBoxLow();
    XMFLOAT3 TempHigh = GetBoxHigh();
    if (TempLow.x == 0.f && TempLow.y == 0.f && TempLow.z == 0.f) SetBoxLow(Low);
    if (TempHigh.x == 0.f && TempHigh.y == 0.f && TempHigh.z == 0.f) SetBoxHigh(High);

    // ─── DX11 バッファ生成 ────────────────────────────────────────────────
    HRESULT hr;
    hr = CreateBuffer11(&m_lpVB, m_VBSize, D3D11_BIND_VERTEX_BUFFER, m_cpuVB.data());
    if (FAILED(hr)) return hr;
    hr = CreateBuffer11(&m_lpIB, m_IBSize, D3D11_BIND_INDEX_BUFFER, m_cpuIB.data());
    if (FAILED(hr)) return hr;

    return S_OK;
}

int CAreaMesh::countTextures(void)
{
    int count = 0;
    for (auto& s : m_LStreams)
        if (s.GetpTexture() != NULL) count++;
    return count;
}
