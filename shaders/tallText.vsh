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
float4x4 OrthographicProj;
float CloakingPhase;
float pX;


// Name: Large font smoke effect shader
// Type: Vertex shader
// Desc: Animated cloud texture based on constant values CloakingPhase and pX
//
VS_OUTPUT vs_main(in VS_INPUT In)
{
    VS_OUTPUT Out;

    // minus x to make effect move right to left (instead of left to right) and multiply by 0.005f to control speed
    float texX = (-In.Position.x) + pX + ((CloakingPhase/64.0f) * 0.005f);
    
    // minus y to make effect move down to up (instead of up to down) and multiply by 0.005f to control speed
    float texY = (-In.Position.y) + ((CloakingPhase/128.0f) * 0.005f);
    
    // we don't set In.Texture2 correctly from the application - we do it here with above values
    In.Texture2.x = texX;
    In.Texture2.y = texY;

    Out.Texture1 = In.Texture1;
    Out.Texture2 = In.Texture2;
    Out.Diffuse  = In.Diffuse;
    
    // finally do our orthographic projection
    Out.Position = mul(In.Position, OrthographicProj);

    return Out;
}