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
	fXRotOffset(3.1415926),
	fYRotOffset(3.1415926),
	fZRotOffset(3.1415926)
{
	/*
	YawInitial = 3.141592f;
	EyePos = Vector3f(0.0f, 1.6f, -5.0f),
    EyeYaw = YawInitial;
	EyePitch = 0.0f;
	EyeRoll = 0.0f;
    LastSensorYaw = 0.0f;
	UpVector = Vector3f(0.0f, 0.0f, 1.0f);
	ForwardVector = Vector3f(0.0f, 1.0f, 0.0f);
	RightVector = Vector3f(1.0f, 0.0f, 0.0f);
	*/
}

plRiftCamera::~plRiftCamera(){
}

void plRiftCamera::initRift(int width, int height){

	System::Init(Log::ConfigureDefaultLog(LogMask_All));

	pManager = *DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
	SFusion.SetPredictionEnabled(true);

	SConfig.SetFullViewport(Util::Render::Viewport(0, 0, width, height));
	SConfig.SetStereoMode(Util::Render::Stereo_LeftRight_Multipass);
	SConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
	fRenderScale = SConfig.GetDistortionScale();

	pfConsole::AddLine("-- Initializing Rift --");

	if(pHMD){
		pSensor = *pHMD->GetSensor();
		pfConsole::AddLine("- Found Rift -");

		 OVR::HMDInfo HMDInfo;
         pHMD->GetDeviceInfo(&HMDInfo);
	} else {
		pfConsole::AddLine("- No HMD found -");
	}

	if (pSensor){
		SFusion.AttachToSensor(pSensor);
		SFusion.SetPredictionEnabled(true);
	}
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}

void plRiftCamera::ApplyStereoViewport(Util::Render::StereoEye eye, bool scaled = false)
{
	Util::Render::StereoEyeParams eyeParams = SConfig.GetEyeRenderParams(eye);

	fRenderScale = SConfig.GetDistortionScale();

	if( !scaled)
		fRenderScale = 1.0f;


	//Viewport stuff
	//--------------
	plViewTransform vt = fPipe->GetViewTransform();
	vt.SetViewPort(eyeParams.VP.x * fRenderScale,
					eyeParams.VP.y * fRenderScale, 
					(eyeParams.VP.x + eyeParams.VP.w)  * fRenderScale, 
					(eyeParams.VP.y + eyeParams.VP.h) * fRenderScale, false);
	
	if(scaled){
		hsMatrix44 eyeTransform, transposed, w2c, inverse;
		OVRTransformToHSTransform(eyeParams.ViewAdjust, &eyeTransform);
		eyeTransform.fMap[0][3] *= -0.3048;	//Convert Rift meters to feet

		hsMatrix44 riftOrientation;
		riftOrientation.Reset();

		if(fVirtualCam){
			//if(fVirtualCam->HasFlags(plVirtualCam1::kHasUpdated)){
				riftOrientation = CalculateRiftCameraOrientation(fVirtualCam->GetCameraPos());
			//}
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
		depth.Set(0.3f, 500.0f);
		vt.SetDepth(depth);
	}

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



hsMatrix44 plRiftCamera::CalculateRiftCameraOrientation(hsPoint3 camPosition){
	//Matrix4f hmdMat(hmdOrient);

	Quatf riftOrientation = SFusion.GetPredictedOrientation();
	Matrix4f tester = Matrix4f(riftOrientation.Inverted());
	Vector3f camPos(camPosition.fX, camPosition.fY, camPosition.fZ);

	/*
	tester *= Matrix4f::RotationX(3.1415926 * -0.5);
	tester *= Matrix4f::RotationY(-3.1415926);
	tester *= Matrix4f::RotationZ(3.1415926 * -0.5);
	*/
	tester *= Matrix4f::RotationX(fXRotOffset);
	tester *= Matrix4f::RotationY(fYRotOffset);
	tester *= Matrix4f::RotationZ(fZRotOffset);

	//tester *= Matrix4f::Translation(-camPos);
	//tester *= Matrix4f::Translation(-camPos) * eyeConfig.ViewAdjust;
	
	hsMatrix44 testerAlt;
	OVRTransformToHSTransform(tester, &testerAlt);
	return testerAlt;
	
	//std::stringstream riftCons;
	//riftCons << riftOrientation.x << ", " << riftOrientation.y << ", " << riftOrientation.z << ", " << riftOrientation.w;
	//fConsole->AddLine(riftCons.str().c_str());
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

hsMatrix44* plRiftCamera::OVRProjectionToHSProjection(Matrix4f OVRmat, hsMatrix44* hsMat, float zMin, float zMax){
	hsMat->Reset(false);
	hsMat->NotIdentity();

	//OVRmat.Transpose();
	hsMat->fMap[0][0] = OVRmat.M[0][0];
	hsMat->fMap[0][1] = OVRmat.M[0][1];
	hsMat->fMap[0][2] = OVRmat.M[0][2];
	hsMat->fMap[0][3] = OVRmat.M[0][3];
	hsMat->fMap[1][0] = OVRmat.M[1][0];
	hsMat->fMap[1][1] = OVRmat.M[1][1];
	hsMat->fMap[1][2] = OVRmat.M[1][2];
	hsMat->fMap[1][3] = OVRmat.M[1][3];
	hsMat->fMap[2][0] = OVRmat.M[2][0];
	hsMat->fMap[2][1] = OVRmat.M[2][1];
	hsMat->fMap[2][2] = zMax / (zMax - zMin); //OVRmat.M[2][2]; //zMin;
	hsMat->fMap[2][3] =  1.0f;//OVRmat.M[2][3]; //zMax;
	hsMat->fMap[3][0] = OVRmat.M[3][0];
	hsMat->fMap[3][1] = OVRmat.M[3][1];
	hsMat->fMap[3][2] = -zMax * zMin / (zMax - zMin);
	hsMat->fMap[3][3] = 0.0;

	//For reference, here is what OVR is outputting
	//----------------------------------------------
	/*
	Matrix4f m;
    float    tanHalfFov = tan(yfov * 0.5f);
  
    m.M[0][0] = 1.0f / (aspect * tanHalfFov);
    m.M[1][1] = 1.0f / tanHalfFov;
    m.M[2][2] = zfar / (znear - zfar);
   // m.M[2][2] = zfar / (zfar - znear);
    m.M[3][2] = -1.0f;
    m.M[2][3] = (zfar * znear) / (znear - zfar);
    m.M[3][3] = 0.0f;

    // Note: Post-projection matrix result assumes Left-Handed coordinate system,    
    //       with Y up, X right and Z forward. This supports positive z-buffer values.
    // This is the case even for RHS cooridnate input.       
    return m;
	*/

	/* fCameraToNDC.fMap[0][0] = 2.f / (fMax.fX - fMin.fX);
        fCameraToNDC.fMap[0][2] = (fMax.fX + fMin.fX) / (fMax.fX - fMin.fX);

        fCameraToNDC.fMap[1][1] = 2.f / (fMax.fY - fMin.fY);
        fCameraToNDC.fMap[1][2] = (fMax.fY + fMin.fY) / (fMax.fY - fMin.fY);

        fCameraToNDC.fMap[2][2] = fMax.fZ / (fMax.fZ - fMin.fZ);
        fCameraToNDC.fMap[2][3] = -fMax.fZ * fMin.fZ / (fMax.fZ - fMin.fZ);

        fCameraToNDC.fMap[3][2] = 1.f;
        fCameraToNDC.fMap[3][3] = 0.f;
		*/

	return hsMat;
}




//LEFTOVER TESTING CODE - Is this useful for richard?


	
	/*Quatf    hmdOrient = SFusion.GetPredictedOrientation();

    float    yaw = 0.0f;
    hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &Player.EyePitch, &Player.EyeRoll);

    Player.EyeYaw += (yaw - Player.LastSensorYaw);
    Player.LastSensorYaw = yaw;
	*/

	/*
    // NOTE: We can get a matrix from orientation as follows:
    // Matrix4f hmdMat(hmdOrient);

    // Test logic - assign quaternion result directly to view:
    // Quatf hmdOrient = SFusion.GetOrientation();
    // View = Matrix4f(hmdOrient.Inverted()) * Matrix4f::Translation(-EyePos);

	//***** --- Working code here


	//***************************


	//fNewCamera->SetRiftOverrideMatrix(testerAlt);
	//fNewCamera-
	
	//
	//float yaw = 0;
	//float pitch = 0;
	//float roll = 0;
	//hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &pitch, &roll);

	//upp = tester.Transform(upp);
	//fNewCamera->SetRiftOverrideUp(hsVector3(upp.x, upp.y, upp.z));
	//fNewCamera->SetRiftOverridePOA(hsVector3(yaw, pitch, roll));
*/