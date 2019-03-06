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
	fUpVector(0.0f, 1.0f, 0.0f),
	fForwardVector(0.0f,0.0f,1.0f),
	fRightVector(1.0f, 0.0f, 0.0f),
	fNear(0.3f),
	fFar(10000.0f)
{
	SetFlags(kUseEulerInput);
}

plRiftCamera::~plRiftCamera(){
	pSensor.Clear();
    pHMD.Clear();
	pManager.Clear();
	fVirtualCam = nil;
	fPipe = nil;
}

void plRiftCamera::initRift(int width, int height){


	pfConsole::AddLine("-- Attempting to initialize Rift --");
	ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_MixedRendering, OVR_MINOR_VERSION, NULL, 0, 0 };

	ovrResult result = ovr_Initialize(&initParams);

	if (OVR_SUCCESS(result)) {
		pfConsole::AddLine("-- Rift initialized. Attempting to create session --");
		result = ovr_Create(pSession, pLuid);
		if (OVR_SUCCESS(result)) {
			pfConsole::AddLine("-- Rift Session created --");
		}
		else {
			pfConsole::AddLine("-- Unable to create Rift Session --");
		}
	}
	else {
		pfConsole::AddLine("-- Unable to initialize LibOVR --");
	}
	
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}

void plRiftCamera::ApplyStereoViewport(Util::Render::StereoEye eye)
{
	Util::Render::StereoEyeParams eyeParams = SConfig.GetEyeRenderParams(eye);

	fRenderScale = 1.0f;
	fRenderScale = SConfig.GetDistortionScale();

	//Viewport stuff
	//--------------
	plViewTransform vt = fPipe->GetViewTransform();
	vt.SetViewPort(eyeParams.VP.x * fRenderScale,
					eyeParams.VP.y * fRenderScale, 
					(eyeParams.VP.x + eyeParams.VP.w)  * fRenderScale, 
					(eyeParams.VP.y + eyeParams.VP.h) * fRenderScale, false);
	
	hsMatrix44 eyeTransform, transposed, w2c, inverse;
	OVRTransformToHSTransform(eyeParams.ViewAdjust, &eyeTransform);
	eyeTransform.fMap[0][3] *= 0.3048;	//Convert Rift meters to feet

	hsMatrix44 riftOrientation;

	if(HasFlags(kUseRawInput)){
		riftOrientation = RawRiftRotation();
	} else if(HasFlags(kUseEulerInput)){
		riftOrientation = EulerRiftRotation();
	}

	eyeTransform = riftOrientation * eyeTransform;
	hsMatrix44 origW2c = fWorldToCam;

	w2c = eyeTransform * origW2c;
	w2c.GetInverse(&inverse);
	vt.SetCameraTransform( w2c, inverse );
	vt.SetWidth(eyeParams.VP.w * fRenderScale);
	vt.SetHeight(eyeParams.VP.h * fRenderScale);
	vt.SetHeight(eyeParams.VP.h * fRenderScale);
	hsPoint2 depth;
	depth.Set(fNear, fFar);
	//vt.SetDepth(depth);
	

	//Projection matrix stuff
	//-------------------------
	hsMatrix44 projMatrix;

	hsMatrix44 oldCamNDC = vt.GetCameraToNDC();
	
	OVRTransformToHSTransform(eyeParams.Projection, &projMatrix);
	vt.SetProjectionMatrix(&projMatrix);

	//fPipe->ReverseCulling();

	fPipe->RefreshMatrices();
    
	fPipe->SetViewTransform(vt);	
	fPipe->SetViewport();
}

hsMatrix44 plRiftCamera::RawRiftRotation(){
	hsMatrix44 outView;
	Quatf riftOrientation = SFusion.GetPredictedOrientation();
	Matrix4f riftMatrix = Matrix4f(riftOrientation.Inverted());

	riftMatrix *= Matrix4f::RotationX(fXRotOffset);
	riftMatrix *= Matrix4f::RotationY(fYRotOffset);
	riftMatrix *= Matrix4f::RotationZ(fZRotOffset);

	hsMatrix44 testerAlt;
	OVRTransformToHSTransform(riftMatrix, &outView);
	return outView;
}


hsMatrix44 plRiftCamera::EulerRiftRotation(){
	hsMatrix44 outView;
	float yaw = 0.0f;

	// Minimal head modeling; should be moved as an option to SensorFusion.
    float headBaseToEyeHeight     = 0.15f;  // Vertical height of eye from base of head
    float headBaseToEyeProtrusion = 0.09f;  // Distance forward of eye from base of head

	Quatf riftOrientation = SFusion.GetPredictedOrientation();
	//Matrix4f tester = Matrix4f(riftOrientation.Inverted());
	//Vector3f camPos(camPosition.fX, camPosition.fY, camPosition.fZ);

	riftOrientation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &fEyePitch, &fEyeRoll);

	fEyeYaw += (yaw - fLastSensorYaw);
    fLastSensorYaw = yaw;

	Matrix4f rollPitchYaw = Matrix4f::RotationY( ReverseRadians(yaw) ) * Matrix4f::RotationX(-fEyePitch) * Matrix4f::RotationZ(fEyeRoll);
	Vector3f up      = rollPitchYaw.Transform(fUpVector);
	Vector3f forward = rollPitchYaw.Transform(fForwardVector);

	Matrix4f riftMatrix = Matrix4f::LookAtLH( Vector3f(0.0f, 0.0f, 0.0f), forward, up);
	riftMatrix *= Matrix4f::RotationX(fXRotOffset);
	riftMatrix *= Matrix4f::RotationY(fYRotOffset);
	riftMatrix *= Matrix4f::RotationZ(fZRotOffset);

	OVRTransformToHSTransform(riftMatrix, &outView);

	//Vector3f empty(0.0f, 0.0f, 0.0f);
    //Vector3f eyeCenterInHeadFrame(0.0f, headBaseToEyeHeight, -headBaseToEyeProtrusion);
	//Vector3f shiftedEyePos = Vector3f(camPosition.fX, camPosition.fY, camPosition.fZ) + rollPitchYaw.Transform(empty);
    //shiftedEyePos.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height
	//Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos + forward, up).Invert();
	//OVRTransformToHSTransform(view, &outView);

	//Vector3f lookAt(Vector3f(camPosition.fX, camPosition.fY, camPosition.fZ) + forward);
	//hsPoint3 targetPOA(forward.x, forward.y, forward.z);
	//hsPoint3 targetPos(shiftedEyePos.x, shiftedEyePos.y, shiftedEyePos.z);
	//hsVector3 targetUp(up.x, up.y, up.z);
	//outView.MakeCamera(&camPosition, &targetPOA, &targetUp);

	return outView;
}


hsMatrix44* plRiftCamera::OVRTransformToHSTransform(Matrix4f OVRmat, hsMatrix44* hsMat)
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
