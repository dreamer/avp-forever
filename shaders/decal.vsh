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
float4x4 WorldViewProj;


// Name: Decal Vertex Shader
// Type: Vertex shader
// Desc: Transform vertices and move them slightly towards the camera to help lessen z-fighting

VS_OUTPUT vs_main(in VS_INPUT In)
{
    VS_OUTPUT Out;

    // do view and projection transform
    Out.Position = mul(In.Position, WorldViewProj);
    
    // move towards camera slightly to try help lessen z-fighting
    Out.Position.z -= 0.5f;

    Out.Texture = In.Texture;
    Out.Diffuse = In.Diffuse;

    return Out;
}