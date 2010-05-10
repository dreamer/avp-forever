// Vertex shader input structure
struct VS_INPUT
{
    float4 Position   : POSITION;
    float  Size	      : PSIZE;
    float4 Colour     : COLOR0;
    float2 Texture    : TEXCOORD0;
};


// Vertex shader output structure
struct VS_OUTPUT
{
    float4 Position   : POSITION;
    float4 Colour     : COLOR0;
};


// Global variables
float4x4 WorldViewProj;


// Name: Simple Vertex Shader
// Type: Vertex shader
// Desc: Vertex transformation and texture coord pass-through
//
VS_OUTPUT vs_main( in VS_INPUT In )
{
    VS_OUTPUT Out;                      //create an output vertex

    Out.Position = mul(In.Position,
                       WorldViewProj);  //apply vertex transformation
//    Out.Texture  = In.Texture;          //copy original texcoords
    
    Out.Colour = In.Colour;

    return Out;                         //return output vertex
}