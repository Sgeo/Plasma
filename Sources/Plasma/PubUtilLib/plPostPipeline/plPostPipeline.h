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

#define DEFAULT_RIFTSCALE 1.71460557f

class plPostPipeline : public hsKeyedObject{
public:
	plPostPipeline();
	virtual ~plPostPipeline();

	static plPostPipeline*    GetInstance() { 
		if(plPostPipeline::fInstance == nil) 
			plPostPipeline::fInstance = new plPostPipeline;
		return plPostPipeline::fInstance; 
	}

	void ReleaseGeometry();
	void ReleaseTextures();

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
		kRiftShaderScaleOut = 2,
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
	plRenderTarget* CreatePostRT(uint16_t width, uint16_t height);
	plRenderTarget* GetPostRT(){ return fPostRT; };

	void SetRealViewport(ovrRecti vp) { return;  fRealVP = vp; };
	void SetViewport(ovrRecti vp, bool resetProjection);
	void SetPrescaledViewport(ovrRecti vp){ fVP = vp; };
	
	void RestoreViewport(){ SetViewport(fRealVP, true); };
	void SetRenderScale(float scale){fRenderScale = scale; };
	void SetDistortionConfig(ovrEyeType eye = ovrEye_Left)
    {
        //fDistortion = config;
		if (eye == ovrEye_Right) {}
            //fDistortion.XCenterOffset = -fDistortion.XCenterOffset;
    }	

	

private:
	static plPostPipeline* fInstance;
	plPipeline* fPipe;
	plRenderTarget* fPostRT;

	float fRenderScale;
	//OVR::Util::Render::DistortionConfig fDistortion;

	plShader* fVsShader;
	plShader* fPsShader;

	ovrRecti fVP;
	ovrRecti fRealVP;

	bool fEnablePost;
};
	

#endif