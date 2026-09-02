struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float3 normal : TEX0;
    float3 world_pos : TEX1;
    float2 uv : TEX2;
};

cbuffer LightBuffer : register(b1) {
    float4 lightDirection;
    float4 lightColor;
    float4 cameraPos;
    float4 lightParams;
};

Texture2D diffuseTex : register(t0);
SamplerState linearSampler : register(s0);

float3 Normalize3(float3 v) {
    float l = length(v);
    return l > 1e-6 ? v / l : float3(0, 1, 0);
}

float4 main(PS_INPUT input) : SV_TARGET {
    float3 n = Normalize3(input.normal);
    float3 l = Normalize3(lightDirection.xyz);
    float ndotl = saturate(dot(n, l));

    float3 ambient = float3(0.10, 0.10, 0.12) * lightColor.w;
    float3 diffuse = lightColor.rgb * lightDirection.w * ndotl;

    float3 v = Normalize3(cameraPos.xyz - input.world_pos);
    float3 h = Normalize3(l + v);
    float ndoth = saturate(dot(n, h));
    float spec = pow(ndoth, lightParams.x) * lightParams.y * lightParams.z;
    float3 specular = lightColor.rgb * spec;

    float3 baseColor = input.color.rgb * diffuseTex.Sample(linearSampler, input.uv).rgb;
    float3 finalColor = baseColor * (ambient + diffuse) + specular;
    return float4(finalColor, input.color.a);
}