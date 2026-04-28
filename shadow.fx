// シャドウマップ書き込みパス専用シェーダー
// CPU側で mW × mLightVP を結合して mWVP として渡す
// PSなし・RenderTargetなし、深度バッファのみに書き込む

cbuffer ShadowCB : register(b0)
{
    matrix mWVP;
}

struct VS_IN  { float4 Pos : POSITION; };
struct VS_OUT { float4 Pos : SV_POSITION; };

VS_OUT VS(VS_IN i)
{
    VS_OUT o;
    o.Pos = mul(i.Pos, mWVP);
    return o;
}
