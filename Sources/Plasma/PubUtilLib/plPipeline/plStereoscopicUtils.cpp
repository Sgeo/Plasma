#include "plStereoscopicUtils.h"
#include "HeadSpin.h"
#include "plViewTransform.h"
#include "plScene/plRenderRequest.h"
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

// MakeRenderRequestsStereo //////////////////////////////////////////////////////////////////////////////
// Sets the viewtransform of a render request list to follow the correct format for stereoscopic rendering
void StereoUtils::MakeRenderRequestsStereo( hsTArray<plRenderRequest*> renderRequests, plViewTransform stereoTransform, plRenderTarget * stereoRT){
	int i;
	for( i = 0; i < renderRequests.GetCount(); i++ )
	{
		plViewTransform vt = renderRequests[i]->GetViewTransform();
		renderRequests[i]->SetRenderTarget(stereoRT);
		
		//hsMatrix44 projMatrix = stereoTransform.GetCameraToNDC();
		//renderRequests[i]->SetFovX(stereoTransform.GetFovX());
		//renderRequests[i]->SetFovY(stereoTransform.GetFovY());
		//vt.SetProjectionMatrix(&projMatrix);
		
		vt.SetViewPort(stereoTransform.GetViewPortLoX(),
			stereoTransform.GetViewPortLoY(), 
			stereoTransform.GetViewPortHiX(),
			stereoTransform.GetViewPortHiY(), false);
		//vt.SetWidth(stereoTransform.GetViewPortHiX() - stereoTransform.GetViewPortLoX());
		//vt.SetHeight(stereoTransform.GetViewPortHiY() - stereoTransform.GetViewPortLoY());

		hsMatrix44 camMat = vt.GetWorldToCamera();
		hsMatrix44 inv;
		camMat.GetInverse(&inv);

		vt.SetCameraTransform(camMat, inv );

		renderRequests[i]->SetViewTransform(vt);
		
	}
}
