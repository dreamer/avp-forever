// Vertex shader input structure
struct VS_INPUT
{
    float4 Position   : POSITION;
    float2 Texture1   : TEXCOORD0;
    float2 Texture2   : TEXCOORD1;
    float2 Texture3   : TEXCOORD2;
};

// Vertex shader output structure
struct VS_OUTPUT
{
    float4 Position   : POSITION;
    float2 Texture1   : TEXCOORD0;
    float2 Texture2   : TEXCOORD1;
    float2 Texture3   : TEXCOORD2;
};

// Global variables
float4x4 OrthographicProj;


// Name: FMV Vertex shader
// Type: Vertex shader
// Desc: Vertex transformation and texture coord pass-through

VS_OUTPUT vs_main(in VS_INPUT In)
{
    VS_OUTPUT Out;

    // do orthographic projection transform
    Out.Position = mul(In.Position, OrthographicProj);

    Out.Texture1 = In.Texture1;
    Out.Texture2 = In.Texture2;
    Out.Texture3 = In.Texture3;

    return Out;
}