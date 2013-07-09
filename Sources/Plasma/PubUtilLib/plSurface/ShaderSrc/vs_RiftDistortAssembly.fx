struct VS_INPUT
{
    float4 Position   : POSITION;
    //float4 Color 	  : COLOR0;
    float2 TexCoord    : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 oPosition	: POSITION;
    //float4 oColor   	: COLOR0;
    float2 oTexCoord	: TEXCOORD0;
};

float4x4 View : register(c4);
float4x4 Texm : register(c8);


VS_OUTPUT vs_main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.oPosition = mul(View, In.Position);
	//Out.oColor = In.Color;
	Out.oTexCoord = mul(Texm, float4(In.TexCoord, 0, 1)).xy;
}

