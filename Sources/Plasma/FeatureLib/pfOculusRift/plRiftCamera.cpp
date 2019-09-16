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

void makeLayerEyeFov(XrSession session, int width, int height, XrSwapchain* swapChains, XrCompositionLayerProjection* out_Layer);
void getViews(XrSession session, XrView * views);

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
	xrDestroySession(pSession);
	xrDestroyInstance(pInstance);
}

void LogCallback(uintptr_t userData, int level, const char* message)
{
	plStatusLog::AddLineS("openxr.log", "OpenXR: [%i] %s", level, message);
}

void plRiftCamera::initRift(int width, int height){


	plStatusLog::AddLineS("openxr.log", "-- Attempting to initialize Rift --");

	
	XrInstanceCreateInfo instanceCreateInfo{XR_TYPE_INSTANCE_CREATE_INFO};
	strcpy(instanceCreateInfo.applicationInfo.applicationName, "Uru Live VR");
	strcpy(instanceCreateInfo.applicationInfo.engineName, "Plasma");
	instanceCreateInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 2);

	const char* extensions[] = { "XR_KHR_opengl_enable" };
	instanceCreateInfo.enabledExtensionCount = 1;
	instanceCreateInfo.enabledExtensionNames = extensions;


	XrResult result = xrCreateInstance(&instanceCreateInfo, &pInstance);

	if (XR_SUCCESS(result)) {
		plStatusLog::AddLineS("openxr.log", "-- Rift initialized with result %i. Attempting to create session --", result);
		//ovr_TraceMessage(ovrLogLevel_Debug, "Testing trace message");

		XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
		systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		xrGetSystem(pInstance, &systemInfo, &pSystemId);

		static auto myWglGetCurrentContext = (decltype(wglGetCurrentContext)*)GetAnyGLFuncAddress("wglGetCurrentContext");
		static auto myWglGetCurrentDC = (decltype(wglGetCurrentDC)*)GetAnyGLFuncAddress("wglGetCurrentDC");
		XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
		graphicsBinding.hDC = myWglGetCurrentDC();
		graphicsBinding.hGLRC = myWglGetCurrentContext();

		XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
		sessionCreateInfo.next = &graphicsBinding;
		sessionCreateInfo.systemId = pSystemId;

		result = xrCreateSession(pInstance, &sessionCreateInfo, &pSession);
		
		plStatusLog::AddLineS("openxr.log", "After session creation");
		if (XR_SUCCESS(result)) {
			plStatusLog::AddLineS("openxr.log", "-- Rift Session created --");
			XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
			referenceSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1;
			referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
			xrCreateReferenceSpace(pSession, &referenceSpaceCreateInfo, &pBaseSpace);
		}
		else {
			plStatusLog::AddLineS("openxr.log", "-- Unable to create Rift Session --");
		}
	}
	else {
		plStatusLog::AddLineS("openxr.log", "-- Unable to initialize LibOVR, code %i --", result);
	}
	pOpenGL = GetModuleHandle("opengl32.dll");
	plStatusLog::AddLineS("openxr.log", "GetModuleHandle(opengl32.dll) = %i", pOpenGL);

	size_t numOfViewConfigViews = 0;
	xrEnumerateViewConfigurationViews(pInstance, pSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &numOfViewConfigViews, nullptr);
	pViewConfigurationViews.reserve(numOfViewConfigViews);
	xrEnumerateViewConfigurationViews(pInstance, pSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, numOfViewConfigViews, &numOfViewConfigViews, pViewConfigurationViews.data());
	for (int i = 0; i < 2; ++i) {
		XrViewConfigurationView viewConfigurationView = pViewConfigurationViews.at(i);
		XrSwapchainCreateInfo swapChainDesc{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
		swapChainDesc.usageFlags = XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
		swapChainDesc.format = GL_RGBA8; // TODO: ???
		swapChainDesc.sampleCount = viewConfigurationView.recommendedSwapchainSampleCount;
		swapChainDesc.width = viewConfigurationView.recommendedImageRectWidth;
		swapChainDesc.height = viewConfigurationView.recommendedImageRectHeight;
		swapChainDesc.faceCount = 1;
		swapChainDesc.arraySize = 1;
		swapChainDesc.mipCount = 1;
		// swapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

		plStatusLog::AddLineS("openxr.log", "About to create texture swap chain");
		static auto myWglGetCurrentContext = (decltype(wglGetCurrentContext)*)GetAnyGLFuncAddress("wglGetCurrentContext");
		plStatusLog::AddLineS("openxr.log", "Current context: %p", myWglGetCurrentContext());
	
		plStatusLog::AddLineS("openxr.log", "Texture swap chain params: %p, %p, %p", pSession, &swapChainDesc, &pTextureSwapChains[i]);
		XrResult swapChainResult = xrCreateSwapchain(pSession, &swapChainDesc, &pTextureSwapChains[i]);
		
	}
	plStatusLog::AddLineS("openxr.log", "Created texture swap chains");
	
	
	
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}

bool plRiftCamera::BeginAndShouldRender()
{
	XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
	XrFrameState frameState;
	xrWaitFrame(pSession, &frameWaitInfo, &frameState);
	XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
	getViews(pSession, pViews, pBaseSpace, frameState.predictedDisplayTime);
	xrBeginFrame(pSession, &frameBeginInfo);
	return frameState.shouldRender;
}

void plRiftCamera::ApplyStereoViewport(int eye)
{

	static auto myWglGetCurrentContext = (decltype(wglGetCurrentContext)*)GetAnyGLFuncAddress("wglGetCurrentContext");
	//plStatusLog::AddLineS("oculus.log", "Current context: %p", myWglGetCurrentContext());
	plViewTransform vt = fPipe->GetViewTransform();


	//Projection matrix stuff
	//-------------------------
	hsMatrix44 projMatrix;

	hsMatrix44 oldCamNDC = vt.GetCameraToNDC();

	XrMatrix4x4f projMatrixXr;
	XrMatrix4x4f_CreateProjectionFov(&projMatrixXr, GraphicsAPI::GRAPHICS_D3D, pViews[eye].fov, fNear, fFar);
	vt.SetProjectionMatrix(&projMatrix);

	//fPipe->ReverseCulling();

	fPipe->RefreshMatrices();
	
	//fRenderScale = SConfig.GetDistortionScale();

	//Viewport stuff
	//--------------
	
	//vt.SetViewPort(eyeParams.VP.x * fRenderScale,
	//				eyeParams.VP.y * fRenderScale, 
	//				(eyeParams.VP.x + eyeParams.VP.w)  * fRenderScale, 
	//				(eyeParams.VP.y + eyeParams.VP.h) * fRenderScale, false);
	//
	hsMatrix44 eyeTransform, transposed, w2c, inverse;


	XrPosef pose = pViews[eye].pose;

	XrPosef_FlipHandedness(&pose);

	pose.position.x *= 3.281;
	pose.position.y *= -3.281;
	pose.position.z *= 3.281;
	pose.position.y += 6.0;
	
	//OVRTransformToHSTransform(riftEyeTransform, &eyeTransform);

	// eyeTransform
	hsMatrix44 origW2c = fWorldToCam;

	w2c = hsMatrix44(origW2c);
	XrMatrix4x4f eyeMatrix;
	XrVector3f scale{ 1.0, 1.0, 1.0 };
	XrMatrix4x4f_CreateTranslationRotationScale(&eyeMatrix, &pose.position, &pose.orientation, &scale);
	hsMatrix44 eyeMatrixHS;
	XRTransformToHSTransform(&eyeMatrix, &eyeMatrixHS);
	w2c = eyeMatrixHS * w2c; // Todo: Is this the correct order? Is this approach ideal instead of decomposing the quaternion?
	w2c.GetInverse(&inverse);
	vt.SetCameraTransform( w2c, inverse );
	//vt.SetWidth(eyeParams.VP.w * fRenderScale);
	//vt.SetHeight(eyeParams.VP.h * fRenderScale);
	//vt.SetHeight(eyeParams.VP.h * fRenderScale);
	hsPoint2 depth;
	depth.Set(fNear, fFar);
	//vt.SetDepth(depth);
	


    
	fPipe->SetViewTransform(vt);	
	fPipe->SetViewport();

}


hsMatrix44* plRiftCamera::XRTransformToHSTransform(XrMatrix4x4f* xrMat, hsMatrix44* hsMat)
{
	hsMat->NotIdentity();
	//OVRmat.Transpose();
	int i,j;
	for(i=0; i < 4; i++){
		for(j=0; j < 4; j++){
			hsMat->fMap[i][j] = xrMat->m[4*i + j]; // TODO: Is this correct?
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

void plRiftCamera::DrawToEye(int eye) {

	static auto myGlEnable = (decltype(glEnable)*)GetAnyGLFuncAddress("glEnable");
	static auto myGlReadBuffer = (decltype(glReadBuffer)*)GetAnyGLFuncAddress("glReadBuffer");
	static auto myGlBindTexture = (decltype(glBindTexture)*)GetAnyGLFuncAddress("glBindTexture");
	static auto myGlCopyTexSubImage2D = (decltype(glCopyTexSubImage2D)*)GetAnyGLFuncAddress("glCopyTexSubImage2D");
	static auto myGlDisable = (decltype(glEnable)*)GetAnyGLFuncAddress("glDisable");

	unsigned int texid;
	ovr_GetTextureSwapChainBufferGL(pSession, pTextureSwapChains[eye], -1, &texid);

	myGlEnable(GL_TEXTURE_2D);
	myGlReadBuffer(GL_BACK);
	myGlBindTexture(GL_TEXTURE_2D, texid);
	myGlCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, pViewConfigurationViews.at(eye).recommendedImageRectWidth, pViewConfigurationViews.at(eye).recommendedImageRectHeight)
	//myGlDisable(GL_TEXTURE_2D);

	ovr_CommitTextureSwapChain(pSession, pTextureSwapChains[eye]);
}

void plRiftCamera::Submit() {
	ovrLayerEyeFov layer;
	ovrLayerHeader* layers = &layer.Header;
	makeLayerEyeFov(pSession, fPipe->GetViewTransform().GetScreenWidth(), fPipe->GetViewTransform().GetScreenHeight(), pTextureSwapChains, &layer);

	ovr_SubmitFrame(pSession, 0, NULL, &layers, 1);
}

void makeLayerEyeFov(ovrSession session, int width, int height, ovrTextureSwapChain* swapChains, ovrLayerEyeFov* out_Layer) {
	out_Layer->Header.Type = ovrLayerType_EyeFov;
	//out_Layer->Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
	out_Layer->Header.Flags = 0;
	out_Layer->ColorTexture[0] = swapChains[0];
	out_Layer->ColorTexture[1] = swapChains[1];
	out_Layer->Viewport[0].Pos.x = 0;
	out_Layer->Viewport[0].Pos.y = 0;
	out_Layer->Viewport[0].Size.w = width;
	out_Layer->Viewport[0].Size.h = height;
	out_Layer->Viewport[1].Pos.x = 0;
	out_Layer->Viewport[1].Pos.y = 0;
	out_Layer->Viewport[1].Size.w = width;
	out_Layer->Viewport[1].Size.h = height;
	out_Layer->Fov[0] = ovr_GetHmdDesc(session).DefaultEyeFov[0];
	out_Layer->Fov[1] = ovr_GetHmdDesc(session).DefaultEyeFov[1];
	ovrPosef eyeRenderPose[2];
	getEyes(session, eyeRenderPose);
	out_Layer->RenderPose[0] = eyeRenderPose[0];
	out_Layer->RenderPose[1] = eyeRenderPose[1];
	out_Layer->SensorSampleTime = 0.0;

}

void XrPosef_FlipHandedness(XrPosef *pose) {
	// From Bradley Austin Davis on Khronos slack
	pose->orientation.x *= -1.0f;
	pose->orientation.w *= -1.0f;
	pose->position.x *= -1.0f;
}

void getViews(XrSession session, XrView* views, XrSpace space, XrTime displayTime) {

	XrViewState _viewState; // TODO: Do I want to use this?

	XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
	viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	viewLocateInfo.space = space;
	viewLocateInfo.displayTime = displayTime;
	uint32_t _numOfViews;
	xrLocateViews(session, &viewLocateInfo, &_viewState, 2, &_numOfViews, views);
}

