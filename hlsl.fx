
cbuffer ConstantBuffer : register(b0)
{
    matrix mWVP;
    float2 mUV;
    float2 padding;
    float4 mCOL;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float4 Col : COLOR;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Col : COLOR;
    float2 Tex : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, mWVP);
    output.Tex = input.Tex + mUV;
    output.Col = mCOL;
    output.Col.a *= input.Col.a;
    return output;
}

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

float4 PS(PS_INPUT input) : SV_Target
{
    float4 color = txDiffuse.Sample(samLinear, input.Tex) * input.Col;
    if (color.a < padding.x) discard;
    return color;
}