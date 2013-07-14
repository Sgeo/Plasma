/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/

#ifndef plPostPipeline_inc
#define plPostPipeline_inc

#include "pnKeyedObject/hsKeyedObject.h" 
#include "pfOculusRift/plRiftCamera.h"
#include "plPipeline.h"

class plPipeline;
class plShader;
class plRenderTarget;
class plViewTransform;

class plPostPipeline : public hsKeyedObject{
public:
	plPostPipeline();
	virtual ~plPostPipeline();

	CLASSNAME_REGISTER( plPostPipeline );
    GETINTERFACE_ANY( plPostPipeline, hsKeyedObject );

	virtual bool MsgReceive(plMessage* msg);

	enum {
		kRefPassthroughVS,
		kRefPassthroughPS,
		kRefRiftDistortVS,
		kRefRiftDistortPS
	};

	enum RiftShaderPSConsts {
		kRiftShaderLensCenter = 0,
		kRiftShaderScreenCenter = 1,
		kRiftShaderScale = 2,
		kRiftShaderScaleIn = 3,
		kRiftShaderHmdWarpParam = 4
	};

	enum RiftShaderVSConsts {
		kRiftShaderView = 0,
		kRiftShaderTexm = 4
	};

	void CreateShaders();
	void UpdateShaders();
	void EnablePostRT();
	void DisablePostRT();
	void RenderPostEffects();
	
	void EnablePostProcessing(bool state){fEnablePost = state; };
	bool GetPostProcessingState(){ return fEnablePost; };

	void SetPipeline(plPipeline* pipe){fPipe = pipe; };
	void CreatePostRT(uint16_t width, uint16_t height);

	void SetRealViewport(OVR::Util::Render::Viewport vp){ fRealVP = vp; };
	void SetViewport(OVR::Util::Render::Viewport vp, bool resetProjection);
	void SetPrescaledViewport(OVR::Util::Render::Viewport vp){ fVP = vp; };
	
	void RestoreViewport(){ SetViewport(fRealVP, true); };
	void SetRenderScale(float scale){fRenderScale = scale; };
	void SetDistortionConfig(OVR::Util::Render::DistortionConfig config, OVR::Util::Render::StereoEye eye = OVR::Util::Render::StereoEye_Left)
    {
        fDistortion = config;
        if (eye ==OVR::Util::Render:: StereoEye_Right)
            fDistortion.XCenterOffset = -fDistortion.XCenterOffset;
    }	

	

private:
	plPipeline* fPipe;
	plRenderTarget* fPostRT;

	float fRenderScale;
	OVR::Util::Render::DistortionConfig fDistortion;

	plShader* fVsShader;
	plShader* fPsShader;

	OVR::Util::Render::Viewport fVP;
	OVR::Util::Render::Viewport fRealVP;

	bool fEnablePost;
};
	

#endif