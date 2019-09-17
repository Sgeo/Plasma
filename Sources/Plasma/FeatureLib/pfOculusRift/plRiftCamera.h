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

#ifndef plRiftCamera_inc
#define plRiftCamera_inc


//Plasma includes
#include "HeadSpin.h"
#include "hsTemplates.h"
#include "hsGeometry3.h"
#include "hsMatrix44.h"
#include "plstring.h"
#include "plGImage/plMipmap.h"

	
#include "plSurface/hsGMaterial.h"
#include "plMessage/plLayRefMsg.h"
#include "plResMgr/plResManager.h"
#include "plResMgr/plKeyFinder.h"
#include "pfCamera/plVirtualCamNeu.h"
#include "pfConsoleCore/pfConsoleEngine.h"
#include "pfConsole/pfConsole.h"
#include "pfConsole/pfConsoleDirSrc.h"
#include "plPipeline/plPlates.h"
#include "pnKeyedObject/hsKeyedObject.h"
#include "pnKeyedObject/plKey.h"
#include "pnKeyedObject/plFixedKey.h"
#include "pnKeyedObject/plUoid.h"
//#include "../../FeatureLib/pfCamera/plVirtualCamNeu.h"

//OpenXR namespace
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_WIN32
#include <Windows.h>
#include <openxr/openxr_platform.h>
#include <openxr/xr_linear.h>

class plPipeline;
class plCameraModifier1;
class plCameraBrain1;
class plSceneObject;
class plKey;
class hsGMaterial;
class plDrawableSpans;
class plCameraProxy;
class plSceneNode;
class plDebugInputInterface;
class plPlate;
class plShader;

class plRiftCamera : public hsKeyedObject{
public:

	//Flags
	enum {
		kUseRawInput,		
		kUseEulerInput
	};

	plRiftCamera();
	virtual ~plRiftCamera();

	CLASSNAME_REGISTER( plRiftCamera );
    GETINTERFACE_ANY( plRiftCamera, hsKeyedObject );

	virtual bool MsgReceive(plMessage* msg);
	void SetFlags(int flag) { fFlags.SetBit(flag); }
    bool HasFlags(int flag) { return fFlags.IsBitSet(flag); }
    void ClearFlags(int flag) { fFlags.ClearBit(flag); }

	void initRift(int width, int height);
	void SetCameraManager(plVirtualCam1* camManager){ fVirtualCam = camManager; };
	void SetPipeline(plPipeline* pipe){ fPipe = pipe; };
	hsMatrix44 RawRiftRotation();
	hsMatrix44 EulerRiftRotation();
	void SetRawRotation(){
		ClearFlags(kUseEulerInput);
		SetFlags(kUseRawInput);
	}
	void SetEulerRotation(){
		ClearFlags(kUseRawInput);
		SetFlags(kUseEulerInput);
	}

	void SetNear(float distance){ fNear = distance; };
	void SetFar(float distance){ fFar = distance; };

	float ReverseRadians(float angle){ return 2 * M_PI - angle; };

	bool BeginAndShouldRender();

	void ApplyLeftEyeViewport(){ ApplyStereoViewport(0); };
	void ApplyRightEyeViewport(){ ApplyStereoViewport(1); };
	void ApplyStereoViewport(int);

	void SetOriginalCamera(hsMatrix44 cam){ fWorldToCam = cam; };
	
	void EnableLeftEyeRender(bool state){ fEyeToRender = EYE_LEFT; };
	void EnableRightEyeRender(bool state){ fEyeToRender = EYE_RIGHT; };
	void EnableBothEyeRender(bool state){ fEyeToRender = EYE_BOTH; };
	int GetEyeToRender(){return fEyeToRender; };

	void EnableStereoRendering(bool state){ fEnableStereoRendering = state; };
	bool GetStereoRenderingState(){ return fEnableStereoRendering; };
	float GetRenderScale(){return fRenderScale;};

	//Util::Render::StereoEyeParams GetEyeParams(Util::Render::StereoEye eye){return SConfig.GetEyeRenderParams(eye); };

	enum eye {EYE_LEFT = 1, EYE_RIGHT, EYE_BOTH};

	//Utils
	hsMatrix44* XRTransformToHSTransform(XrMatrix4x4f* xrmat, hsMatrix44* hsMat);

	void * GetAnyGLFuncAddress(const char * name);

	void DrawToEye(int eye);

	void Submit();

	void SetXOffsetRotation(float offset){fXRotOffset = 3.1415926f * offset;};
	void SetYOffsetRotation(float offset){fYRotOffset = 3.1415926f * offset;};
	void SetZOffsetRotation(float offset){fZRotOffset = 3.1415926f * offset;};

private:

	//Plasma objects
	plVirtualCam1* fVirtualCam;
	plPipeline* fPipe;
	int fEyeToRender;
	hsMatrix44 fWorldToCam;

	//OpenXR objects
	XrInstance pInstance;
	XrSession          pSession;
	XrSystemId pSystemId = XR_NULL_SYSTEM_ID;
	XrSpace pBaseSpace;
	XrView pViews[2];
	std::vector<XrViewConfigurationView> pViewConfigurationViews;
	XrFrameState pFrameState;
	HMODULE             pOpenGL;
	XrSwapchain pTextureSwapChains[2];
	std::vector<XrSwapchainImageOpenGLKHR> pSwapChainImages[2];

	bool fEnableStereoRendering;
	float fRenderScale;

	float fXRotOffset, fYRotOffset, fZRotOffset;

    float               fEyeYaw;         // Rotation around Y, CCW positive when looking at RHS (X,Z) plane.
    float               fEyePitch;       // Pitch. If sensor is plugged in, only read from sensor.
    float               fEyeRoll;        // Roll, only accessible from Sensor.
    float               fLastSensorYaw;  // Stores previous Yaw value from to support computing delta.
	float				fYawInitial;
	hsBitVector         fFlags;
	float fNear, fFar;

	void plRiftCamera::makeLayerEyeFov(int width, int height, XrCompositionLayerProjection* out_Layer);

};

#endif