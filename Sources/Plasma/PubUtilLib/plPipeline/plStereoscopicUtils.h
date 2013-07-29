#ifndef _plStereoscopicUtils_h
#define _plStereoscopicUtils_h

#include "HeadSpin.h"
#include "plViewTransform.h"
#include "hsMatrix44.h"
#include "plScene/plRenderRequest.h"
#include "plStereoscopicUtils.h"

class plViewTransform;
class hsMatrix44;
class plRenderRequest;

struct plStereoViewport
{
	int x, y;
	int w, h;

	plStereoViewport() {}
	plStereoViewport(int x1, int y1, int w1, int h1) : x(x1), y(y1), w(w1), h(h1) { }

	bool operator == (const plStereoViewport& vp) const
	{ return (x == vp.x) && (y == vp.y) && (w == vp.w) && (h == vp.h); }
	bool operator != (const plStereoViewport& vp) const
	{ return !operator == (vp); }
};

namespace StereoUtils {

	class plStereoscopicUtils {
	};

	void ApplyStereoViewToTransform(plViewTransform * vt, hsMatrix44 eyeTransform, hsMatrix44 w2c);
	void ApplyStereoProjectionToTransform(plViewTransform * vt, hsMatrix44 projMatrix);
	void ApplyStereoViewportToTransform(plViewTransform * vt, int xMin, int yMin, int xMax, int yMax, float renderScale);
	
	void MakeRenderRequestsStereo( hsTArray<plRenderRequest*> renderRequests, plViewTransform stereoTransform, plRenderTarget * stereoRT, bool skip);
}

#endif