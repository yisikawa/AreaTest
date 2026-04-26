

//======================================================================
// INCLUDE
//======================================================================
#include <windows.h>
#include "Dx.h"

//======================================================================
// GLOBAL
//======================================================================
static ID3D11Device*            g_pDevice       = nullptr;
static ID3D11DeviceContext*     g_pContext       = nullptr;
static IDXGISwapChain*          g_pSwapChain     = nullptr;
static ID3D11RenderTargetView*  g_pRTV           = nullptr;
static ID3D11DepthStencilView*  g_pDSV           = nullptr;
static ID3D11Texture2D*         g_pDepthStencil  = nullptr;

//======================================================================
// アクセサ
//======================================================================
ID3D11Device*            GetDevice11(void)        { return g_pDevice; }
ID3D11DeviceContext*     GetContext(void)          { return g_pContext; }
IDXGISwapChain*          GetSwapChain(void)        { return g_pSwapChain; }
ID3D11RenderTargetView*  GetRenderTargetView(void) { return g_pRTV; }
ID3D11DepthStencilView*  GetDepthStencilView(void) { return g_pDSV; }

//======================================================================
//      DX11 デバイス・スワップチェーン初期化
//======================================================================
bool InitD3D11(HWND hWnd, int width, int height)
{
    HRESULT hr;

    //------------------------------------------------------------------
    // スワップチェーン記述子
    //------------------------------------------------------------------
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount                         = 1;
    scd.BufferDesc.Width                    = width;
    scd.BufferDesc.Height                   = height;
    scd.BufferDesc.Format                   = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator    = 60;
    scd.BufferDesc.RefreshRate.Denominator  = 1;
    scd.BufferUsage                         = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow                        = hWnd;
    scd.SampleDesc.Count                    = 1;
    scd.SampleDesc.Quality                  = 0;
    scd.Windowed                            = TRUE;
    scd.SwapEffect                          = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL featureLevel;

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    //------------------------------------------------------------------
    // デバイスとスワップチェーンを同時に作成
    //------------------------------------------------------------------
    hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &scd,
        &g_pSwapChain,
        &g_pDevice,
        &featureLevel,
        &g_pContext
    );
    if (FAILED(hr)) {
        MessageBox(nullptr, "DX11 デバイスの作成に失敗しました", "Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    //------------------------------------------------------------------
    // バックバッファからレンダーターゲットビューを作成
    //------------------------------------------------------------------
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr)) {
        MessageBox(nullptr, "バックバッファの取得に失敗しました", "Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    hr = g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRTV);
    pBackBuffer->Release();
    if (FAILED(hr)) {
        MessageBox(nullptr, "レンダーターゲットビューの作成に失敗しました", "Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    //------------------------------------------------------------------
    // 深度ステンシルテクスチャとビューを作成
    //------------------------------------------------------------------
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width              = width;
    depthDesc.Height             = height;
    depthDesc.MipLevels          = 1;
    depthDesc.ArraySize          = 1;
    depthDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count   = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage              = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;

    hr = g_pDevice->CreateTexture2D(&depthDesc, nullptr, &g_pDepthStencil);
    if (FAILED(hr)) {
        MessageBox(nullptr, "深度バッファの作成に失敗しました", "Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    hr = g_pDevice->CreateDepthStencilView(g_pDepthStencil, nullptr, &g_pDSV);
    if (FAILED(hr)) {
        MessageBox(nullptr, "深度ステンシルビューの作成に失敗しました", "Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    //------------------------------------------------------------------
    // レンダーターゲットと深度バッファをコンテキストに設定
    //------------------------------------------------------------------
    g_pContext->OMSetRenderTargets(1, &g_pRTV, g_pDSV);

    //------------------------------------------------------------------
    // ビューポート設定
    //------------------------------------------------------------------
    D3D11_VIEWPORT vp = {};
    vp.Width    = static_cast<FLOAT>(width);
    vp.Height   = static_cast<FLOAT>(height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    g_pContext->RSSetViewports(1, &vp);

    return true;
}

//======================================================================
//      DX11 リソース解放
//======================================================================
void ReleaseD3D11(void)
{
    if (g_pDSV)         { g_pDSV->Release();         g_pDSV         = nullptr; }
    if (g_pDepthStencil){ g_pDepthStencil->Release(); g_pDepthStencil= nullptr; }
    if (g_pRTV)         { g_pRTV->Release();          g_pRTV         = nullptr; }
    if (g_pSwapChain)   { g_pSwapChain->Release();    g_pSwapChain   = nullptr; }
    if (g_pContext)     { g_pContext->Release();      g_pContext     = nullptr; }
    if (g_pDevice)      { g_pDevice->Release();       g_pDevice      = nullptr; }
}
