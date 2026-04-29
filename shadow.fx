// シャドウマップ書き込みパス専用シェーダー
// CPU側で mW × mLightVP を結合して mWVP として渡す
// PSなし・RenderTargetなし、深度バッファのみに書き込む

cbuffer ShadowCB : register(b0)
{
    matrix mWVP;
    float4 mShadow;
}

struct VS_IN
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

VS_OUT VS(VS_IN i)
{
    VS_OUT o;
    o.Pos = mul(i.Pos, mWVP);
    o.Tex = i.Tex;
    return o;
}

Texture2D    txDiffuse : register(t0);
SamplerState samLinear : register(s0);

void PS(VS_OUT i)
{
    float alpha = txDiffuse.Sample(samLinear, i.Tex).a;
    if (alpha < mShadow.x) discard;
}
