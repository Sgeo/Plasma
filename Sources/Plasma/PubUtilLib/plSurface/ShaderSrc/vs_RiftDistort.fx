float4x4 View : register(c4);
float4x4 Texm : register(c8);
void main(in float4 Position : POSITION, in float4 Color : COLOR0, in float2 TexCoord : TEXCOORD0, out float4 oPosition : SV_Position, out float4 oColor : COLOR, out float2 oTexCoord : TEXCOORD0)
{
	oPosition = mul(View, Position);
	oTexCoord = mul(Texm, float4(TexCoord,0,1));
	oColor = Color;
};