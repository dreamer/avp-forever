// Vertex shader input structure
struct VS_INPUT
{
    float4 Position   : POSITION;
    float4 Diffuse    : COLOR0;
    float2 Texture    : TEXCOORD0;
};

// Vertex shader output structure
struct VS_OUTPUT
{
    float4 Position   : POSITION;
    float4 Diffuse    : COLOR0;
    float2 Texture    : TEXCOORD0;
};


// Global variables
// Melanikus 18/11/11 NVidia 3D Vision Fix
// 
float4x4 Projection;

// Name: RHW Pretransformed Vertex Shader
// Type: Vertex shader
// Desc: Does nothing

VS_OUTPUT vs_main(in VS_INPUT In)
{
    VS_OUTPUT Out;

    Out.Position = mul(In.Position, Projection);
    Out.Texture  = In.Texture;
    Out.Diffuse  = In.Diffuse;

    return Out;
}