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
#include "hsQuat.h"
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


void getViews(XrSession session, XrView* views, XrSpace space, XrTime displayTime);
void posef_flip_z(XrPosef *pose);

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

	pOpenGL = GetModuleHandle("opengl32.dll");
	plStatusLog::AddLineS("openxr.log", "opengl32.dll = %p", pOpenGL);
	plStatusLog::AddLineS("openxr.log", "-- Attempting to initialize Rift --");

	
	XrInstanceCreateInfo instanceCreateInfo{XR_TYPE_INSTANCE_CREATE_INFO};
	strcpy(instanceCreateInfo.applicationInfo.applicationName, "Uru Live VR");
	strcpy(instanceCreateInfo.applicationInfo.engineName, "Plasma");
	instanceCreateInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 2);

	const char* extensions[] = { "XR_KHR_opengl_enable" };
	instanceCreateInfo.enabledExtensionCount = 1;
	instanceCreateInfo.enabledExtensionNames = extensions;


	XrResult result = xrCreateInstance(&instanceCreateInfo, &pInstance);

	if (XR_SUCCEEDED(result)) {
		plStatusLog::AddLineS("openxr.log", "-- Rift initialized with result %i. Attempting to create session --", result);
		//ovr_TraceMessage(ovrLogLevel_Debug, "Testing trace message");

		XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
		systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		XR_REPORT(xrGetSystem(pInstance, &systemInfo, &pSystemId));

		plStatusLog::AddLineS("openxr.log", "-- System ID: %i --", pSystemId);

		static auto myWglGetCurrentContext = (decltype(wglGetCurrentContext)*)GetAnyGLFuncAddress("wglGetCurrentContext");
		static auto myWglGetCurrentDC = (decltype(wglGetCurrentDC)*)GetAnyGLFuncAddress("wglGetCurrentDC");
		static auto myGlGetError = (decltype(glGetError)*)GetAnyGLFuncAddress("glGetError");
		XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
		plStatusLog::AddLineS("openxr.log", "-- myWglGetCurrentContext = %p; myWglGetCurrentDC = %p --", myWglGetCurrentContext, myWglGetCurrentDC);
		graphicsBinding.hDC = myWglGetCurrentDC();
		graphicsBinding.hGLRC = myWglGetCurrentContext();

		plStatusLog::AddLineS("openxr.log", "-- DC=%p GLRC=%p", graphicsBinding.hDC, graphicsBinding.hGLRC);

		static auto myGlGetString = (decltype(glGetString)*)GetAnyGLFuncAddress("glGetString");
		const GLubyte* version = myGlGetString(GL_VERSION);
		plStatusLog::AddLineS("openxr.log", "GL_VERSION = %s", version);

		XrGraphicsRequirementsOpenGLKHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
		XR_REPORT(xrGetOpenGLGraphicsRequirementsKHR(pInstance, pSystemId, &graphicsRequirements));

		plStatusLog::AddLineS("openxr.log", "Requirements: Minimum: %i.%i.%i, Maximum: %i.%i.%i"
			, XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported)
			, XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported)
			, XR_VERSION_PATCH(graphicsRequirements.minApiVersionSupported)
			, XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported)
			, XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported)
			, XR_VERSION_PATCH(graphicsRequirements.maxApiVersionSupported));

		XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
		sessionCreateInfo.next = &graphicsBinding;
		sessionCreateInfo.systemId = pSystemId;
		sessionCreateInfo.createFlags = 0;

		result = xrCreateSession(pInstance, &sessionCreateInfo, &pSession);
		
		plStatusLog::AddLineS("openxr.log", "After session creation");
		if (XR_SUCCEEDED(result)) {
			plStatusLog::AddLineS("openxr.log", "-- Rift Session created --");
			XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
			referenceSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1;
			referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
			XR_REPORT(xrCreateReferenceSpace(pSession, &referenceSpaceCreateInfo, &pBaseSpace));
		}
		else {
			plStatusLog::AddLineS("openxr.log", "-- Unable to create Rift Session. Cause: %i --", result);
			auto glerror = myGlGetError();
			plStatusLog::AddLineS("openxr.log", "GL Error: %i", glerror);
		}
	}
	else {
		plStatusLog::AddLineS("openxr.log", "-- Unable to initialize LibOVR, code %i --", result);
	}
	plStatusLog::AddLineS("openxr.log", "GetModuleHandle(opengl32.dll) = %i", pOpenGL);

	uint32_t numOfViewConfigViews = 0;
	XR_REPORT(xrEnumerateViewConfigurationViews(pInstance, pSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &numOfViewConfigViews, nullptr));
	pViewConfigurationViews.clear();
	pViewConfigurationViews.resize(numOfViewConfigViews, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	plStatusLog::AddLineS("openxr.log", "numOfViewConfigViews = %i, pointer to pViewConfigurationViews = %p", numOfViewConfigViews, pViewConfigurationViews.data());
	XR_REPORT(xrEnumerateViewConfigurationViews(pInstance, pSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, numOfViewConfigViews, &numOfViewConfigViews, pViewConfigurationViews.data()));
	uint32_t num_formats = 0;
	xrEnumerateSwapchainFormats(pSession, num_formats, &num_formats, nullptr);
	std::vector<int64_t> formats(num_formats);
	xrEnumerateSwapchainFormats(pSession, num_formats, &num_formats, formats.data());
	std::vector<int64_t>::iterator format_iterator;
	for (format_iterator = formats.begin(); format_iterator != formats.end(); ++format_iterator) {
		plStatusLog::AddLineS("openxr.log", "Format: %x", *format_iterator);
	}
	for (int i = 0; i < 2; ++i) {
		XrViewConfigurationView viewConfigurationView = pViewConfigurationViews.at(i);
		XrSwapchainCreateInfo swapChainDesc{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
		swapChainDesc.usageFlags =  XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainDesc.format = 0x8C43; // GL_SRGB8_ALPHA8_EXT
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

		uint32_t numOfSwapchainImages = 0;
		xrEnumerateSwapchainImages(pTextureSwapChains[i], 0, &numOfSwapchainImages, nullptr);
		pSwapChainImages[i].clear();
		pSwapChainImages[i].resize(numOfSwapchainImages, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
		XR_REPORT(xrEnumerateSwapchainImages(pTextureSwapChains[i], numOfSwapchainImages, &numOfSwapchainImages, reinterpret_cast<XrSwapchainImageBaseHeader*>(pSwapChainImages[i].data())));
		
	}
	plStatusLog::AddLineS("openxr.log", "Created texture swap chains");

	
	
	
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}

bool plRiftCamera::BeginAndShouldRender()
{
	if (!sessionRunning) {
		return false;
	}
	XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
	XR_REPORT_FAILURE(xrWaitFrame(pSession, &frameWaitInfo, &pFrameState));
	XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
	getViews(pSession, pViews, pBaseSpace, pFrameState.predictedDisplayTime);
	XR_REPORT_FAILURE(xrBeginFrame(pSession, &frameBeginInfo));
	shouldRender = pFrameState.shouldRender;
	return pFrameState.shouldRender;
}

void plRiftCamera::ApplyStereoViewport(int eye)
{
	if (!sessionRunning) {
		return;
	}

	static auto myWglGetCurrentContext = (decltype(wglGetCurrentContext)*)GetAnyGLFuncAddress("wglGetCurrentContext");
	//plStatusLog::AddLineS("oculus.log", "Current context: %p", myWglGetCurrentContext());
	plViewTransform vt = fPipe->GetViewTransform();


	//Projection matrix stuff
	//-------------------------
	hsMatrix44 projMatrix;

	hsMatrix44 oldCamNDC = vt.GetCameraToNDC();

	XrMatrix4x4f projMatrixXr;
	XrMatrix4x4f scaleBeforeProjMatrixXr;
	XrMatrix4x4f projMatrixXrFlipped;
	XrMatrix4x4f_CreateProjectionFov(&projMatrixXr, GraphicsAPI::GRAPHICS_D3D, pViews[eye].fov, fNear, fFar);
	XrMatrix4x4f_CreateScale(&scaleBeforeProjMatrixXr, 1.0, 1.0, -1.0);
	XrMatrix4x4f_Multiply(&projMatrixXrFlipped, &projMatrixXr, &scaleBeforeProjMatrixXr);
	XRTransformToHSTransform(&projMatrixXrFlipped, &projMatrix);
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

	posef_flip_z(&pose);

	pose.position.x *= 3.281;
	pose.position.y *= 3.281;
	pose.position.y -= 6.0;
	pose.position.z *= 3.281;
	
	//OVRTransformToHSTransform(riftEyeTransform, &eyeTransform);

	// eyeTransform
	hsMatrix44 origW2c = fWorldToCam;

	w2c = hsMatrix44(origW2c);

	//hsMatrix44 vrCam, toVrCam, rotMatrix;
	//vrCam.Reset();
	//hsQuat(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w).MakeMatrix(&rotMatrix);
	//vrCam = rotMatrix * vrCam;
	//vrCam.Translate(&hsVector3(pose.position.x, pose.position.y, -pose.position.z));
	//vrCam.GetInverse(&toVrCam);

	//c2w.Translate(&hsVector3(pose.position.x, pose.position.z, pose.position.y - 6.0));

	XrMatrix4x4f vrCamXR;
	XrVector3f scale{ 1.0, 1.0, 1.0 };
	XrMatrix4x4f_CreateTranslationRotationScale(&vrCamXR, &pose.position, &pose.orientation, &scale);
	XrMatrix4x4f toVrCamXR;
	XrMatrix4x4f_Invert(&toVrCamXR, &vrCamXR);
	hsMatrix44 toVrCam;
	XRTransformToHSTransform(&toVrCamXR, &toVrCam);

	w2c = toVrCam * w2c;
	
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
			hsMat->fMap[i][j] = xrMat->m[i + 4*j]; // Transpose
		}
	}

	return hsMat;
}

// From https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
void *plRiftCamera::GetAnyGLFuncAddress(const char *name)
{
	plStatusLog::AddLineS("openxr.log", "Retrieving GL function %s", name);
	static auto myWglGetProcAddress = (decltype(wglGetProcAddress)*)GetProcAddress(pOpenGL, "wglGetProcAddress");
	void *p = (void *)myWglGetProcAddress(name);
	if (p == 0 ||
		(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1))
	{
		p = (void *)GetProcAddress(pOpenGL, name);
	}
	plStatusLog::AddLineS("openxr.log", "Retrieved %s = %p", name, p);

	return p;
}

void plRiftCamera::DrawToEye(int eye) {
	if (!sessionRunning || !shouldRender) {
		return;
	}

	static auto myGlEnable = (decltype(glEnable)*)GetAnyGLFuncAddress("glEnable");
	static auto myGlReadBuffer = (decltype(glReadBuffer)*)GetAnyGLFuncAddress("glReadBuffer");
	static auto myGlBindTexture = (decltype(glBindTexture)*)GetAnyGLFuncAddress("glBindTexture");
	static auto myGlCopyTexSubImage2D = (decltype(glCopyTexSubImage2D)*)GetAnyGLFuncAddress("glCopyTexSubImage2D");
	static auto myGlDisable = (decltype(glEnable)*)GetAnyGLFuncAddress("glDisable");

	unsigned int texid;

	uint32_t image_index;
	XrSwapchainImageAcquireInfo swapChainImageAcquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
	xrAcquireSwapchainImage(pTextureSwapChains[eye], &swapChainImageAcquireInfo, &image_index);

	XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	waitInfo.timeout = XR_INFINITE_DURATION;
	xrWaitSwapchainImage(pTextureSwapChains[eye], &waitInfo);

	texid = pSwapChainImages[eye].at(image_index).image;

	myGlEnable(GL_TEXTURE_2D);
	myGlReadBuffer(GL_BACK);
	myGlBindTexture(GL_TEXTURE_2D, texid);
	myGlCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, pViewConfigurationViews.at(eye).recommendedImageRectWidth, pViewConfigurationViews.at(eye).recommendedImageRectHeight);
		//myGlDisable(GL_TEXTURE_2D);
	XrSwapchainImageReleaseInfo swapchainImageReleaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
	xrReleaseSwapchainImage(pTextureSwapChains[eye], &swapchainImageReleaseInfo);
}

void plRiftCamera::Poll() {
	XrEventDataBuffer dataBuffer{ XR_TYPE_EVENT_DATA_BUFFER };
	while (XR_UNQUALIFIED_SUCCESS(xrPollEvent(pInstance, &dataBuffer))) {
		switch (dataBuffer.type) {
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
			XrEventDataSessionStateChanged* sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&dataBuffer);
			if (sessionStateChanged->state == XR_SESSION_STATE_READY) {
				XrSessionBeginInfo sessionBeginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
				sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
				XR_REPORT(xrBeginSession(pSession, &sessionBeginInfo));
				sessionRunning = true;
			}
			else if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING) {
				xrEndSession(pSession);
				sessionRunning = false;
			}
			break;
		}
		dataBuffer.type = XR_TYPE_EVENT_DATA_BUFFER;
		dataBuffer.next = nullptr;
	}

}

void plRiftCamera::Submit() {

	if (!sessionRunning) {
		return;
	}
	std::vector<XrCompositionLayerBaseHeader*> layers;

	XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	if (shouldRender) {
		XrCompositionLayerProjectionView layerViews[2];
		layer.viewCount = 2;
		layer.views = layerViews;
		makeLayerEyeFov(&layer, layerViews);
		layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
	}


	XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
	frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	frameEndInfo.displayTime = pFrameState.predictedDisplayTime;
	frameEndInfo.layers = layers.data();
	frameEndInfo.layerCount = layers.size();
	xrEndFrame(pSession, &frameEndInfo);


}

void plRiftCamera::makeLayerEyeFov(XrCompositionLayerProjection* out_Layer, XrCompositionLayerProjectionView* views) {

	out_Layer->space = pBaseSpace;
	
	for (int i = 0; i < 2; i++) {
		views[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
		views[i].next = nullptr;
		views[i].subImage.imageArrayIndex = 0;
		views[i].subImage.imageRect.offset = { 0, 0 };
		views[i].subImage.imageRect.extent = { (int32_t)pViewConfigurationViews[i].recommendedImageRectWidth, (int32_t)pViewConfigurationViews[i].recommendedImageRectHeight };
		views[i].subImage.swapchain = pTextureSwapChains[i];
		views[i].fov = pViews[i].fov;
		views[i].pose = pViews[i].pose;
	}

}

void posef_flip_z(XrPosef *pose) {
	// From Bradley Austin Davis on Khronos slack
	// Change to flip z instead of x: +z is forward in Plasma view space
	pose->orientation.z *= -1.0f;
	pose->orientation.w *= -1.0f;
	pose->position.z *= -1.0f;
}

void getViews(XrSession session, XrView* views, XrSpace space, XrTime displayTime) {

	XrViewState _viewState{ XR_TYPE_VIEW_STATE }; // TODO: Do I want to use this?

	XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
	viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	viewLocateInfo.space = space;
	viewLocateInfo.displayTime = displayTime;
	uint32_t _numOfViews;
	XR_REPORT_FAILURE(xrLocateViews(session, &viewLocateInfo, &_viewState, 2, &_numOfViews, views));
}

