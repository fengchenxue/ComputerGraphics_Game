#include"ShaderStruct.hlsli"
cbuffer cbVS_S : register(b0)
{
    float4x4 VP;
};

StructuredBuffer<VS_INSTANCE_GENERAL> InstanceBuffer : register(t0);

PS_INPUT_GENERAL mainVS(VS_INPUT_STATIC input)
{
    VS_INSTANCE_GENERAL instance = InstanceBuffer[input.InstanceID];
    
    PS_INPUT_GENERAL output;
   
    output.position = mul(input.position, instance.W);
    output.position = mul(output.position, VP);
    
    output.Normal = mul(input.normal, (float3x3) instance.W);
    output.Normal = normalize(output.Normal);
    
    output.Tangent = mul(input.tangent, (float3x3) instance.W);
    output.Tangent = normalize(output.Tangent);
    
    output.TexCoords = input.TexCoords;
    output.MaterialIndex = instance.MaterialIndex;
    
    return output;
}

