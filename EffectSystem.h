#pragma once

#include <string>
#include <vector>
#include "Dx.h"
#include "List.h"
#include "AreaMesh.h"

#pragma pack(push, 2)

typedef struct {
    DWORD         L;
    unsigned char numImage;
    unsigned char numNonImage;
    WORD          numFace, numFace1, numFace2, Flag;
    char          name[16];
} DAT1FH;

typedef struct {
    WORD S1, S2, S3, S4;
    char name[16];
    WORD S5, S6;
} DAT21H;

typedef struct {
    char  name[16];
    BYTE  Ver;
    BYTE  nazo;
    WORD  Type;
    WORD  Flip;
} DAT2AH;

typedef struct {
    float x, y, z;
    float hx, hy, hz;
    DWORD color;
    float u, v;
} EFFECTVERTEX;

typedef struct {
    float x, y, z;
    DWORD color;
    float u, v;
} EFFECT2VERTEX;

#pragma pack(pop)

//======================================================================
// CEffectModel  Phase2: DX11 バッファ
//======================================================================
typedef class CEffectModel : public CListBase
{
public:
    int            m_texNo;
    int            m_ModelType;
    DWORD          m_ModelTotal;
    DWORD          m_ModelNo;
    unsigned long  m_VertexSize;
    CTexture*      m_pTexture;
    unsigned long  m_NumVertices, m_NumFaces, m_NumIndex, m_VBSize, m_IBSize, m_FVF;
    ID3D11Buffer*  m_lpVB;
    ID3D11Buffer*  m_lpIB;
    std::vector<BYTE> m_cpuVB;  // MQO エクスポート用 CPU コピー
    std::vector<BYTE> m_cpuIB;
    std::string    m_Name;
    std::string    m_type;

    CEffectModel();
    virtual ~CEffectModel();
    virtual HRESULT LoadEffectModel(char* pFile);
    virtual HRESULT LoadEffectModel2(char* pFile);
}
CEffectModel, *LPCEffectModel;

//======================================================================
// CKeyFrame
//======================================================================
typedef class CKeyFrame : public CListBase
{
private:
    std::vector<float> m_keys;
    std::vector<float> m_values;
    BOOL  m_isLoop;
    DWORD m_startTime;
    DWORD m_duration;
public:
    std::string m_type;
    CKeyFrame();
    ~CKeyFrame();
    void outputValue(HWND listObj);
    void GetKeyFrame(char* pBuf);
    void CreateKey(int numKey);
    void SetKeyValue(int index, float key, float value);
    void SetDuration(DWORD duration);
    void SetLoopFlag(DWORD isLoop);
    void SetStartTime(DWORD start);
    BOOL GetValue(DWORD time, float* pValue, BOOL* pIsEnd);
protected:
    float GetFraction(DWORD time);
    int   GetBeginIndex(float fraction);
}
CKeyFrame, *LPCKeyFrame;

//======================================================================
// CEffect  Phase6: D3DXVECTOR3→XMFLOAT3, D3DXMATRIX→XMFLOAT4X4
//======================================================================
typedef class CEffect : public CListBase
{
public:
    bool         param[128];
    int          m_bPos;
    XMFLOAT4X4   m_mRootTransform;
    int          m_ModelType;
    int          m_no, m_1fdiv;
    int          m_interval;
    int          m_subID;
    std::string  m_class;
    std::string  m_name;
    std::string  m_target;
    CAreaMesh*   m_pAreaMesh;
    CEffectModel* m_pEffectModel;
    DWORD        m_lifeTime;
    int          m_dir1, m_dir2;
    XMFLOAT3     m_uv, m_r1F, m_h1F, m_s1F, m_r06,
                 m_p01, m_p02, m_p03,
                 m_r09, m_r0A, m_r0B, m_r0C,
                 m_s0F, m_s10, m_s11, m_s12, m_s13;
    XMFLOAT4     m_color;   // replaces D3DCOLORVALUE
    DWORD        m_kind1, m_kind2;
    float        m_08dist;
    XMFLOAT2     m_uvAccum;  // UV累積オフセット（毎フレーム m_uv を加算）
    DWORD        m_uvTimer;  // 現サイクル開始時刻(ms)
    CKeyFrame   *m_Rd, *m_Gr, *m_Bl, *m_Al,
                *m_kfpx, *m_kfpy, *m_kfpz,
                *m_kfrx, *m_kfry, *m_kfrz,
                *m_kfsx, *m_kfsy, *m_kfsz,
                *m_kfu, *m_kfv;

    CEffect();
    ~CEffect();
    virtual void GetEffectMatrix(char* pBuf, CKeyFrame* pKeyFrame);
    virtual void InitData(void);
    virtual void Set1F(int no);
    virtual void outputProp(HWND listObj);
}
CEffect, *LPCEffect;
