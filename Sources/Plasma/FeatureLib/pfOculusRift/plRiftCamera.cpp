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

#include "plRiftCamera.h"

#include "HeadSpin.h"
#include "hsTemplates.h"
#include "hsGeometry3.h"
#include "hsMatrix44.h"
#include "plstring.h"
#include "plGImage/plMipmap.h"
#include "plSurface/plLayer.h"
#include "plSurface/hsGMaterial.h"
#include "plMessage/plLayRefMsg.h"
#include "plResMgr/plResManager.h"
#include "plResMgr/plKeyFinder.h"
#include "pfCamera/plVirtualCamNeu.h"
#include "pfConsoleCore/pfConsoleEngine.h"
#include "pfConsole/pfConsole.h"
#include "pfConsole/pfConsoleDirSrc.h"
#include "plStatusLog/plStatusLog.h"
#include "plPipeline/plPlates.h"
#include "plPipeline.h"
#include "plViewTransform.h"
#include "pnKeyedObject/hsKeyedObject.h"
#include "pnKeyedObject/plKey.h"
#include "pnKeyedObject/plFixedKey.h"
#include "pnKeyedObject/plUoid.h"

plRiftCamera::plRiftCamera() : 
	fEnableStereoRendering(true),
	fRenderScale(1.0f),
	fEyeToRender(EYE_LEFT),
	fXRotOffset(M_PI),
	fYRotOffset(M_PI),
	fZRotOffset(M_PI),
	fEyeYaw(0.0f),
	fNear(0.3f),
	fFar(10000.0f)
{
	SetFlags(kUseEulerInput);
}

plRiftCamera::~plRiftCamera(){
	ovr_Destroy(pSession);
	ovr_Shutdown();
}

void LogCallback(uintptr_t userData, int level, const char* message)
{
	plStatusLog::AddLineS("oculus.log", "Oculus: [%i] %s", level, message);
}

void plRiftCamera::initRift(int width, int height){


	plStatusLog::AddLineS("oculus.log", "-- Attempting to initialize Rift --");
	ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_Debug, OVR_MINOR_VERSION, LogCallback, 0, 0 };

	ovrResult result = ovr_Initialize(&initParams);

	if (OVR_SUCCESS(result)) {
		plStatusLog::AddLineS("oculus.log", "-- Rift initialized with result %i. Attempting to create session --", result);
		ovr_TraceMessage(ovrLogLevel_Debug, "Testing trace message");
		result = ovr_Create(&pSession, &pLuid);
		
		plStatusLog::AddLineS("oculus.log", "After session creation");
		if (OVR_SUCCESS(result)) {
			plStatusLog::AddLineS("oculus.log", "-- Rift Session created --");
			ovr_RecenterTrackingOrigin(pSession);
		}
		else {
			plStatusLog::AddLineS("oculus.log", "-- Unable to create Rift Session --");
		}
	}
	else {
		plStatusLog::AddLineS("oculus.log", "-- Unable to initialize LibOVR, code %i --", result);
	}
	
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}

void plRiftCamera::ApplyStereoViewport(ovrEyeType eye)
{
	plViewTransform vt = fPipe->GetViewTransform();

	ovrFovPort fovPort;
	fovPort.DownTan = tan(vt.GetFovY() / 2);
	fovPort.UpTan = tan(vt.GetFovY() / 2);
	fovPort.LeftTan = tan(vt.GetFovX() / 2);
	fovPort.RightTan = tan(vt.GetFovX() / 2);
	ovrEyeRenderDesc eyeRenderDesc = ovr_GetRenderDesc(pSession, eye, fovPort);

	
	//fRenderScale = SConfig.GetDistortionScale();

	//Viewport stuff
	//--------------
	
	//vt.SetViewPort(eyeParams.VP.x * fRenderScale,
	//				eyeParams.VP.y * fRenderScale, 
	//				(eyeParams.VP.x + eyeParams.VP.w)  * fRenderScale, 
	//				(eyeParams.VP.y + eyeParams.VP.h) * fRenderScale, false);
	//
	hsMatrix44 eyeTransform, transposed, w2c, inverse;

	ovrTrackingState trackingState = ovr_GetTrackingState(pSession, 0.0, false);
	ovrPosef eyePoses[2];
	ovr_CalcEyePoses(trackingState.HeadPose.ThePose, &eyeRenderDesc.HmdToEyePose, eyePoses);
	//ovrPosef_FlipHandedness(&eyePoses[eye], &eyePoseFlipped);
	OVR::Matrix4f riftEyeTransform(eyePoses[eye]);
	OVRTransformToHSTransform(riftEyeTransform, &eyeTransform);
	//eyeTransform.fMap[0][3] *= 0.3048;	//Convert Rift meters to feet
	eyeTransform.Scale(&hsVector3(0.3048, 0.3048, 0.3048));

	// eyeTransform
	hsMatrix44 origW2c = fWorldToCam;

	w2c = eyeTransform * origW2c;
	w2c.GetInverse(&inverse);
	vt.SetCameraTransform( w2c, inverse );
	//vt.SetWidth(eyeParams.VP.w * fRenderScale);
	//vt.SetHeight(eyeParams.VP.h * fRenderScale);
	//vt.SetHeight(eyeParams.VP.h * fRenderScale);
	hsPoint2 depth;
	depth.Set(fNear, fFar);
	//vt.SetDepth(depth);
	

	//Projection matrix stuff
	//-------------------------
	hsMatrix44 projMatrix;

	hsMatrix44 oldCamNDC = vt.GetCameraToNDC();
	
	OVRTransformToHSTransform(OVR::CreateProjection(true, false, fovPort, OVR::StereoEye(eye)), &projMatrix);
	vt.SetProjectionMatrix(&projMatrix);

	//fPipe->ReverseCulling();

	fPipe->RefreshMatrices();
    
	fPipe->SetViewTransform(vt);	
	fPipe->SetViewport();

}


hsMatrix44* plRiftCamera::OVRTransformToHSTransform(ovrMatrix4f OVRmat, hsMatrix44* hsMat)
{
	hsMat->NotIdentity();
	//OVRmat.Transpose();
	int i,j;
	for(i=0; i < 4; i++){
		for(j=0; j < 4; j++){
			hsMat->fMap[i][j] = OVRmat.M[i][j];
		}
	}

	return hsMat;
}
