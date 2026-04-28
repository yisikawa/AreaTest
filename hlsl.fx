
cbuffer ConstantBuffer : register(b0)
{
    matrix mWVP;
    float2 mUV;
    float2 padding;
    float4 mCOL;
    matrix mW;
    float4 mLightDir;     // 太陽方向(ワールド空間・正規化済み)
    float4 mLightColor;   // 太陽光色 × 強度
    float4 mAmbientColor; // 環境光色
    float4 mCameraPos;    // カメラワールド座標
    float4 mSpecular;     // x=shininess, y=強度
    matrix mLightVP;      // ライトのView×Projection
    float4 mShadow;       // x=depthBias, y=shadow強度(0〜1), z=1/シャドウマップサイズ
}

struct VS_INPUT
{
    float4 Pos    : POSITION;
    float3 Normal : NORMAL;
    float4 Col    : COLOR;
    float2 Tex    : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos         : SV_POSITION;
    float4 Col         : COLOR;
    float2 Tex         : TEXCOORD0;
    float3 Normal      : TEXCOORD1;
    float3 WorldPos    : TEXCOORD2;
    float4 ShadowCoord : TEXCOORD3;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos      = mul(input.Pos, mWVP);
    output.Tex      = input.Tex + mUV;
    output.Col      = mCOL;
    output.Col.a   *= input.Col.a;

    float4 worldPos    = mul(input.Pos, mW);
    output.WorldPos    = worldPos.xyz;
    output.Normal      = normalize(mul(input.Normal, (float3x3)mW));
    output.ShadowCoord = mul(worldPos, mLightVP);
    return output;
}

Texture2D              txDiffuse : register(t0);
Texture2D              txShadow  : register(t1);
SamplerState           samLinear : register(s0);
SamplerComparisonState samShadow : register(s1);

float4 PS(PS_INPUT input) : SV_Target
{
    float4 texColor = txDiffuse.Sample(samLinear, input.Tex) * input.Col;
    if (texColor.a < padding.x) discard;

    float3 N = normalize(input.Normal);
    float3 L = normalize(-mLightDir.xyz);
    float3 V = normalize(mCameraPos.xyz - input.WorldPos);
    float3 R = reflect(-L, N);

    float3 ambient  = mAmbientColor.rgb * texColor.rgb;
    float  diff     = max(dot(N, L), 0.0);
    float3 diffuse  = diff * mLightColor.rgb * texColor.rgb;
    float  spec     = pow(max(dot(R, V), 0.0), mSpecular.x) * mSpecular.y;
    float3 specular = spec * mLightColor.rgb;

    // PCF シャドウ (3×3 カーネル、GREATER_EQUAL比較: 1=受光, 0=影)
    float3 proj  = input.ShadowCoord.xyz / input.ShadowCoord.w;
    float2 suv   = float2(proj.x * 0.5 + 0.5, -proj.y * 0.5 + 0.5);
    float  depth = proj.z - mShadow.x;
    float  ts    = mShadow.z;

    float shadow = 0.0;
    [unroll] for (int dx = -1; dx <= 1; dx++)
    [unroll] for (int dy = -1; dy <= 1; dy++)
        shadow += txShadow.SampleCmpLevelZero(samShadow, suv + float2(dx, dy) * ts, depth);
    shadow /= 9.0;

    // mShadow.y=1: 影部分=環境光のみ / mShadow.y=0: 影なし
    float  visibility = lerp(1.0 - mShadow.y, 1.0, shadow);
    return float4(ambient + (diffuse + specular) * visibility, texColor.a);
}
