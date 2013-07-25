#include "plStereoscopicUtils.h"
#include "HeadSpin.h"
#include "plViewTransform.h"
#include "hsMatrix44.h"

void StereoUtils::ApplyStereoProjectionToTransform(plViewTransform * vt, hsMatrix44 projMatrix){
	vt->SetProjectionMatrix(&projMatrix);
}

void StereoUtils::ApplyStereoViewportToTransform(plViewTransform * vt, int xMin, int yMin, int xMax, int yMax, float renderScale = 1.0f){
	vt->SetViewPort(xMin * renderScale, yMin * renderScale, xMax * renderScale, yMax * renderScale, false);
	vt->SetWidth((xMax - xMin) * renderScale);
	vt->SetHeight((yMax - yMin) * renderScale);
	//hsPoint2 depth;
	//depth.Set(fNear, fFar);
	//vt.SetDepth(depth);
}

void StereoUtils::ApplyStereoViewToTransform(plViewTransform * vt, hsMatrix44 eyeTransform, hsMatrix44 w2c){
	hsMatrix44 inverse, outW2c;
	outW2c = eyeTransform * w2c;
	outW2c.GetInverse(&inverse);
	vt->SetCameraTransform( outW2c, inverse );
}