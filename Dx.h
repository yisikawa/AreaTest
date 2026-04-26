
#pragma once

//======================================================================
// INCLUDE  (DX11)
//======================================================================
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

//======================================================================
// PROTOTYPE
//======================================================================
bool InitD3D11(HWND hWnd, int width, int height);
void ReleaseD3D11(void);

ID3D11Device*             GetDevice11(void);
ID3D11DeviceContext*      GetContext(void);
IDXGISwapChain*           GetSwapChain(void);
ID3D11RenderTargetView*   GetRenderTargetView(void);
ID3D11DepthStencilView*   GetDepthStencilView(void);

//======================================================================
// DirectXMath 変換ヘルパー
//======================================================================
inline XMMATRIX LoadM(const XMFLOAT4X4& m) noexcept { return XMLoadFloat4x4(&m); }
inline void     StoreM(XMFLOAT4X4& o, XMMATRIX m)   noexcept { XMStoreFloat4x4(&o, m); }
inline XMVECTOR LoadV(const XMFLOAT3& v)             noexcept { return XMLoadFloat3(&v); }
inline void     StoreV(XMFLOAT3& o, XMVECTOR v)      noexcept { XMStoreFloat3(&o, v); }
inline XMVECTOR LoadV4(const XMFLOAT4& v)            noexcept { return XMLoadFloat4(&v); }
inline void     StoreV4(XMFLOAT4& o, XMVECTOR v)     noexcept { XMStoreFloat4(&o, v); }
