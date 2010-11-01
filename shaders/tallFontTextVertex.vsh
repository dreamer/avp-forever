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
float	CloakingPhase;
float	pX;


// Name: Large font smoke effect shader
// Type: Vertex shader
// Desc: Animated cloud texture based on constant values CloakingPhase and pX
//
VS_OUTPUT vs_main( in VS_INPUT In )
{
    VS_OUTPUT Out;	// create an output vertex
    
    float texX = pX + (CloakingPhase/64.0f) * 0.005f;
    float texY = (CloakingPhase/128.0f) * 0.005f;
    
    // -= here instead of += to get the effect to move left to right 
    In.Texture2.x -= texX;
    In.Texture2.y -= texY;

    Out.Position = mul(In.Position, WorldViewProj);	// apply vertex transformation

    Out.Texture1 = In.Texture1;	// copy original texcoords
    Out.Texture2 = In.Texture2; 
    
    Out.Diffuse = In.Diffuse;

    return Out;	// return output vertex
}