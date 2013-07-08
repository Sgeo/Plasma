// Texture is given SBS horizontally compressed
#define SBSOffset float2(1,0)
#define SBSFactor float2(2,1)

// Stereo Lens center offset, (eye out of cente)
#define StereoOffset float2(0.02,0.0)
 
sampler s0;

#define LensCenter float2(0.5f,0.5f)
#define ScreenCenter float2(0.5,0.5)
#define Scale float2(0.32,0.45)       /*Complete image: 0.35,0.45*/
#define ScaleIn float2(2,2)
#define HmdWarpParam float4(1,0.22,0.24,0)

// Scales input texture coordinates for distortion.
// ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
// larger due to aspect ratio.
float2 HmdWarp(float2 in01)
{
   float2 theta = (in01 - LensCenter) * ScaleIn; // Scales to [-1, 1]
   float rSq = length(theta);// * theta.x + theta.y * theta.y;
   float2 theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + 
                   HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);
   return LensCenter + Scale * theta1;
}

// You should be able to compile this for the PS_2_0 target
float4 main(float2 tex : TEXCOORD0) : COLOR
{
//return float4(0.8,0.1,0.3,0.3);
    if( tex.x<0.5)
    {
   tex = (tex * SBSFactor) - float2(0.0,0.0) - StereoOffset;
   float2 tc = HmdWarp(tex);
   if (any(clamp(tc, ScreenCenter - float2(0.5, 0.5), ScreenCenter + float2(0.5, 0.5) ) - tc) )       return 0;
   tc = tc/ SBSFactor;
   return tex2D(s0, tc);
    }
    if( tex.x>0.5)
    {
   tex = (tex *  SBSFactor) -  SBSOffset +  StereoOffset;
   float2 tc = HmdWarp(tex);
   if (any(clamp(tc, ScreenCenter - float2(0.5, 0.5), ScreenCenter + float2(0.5, 0.5) ) - tc) )       return 0;
   tc = float2(0.0,0)+ tc/ SBSFactor;
   return tex2D(s0, tc);
    }
   
    return float4(0.8,0.1,0.3,0.3);
}