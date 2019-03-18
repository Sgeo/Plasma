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

#include <windows.h>
#include <gl/GL.h>
#include <wingdi.h>

void makeLayerEyeFov(ovrSession session, ovrFovPort fov, ovrTextureSwapChain* swapChains, ovrLayerEyeFov* out_Layer);
void getEyes(ovrSession session, ovrFovPort fov, ovrPosef* eyes);

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
	pOpenGL = GetModuleHandle("opengl32.dll");
	plStatusLog::AddLineS("oculus.log", "GetModuleHandle(opengl32.dll) = %i", pOpenGL);
	ovrTextureSwapChainDesc swapChainDesc;
	swapChainDesc.Type = ovrTexture_2D;
	swapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	swapChainDesc.ArraySize = 1;
	swapChainDesc.Width = ovr_GetHmdDesc(pSession).Resolution.w/2;
	swapChainDesc.Height = ovr_GetHmdDesc(pSession).Resolution.h;
	swapChainDesc.MipLevels = 1;
	swapChainDesc.SampleCount = 1;
	swapChainDesc.StaticImage = 0;
	swapChainDesc.MiscFlags = 0;
	swapChainDesc.BindFlags = 0;
	plStatusLog::AddLineS("oculus.log", "About to create texture swap chain");
	static auto myWglGetCurrentContext = (decltype(wglGetCurrentContext)*)GetAnyGLFuncAddress("wglGetCurrentContext");
	plStatusLog::AddLineS("oculus.log", "Current context: %p", myWglGetCurrentContext());
	for (int i = 0; i < 2; ++i) {
		plStatusLog::AddLineS("oculus.log", "Texture swap chain params: %p, %p, %p", pSession, &swapChainDesc, &pTextureSwapChains[i]);
		ovrResult swapChainResult = ovr_CreateTextureSwapChainGL(pSession, &swapChainDesc, &pTextureSwapChains[i]);
	}
	plStatusLog::AddLineS("oculus.log", "Created texture swap chains");
	
	
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}

void plRiftCamera::ApplyStereoViewport(ovrEyeType eye)
{

	static auto myWglGetCurrentContext = (decltype(wglGetCurrentContext)*)GetAnyGLFuncAddress("wglGetCurrentContext");
	//plStatusLog::AddLineS("oculus.log", "Current context: %p", myWglGetCurrentContext());
	plViewTransform vt = fPipe->GetViewTransform();

	ovrFovPort fovPort;
	fovPort.DownTan = tan(vt.GetFovY() / 2);
	fovPort.UpTan = tan(vt.GetFovY() / 2);
	fovPort.LeftTan = tan(vt.GetFovX() / 2);
	fovPort.RightTan = tan(vt.GetFovX() / 2);
	pFovPort = fovPort;
	
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
	ovrPosef eyePoseFlipped;
	getEyes(pSession, fovPort, eyePoses);
	ovrPosef_FlipHandedness(&eyePoses[eye], &eyePoseFlipped);
	eyePoseFlipped.Position.x *= 3.281;
	eyePoseFlipped.Position.y *= -3.281;
	eyePoseFlipped.Position.z *= 3.281;
	eyePoseFlipped.Orientation;
	OVR::Matrix4f riftEyeTransform(eyePoseFlipped);
	
	//OVRTransformToHSTransform(riftEyeTransform, &eyeTransform);

	// eyeTransform
	hsMatrix44 origW2c = fWorldToCam;

	w2c = hsMatrix44(origW2c);
	w2c.Translate(&hsVector3(eyePoseFlipped.Position.x, eyePoseFlipped.Position.y, eyePoseFlipped.Position.z));
	float rotx, roty, rotz;
	// I'm uncertain why the indicated values for RotateDirection and HandedSystem work, but they do.
	riftEyeTransform.ToEulerAngles<OVR::Axis::Axis_X, OVR::Axis::Axis_Y, OVR::Axis::Axis_Z, OVR::RotateDirection::Rotate_CW, OVR::HandedSystem::Handed_R>(&rotx, &roty, &rotz);
	w2c.Rotate(0, rotx);
	w2c.Rotate(1, roty);
	w2c.Rotate(2, rotz);
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

// From https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
void *plRiftCamera::GetAnyGLFuncAddress(const char *name)
{
	static auto myWglGetProcAddress = (decltype(wglGetProcAddress)*)GetProcAddress(pOpenGL, "wglGetProcAddress");
	void *p = (void *)myWglGetProcAddress(name);
	if (p == 0 ||
		(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1))
	{
		p = (void *)GetProcAddress(pOpenGL, name);
	}

	return p;
}

void plRiftCamera::DrawToEye(ovrEyeType eye) {

	static auto myGlEnable = (decltype(glEnable)*)GetAnyGLFuncAddress("glEnable");
	static auto myGlReadBuffer = (decltype(glReadBuffer)*)GetAnyGLFuncAddress("glReadBuffer");
	static auto myGlBindTexture = (decltype(glBindTexture)*)GetAnyGLFuncAddress("glBindTexture");
	static auto myGlCopyTexSubImage2D = (decltype(glCopyTexSubImage2D)*)GetAnyGLFuncAddress("glCopyTexSubImage2D");
	static auto myGlDisable = (decltype(glEnable)*)GetAnyGLFuncAddress("glDisable");

	unsigned int texid;
	ovr_GetTextureSwapChainBufferGL(pSession, pTextureSwapChains[eye], -1, &texid);

	myGlEnable(GL_TEXTURE_2D);
	myGlReadBuffer(GL_FRONT);
	myGlBindTexture(GL_TEXTURE_2D, texid);
	myGlCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, ovr_GetHmdDesc(pSession).Resolution.w / 2, ovr_GetHmdDesc(pSession).Resolution.h);
	myGlDisable(GL_TEXTURE_2D);

	ovr_CommitTextureSwapChain(pSession, pTextureSwapChains[eye]);
}

void plRiftCamera::Submit() {
	ovrLayerEyeFov layer;
	ovrLayerHeader* layers = &layer.Header;
	makeLayerEyeFov(pSession, pFovPort, pTextureSwapChains, &layer);

	ovr_SubmitFrame(pSession, 0, NULL, &layers, 1);
}

void makeLayerEyeFov(ovrSession session, ovrFovPort fov, ovrTextureSwapChain* swapChains, ovrLayerEyeFov* out_Layer) {
	out_Layer->Header.Type = ovrLayerType_EyeFov;
	//out_Layer->Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
	out_Layer->Header.Flags = 0;
	out_Layer->ColorTexture[0] = swapChains[0];
	out_Layer->ColorTexture[1] = swapChains[1];
	out_Layer->Viewport->Pos.x = 0;
	out_Layer->Viewport->Pos.y = 0;
	out_Layer->Viewport->Size.w = ovr_GetHmdDesc(session).Resolution.w/2;
	out_Layer->Viewport->Size.h = ovr_GetHmdDesc(session).Resolution.h;
	out_Layer->Fov->DownTan = fov.DownTan;
	out_Layer->Fov->UpTan = fov.UpTan;
	out_Layer->Fov->LeftTan = fov.LeftTan;
	out_Layer->Fov->RightTan = fov.RightTan;
	ovrPosef eyeRenderPose[2];
	getEyes(session, fov, eyeRenderPose);
	out_Layer->RenderPose[0] = eyeRenderPose[0];
	out_Layer->RenderPose[1] = eyeRenderPose[1];
	out_Layer->SensorSampleTime = 0.0;

}

void getEyes(ovrSession session, ovrFovPort fov, ovrPosef* eyes) {
	plStatusLog::AddLineS("oculus.log", "About to get eyes");
	ovrEyeRenderDesc eyeRenderDesc[2];
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, fov);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, fov);
	ovrPosef HmdToEyePose[2] = { eyeRenderDesc[0].HmdToEyePose, eyeRenderDesc[1].HmdToEyePose };
	ovr_GetEyePoses(session, 0, false, HmdToEyePose, eyes, NULL);
	plStatusLog::AddLineS("oculus.log", "Got eyes");
}
