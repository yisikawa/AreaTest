#pragma once

#include <string>
#include <list>
#include <vector>
#include "Dx.h"
#include "List.h"

// CArea クラスの前方宣言
class CArea;

//======================================================================
// 頂点構造体 (36 バイト, Phase6: XMFLOAT3)
//======================================================================
#pragma pack(push, 2)
typedef struct _D3DTEXVERTEX
{
    XMFLOAT3 v, n;   // 座標, 法線 (12+12=24 bytes)
    DWORD    color;  // 色 BGRA (4 bytes)
    float    tu, tv; // UV (8 bytes)
} D3DTEXVERTEX;
#pragma pack(pop)

//======================================================================
// マテリアル (テクスチャ) クラス  Phase3: DX11 SRV
//======================================================================
typedef class CTexture : public CListBase
{
protected:
    ID3D11ShaderResourceView* m_pSRV;

public:
    std::string              m_TexName;
    std::vector<uint32_t>    m_cpuData;    // BGRA ピクセル (Phase7 PNG 保存用)
    UINT                     m_texWidth;
    UINT                     m_texHeight;

    CTexture();
    virtual ~CTexture();
    virtual void SetTexName(const char* name)  { m_TexName = name; }
    virtual const char* GetTexName(void)       { return m_TexName.c_str(); }
    virtual void SetTexture(ID3D11ShaderResourceView* pSRV);
    virtual ID3D11ShaderResourceView* GetTexture(void) { return m_pSRV; }
}
CTexture, LPCTexture;

//======================================================================
// ストリームタイプクラス  Phase2: D3D11_PRIMITIVE_TOPOLOGY
//======================================================================
typedef class CStream
{
protected:
    CTexture*                  m_pTexture;
    bool                       m_StencilFlag;
    short                      m_AlphaFlag;
    D3D11_PRIMITIVE_TOPOLOGY   m_PrimitiveType;
    unsigned long              m_IndexStart;
    unsigned long              m_FaceCount;

public:
    int           m_TexNo;
    unsigned long m_vertNum;
    unsigned long m_faceNum;

    CStream();
    virtual ~CStream();
    virtual void SetpTexture(CTexture* p)                  { m_pTexture    = p; }
    virtual CTexture* GetpTexture(void)                    { return m_pTexture; }
    virtual void SetStencilFlag(bool f)                    { m_StencilFlag = f; }
    virtual short GetStencilFlag(void)                     { return m_StencilFlag; }
    virtual void SetAlphaFlag(short f)                     { m_AlphaFlag   = f; }
    virtual short GetAlphaFlag(void)                       { return m_AlphaFlag; }
    virtual void SetMeshAttr(D3D11_PRIMITIVE_TOPOLOGY type,
                             unsigned long index_start,
                             unsigned long face_count);
    virtual D3D11_PRIMITIVE_TOPOLOGY GetPrimitiveType(void);
    virtual unsigned long GetIndexStart(void);
    virtual unsigned long GetFaceCount(void);
}
CStream, LPCStream;

//======================================================================
// エリアメッシュクラス  Phase2: DX11 バッファ
//======================================================================
typedef class CAreaMesh : public CListBase
{
    friend class CArea;

protected:
    std::list<CStream>  m_LStreams;
    XMFLOAT3            m_BoxLow, m_BoxHigh;   // Phase6: XMFLOAT3
    unsigned long       m_NumVertices, m_NumFaces, m_NumIndex, m_VBSize, m_IBSize, m_FVF;
    ID3D11Buffer*       m_lpVB;
    ID3D11Buffer*       m_lpIB;
    std::string         m_AreaName;
    std::string         m_AreaType;

public:
    std::vector<BYTE>   m_cpuVB;  // CPU コピー (MQO エクスポート用)
    std::vector<BYTE>   m_cpuIB;

    CAreaMesh();
    virtual ~CAreaMesh();

    virtual void      SetBoxLow(XMFLOAT3 v)    { m_BoxLow  = v; }
    virtual XMFLOAT3  GetBoxLow(void)           { return m_BoxLow; }
    virtual void      SetBoxHigh(XMFLOAT3 v)   { m_BoxHigh = v; }
    virtual XMFLOAT3  GetBoxHigh(void)          { return m_BoxHigh; }

    virtual void          SetNumVertices(unsigned long n) { m_NumVertices = n; }
    virtual unsigned long GetNumVertices(void)            { return m_NumVertices; }
    virtual void          SetNumFaces(unsigned long n)    { m_NumFaces    = n; }
    virtual unsigned long GetNumFaces(void)               { return m_NumFaces; }

    virtual void          SetlpVB(ID3D11Buffer* p) { m_lpVB = p; }
    virtual ID3D11Buffer* GetlpVB(void)            { return m_lpVB; }
    virtual void          SetlpIB(ID3D11Buffer* p) { m_lpIB = p; }
    virtual ID3D11Buffer* GetlpIB(void)            { return m_lpIB; }

    virtual void        SetAreaName(const char* s) { m_AreaName = s; }
    virtual const char* GetAreaName(void)          { return m_AreaName.c_str(); }
    virtual void        SetAreaType(const char* s) { m_AreaType = s; }
    virtual const char* GetAreaType(void)          { return m_AreaType.c_str(); }

    virtual HRESULT LoadAreaMesh(char* pFile, CArea* pArea, unsigned long FVF);
    virtual int     countTextures(void);
}
CAreaMesh, *LPCAreaMesh;
