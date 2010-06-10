// Vertex shader input structure
struct VS_INPUT
{
    float4 Position   : POSITION;
    float4 Diffuse    : COLOR0;
    float2 Texture1   : TEXCOORD0;
    float2 Texture2   : TEXCOORD1;
};


// Vertex shader output structure
struct VS_OUTPUT
{
    float4 Position   : POSITION;
    float4 Diffuse    : COLOR0;
    float2 Texture1   : TEXCOORD0;
    float2 Texture2   : TEXCOORD1;
};


// Global variables
float4x4 WorldViewProj;
int	 CloakingPhase;
int 	 pX;


// Name: Simple Vertex Shader
// Type: Vertex shader
// Desc: Vertex transformation and texture coord pass-through
//
VS_OUTPUT vs_main( in VS_INPUT In )
{
    VS_OUTPUT Out;                      //create an output vertex
    
    float texX = pX + (CloakingPhase/64) * 0.005f;
    float texY = (CloakingPhase/128) * 0.005f;
    
    In.Texture2.x += texX;
    In.Texture2.y += texY;

    Out.Position = mul(In.Position, WorldViewProj);  //apply vertex transformation

    Out.Texture1 = In.Texture1;          //copy original texcoords
    Out.Texture2 = In.Texture2; 
    
    Out.Diffuse = In.Diffuse;

    return Out;                         //return output vertex
}