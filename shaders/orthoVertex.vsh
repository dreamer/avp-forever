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
    float4 Specular   : COLOR1;
    float2 Texture    : TEXCOORD0;
};

// Global variables
float4x4 OrthographicProj;


// Name: Simple Vertex Shader
// Type: Vertex shader
// Desc: Vertex transformation and texture coord pass-through

VS_OUTPUT vs_main(in VS_INPUT In)
{
    VS_OUTPUT Out;

    // do orthographic projection transform
    Out.Position = mul(In.Position, OrthographicProj);

    Out.Texture = In.Texture;
    Out.Diffuse = In.Diffuse;
    
    Out.Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    return Out;
}