struct VS_INPUT
{
    float4 Position   : POSITION;
    float4 Color      : COLOR0; 	
    float2 TexCoord   : TEXCOORD0;
    float2 TexCoord1  : TEXCOORD1;
};


struct VS_OUTPUT
{
    float4 oPosition   : POSITION;
    float4 oColor      : COLOR;
    float2 oTexCoord    : TEXCOORD0;
};

float4x4 View : register(c0);
float4x4 Texm : register(c4);

VS_OUTPUT vs_main(in VS_INPUT In)
{
    VS_OUTPUT Out;
    
    //Out.oPosition = mul(View, In.Position);
    Out.oPosition = In.Position;
    Out.oTexCoord = mul(Texm, float4(In.TexCoord,0,1));
    Out.oTexCoord.y = 1 - Out.oTexCoord.y;		//Flip the V texcoords because the pixel shader is stupid
    //Out.oTexCoord = In.TexCoord;
    Out.oColor = In.Color;

    return Out;
}