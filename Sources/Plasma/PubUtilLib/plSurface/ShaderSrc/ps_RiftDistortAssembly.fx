// D3D9 version of the Rift barrel warp shader (ps_2_0 compatible)
sampler tex : register(s0);

float2 LensCenter : register(c0);
float2 ScreenCenter : register(c1);
float2 Scale : register(c2);
float2 ScaleIn : register(c3);
float4 HmdWarpParam : register(c4);

// Scales input texture coordinates for distortion.
// ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
// larger due to aspect ratio.
float2 HmdWarp(float2 in01)
{
   float2 theta = (in01 - LensCenter) * ScaleIn; // Scales to [-1, 1]
   float rSq = theta.x * theta.x + theta.y * theta.y;
   float2 theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + 
                   HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);
   return LensCenter + Scale * theta1;
}

// You should be able to compile this for the PS_2_0 target
float4 ps_main(in float4 oPosition : SV_Position, in float4 oColor : COLOR,
            in float2 oTexCoord : TEXCOORD0) : SV_Target
{
   float2 tc = HmdWarp(oTexCoord);
   if (any(clamp(tc, ScreenCenter - float2(0.25, 0.5), ScreenCenter + float2(0.25, 0.5) ) - tc) )
       return 0;
   return tex2D(tex, tc);
}

