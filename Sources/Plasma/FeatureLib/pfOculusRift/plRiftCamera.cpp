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
#include "plPipeline\plRenderTarget.h"
#include "../../Apps/plClient/plClient.h"


plRiftCamera::plRiftCamera() : 
	fEnableStereoRendering(true),
	fXRotOffset(M_PI),
	fYRotOffset(M_PI),
	fZRotOffset(M_PI)
	
{
	SetFlags(kUseEulerInput);
}

plRiftCamera::~plRiftCamera(){
	fVirtualCam = nil;
	fPipe = nil;
	ovrHmd_Destroy(fHmd);
	ovr_Shutdown();
}

void plRiftCamera::InitRift(){
	pfConsole::AddLine("-- Initializing Rift --");
	ovr_Initialize();

	fHmd = ovrHmd_Create(0);

	if (!fHmd){
		pfConsole::AddLine("!! No Rift found! Creating fake debug HMD !!");
		fHmd = ovrHmd_CreateDebug(ovrHmd_DK2);
		bHmdIsDebug = true;
	}

	ovrSizei resolution = fHmd->Resolution;

	// Start the sensor which provides the Rift’s pose and motion.
	ovrHmd_ConfigureTracking(fHmd, ovrTrackingCap_Orientation | 
									ovrTrackingCap_MagYawCorrection | 
									ovrTrackingCap_Position, 0);

	ovrFovPort eyeFov[2];
	eyeFov[0] = fHmd->DefaultEyeFov[0];
	eyeFov[1] = fHmd->DefaultEyeFov[1];

	// Configure Stereo settings.
	Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(fHmd, ovrEye_Left, eyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(fHmd, ovrEye_Right, eyeFov[1], 1.0f);

	fRenderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	fRenderTargetSize.h = max(recommenedTex0Size.h, recommenedTex1Size.h);
	const int eyeRenderMultisample = 1;

	pfConsole::AddLine("RT width:" + recommenedTex0Size.w);
	pfConsole::AddLine("RT height:" + recommenedTex0Size.h);

	fSteroRenderTarget = CreateRenderTarget(fRenderTargetSize.w, fRenderTargetSize.h);
	fSteroRenderTarget->SetScreenRenderTarget(true);

	// Pass D3D texture data, including ID3D11Texture2D and ID3D11ShaderResourceView pointers.
	//Texture* rtt = (Texture*)pRendertargetTexture;
	fEyeTexture[0].D3D9.Header.API = ovrRenderAPI_D3D9;
	fEyeTexture[0].D3D9.Header.TextureSize = fRenderTargetSize;
	fEyeTexture[0].D3D9.Header.RenderViewport = Recti(fRenderTargetSize);

	// Right eye uses the same texture
	fEyeTexture[1] = fEyeTexture[0];

	//Attach render target textures to rift
	fPipe->AttachRiftCam(this, fSteroRenderTarget);


	//Set up HMD rendering options
	// Hmd caps.
	unsigned hmdCaps = ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction;
	ovrHmd_SetEnabledCaps(fHmd, hmdCaps);

	ovrD3D9Config config;
	
	config.D3D9.Header.API = ovrRenderAPI_D3D9;
	config.D3D9.Header.RTSize = fRenderTargetSize;
	//config.D3D9.Header.Multisample = Params.Multisample;
	config.D3D9.pDevice = fDirect3DDevice;
	fDirect3DDevice->GetSwapChain(0, &config.D3D9.pSwapChain);

	unsigned distortionCaps = ovrDistortionCap_Chromatic |
		ovrDistortionCap_Vignette |
		ovrDistortionCap_SRGB | 
		ovrDistortionCap_Overdrive | 
		ovrDistortionCap_TimeWarp;

	ovrHmd_ConfigureRendering(fHmd, &config.Config, distortionCaps, eyeFov, fEyeRenderDesc);
	ovrHmd_AttachToWindow(fHmd, plClient::GetInstance()->GetWindowHandle(), NULL, NULL);
}

plRenderTarget* plRiftCamera::CreateRenderTarget(uint16_t width, uint16_t height){
	// Create our render target
	const uint16_t flags = plRenderTarget::kIsOffscreen;
	const uint8_t bitDepth(32);
	const uint8_t zDepth(-1);
	const uint8_t stencilDepth(-1);

	plRenderTarget* renderTarget = new plRenderTarget(flags, (uint16_t)(width), (uint16_t)(height), bitDepth, zDepth, stencilDepth);

	return renderTarget;
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}


HmdTransform plRiftCamera::GetHmdTransform(){
	// Query the HMD for the current tracking state.	ovrTrackingState ts = ovrHmd_GetTrackingState(fHmd, ovr_GetTimeInSeconds());
	return HmdTransform();
}

void plRiftCamera::ApplyViewport(ovrRecti viewport, bool resetProjection = false)
{
	fVp = viewport;

	plViewTransform vt = fPipe->GetViewTransform();

	vt.SetViewPort(fVp.Pos.x, fVp.Pos.y, fVp.Pos.x + fVp.Size.w, fVp.Pos.y + fVp.Size.h, false);

	if (resetProjection){
		vt.SetWidth((float)fVp.Size.w);
		vt.SetHeight((float)fVp.Size.h);
		vt.SetDepth(0.3f, 500.0f);
		vt.ResetProjectionMatrix();
	}

	fPipe->SetViewTransform(vt);
	fPipe->SetViewport();
}


void plRiftCamera::StartFrame(){
	ovrFrameTiming hmdFrameTiming = ovrHmd_BeginFrame(fHmd, 0);

}

void plRiftCamera::EndFrame(){

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
