
#pragma once

//======================================================================
// INCLUDE
//======================================================================
#include <windows.h>
#include <commctrl.h>
#include "Dx.h"

#define IDC_STATIC -1

class CAreaMesh;
class CEffectModel;
class CEffect;

//======================================================================
// 定数バッファ構造体 (hlsl.fx に対応: 320 バイト)
//======================================================================
struct CBData
{
    XMFLOAT4X4 mWVP;          //  64 bytes
    float      mUV[2];        //   8 bytes
    float      padding[2];    //   8 bytes
    XMFLOAT4   mCOL;          //  16 bytes
    XMFLOAT4X4 mN;            //  64 bytes  normal matrix
    XMFLOAT4X4 mW;            //  64 bytes  ワールド行列
    XMFLOAT4   mLightDir;     //  16 bytes  太陽方向(ワールド空間)
    XMFLOAT4   mLightColor;   //  16 bytes  太陽光色 × 強度
    XMFLOAT4   mAmbientColor; //  16 bytes  環境光色
    XMFLOAT4   mCameraPos;    //  16 bytes  カメラワールド座標
    XMFLOAT4   mSpecular;     //  16 bytes  x=shininess, y=強度
    XMFLOAT4X4 mLightVP;      //  64 bytes  ライトのView×Projection
    XMFLOAT4   mShadow;       //  16 bytes  x=depthBias, y=shadow強度, z=1/shadowMapSize
};

struct ShadowCBData
{
    XMFLOAT4X4 mWVP;
    XMFLOAT4   mShadow;       // x=alphaRef
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
ID3D11RasterizerState*    GetRastCCW(void);
ID3D11RasterizerState*    GetRastCW(void);
ID3D11RasterizerState*    GetRastNone(void);
ID3D11RasterizerState*    GetRastWireframe(void);
ID3D11DepthStencilState*  GetDSSNormal(void);
ID3D11SamplerState*       GetSampler(void);
ID3D11SamplerState*       GetShadowSampler(void);
ID3D11ShaderResourceView* GetWhiteSRV(void);

// 境界エッジハイライト
void          SetHighlightMesh(CAreaMesh* pMesh);
ID3D11Buffer* GetHighlightLineBuf(void);
UINT          GetHighlightLineCount(void);
CAreaMesh*    GetHighlightTarget(void);

// EffectModel ハイライト
void          SetHighlightEffMdl(CEffectModel* pEfm);
CEffectModel* GetHighlightEffMdl(void);

// Effect ハイライト（トランスフォーム付き単体描画用）
void          SetHighlightEffect(CEffect* pEff);
CEffect*      GetHighlightEffect(void);
