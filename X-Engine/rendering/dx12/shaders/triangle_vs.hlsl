struct VS_INPUT {
    float3 pos : POSITION;
    float4 color : COLOR;
};

float4 main(VS_INPUT input) : SV_POSITION {
    return float4(input.pos, 1.0f);
}
