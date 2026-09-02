struct VS_INPUT {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float3 normal : TEX0;
    float3 world_pos : TEX1;
    float2 uv : TEX2;
};

cbuffer TransformBuffer : register(b0) {
    float4x4 worldViewProj;
    float4 tint;
};

cbuffer LightBuffer : register(b1) {
    float4 lightDirection;
    float4 lightColor;
    float4 cameraPos;
    float4 lightParams;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.pos = mul(float4(input.pos, 1.0f), worldViewProj);
    output.color = input.color * tint;
    output.normal = input.normal;
    output.world_pos = input.pos;
    output.uv = input.uv;
    return output;
}