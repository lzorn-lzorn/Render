struct VSInput
{
    float4 Position : POSITION;
    float4 Normal : NORMAL;
};

struct VSOutput
{
    float4 Position : SV_Position;
};

struct DrawPushConstants
{
    float4x4 ModelViewProjection;
    float4x4 Model;
};

[[vk::push_constant]]
DrawPushConstants Push;

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(Push.ModelViewProjection, input.Position);
    return output;
}
