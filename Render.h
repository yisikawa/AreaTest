
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <windows.h>
#include <commctrl.h>
#include "Dx.h"

#define IDC_STATIC -1

//======================================================================
// 定数バッファ構造体 (hlsl.fx に対応: 96 バイト)
//======================================================================
struct CBData
{
    XMFLOAT4X4 mWVP;      // 64 bytes
    float      mUV[2];    //  8 bytes
    float      padding[2];//  8 bytes
    XMFLOAT4   mCOL;      // 16 bytes
};

//======================================================================
// PROTOTYPE
//======================================================================
bool  InitRender(void);
void  UnInitRender(void);
void  Rendering(void);
bool  Create3DSpace(void);
bool  InitD3D(void);
void  ReleaseD3D(void);

// DX11 バッファ生成ヘルパー (Phase2)
HRESULT CreateBuffer11(ID3D11Buffer** ppBuf, UINT byteSize, UINT bindFlags,
                       const void* pInitData = nullptr);

// DX11 レンダーステートオブジェクト アクセサ (Phase5)
ID3D11BlendState*        GetBlendNone(void);
ID3D11BlendState*        GetBlendAlpha(void);
ID3D11RasterizerState*   GetRastCCW(void);
ID3D11RasterizerState*   GetRastCW(void);
ID3D11RasterizerState*   GetRastNone(void);
ID3D11DepthStencilState* GetDSSNormal(void);
ID3D11SamplerState*      GetSampler(void);
