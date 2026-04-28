#define _CRT_SECURE_NO_WARNINGS
#include <memory>
#include <stdio.h>
#include <wincodec.h>
#include "Render.h"
#include "Area.h"
#include "EffectSystem.h"
#include <list>
#include "Utils.h"

#pragma comment(lib, "windowscodecs.lib")

using namespace std;

BOOL GetFileNameFromDno(LPSTR filename, DWORD dwID);
DWORD ConvertStr2Dno(char* DataName);

#define SAFE_RELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; }
constexpr float PAI  = 3.14159265358979323846f;
constexpr float PAI2 = PAI * 2.0f;

extern BOOL       g_mIsUseSoftware;
extern XMFLOAT4X4 g_mProjection, g_mView;
extern XMFLOAT3   g_mAt, g_mEye, g_mUp;
extern XMFLOAT3   g_mLightDir;
extern float      g_mTime;
extern float      g_mDispArea;
extern float      g_mDispTree;
extern XMFLOAT3   g_mEntry;
extern int        g_mAreaBright;
extern char       g_mWeather[];
extern float      g_mLightDist;
extern XMFLOAT3   g_mLightPosition;
extern XMFLOAT4X4 g_mViewLight;
extern char       g_className[];
extern char       g_meshPath[];
extern char       g_texPath[];

static bool IsMirrorMatrix(const XMFLOAT4X4* pMat)
{
    return XMVectorGetX(XMMatrixDeterminant(LoadM(*pMat))) < 0.f;
}

//======================================================================
// DXT1 ブロックデコーダ
//======================================================================
static void DecodeDXT1Block(const BYTE* src, uint32_t* dst,
                            UINT bx, UINT by, UINT width, UINT height,
                            bool forceOpaque)
{
    uint16_t c0 = *(uint16_t*)src;
    uint16_t c1 = *(uint16_t*)(src + 2);
    uint32_t idx = *(uint32_t*)(src + 4);

    int r0 = ((c0>>11)&0x1F)*255/31, g0 = ((c0>>5)&0x3F)*255/63, b0 = (c0&0x1F)*255/31;
    int r1 = ((c1>>11)&0x1F)*255/31, g1 = ((c1>>5)&0x3F)*255/63, b1 = (c1&0x1F)*255/31;

    uint32_t col[4];
    col[0] = 0xFF000000u|(r0<<16)|(g0<<8)|b0;
    col[1] = 0xFF000000u|(r1<<16)|(g1<<8)|b1;
    if (c0 > c1 || forceOpaque) {
        col[2] = 0xFF000000u|(((2*r0+r1)/3)<<16)|(((2*g0+g1)/3)<<8)|((2*b0+b1)/3);
        col[3] = 0xFF000000u|(((r0+2*r1)/3)<<16)|(((g0+2*g1)/3)<<8)|((b0+2*b1)/3);
    } else {
        col[2] = 0xFF000000u|(((r0+r1)/2)<<16)|(((g0+g1)/2)<<8)|((b0+b1)/2);
        col[3] = 0x00000000u;
    }
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            UINT x = bx+px, y = by+py;
            if (x < width && y < height)
                dst[y*width+x] = col[(idx >> (2*(py*4+px))) & 3];
        }
    }
}

//======================================================================
// DXT3 ブロックデコーダ
//======================================================================
static void DecodeDXT3Block(const BYTE* src, uint32_t* dst,
                            UINT bx, UINT by, UINT width, UINT height)
{
    const BYTE* aptr = src;
    const BYTE* cptr = src + 8;
    uint16_t c0 = *(uint16_t*)cptr;
    uint16_t c1 = *(uint16_t*)(cptr+2);
    uint32_t cidx = *(uint32_t*)(cptr+4);

    int r0=((c0>>11)&0x1F)*255/31, g0=((c0>>5)&0x3F)*255/63, b0=(c0&0x1F)*255/31;
    int r1=((c1>>11)&0x1F)*255/31, g1=((c1>>5)&0x3F)*255/63, b1=(c1&0x1F)*255/31;
    uint32_t rgb[4];
    rgb[0] = (r0<<16)|(g0<<8)|b0;
    rgb[1] = (r1<<16)|(g1<<8)|b1;
    rgb[2] = (((2*r0+r1)/3)<<16)|(((2*g0+g1)/3)<<8)|((2*b0+b1)/3);
    rgb[3] = (((r0+2*r1)/3)<<16)|(((g0+2*g1)/3)<<8)|((b0+2*b1)/3);

    for (int py = 0; py < 4; py++) {
        uint16_t arow = *(uint16_t*)(aptr + py*2);
        for (int px = 0; px < 4; px++) {
            UINT x = bx+px, y = by+py;
            if (x < width && y < height) {
                int a8 = ((arow >> (px*4)) & 0xF) * 255 / 15;
                int ci = (cidx >> (2*(py*4+px))) & 3;
                dst[y*width+x] = ((uint32_t)a8<<24) | rgb[ci];
            }
        }
    }
}

//======================================================================
// WIC PNG 保存 (Phase 7)
//======================================================================
static HRESULT SaveTextureToPNG(const char* path, CTexture* pTex)
{
    if (!pTex || pTex->m_cpuData.empty()) return E_FAIL;
    UINT w = pTex->m_texWidth, h = pTex->m_texHeight;
    if (!w || !h) return E_FAIL;

    wchar_t wpath[512];
    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, 512);

    IWICImagingFactory* pWIC = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWIC));
    if (FAILED(hr)) return hr;

    IWICStream*            pStream = nullptr;
    IWICBitmapEncoder*     pEnc    = nullptr;
    IWICBitmapFrameEncode* pFrame  = nullptr;
    IPropertyBag2*         pProps  = nullptr;

    hr = pWIC->CreateStream(&pStream);
    if (SUCCEEDED(hr)) hr = pStream->InitializeFromFilename(wpath, GENERIC_WRITE);
    if (SUCCEEDED(hr)) hr = pWIC->CreateEncoder(GUID_ContainerFormatPng, nullptr, &pEnc);
    if (SUCCEEDED(hr)) hr = pEnc->Initialize(pStream, WICBitmapEncoderNoCache);
    if (SUCCEEDED(hr)) hr = pEnc->CreateNewFrame(&pFrame, &pProps);
    if (SUCCEEDED(hr)) hr = pFrame->Initialize(pProps);
    if (SUCCEEDED(hr)) hr = pFrame->SetSize(w, h);
    WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
    if (SUCCEEDED(hr)) hr = pFrame->SetPixelFormat(&fmt);
    if (SUCCEEDED(hr)) hr = pFrame->WritePixels(h, w*4, w*h*4,
                                                 (BYTE*)pTex->m_cpuData.data());
    if (SUCCEEDED(hr)) hr = pFrame->Commit();
    if (SUCCEEDED(hr)) hr = pEnc->Commit();

    if (pProps)  pProps->Release();
    if (pFrame)  pFrame->Release();
    if (pEnc)    pEnc->Release();
    if (pStream) pStream->Release();
    pWIC->Release();
    return hr;
}

//======================================================================
// コンストラクタ
//======================================================================
CArea::CArea()
{
    m_pInputLayout  = nullptr;
    m_pVS           = nullptr;
    m_pPS           = nullptr;
    m_pCB           = nullptr;
    m_pShadowTex    = nullptr;
    m_pShadowDSV    = nullptr;
    m_pShadowSRV    = nullptr;
    m_pShadowVS     = nullptr;
    m_pShadowLayout = nullptr;
    m_pShadowCB     = nullptr;
    memset(&m_shadowVP, 0, sizeof(m_shadowVP));
    StoreM(m_mLightVP, XMMatrixIdentity());
    m_mArea         = 0;
    m_VertexSize   = 0;
    m_Textures.Init();
    StoreM(m_mRootTransform, XMMatrixScaling(1.f, -1.f, 1.f));
    m_pObjInfo     = nullptr;
    m_nObj         = 0;
    m_AreaMeshs.Init();
    m_EffMeshs.Init();
}

//======================================================================
// デストラクタ
//======================================================================
CArea::~CArea()
{
    SAFE_RELEASE(m_pInputLayout);
    SAFE_RELEASE(m_pVS);
    SAFE_RELEASE(m_pPS);
    SAFE_RELEASE(m_pCB);
    ReleaseShadowMap();
    m_Textures.Release();
    CAreaMesh* pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
    while (pAreaMesh) {
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
    while (pTexture != NULL) {
        pTexture->~CTexture();
        pTexture = (CTexture*)pTexture->Next;
    }
    m_Textures.Release();
    delete[] m_pObjInfo; m_pObjInfo = NULL;
}

//======================================================================
// データ初期化
//======================================================================
void CArea::InitData(void)
{
    m_Textures.Release();
    CAreaMesh* pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
    while (pAreaMesh) {
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
    while (pTexture != NULL) {
        pTexture->~CTexture();
        pTexture = (CTexture*)pTexture->Next;
    }
    m_Textures.Release();
    delete[] m_pObjInfo; m_pObjInfo = NULL;
    m_nObj = 0;
}

//======================================================================
// テクスチャ読み込み (Phase 3: DX11 + CPU デコード)
//======================================================================
HRESULT CArea::LoadTextureFromFile(char* FileName)
{
    HRESULT hr = S_OK;
    std::unique_ptr<char[]> auto_pdat;
    char* pdat = NULL, path[512];
    int dwSize;
    unsigned long cnt;
    strcpy(path, FileName);
    if (strlen(g_texPath) > 0)
        convert_path(path, g_texPath);

    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        dwSize = GetFileSize(hFile, NULL);
        auto_pdat.reset(new char[dwSize]());
        pdat = auto_pdat.get();
        ReadFile(hFile, pdat, dwSize, &cnt, NULL);
        CloseHandle(hFile);
        hr = 0;
    } else {
        return -1;
    }

    int type, pos = 0, next;
    while (pos < dwSize) {
        next = *((int*)(pdat+pos+4)); next >>= 3; next &= 0x7ffff0;
        if (next < 16) break;
        if (next + pos > dwSize) break;
        type = *((int*)(pdat+pos+4)); type &= 0x7f;
        switch (type) {
        case 0x20: {
            CTexture* pTexture = new CTexture;
            m_Textures.InsertEnd(pTexture);

            char TexName[18];
            strncpy(TexName, pdat+pos+16+1, 16); TexName[16] = '\0';
            pTexture->SetTexName(TexName);

            UINT xx = *(UINT*)(pdat+pos+33+4);
            UINT yy = *(UINT*)(pdat+pos+33+8);
            pTexture->m_texWidth  = xx;
            pTexture->m_texHeight = yy;
            pTexture->m_cpuData.resize((size_t)xx * yy, 0);
            uint32_t* pPixels = pTexture->m_cpuData.data();

            const BYTE* pixBase = (const BYTE*)(pdat+pos+33+0x28);
            DWORD fourcc = *(DWORD*)pixBase;

            if (fourcc == 'DXT3') {
                const BYTE* blocks = pixBase + 12;
                for (UINT by = 0; by < yy; by += 4)
                    for (UINT bx = 0; bx < xx; bx += 4)
                        DecodeDXT3Block(blocks+((by/4)*(xx/4)+(bx/4))*16,
                                        pPixels, bx, by, xx, yy);
            } else if (fourcc == 'DXT1') {
                const BYTE* blocks = pixBase + 12;
                for (UINT by = 0; by < yy; by += 4)
                    for (UINT bx = 0; bx < xx; bx += 4)
                        DecodeDXT1Block(blocks+((by/4)*(xx/4)+(bx/4))*8,
                                        pPixels, bx, by, xx, yy, false);
            } else {
                const DWORD* pal = (const DWORD*)pixBase;
                const BYTE*  idx = (const BYTE*)(pixBase + 0x400);
                for (DWORD jy = 0; jy < yy; jy++)
                    for (DWORD jx = 0; jx < xx; jx++)
                        pPixels[(yy-jy-1)*xx+jx] = pal[idx[jy*xx+jx]];
            }

            D3D11_TEXTURE2D_DESC td = {};
            td.Width            = xx;
            td.Height           = yy;
            td.MipLevels        = 1;
            td.ArraySize        = 1;
            td.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage            = D3D11_USAGE_DEFAULT;
            td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA sd = {};
            sd.pSysMem     = pPixels;
            sd.SysMemPitch = xx * sizeof(uint32_t);

            ID3D11Texture2D* pTex2D = nullptr;
            hr = GetDevice11()->CreateTexture2D(&td, &sd, &pTex2D);
            if (FAILED(hr)) break;

            ID3D11ShaderResourceView* pSRV = nullptr;
            hr = GetDevice11()->CreateShaderResourceView(pTex2D, nullptr, &pSRV);
            pTex2D->Release();
            if (SUCCEEDED(hr)) {
                pTexture->SetTexture(pSRV);
                pSRV->Release();
            }
            break;
        }
        }
        pos += next;
    }
    return hr;
}

//======================================================================
// キーフレーム・エフェクト読み込み
//======================================================================
HRESULT CArea::LoadEffectFromFile(char* FileName)
{
    HRESULT hr = S_OK;
    std::unique_ptr<char[]> auto_pdat;
    char* pdat = NULL;
    int dwSize;
    CKeyFrame* pKeyFrame;
    CEffect* pEffect;
    unsigned long cnt;

    HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        dwSize = GetFileSize(hFile, NULL);
        auto_pdat.reset(new char[dwSize]());
        pdat = auto_pdat.get();
        ReadFile(hFile, pdat, dwSize, &cnt, NULL);
        CloseHandle(hFile);
        hr = 0;
    } else {
        return -1;
    }

    int type, pos = 0, next;
    while (pos < dwSize) {
        next = *((int*)(pdat+pos+4)); next >>= 3; next &= 0x7ffff0;
        if (next < 16) break;
        if (next + pos > dwSize) break;
        type = *((int*)(pdat+pos+4)); type &= 0x7f;
        switch (type) {
        case 0x19:
            pKeyFrame = new CKeyFrame;
            pKeyFrame->GetKeyFrame(pdat+pos);
            m_KeyFrames.InsertTop(pKeyFrame);
            break;
        }
        pos += next;
    }
    pos = 0;
    std::string className;
    while (pos < dwSize) {
        next = *((int*)(pdat+pos+4)); next >>= 3; next &= 0x7ffff0;
        if (next < 16) break;
        if (next + pos > dwSize) break;
        type = *((int*)(pdat+pos+4)); type &= 0x7f;
        switch (type) {
        case 0x00: className.clear(); break;
        case 0x01: className.assign(pdat+pos, 4); break;
        case 0x05:
            pEffect = new CEffect;
            pEffect->m_class = className;
            pEffect->InitData();
            pEffect->GetEffectMatrix(pdat+pos, (CKeyFrame*)m_KeyFrames.Top());
            m_Effects.InsertTop(pEffect);
            break;
        }
        pos += next;
    }
    pEffect = (CEffect*)m_Effects.Top();
    while (pEffect) {
        pEffect->m_pAreaMesh = NULL;
        CAreaMesh* pAreaMesh = (CAreaMesh*)m_EffMeshs.Top();
        while (pAreaMesh) {
            if (!memcmp(pEffect->m_target.c_str(), pAreaMesh->m_AreaType.c_str(), 4)) {
                pEffect->m_pAreaMesh = pAreaMesh;
                break;
            }
            pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
        }
        pEffect->m_pEffectModel = NULL;
        CEffectModel* pEffectModel = (CEffectModel*)m_EffectModels.Top();
        while (pEffectModel) {
            if (!memcmp(pEffect->m_target.c_str(), pEffectModel->m_type.c_str(), 4) &&
                pEffect->m_ModelType == pEffectModel->m_ModelType) {
                pEffect->m_pEffectModel = pEffectModel;
                break;
            }
            pEffectModel = (CEffectModel*)pEffectModel->Next;
        }
        pEffect = (CEffect*)pEffect->Next;
    }
    return hr;
}

//======================================================================
// エフェクトモデル読み込み (0x1F)
//======================================================================
HRESULT CArea::LoadEffectModelFromFile(char* FileName)
{
    CEffectModel* pEffectModel;
    CTexture* pTexture;
    DAT2AH* pHeader;
    HRESULT hr = S_OK;
    std::unique_ptr<char[]> auto_pdat;
    char* pdat = NULL;
    int dwSize;
    unsigned long cnt;

    HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        dwSize = GetFileSize(hFile, NULL);
        auto_pdat.reset(new char[dwSize]());
        pdat = auto_pdat.get();
        ReadFile(hFile, pdat, dwSize, &cnt, NULL);
        CloseHandle(hFile);
        hr = 0;
    } else {
        return -1;
    }

    int type, pos = 0, next;
    while (pos < dwSize) {
        next = *((int*)(pdat+pos+4)); next >>= 3; next &= 0x7ffff0;
        if (next < 16) break;
        if (next + pos > dwSize) break;
        type = *((int*)(pdat+pos+4)); type &= 0x7f;
        switch (type) {
        case 0x1F:
            pHeader = (DAT2AH*)(pdat+pos);
            pEffectModel = new CEffectModel;
            m_EffectModels.InsertTop(pEffectModel);
            pEffectModel->LoadEffectModel(pdat+pos);
            pTexture = (CTexture*)m_Textures.Top();
            {
                int texno = 0;
                while (pTexture != NULL) {
                    if (!memcmp(pEffectModel->m_Name.c_str(), pTexture->m_TexName.c_str(), 16)) {
                        pEffectModel->m_texNo    = texno;
                        pEffectModel->m_pTexture = pTexture;
                        break;
                    }
                    texno++;
                    pTexture = (CTexture*)pTexture->Next;
                }
            }
            break;
        }
        pos += next;
    }
    return hr;
}

//======================================================================
// エフェクト2モデル読み込み (0x21)
//======================================================================
HRESULT CArea::LoadEffectModel2FromFile(char* FileName)
{
    DAT2AH* pHeader;
    HRESULT hr = S_OK;
    std::unique_ptr<char[]> auto_pdat;
    char* pdat = NULL;
    int dwSize;
    unsigned long cnt;

    HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        dwSize = GetFileSize(hFile, NULL);
        auto_pdat.reset(new char[dwSize]());
        pdat = auto_pdat.get();
        ReadFile(hFile, pdat, dwSize, &cnt, NULL);
        CloseHandle(hFile);
        hr = 0;
    } else {
        return -1;
    }

    int type, pos = 0, next;
    CTexture* pTexture;
    while (pos < dwSize) {
        next = *((int*)(pdat+pos+4)); next >>= 3; next &= 0x7ffff0;
        if (next < 16) break;
        if (next + pos > dwSize) break;
        type = *((int*)(pdat+pos+4)); type &= 0x7f;
        switch (type) {
        case 0x21: {
            pHeader = (DAT2AH*)(pdat+pos);
            CEffectModel* pEffectModel = new CEffectModel;
            m_EffectModels.InsertTop(pEffectModel);
            pEffectModel->LoadEffectModel2(pdat+pos);
            if (pEffectModel->m_ModelTotal <= 0) {
                m_EffectModels.Erase(pEffectModel);
                delete pEffectModel;
            } else {
                pTexture = (CTexture*)m_Textures.Top();
                int texno = 0;
                while (pTexture != NULL) {
                    if (!memcmp(pEffectModel->m_Name.c_str(), pTexture->m_TexName.c_str(), 16)) {
                        pEffectModel->m_texNo    = texno;
                        pEffectModel->m_pTexture = pTexture;
                        break;
                    }
                    texno++;
                    pTexture = (CTexture*)pTexture->Next;
                }
            }
            break;
        }
        }
        pos += next;
    }
    return hr;
}

//======================================================================
// MMB / MZB デコード (変更なし)
//======================================================================
void CArea::DecodeMMBSub(BYTE* p)
{
    if (p[6] == 0xFF && p[7] == 0xFF) {
        int decode_length = (p[0]<<0)|(p[1]<<8)|(p[2]<<16);
        DWORD key1 = p[5] ^ 0xF0;
        DWORD key2 = key_table2[key1];
        DWORD decode_count = ((decode_length - 8) & ~0xf) / 2;
        DWORD* data1 = (DWORD*)(p+8+0);
        DWORD* data2 = (DWORD*)(p+8+decode_count);
        for (DWORD pos2 = 0; pos2 < decode_count; pos2 += 8) {
            if (key2 & 1) {
                DWORD tmp;
                tmp = data1[0]; data1[0] = data2[0]; data2[0] = tmp;
                tmp = data1[1]; data1[1] = data2[1]; data2[1] = tmp;
            }
            key1 += 9; key2 += key1;
            data1 += 2; data2 += 2;
        }
    }
}

void CArea::DecodeMMB(BYTE* p)
{
    if (p[3] >= 5) {
        int decode_length = (p[0]<<0)|(p[1]<<8)|(p[2]<<16);
        DWORD key = key_table[p[5] ^ 0xF0];
        int key_counter = 0;
        for (int pos2 = 8; pos2 < decode_length; pos2++) {
            DWORD x = ((key & 0xFF) << 8) | (key & 0xFF);
            key += ++key_counter;
            p[pos2] ^= (x >> (key & 7));
            key += ++key_counter;
        }
    }
    DecodeMMBSub(p);
}

void CArea::DecodeMZB(BYTE* p)
{
    if (p[3] >= 0x1B) {
        int decode_length = (p[0]<<0)|(p[1]<<8)|(p[2]<<16);
        DWORD key = key_table[p[7] ^ 0xFF];
        int key_counter = 0;
        for (int pos2 = 8; pos2 < decode_length; ) {
            int xor_length = ((key >> 4) & 7) + 16;
            if ((key & 1) && (pos2 + xor_length < decode_length))
                for (int i2 = 0; i2 < xor_length; i2++) p[pos2+i2] ^= 0xFF;
            key += ++key_counter;
            pos2 += xor_length;
        }
        int node_count = (p[4]<<0)|(p[5]<<8)|(p[6]<<16);
        TEMPOBJINFO* node = (TEMPOBJINFO*)(p+32);
        for (int i2 = 0; i2 < node_count; i2++) {
            for (int j = 0; j < 16; j++) node->id[j] ^= 0x55;
            node++;
        }
    }
}

//======================================================================
// 頂点シェーダー作成 (Phase 4: DX11)
//======================================================================
bool CArea::CreateVertexShader(void)
{
    HRESULT hr;
    ID3DBlob* pVSBlob  = nullptr;
    ID3DBlob* pPSBlob  = nullptr;
    ID3DBlob* pErrBlob = nullptr;

    hr = D3DCompileFromFile(L"hlsl.fx", nullptr, nullptr,
                            "VS", "vs_4_0", 0, 0, &pVSBlob, &pErrBlob);
    if (FAILED(hr)) { if (pErrBlob) pErrBlob->Release(); return false; }

    hr = GetDevice11()->CreateVertexShader(pVSBlob->GetBufferPointer(),
                                           pVSBlob->GetBufferSize(),
                                           nullptr, &m_pVS);
    if (FAILED(hr)) { pVSBlob->Release(); return false; }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM,   0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = GetDevice11()->CreateInputLayout(layout, 4,
                                          pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(),
                                          &m_pInputLayout);
    pVSBlob->Release();
    if (FAILED(hr)) return false;

    hr = D3DCompileFromFile(L"hlsl.fx", nullptr, nullptr,
                            "PS", "ps_4_0", 0, 0, &pPSBlob, &pErrBlob);
    if (FAILED(hr)) { if (pErrBlob) pErrBlob->Release(); return false; }

    hr = GetDevice11()->CreatePixelShader(pPSBlob->GetBufferPointer(),
                                          pPSBlob->GetBufferSize(),
                                          nullptr, &m_pPS);
    pPSBlob->Release();
    if (FAILED(hr)) return false;

    hr = CreateBuffer11(&m_pCB, sizeof(CBData), D3D11_BIND_CONSTANT_BUFFER);
    return SUCCEEDED(hr);
}

//======================================================================
// シャドウマップ初期化
//======================================================================
bool CArea::InitShadowMap()
{
    ID3D11Device* dev = GetDevice11();

    // 深度テクスチャ (R32_TYPELESS: DSV と SRV を同時バインド可)
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = td.Height  = SHADOW_MAP_SIZE;
    td.MipLevels = td.ArraySize = 1;
    td.Format         = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count = 1;
    td.BindFlags      = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &m_pShadowTex))) return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC dvd = {};
    dvd.Format        = DXGI_FORMAT_D32_FLOAT;
    dvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(dev->CreateDepthStencilView(m_pShadowTex, &dvd, &m_pShadowDSV))) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvd.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels       = 1;
    if (FAILED(dev->CreateShaderResourceView(m_pShadowTex, &srvd, &m_pShadowSRV))) return false;

    // shadow.fx のコンパイルと VS 生成
    ID3DBlob* pVSBlob  = nullptr;
    ID3DBlob* pErrBlob = nullptr;
    if (FAILED(D3DCompileFromFile(L"shadow.fx", nullptr, nullptr,
                                  "VS", "vs_4_0", 0, 0, &pVSBlob, &pErrBlob))) {
        if (pErrBlob) pErrBlob->Release();
        return false;
    }
    HRESULT hr = dev->CreateVertexShader(pVSBlob->GetBufferPointer(),
                                         pVSBlob->GetBufferSize(), nullptr, &m_pShadowVS);
    if (FAILED(hr)) { pVSBlob->Release(); return false; }

    // POSITION のみの入力レイアウト (頂点バッファの先頭 float3 を使用)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = dev->CreateInputLayout(layout, 1,
                                pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
                                &m_pShadowLayout);
    pVSBlob->Release();
    if (FAILED(hr)) return false;

    hr = CreateBuffer11(&m_pShadowCB, sizeof(XMFLOAT4X4), D3D11_BIND_CONSTANT_BUFFER);
    if (FAILED(hr)) return false;

    m_shadowVP = { 0.f, 0.f, (float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE, 0.f, 1.f };
    return true;
}

//======================================================================
// シャドウパス (ライト視点からの深度書き込み)
//======================================================================
void CArea::ShadowPass(float PosX, float PosZ)
{
    if (!m_pShadowVS || !m_pShadowDSV || !m_pShadowCB) return;

    ID3D11DeviceContext* ctx = GetContext();

    // ライトの VP 行列を計算 (平行光源 → 正射影)
    XMVECTOR lightDir    = XMVector3Normalize(LoadV(g_mLightDir));
    XMVECTOR sceneCenter = XMVectorSet(PosX, 20.f, PosZ, 1.f);
    XMVECTOR lightPos    = sceneCenter - lightDir * 600.f;

    XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    if (fabsf(XMVectorGetY(lightDir)) > 0.99f)
        up = XMVectorSet(0.f, 0.f, 1.f, 0.f);

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, sceneCenter, up);
    XMMATRIX lightProj = XMMatrixOrthographicLH(1000.f, 1000.f, 1.f, 1400.f);
    XMMATRIX lightVP   = XMMatrixMultiply(lightView, lightProj);
    StoreM(m_mLightVP, lightVP);

    // 前フレームの shadow SRV をアンバインドしてから DSV にセット
    ID3D11ShaderResourceView* nullSrv = nullptr;
    ctx->PSSetShaderResources(1, 1, &nullSrv);

    UINT numVP = 1;
    D3D11_VIEWPORT savedVP;
    ctx->RSGetViewports(&numVP, &savedVP);

    ctx->OMSetRenderTargets(0, nullptr, m_pShadowDSV);
    ctx->ClearDepthStencilView(m_pShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->RSSetViewports(1, &m_shadowVP);
    ctx->IASetInputLayout(m_pShadowLayout);
    ctx->VSSetShader(m_pShadowVS, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &m_pShadowCB);
    ctx->RSSetState(GetRastCCW());

    XMMATRIX rootWorld = LoadM(m_mRootTransform);

    for (int i = 0; i < m_nObj; i++) {
        CAreaMesh* pAreaMesh = m_pObjInfo[i].pAreaMesh;
        if (!pAreaMesh) continue;

        XMMATRIX world = XMMatrixMultiply(LoadM(m_pObjInfo[i].mMat), rootWorld);
        XMMATRIX wvp   = XMMatrixMultiply(world, lightVP);

        XMFLOAT4X4 wvpT;
        XMStoreFloat4x4(&wvpT, XMMatrixTranspose(wvp));

        D3D11_MAPPED_SUBRESOURCE msr = {};
        ctx->Map(m_pShadowCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        memcpy(msr.pData, &wvpT, sizeof(XMFLOAT4X4));
        ctx->Unmap(m_pShadowCB, 0);

        UINT stride = sizeof(D3DTEXVERTEX), offset = 0;
        ID3D11Buffer* vb = pAreaMesh->GetlpVB();
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(pAreaMesh->GetlpIB(), DXGI_FORMAT_R16_UINT, 0);

        for (auto& s : pAreaMesh->m_LStreams) {
            ctx->IASetPrimitiveTopology(s.GetPrimitiveType());
            ctx->DrawIndexed((UINT)s.GetFaceCount() + 2, (UINT)s.GetIndexStart(), 0);
        }
    }

    // メインパスのレンダーターゲットとビューポートを復元
    ID3D11RenderTargetView* rtv = GetRenderTargetView();
    ctx->OMSetRenderTargets(1, &rtv, GetDepthStencilView());
    ctx->RSSetViewports(1, &savedVP);
}

//======================================================================
// シャドウマップ解放
//======================================================================
void CArea::ReleaseShadowMap()
{
    SAFE_RELEASE(m_pShadowCB);
    SAFE_RELEASE(m_pShadowLayout);
    SAFE_RELEASE(m_pShadowVS);
    SAFE_RELEASE(m_pShadowSRV);
    SAFE_RELEASE(m_pShadowDSV);
    SAFE_RELEASE(m_pShadowTex);
}

//======================================================================
// エリアメッシュ読み込み (Phase 6: DirectXMath)
//======================================================================
HRESULT CArea::LoadAreaFromFile(char* FileName, unsigned long FVF)
{
    std::string    MeshName;
    TEMPOBJINFO*   pTobj;
    CAreaMesh*     pAreaMesh;
    HRESULT        hr = S_OK;
    unsigned long  cnt;
    std::unique_ptr<char[]> auto_pFileBuf;
    char* pFileBuf = NULL, path[512];
    int   mFileSize = 0;

    strcpy(path, FileName);
    if (strlen(g_meshPath) > 0)
        convert_path(path, g_meshPath);

    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        mFileSize = GetFileSize(hFile, NULL);
        auto_pFileBuf.reset(new char[mFileSize]());
        pFileBuf = auto_pFileBuf.get();
        hr = ReadFile(hFile, pFileBuf, mFileSize, &cnt, NULL);
        CloseHandle(hFile);
        hr = 0;
    } else {
        return -1;
    }

    int mzbcnt = 0, i, type, pos = 0, next;
    while (pos < mFileSize) {
        next = *((int*)(pFileBuf+pos+4)); next >>= 3; next &= 0x7ffff0;
        if (next < 16) break;
        if (next + pos > mFileSize) break;
        type = *((int*)(pFileBuf+pos+4)); type &= 0x7f;
        switch (type) {
        case 0x1c:
            DecodeMZB((BYTE*)&pFileBuf[pos+16]);
            if (mzbcnt == 0) {
                m_nObj = (*(int*)(pFileBuf+pos+4+16)) & 0xffffff;
                m_pObjInfo = new OBJINFO[m_nObj]();
                pTobj = (TEMPOBJINFO*)(pFileBuf+pos+32+16);
                for (i = 0; i < m_nObj; i++, pTobj++)
                    memcpy(&m_pObjInfo[i], pTobj, sizeof(TEMPOBJINFO));
            } else {
                int nObj = (*(int*)(pFileBuf+pos+4+16)) & 0xffffff;
                OBJINFO* pObjInfo = new OBJINFO[m_nObj + nObj]();
                pTobj = (TEMPOBJINFO*)(pFileBuf+pos+32+16);
                memcpy(pObjInfo, m_pObjInfo, sizeof(OBJINFO)*m_nObj);
                for (i = 0; i < nObj; i++, pTobj++)
                    memcpy(pObjInfo+m_nObj+i, pTobj, sizeof(TEMPOBJINFO));
                delete[] m_pObjInfo; m_pObjInfo = NULL;
                m_pObjInfo = pObjInfo;
                m_nObj += nObj;
            }
            mzbcnt++;
            break;
        }
        pos += next;
    }

    pos = 0;
    while (pos < mFileSize) {
        next = *((int*)(pFileBuf+pos+4)); next >>= 3; next &= 0x7ffff0;
        if (next < 16) break;
        if (next + pos > mFileSize) break;
        type = *((int*)(pFileBuf+pos+4)); type &= 0x7f;
        switch (type) {
        case 0x2e:
            DecodeMMB((BYTE*)&pFileBuf[pos+16]);
            MeshName.assign(pFileBuf+pos+32, 16);
            for (i = 0; i < m_nObj; i++)
                if (!memcmp(MeshName.c_str(), m_pObjInfo[i].mObj.id, 16)) break;
            {
                pAreaMesh = new CAreaMesh;
                if (!pAreaMesh) return -1;
                pAreaMesh->m_AreaName.assign(pFileBuf+pos+32, 16);
                pAreaMesh->LoadAreaMesh(pFileBuf+pos, this, FVF);
                if (!pAreaMesh->GetlpVB() || !pAreaMesh->GetlpIB()) {
                    delete pAreaMesh;
                } else if (i >= m_nObj) {
                    m_EffMeshs.InsertEnd(pAreaMesh);
                } else {
                    m_AreaMeshs.InsertEnd(pAreaMesh);
                }
            }
            break;
        }
        pos += next;
    }

    for (i = 0; i < m_nObj; i++) {
        float sx = m_pObjInfo[i].mObj.fScaleX;
        float sy = m_pObjInfo[i].mObj.fScaleY;
        float sz = m_pObjInfo[i].mObj.fScaleZ;
        float rx = m_pObjInfo[i].mObj.fRotX;
        float ry = m_pObjInfo[i].mObj.fRotY;
        float rz = m_pObjInfo[i].mObj.fRotZ;
        float tx = m_pObjInfo[i].mObj.fTransX;
        float ty = m_pObjInfo[i].mObj.fTransY;
        float tz = m_pObjInfo[i].mObj.fTransZ;

        XMMATRIX xm = XMMatrixIdentity();
        xm = XMMatrixMultiply(xm, XMMatrixScaling(sx, sy, sz));
        xm = XMMatrixMultiply(xm, XMMatrixRotationX(rx));
        xm = XMMatrixMultiply(xm, XMMatrixRotationY(ry));
        xm = XMMatrixMultiply(xm, XMMatrixRotationZ(rz));
        xm = XMMatrixMultiply(xm, XMMatrixTranslation(tx, ty, tz));
        StoreM(m_pObjInfo[i].mMat, xm);

        m_pObjInfo[i].pAreaMesh = NULL;
        pAreaMesh = (CAreaMesh*)m_AreaMeshs.Top();
        while (pAreaMesh) {
            if (!memcmp(pAreaMesh->m_AreaName.c_str(), m_pObjInfo[i].mObj.id, 16)) {
                m_pObjInfo[i].pAreaMesh = pAreaMesh;
                break;
            }
            pAreaMesh = (CAreaMesh*)pAreaMesh->Next;
        }
        if (!pAreaMesh) continue;
        if ((m_pObjInfo[i].mObj.fe & 0xfff0fff0) != 0) continue;
    }
    return hr;
}

//======================================================================
// レンダリング (Phase 5: DX11)
//======================================================================
unsigned long CArea::Rendering(float PosX, float PosY, float PosZ, float alphaRef)
{
    if (!m_pVS || !m_pPS || !m_pInputLayout || !m_pCB) return 0;

    // シャドウパス (メインパスより前に実行)
    ShadowPass(PosX, PosZ);

    CAreaMesh* pAreaMesh;
    float DispArea;
    unsigned long count = 0;
    ID3D11DeviceContext* ctx = GetContext();

    ctx->VSSetShader(m_pVS, nullptr, 0);
    ctx->PSSetShader(m_pPS, nullptr, 0);
    ctx->IASetInputLayout(m_pInputLayout);
    ctx->VSSetConstantBuffers(0, 1, &m_pCB);
    ctx->PSSetConstantBuffers(0, 1, &m_pCB);
    ID3D11SamplerState* samp = GetSampler();
    ctx->PSSetSamplers(0, 1, &samp);
    ctx->OMSetDepthStencilState(GetDSSNormal(), 0);

    XMMATRIX view = LoadM(g_mView);
    XMMATRIX proj = LoadM(g_mProjection);

    for (int i = 0; i < m_nObj; i++) {
        if ((m_pObjInfo[i].mObj.fe & 0xfff0ffff) == 0)
            DispArea = g_mDispArea;
        else
            DispArea = g_mDispTree;

        if ((pAreaMesh = m_pObjInfo[i].pAreaMesh) == NULL) continue;

        XMMATRIX world = XMMatrixMultiply(LoadM(m_pObjInfo[i].mMat),
                                          LoadM(m_mRootTransform));

        // バウンディングボックスカリング
        XMFLOAT3 BL  = pAreaMesh->GetBoxLow();
        XMFLOAT3 BH  = pAreaMesh->GetBoxHigh();
        XMFLOAT3 BL2 = { BL.x, BL.y, BH.z };
        XMFLOAT3 BH2 = { BH.x, BH.y, BL.z };
        StoreV(BL,  XMVector3TransformCoord(LoadV(BL),  world));
        StoreV(BH,  XMVector3TransformCoord(LoadV(BH),  world));
        StoreV(BL2, XMVector3TransformCoord(LoadV(BL2), world));
        StoreV(BH2, XMVector3TransformCoord(LoadV(BH2), world));
        if (Min4(BL.x,BH.x,BL2.x,BH2.x) > PosX+DispArea) continue;
        if (Max4(BL.x,BH.x,BL2.x,BH2.x) < PosX-DispArea) continue;
        if (Min4(BL.z,BH.z,BL2.z,BH2.z) > PosZ+DispArea) continue;
        if (Max4(BL.z,BH.z,BL2.z,BH2.z) < PosZ-DispArea) continue;

        UINT stride = sizeof(D3DTEXVERTEX), offset = 0;
        ID3D11Buffer* vb = pAreaMesh->GetlpVB();
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(pAreaMesh->GetlpIB(), DXGI_FORMAT_R16_UINT, 0);

        XMMATRIX wvp = XMMatrixMultiply(world, XMMatrixMultiply(view, proj));
        bool isMirror = IsMirrorMatrix(&m_pObjInfo[i].mMat);

        for (auto& s : pAreaMesh->m_LStreams) {
            // ラスタライザ (DX9互換: プリミティブ型で分岐)
            auto topo = s.GetPrimitiveType();
            bool isTriangle = (topo == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP ||
                               topo == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            if (isTriangle) {
                ctx->RSSetState(isMirror ? GetRastCW() : GetRastCCW());
            } else {
                bool scaleMirror = (m_pObjInfo[i].mObj.fScaleX *
                                    m_pObjInfo[i].mObj.fScaleZ) < 0.f;
                if (s.GetAlphaFlag() & 0x02 || s.GetStencilFlag())
                    ctx->RSSetState(GetRastNone());
                else
                    ctx->RSSetState(scaleMirror ? GetRastCW() : GetRastCCW());
            }

            // ブレンド
            float bf[4] = {};
            if ((s.GetAlphaFlag() & 0x08) && !s.GetStencilFlag())
                ctx->OMSetBlendState(GetBlendAlpha(), bf, 0xFFFFFFFF);
            else
                ctx->OMSetBlendState(GetBlendNone(), bf, 0xFFFFFFFF);

            // 定数バッファ更新
            D3D11_MAPPED_SUBRESOURCE msr = {};
            ctx->Map(m_pCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
            CBData* cb = (CBData*)msr.pData;
            XMStoreFloat4x4(&cb->mWVP, XMMatrixTranspose(wvp));
            cb->mUV[0] = 0.f; cb->mUV[1] = 0.f;
            cb->padding[0] = s.GetStencilFlag() ? alphaRef : 0.0f; cb->padding[1] = 0.f;
            cb->mCOL = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
            XMStoreFloat4x4(&cb->mW, XMMatrixTranspose(world));
            cb->mLightDir     = XMFLOAT4(g_mLightDir.x, g_mLightDir.y, g_mLightDir.z, 0.f);
            cb->mLightColor   = XMFLOAT4(1.0f, 0.95f, 0.8f, 1.f);
            cb->mAmbientColor = XMFLOAT4(0.2f, 0.22f, 0.3f, 1.f);
            cb->mCameraPos    = XMFLOAT4(g_mEye.x, g_mEye.y, g_mEye.z, 1.f);
            cb->mSpecular     = XMFLOAT4(32.f, 0.3f, 0.f, 0.f);
            XMStoreFloat4x4(&cb->mLightVP, XMMatrixTranspose(LoadM(m_mLightVP)));
            cb->mShadow       = XMFLOAT4(0.002f, -1.0f, 1.0f / SHADOW_MAP_SIZE, 0.f);
            ctx->Unmap(m_pCB, 0);

            // テクスチャ
            CTexture* pTex = s.GetpTexture();
            ID3D11ShaderResourceView* srv = pTex ? pTex->GetTexture() : nullptr;
            ctx->PSSetShaderResources(0, 1, &srv);
            ctx->PSSetShaderResources(1, 1, &m_pShadowSRV); // t1: shadow map
            ID3D11SamplerState* shadowSam = GetShadowSampler();
            ctx->PSSetSamplers(1, 1, &shadowSam);            // s1: 比較サンプラー

            ctx->IASetPrimitiveTopology(s.GetPrimitiveType());
            ctx->DrawIndexed((UINT)s.GetFaceCount() + 2, (UINT)s.GetIndexStart(), 0);
        }
        count += pAreaMesh->GetNumFaces();
    }

    float bf[4] = {};
    ctx->OMSetBlendState(GetBlendNone(), bf, 0xFFFFFFFF);
    return count;
}

//======================================================================
// MAPデータ読み込み
//======================================================================
bool CArea::LoadMAP()
{
    unsigned long FVF = 0x152; // XYZ|NORMAL|DIFFUSE|TEX1
    char FileName[512], FileName2[512];

    InitData();
    if ((GetArea() & 0xeffff) == 0) return true;

    GetFileNameFromDno(FileName,  GetArea());
    GetFileNameFromDno(FileName2, ConvertStr2Dno("0-0"));

    HRESULT hr;
    hr = LoadTextureFromFile(FileName2); if (hr) return false;
    hr = LoadTextureFromFile(FileName);  if (hr) return false;
    hr = LoadAreaFromFile(FileName, FVF); if (hr) return false;
    InitEffectModel();
    LoadEffectModelFromFile(FileName);
    LoadEffectModelFromFile(FileName2);
    LoadEffectModel2FromFile(FileName);
    LoadEffectModel2FromFile(FileName2);
    InitEffect();
    LoadEffectFromFile(FileName);
    return true;
}

//======================================================================
// MQO セーブ共通ヘルパー
//======================================================================
void CArea::PrepareMQOPath(char* path, char* dirPath, const char* FPath, const char* FName)
{
    char* ptr;
    char tempFName[256];
    strcpy(dirPath, FPath);
    strcpy(tempFName, FName);
    if ((ptr = strrstr(dirPath, tempFName))) *ptr = '\0';
    if ((ptr = strrstr(tempFName, ".mqo"))) *ptr = '\0';
    sprintf(path, "%s%s.mqo", dirPath, tempFName);
}

void CArea::WriteMQOHeader(FILE* fd, CList& textures, const char* dirPath, bool isEffectModel)
{
    fprintf(fd, "Metasequoia Document\nFormat Text Ver 1.0\n\nScene {\n");
    fprintf(fd, "	pos 0.0000 0.0000 5000.0000\n");
    fprintf(fd, "	lookat 0.0000 0.0000 0.0000\n");
    fprintf(fd, "	head 0.0000\n");
    fprintf(fd, "	pich 0.0000\n");
    fprintf(fd, "	ortho 1\n");
    fprintf(fd, "	zoom2 2.0000\n");
    fprintf(fd, "	amb 0.250 0.250 0.250\n}\nMaterial ");
    fprintf(fd, "%d {\n", textures.Count);
    CTexture* pTexture = (CTexture*)textures.Top();
    while (pTexture != NULL) {
        char texName[256];
        if (isEffectModel)
            strcpy(texName, pTexture->m_TexName.c_str());
        else
            strcpy(texName, pTexture->GetTexName());
        Trim(texName);
        if (isEffectModel)
            fprintf(fd, "    \"%s\" col(1.000 1.000 1.000 1.000)", texName);
        else
            fprintf(fd, "    \"%s\" shader(3) dbls(1) col(1.000 1.000 1.000 1.000)", texName);
        fprintf(fd, " dif(1.000) amb(0.250) emi(0.250) spc(0.000) power(5.00) tex(\"%s.png\")\n", texName);
        char texpath[256];
        sprintf(texpath, "%s%s.png", dirPath, texName);
        SaveTextureToPNG(texpath, pTexture);
        pTexture = (CTexture*)pTexture->Next;
    }
    fprintf(fd, "}\n");
}

void CArea::WriteMQOAreaMesh(FILE* fd, CAreaMesh* pAreaMesh,
                             const XMFLOAT4X4& AreaMatrix,
                             bool useMirrorLogic, bool isEffect)
{
    char areaNameBuf[18];
    strncpy(areaNameBuf, pAreaMesh->m_AreaName.c_str(), 17);
    areaNameBuf[17] = '\0';
    for (int i = 0; i < 18; i++)
        if (areaNameBuf[i] == 0x20) areaNameBuf[i] = 0;

    const D3DTEXVERTEX* pV     = reinterpret_cast<const D3DTEXVERTEX*>(pAreaMesh->m_cpuVB.data());
    const WORD*         pIndex = reinterpret_cast<const WORD*>(pAreaMesh->m_cpuIB.data());

    list<CStream>::iterator its2 = pAreaMesh->m_LStreams.begin();
    list<CStream>::iterator ite2 = pAreaMesh->m_LStreams.end();
    for (int count = 0; its2 != ite2; its2++, count++) {
        fprintf(fd, "Object \"%s%02d\" {\n", areaNameBuf, count);
        fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 1.000 1.000 1.000\n   color_type 0\n");

        const WORD* pI = pIndex + its2->GetIndexStart();
        int idxmin = 65535, idxmax = 0;
        for (unsigned int i = 0; i < its2->GetFaceCount()+2; i++) {
            WORD i1 = *pI++;
            if (i1 > idxmax) idxmax = i1;
            if (i1 < idxmin) idxmin = i1;
        }
        fprintf(fd, "vertex %d {\n", idxmax - idxmin + 1);
        for (unsigned long verCnt = idxmin; verCnt <= (unsigned long)idxmax; verCnt++) {
            XMFLOAT3 mVer;
            StoreV(mVer, XMVector3TransformCoord(LoadV((pV+verCnt)->v), LoadM(AreaMatrix)));
            if (isEffect)
                fprintf(fd, "        %4.4f %4.4f %4.4f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
            else
                fprintf(fd, "        %5.5f %5.5f %5.5f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
        }
        fprintf(fd, "\t}\n");

        if (its2->GetPrimitiveType() == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP) {
            pI = pIndex + its2->GetIndexStart();
            int nFace = 0;
            for (int nVer = 0; nVer < (int)its2->GetFaceCount()+2; ) {
                int i1 = *pI++ - idxmin; nVer++;
                int i2 = *pI++ - idxmin; nVer++;
                while (nVer < (int)its2->GetFaceCount()+2) {
                    int i3 = *pI++ - idxmin; nVer++;
                    if (i2 == i3) { pI++; nVer++; break; }
                    nFace++; i1 = i2; i2 = i3;
                }
            }
            fprintf(fd, "face %d {\n", nFace);
            pI = pIndex + its2->GetIndexStart();
            for (int nVer = 0; nVer < (int)its2->GetFaceCount()+2; ) {
                int i1 = (*pI++) - idxmin; nVer++;
                int i2 = (*pI++) - idxmin; nVer++;
                while (nVer < (int)its2->GetFaceCount()+2) {
                    int i3 = (*pI++) - idxmin; nVer++;
                    if (i2 == i3) { pI++; nVer++; break; }
                    int t1, t2, t3;
                    if (nVer % 2) { t1 = i3; t2 = i2; t3 = i1; }
                    else          { t1 = i1; t2 = i2; t3 = i3; }
                    if (useMirrorLogic && IsMirrorMatrix(&AreaMatrix)) {
                        fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
                            t1,t2,t3, its2->m_TexNo,
                            (pV+idxmin+t1)->tu,(pV+idxmin+t1)->tv,
                            (pV+idxmin+t2)->tu,(pV+idxmin+t2)->tv,
                            (pV+idxmin+t3)->tu,(pV+idxmin+t3)->tv);
                    } else if (useMirrorLogic) {
                        fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
                            t1,t3,t2, its2->m_TexNo,
                            (pV+idxmin+t1)->tu,(pV+idxmin+t1)->tv,
                            (pV+idxmin+t3)->tu,(pV+idxmin+t3)->tv,
                            (pV+idxmin+t2)->tu,(pV+idxmin+t2)->tv);
                    } else {
                        fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
                            t1,t2,t3, its2->m_TexNo,
                            (pV+idxmin+t1)->tu,(pV+idxmin+t1)->tv,
                            (pV+idxmin+t2)->tu,(pV+idxmin+t2)->tv,
                            (pV+idxmin+t3)->tu,(pV+idxmin+t3)->tv);
                    }
                    i1 = i2; i2 = i3;
                }
            }
            fprintf(fd, "\t}\n");
        } else if (its2->GetPrimitiveType() == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
            fprintf(fd, "face %d {\n", its2->GetFaceCount());
            pI = pIndex + its2->GetIndexStart();
            for (unsigned int i = 0; i < its2->GetFaceCount(); i++) {
                int i1 = (*pI++)-idxmin, i2 = (*pI++)-idxmin, i3 = (*pI++)-idxmin;
                int t1 = i3, t2 = i2, t3 = i1;
                if (useMirrorLogic && IsMirrorMatrix(&AreaMatrix)) {
                    fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
                        t1,t2,t3, its2->m_TexNo,
                        (pV+idxmin+t1)->tu,(pV+idxmin+t1)->tv,
                        (pV+idxmin+t2)->tu,(pV+idxmin+t2)->tv,
                        (pV+idxmin+t3)->tu,(pV+idxmin+t3)->tv);
                } else if (useMirrorLogic) {
                    fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
                        t1,t3,t2, its2->m_TexNo,
                        (pV+idxmin+t1)->tu,(pV+idxmin+t1)->tv,
                        (pV+idxmin+t3)->tu,(pV+idxmin+t3)->tv,
                        (pV+idxmin+t2)->tu,(pV+idxmin+t2)->tv);
                } else {
                    fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
                        t1,t2,t3, its2->m_TexNo,
                        (pV+idxmin+t1)->tu,(pV+idxmin+t1)->tv,
                        (pV+idxmin+t2)->tu,(pV+idxmin+t2)->tv,
                        (pV+idxmin+t3)->tu,(pV+idxmin+t3)->tv);
                }
            }
            fprintf(fd, "\t}\n");
        }
        fprintf(fd, "}\n");
    }
}

void CArea::WriteMQOEffectModel(FILE* fd, CEffectModel* pEffMdl,
                                const XMFLOAT4X4& EffectMatrix)
{
    char objName[256];
    strcpy(objName, pEffMdl->m_Name.c_str()); Trim(objName);
    fprintf(fd, "Object \"%s\" {\n", objName);
    fprintf(fd, "    visivle 15\n    locking 0\n    shading 1\n   facet 59.5\n    color 0.898 0.498 0.698\n   color_type 0\n");

    const EFFECTVERTEX* pVertex = reinterpret_cast<const EFFECTVERTEX*>(pEffMdl->m_cpuVB.data());
    const WORD*         pIndex  = reinterpret_cast<const WORD*>(pEffMdl->m_cpuIB.data());

    fprintf(fd, "vertex %d {\n", pEffMdl->m_NumVertices);
    for (unsigned long i = 0; i <= pEffMdl->m_NumVertices; i++) {
        XMFLOAT3 pos = { pVertex[i].x, pVertex[i].y, pVertex[i].z };
        XMFLOAT3 mVer;
        StoreV(mVer, XMVector3TransformCoord(LoadV(pos), LoadM(EffectMatrix)));
        fprintf(fd, "        %5.5f %5.5f %5.5f\n", mVer.x*10., mVer.y*10., mVer.z*10.);
    }
    fprintf(fd, "\t}\n");

    fprintf(fd, "face %d {\n", pEffMdl->m_NumFaces);
    const WORD* pI = pIndex;
    for (unsigned long i = 0; i < pEffMdl->m_NumFaces; i++) {
        int i1 = *pI++, i2 = *pI++, i3 = *pI++;
        int t1 = i3, t2 = i2, t3 = i1;
        fprintf(fd, "\t\t3 V(%3d %3d %3d) M(%2d) UV(%1.5f %1.5f %1.5f %1.5f %1.5f %1.5f)\n",
            t1,t2,t3, pEffMdl->m_texNo,
            pVertex[t1].u, pVertex[t1].v, pVertex[t2].u,
            pVertex[t2].v, pVertex[t3].u, pVertex[t3].v);
    }
    fprintf(fd, "\t}\n}\n");
}

//======================================================================
// MQOセーブ — 通常エリアデータ
//======================================================================
bool CArea::saveMQO(char* FPath, char* FName, float posX, float posY, float posZ)
{
    FILE* fd;
    char  path[256], dirPath[256];
    CAreaMesh* pAreaMesh;
    float DispArea;

    XMFLOAT4X4 RootMatrix;
    StoreM(RootMatrix, XMMatrixRotationZ(PAI));
    PrepareMQOPath(path, dirPath, FPath, FName);
    if ((fd = fopen(path, "w")) == NULL) return false;
    WriteMQOHeader(fd, m_Textures, dirPath, false);

    for (int num = 0; num < m_nObj; num++) {
        if ((m_pObjInfo[num].mObj.fe & 0xfff0ffff) == 0)
            DispArea = g_mDispArea;
        else
            DispArea = g_mDispTree;
        if ((pAreaMesh = m_pObjInfo[num].pAreaMesh) == NULL) continue;

        XMFLOAT4X4 AreaMatrix = m_pObjInfo[num].mMat;
        StoreM(AreaMatrix, XMMatrixMultiply(LoadM(AreaMatrix), LoadM(RootMatrix)));

        XMFLOAT3 BL  = pAreaMesh->GetBoxLow();
        XMFLOAT3 BH  = pAreaMesh->GetBoxHigh();
        XMFLOAT3 BL2 = { BL.x, BL.y, BH.z };
        XMFLOAT3 BH2 = { BH.x, BH.y, BL.z };
        XMMATRIX am = LoadM(AreaMatrix);
        StoreV(BL,  XMVector3TransformCoord(LoadV(BL),  am));
        StoreV(BH,  XMVector3TransformCoord(LoadV(BH),  am));
        StoreV(BL2, XMVector3TransformCoord(LoadV(BL2), am));
        StoreV(BH2, XMVector3TransformCoord(LoadV(BH2), am));
        if (Min4(BL.x,BH.x,BL2.x,BH2.x) > posX+DispArea) continue;
        if (Max4(BL.x,BH.x,BL2.x,BH2.x) < posX-DispArea) continue;
        if (Min4(BL.z,BH.z,BL2.z,BH2.z) > posZ+DispArea) continue;
        if (Max4(BL.z,BH.z,BL2.z,BH2.z) < posZ-DispArea) continue;

        WriteMQOAreaMesh(fd, pAreaMesh, AreaMatrix, true, false);
    }
    fprintf(fd, "EOF");
    fclose(fd);
    return true;
}

//======================================================================
// MQOセーブ — エフェクトエリアデータ
//======================================================================
bool CArea::saveMQO2(char* FPath, char* FName, float posX, float posY, float posZ)
{
    FILE* fd;
    char  path[256], dirPath[256];
    CEffect* pEffect;
    CAreaMesh* pAreaMesh;

    XMFLOAT4X4 RootMatrix;
    StoreM(RootMatrix, XMMatrixRotationZ(PAI));
    PrepareMQOPath(path, dirPath, FPath, FName);
    if ((fd = fopen(path, "w")) == NULL) return false;
    WriteMQOHeader(fd, m_Textures, dirPath, false);

    pEffect = (CEffect*)m_Effects.Top();
    while (pEffect) {
        if (memcmp(pEffect->m_class.c_str(), g_className, 4)) {
            pEffect = (CEffect*)pEffect->Next; continue;
        }
        if ((pAreaMesh = pEffect->m_pAreaMesh) == NULL) {
            pEffect = (CEffect*)pEffect->Next; continue;
        }
        XMFLOAT4X4 AreaMatrix = pEffect->m_mRootTransform;
        StoreM(AreaMatrix, XMMatrixMultiply(LoadM(AreaMatrix), LoadM(RootMatrix)));
        WriteMQOAreaMesh(fd, pAreaMesh, AreaMatrix, false, true);
        pEffect = (CEffect*)pEffect->Next;
    }
    fprintf(fd, "EOF");
    fclose(fd);
    return true;
}

//======================================================================
// MQOセーブ — エフェクトモデルデータ
//======================================================================
bool CArea::saveMQO3(char* FPath, char* FName)
{
    FILE* fd;
    char  path[256], dirPath[256];
    CEffect* pEffect;
    CEffectModel* pEffMdl;

    XMFLOAT4X4 RootMatrix;
    StoreM(RootMatrix, XMMatrixRotationZ(PAI));
    PrepareMQOPath(path, dirPath, FPath, FName);
    if ((fd = fopen(path, "w")) == NULL) return false;
    WriteMQOHeader(fd, m_Textures, dirPath, true);

    pEffect = (CEffect*)m_Effects.Top();
    while (pEffect) {
        if (memcmp(pEffect->m_class.c_str(), g_className, 4)) {
            pEffect = (CEffect*)pEffect->Next; continue;
        }
        if ((pEffMdl = pEffect->m_pEffectModel) == NULL) {
            pEffect = (CEffect*)pEffect->Next; continue;
        }
        XMFLOAT4X4 EffectMatrix = pEffect->m_mRootTransform;
        StoreM(EffectMatrix, XMMatrixMultiply(LoadM(EffectMatrix), LoadM(RootMatrix)));
        WriteMQOEffectModel(fd, pEffMdl, EffectMatrix);
        pEffect = (CEffect*)pEffect->Next;
    }
    fprintf(fd, "EOF");
    fclose(fd);
    return true;
}
