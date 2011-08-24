// Vertex shader input structure
struct VS_INPUT
{
    float4 Position   : POSITION;
    float4 Diffuse    : COLOR0;
    float4 Specular   : COLOR1;
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
float4x4 WorldViewProj;


// Name: Main Vertex Shader
// Type: Vertex shader
// Desc: Does view and projection vertex transform for most geometry in the game, 
// and passthrough for Texture, Diffuse and Specular colours 

VS_OUTPUT vs_main(in VS_INPUT In)
{
    VS_OUTPUT Out;

    Out.Position = mul(In.Position, WorldViewProj);

    Out.Texture  = In.Texture;
    Out.Diffuse  = In.Diffuse;
    Out.Specular = In.Specular;

    return Out;
}