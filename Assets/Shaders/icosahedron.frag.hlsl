struct PSInput
{
    float4 Position : SV_Position;
    float3 WorldNormal : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target0
{
    const float3 normal = normalize(input.WorldNormal);
    const float3 lightDirection = normalize(float3(0.4, 0.8, 0.2));

    const float ndotl = saturate(dot(normal, lightDirection));
    const float3 baseColor = float3(0.20, 0.62, 0.95);
    const float3 litColor = baseColor * (0.25 + 0.75 * ndotl);
    const float3 rim = 0.12 * abs(normal);

    return float4(litColor + rim, 1.0);
}
