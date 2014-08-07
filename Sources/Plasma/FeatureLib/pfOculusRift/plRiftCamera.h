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
#include "hsQuat.h"
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
#include "plPipeline.h"
#include <d3d9.h>
#include "pnKeyedObject/hsKeyedObject.h"
#include "pnKeyedObject/plKey.h"
#include "pnKeyedObject/plFixedKey.h"
#include "pnKeyedObject/plUoid.h"
//#include "../../FeatureLib/pfCamera/plVirtualCamNeu.h"

//Rift 
#define OVR_D3D_VERSION 9
#include "OVR.h"
#include "..\Src\OVR_CAPI.h"
#include "..\Src\OVR_CAPI_D3D.h"

using namespace OVR;

class plClient;
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
class plRenderTarget;

struct HmdTransform {
	hsQuat rotation;
	hsVector3 position;
};

class plRiftCamera : public hsKeyedObject
{
private:

	//Plasma objects
	plVirtualCam1* fVirtualCam;
	plPipeline* fPipe;
	IDirect3DDevice9* fDirect3DDevice;

	bool fEnableStereoRendering;

	float fXRotOffset, fYRotOffset, fZRotOffset;
	hsBitVector         fFlags;

	//OVR vars
	ovrHmd fHmd;
	plRenderTarget* fSteroRenderTarget;
	bool bHmdIsDebug = false;
	ovrRecti fVp;

	ovrEyeRenderDesc    fEyeRenderDesc[2];
	Matrix4f            fProjection[2];          // Projection matrix for eye.
	Matrix4f            fOrthoProjection[2];     // Projection for 2D.
	ovrPosef            fEyeRenderPose[2];       // Poses we used for rendering.
	ovrD3D9Texture      fEyeTexture[2];
	Sizei				fRenderTargetSize;

	
public:

	//Flags
	enum {
		kUseRawInput,
		kUseEulerInput
	};

	plRiftCamera();
	virtual ~plRiftCamera();

	CLASSNAME_REGISTER(plRiftCamera);
	GETINTERFACE_ANY(plRiftCamera, hsKeyedObject);

	virtual bool MsgReceive(plMessage* msg);
	void SetFlags(int flag) { fFlags.SetBit(flag); }
	bool HasFlags(int flag) { return fFlags.IsBitSet(flag); }
	void ClearFlags(int flag) { fFlags.ClearBit(flag); }

	void InitRift();
	plRenderTarget* CreateRenderTarget(uint16_t width, uint16_t height);
	void SetCameraManager(plVirtualCam1* camManager){ fVirtualCam = camManager; };
	void SetPipeline(plPipeline* pipe){
		fPipe = pipe;
	};
	void SetD3DDevice(IDirect3DDevice9 *device){
		fDirect3DDevice = device;
	}
	ovrD3D9Texture GetOVRTex(int index){ return fEyeTexture[index]; }

	void EnableStereoRendering(bool state){ fEnableStereoRendering = state; };
	bool GetStereoRenderingState(){ return fEnableStereoRendering; };

	//Utils
	hsMatrix44* OVRTransformToHSTransform(Matrix4f OVRmat, hsMatrix44* hsMat);

	void SetXOffsetRotation(float offset){ fXRotOffset = M_PI * offset; };
	void SetYOffsetRotation(float offset){ fYRotOffset = M_PI * offset; };
	void SetZOffsetRotation(float offset){ fZRotOffset = M_PI * offset; };

	HmdTransform GetHmdTransform();
	void ApplyViewport();

	//Frame rendering
	void StartFrame();
	void EndFrame();
};

#endif