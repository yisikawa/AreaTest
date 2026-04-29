

//======================================================================
// INCLUDE
//======================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "WinMain.h"
#include "Render.h"
#include "Area.h"

// WinMain.cpp で定義されている変換関数の前方宣言
extern DWORD ConvertStr2Dno2(char* DataName);

//======================================================================
// DEFINE
//======================================================================
#define SAFE_RELEASE(p)     if ((p) != NULL) { (p)->Release(); (p) = NULL; }
#define PAI                 (3.1415926535897932384626433832795f)
#define PAI2                (PAI * 2.0f)

//======================================================================
// GLOBAL  (Phase6: D3DXMATRIX/D3DXVECTOR3 → XMFLOAT4X4/XMFLOAT3)
//======================================================================
float       g_mTime             = 0.f;
        CArea       g_mArea;
extern  HWND        hDlg1, hTrack;
extern  float       g_mDispArea;
extern  float       g_mDispTree;
float               g_mFov          = PAI / 4.f;
float               g_mAspect		= 1.78f;
float               g_mNear_z       = 0.1f;
float               g_mFar_z        = 1400.0f;

// ライト方向 (WinMain.cpp がマウス操作で更新, シェーダーには未使用)
XMFLOAT3    g_mLightDir     = { 0.3f, -1.0f, 0.3f };
XMFLOAT3    g_mLightDirBase = { 0.3f, -1.0f, 0.3f };

// Phase2 互換グローバル (Area.cpp が extern 宣言しているため定義だけ残す)
BOOL        g_mIsUseSoftware    = FALSE;
float       g_mLightDist        = 100.f;
XMFLOAT3    g_mLightPosition    = { 0.f, 0.f, 0.f };
XMFLOAT4X4  g_mViewLight;   // identity で初期化

static float    fTime = 0;

// 行列・カメラ (Phase6: XMFLOAT4X4/XMFLOAT3)
XMFLOAT4X4  g_mProjection, g_mView, g_mEyeMat;
float       g_mEyeScale = 1.f, g_mEyeAlph = 0.f, g_mEyeBeta = 0.f;
float       g_mLightAlph = 0.f, g_mLightBeta = 0.f;
XMFLOAT3    g_mEye;
XMFLOAT3    g_mEyebase  = { 0.0f,  1.1f, -4.5f };
XMFLOAT3    g_mAt       = { 0.0f,  1.1f,  0.0f };
XMFLOAT3    g_mUp       = { 0.0f,  1.0f,  0.0f };

extern long g_mScreenWidth;
extern long g_mScreenHeight;

// DX11 レンダーステートオブジェクト
static ID3D11BlendState*        g_pBlendNone    = nullptr;
static ID3D11BlendState*        g_pBlendAlpha   = nullptr;
static ID3D11RasterizerState*   g_pRastCCW      = nullptr;
static ID3D11RasterizerState*   g_pRastCW       = nullptr;
static ID3D11RasterizerState*   g_pRastNone     = nullptr;
static ID3D11DepthStencilState* g_pDSSNormal    = nullptr;
static ID3D11SamplerState*      g_pSampler       = nullptr;
static ID3D11SamplerState*      g_pShadowSampler = nullptr;

//======================================================================
// アクセサ
//======================================================================
ID3D11BlendState*        GetBlendNone(void)   { return g_pBlendNone; }
ID3D11BlendState*        GetBlendAlpha(void)  { return g_pBlendAlpha; }
ID3D11RasterizerState*   GetRastCCW(void)     { return g_pRastCCW; }
ID3D11RasterizerState*   GetRastCW(void)      { return g_pRastCW; }
ID3D11RasterizerState*   GetRastNone(void)    { return g_pRastNone; }
ID3D11DepthStencilState* GetDSSNormal(void)   { return g_pDSSNormal; }
ID3D11SamplerState*      GetSampler(void)       { return g_pSampler; }
ID3D11SamplerState*      GetShadowSampler(void) { return g_pShadowSampler; }

//======================================================================
//      DX11 バッファ生成 (Phase2)
//======================================================================
HRESULT CreateBuffer11(ID3D11Buffer** ppBuf, UINT byteSize, UINT bindFlags,
                       const void* pInitData)
{
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth      = byteSize;
    bd.Usage          = (pInitData != nullptr) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = bindFlags;
    bd.CPUAccessFlags = (pInitData != nullptr) ? 0 : D3D11_CPU_ACCESS_WRITE;

    if (pInitData)
    {
        D3D11_SUBRESOURCE_DATA sd = {};
        sd.pSysMem = pInitData;
        return GetDevice11()->CreateBuffer(&bd, &sd, ppBuf);
    }
    return GetDevice11()->CreateBuffer(&bd, nullptr, ppBuf);
}

//======================================================================
//      DX11 初期化 (旧 InitD3D 相当)
//======================================================================
bool InitD3D(void)
{
    return InitD3D11(GetWindow(),
                     static_cast<int>(GetScreenWidth()),
                     static_cast<int>(GetScreenHeight()));
}

//======================================================================
//      DX11 解放 (旧 ReleaseD3D 相当)
//======================================================================
void ReleaseD3D(void)
{
    ReleaseD3D11();
}

//======================================================================
//      レンダリング
//======================================================================
void Rendering(void)
{
    static unsigned long OldTime = timeGetTime();
    unsigned long NowTime = timeGetTime();
    fTime = (float)(NowTime - OldTime) / 1000.0f;
    OldTime = NowTime;

    static const float clearColor[4] = { 0.8f, 0.85f, 0.9f, 1.0f };
    GetContext()->ClearRenderTargetView(GetRenderTargetView(), clearColor);
    GetContext()->ClearDepthStencilView(GetDepthStencilView(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    unsigned long poly = 0;
    poly += g_mArea.Rendering(g_mAt.x, g_mAt.y, g_mAt.z);
    AdDrawPolygons(poly);

    GetSwapChain()->Present(0, 0);
}

//======================================================================
//      3D 空間の生成  (Phase6: DirectXMath)
//======================================================================
bool Create3DSpace(void)
{
    // プロジェクション行列
    StoreM(g_mProjection,
           XMMatrixPerspectiveFovLH(g_mFov, g_mAspect, g_mNear_z, g_mFar_z));

    // ビュー行列
    StoreM(g_mEyeMat, XMMatrixIdentity());
    StoreM(g_mView,
           XMMatrixLookAtLH(LoadV(g_mEye), LoadV(g_mAt), LoadV(g_mUp)));

    // ライト方向初期化
    StoreV(g_mLightDirBase,
           XMVector3Normalize(XMVectorSet(0.3f, -1.0f, 0.3f, 0.f)));
    g_mLightDir = g_mLightDirBase;

    // g_mViewLight を単位行列で初期化
    StoreM(g_mViewLight, XMMatrixIdentity());

    return true;
}

//======================================================================
//      レンダーステートオブジェクト生成
//======================================================================
static bool CreateRenderStates(void)
{
    ID3D11Device* dev = GetDevice11();

    // ブレンドステート: アルファ合成なし
    {
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable           = FALSE;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(dev->CreateBlendState(&bd, &g_pBlendNone))) return false;
    }
    // ブレンドステート: SrcAlpha / InvSrcAlpha
    {
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable           = TRUE;
        bd.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        bd.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(dev->CreateBlendState(&bd, &g_pBlendAlpha))) return false;
    }
    // ラスタライザ: カリング CCW (通常)
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode              = D3D11_FILL_SOLID;
        rd.CullMode              = D3D11_CULL_BACK;
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable       = TRUE;
        if (FAILED(dev->CreateRasterizerState(&rd, &g_pRastCCW))) return false;
    }
    // ラスタライザ: カリング CW (ミラー反転メッシュ)
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode              = D3D11_FILL_SOLID;
        rd.CullMode              = D3D11_CULL_FRONT;
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable       = TRUE;
        if (FAILED(dev->CreateRasterizerState(&rd, &g_pRastCW))) return false;
    }
    // ラスタライザ: カリングなし (透明・両面)
    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode              = D3D11_FILL_SOLID;
        rd.CullMode              = D3D11_CULL_NONE;
        rd.FrontCounterClockwise = FALSE;
        rd.DepthClipEnable       = TRUE;
        if (FAILED(dev->CreateRasterizerState(&rd, &g_pRastNone))) return false;
    }
    // 深度ステンシルステート: 深度テスト有効、ステンシル無効
    {
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable    = TRUE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dd.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
        if (FAILED(dev->CreateDepthStencilState(&dd, &g_pDSSNormal))) return false;
    }
    // サンプラー: リニアフィルタ、ラップ
    {
        D3D11_SAMPLER_DESC sd = {};
        sd.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MaxLOD         = D3D11_FLOAT32_MAX;
        if (FAILED(dev->CreateSamplerState(&sd, &g_pSampler))) return false;
    }
    // 比較サンプラー: シャドウマップ用 (GREATER_EQUAL, ボーダー=白=受光)
    {
        D3D11_SAMPLER_DESC sd = {};
        sd.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        sd.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
        sd.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
        sd.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
        sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 1.0f;
        sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        sd.MaxLOD         = D3D11_FLOAT32_MAX;
        if (FAILED(dev->CreateSamplerState(&sd, &g_pShadowSampler))) return false;
    }
    return true;
}

//======================================================================
//      初期化
//======================================================================
bool InitRender(void)
{
    char ComboString[128];
    int  w1, w2, w3;
    char ww[128];

    if (!Create3DSpace()) {
        MessageBox(NULL, "It failed to initialize", "Error", MB_OK);
        return false;
    }
    if (!CreateRenderStates()) {
        MessageBox(NULL, "Failed to create render states", "Error", MB_OK);
        return false;
    }

    GetWindowText(GetDlgItem(hDlg1, IDC_COMBO1), ComboString, sizeof(ComboString));
    sscanf(ComboString, "%d-%d-%d,%s", &w1, &w2, &w3, ww);
    g_mArea.SetArea(ConvertStr2Dno2(ComboString));
    if (!g_mArea.LoadMAP()) return false;
    g_mArea.CreateVertexShader();
    if (!g_mArea.InitShadowMap()) {
        MessageBox(NULL, "Failed to init shadow map", "Error", MB_OK);
        return false;
    }
    return true;
}

//======================================================================
//      開放
//======================================================================
void UnInitRender(void)
{
    SAFE_RELEASE(g_pBlendNone);
    SAFE_RELEASE(g_pBlendAlpha);
    SAFE_RELEASE(g_pRastCCW);
    SAFE_RELEASE(g_pRastCW);
    SAFE_RELEASE(g_pRastNone);
    SAFE_RELEASE(g_pDSSNormal);
    SAFE_RELEASE(g_pSampler);
    SAFE_RELEASE(g_pShadowSampler);
}
