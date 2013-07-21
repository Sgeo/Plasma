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
ne
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

#include "plPostPipeline.h"
#include "plPipeline.h"
#include "plPipeline/plRenderTarget.h"
#include "plPipeline/plPlates.h"
#include "plViewTransform.h"
//#include "plPipeline/plDXTextureRef.h"
#include "plSurface/plLayer.h"
#include "plSurface/plShader.h"
#include "plSurface/plShaderTable.h"
#include "plSurface/hsGMaterial.h"
#include "plGImage/plMipmap.h"
#include "plResMgr/plResManager.h"
#include "plResMgr/plKeyFinder.h"
#include "plMessage/plLayRefMsg.h"
#include "hsTemplates.h"
//#include "plPipeline/plDXRenderTargetRef.h"


plPostPipeline* plPostPipeline::fInstance=nil;

plPostPipeline::plPostPipeline(): 
	fEnablePost(true), 
	fRenderScale(DEFAULT_RIFTSCALE)
{
		CreateShaders();
}

plPostPipeline::~plPostPipeline(){
	delete fVsShader;
	delete fPsShader;
	ReleaseGeometry();
	ReleaseTextures();
	fPipe = nil;
	fInstance = nil;
}

void plPostPipeline::ReleaseGeometry(){
}

void plPostPipeline::ReleaseTextures(){
	fPostRT->UnRef();
	fPostRT = nil;
}

bool plPostPipeline::MsgReceive(plMessage* msg)
{
	return true;
}

void plPostPipeline::CreateShaders(){
	int numVSConsts = 8;
	int numPSConsts = 5;

	//if(fVsShader)
		//delete(fVsShader);
	//if(fPsShader)
		//delete(fPsShader);

	fVsShader = new plShader;
	//plString buff = plString::Format("%s_PassthroughVS", GetKey()->GetName().c_str());
	//hsgResMgr::ResMgr()->NewKey(buff, fVsShader, GetKey()->GetUoid().GetLocation());
	fVsShader->SetIsPixelShader(false);
	fVsShader->SetInputFormat(1);
	fVsShader->SetOutputFormat(0);
	fVsShader->SetNumConsts(numVSConsts);
	fVsShader->SetNumPipeConsts(0);
	//fVsShader->SetPipeConst(0, plPipeConst::kLocalToNDC, plGrassVS::kLocalToNDC);
	fVsShader->SetDecl(plShaderTable::Decl(plShaderID::vs_RiftDistortAssembly));
	//hsgResMgr::ResMgr()->SendRef(fVsShader->GetKey(), new plGenRefMsg(GetKey(), plRefMsg::kOnRequest, 0, kRefPassthroughVS), plRefFlags::kActiveRef);

	fPsShader = new plShader;
	//buff = plString::Format("%s_RiftDistortPS", GetKey()->GetName().c_str());
	//hsgResMgr::ResMgr()->NewKey(buff, fPsShader, GetKey()->GetUoid().GetLocation());
	fPsShader->SetIsPixelShader(true);
	fPsShader->SetNumConsts(numPSConsts);
	fPsShader->SetInputFormat(0);
	fPsShader->SetOutputFormat(0);
	fPsShader->SetDecl(plShaderTable::Decl(plShaderID::ps_RiftDistortAssembly));
	//fPsShader->SetDecl(plShaderTable::Decl(plShaderID::ps_RiftDistortAssembly));
	//hsgResMgr::ResMgr()->SendRef(fPsShader->GetKey(), new plGenRefMsg(GetKey(), plRefMsg::kOnRequest, 0, kRefRiftDistortPS), plRefFlags::kActiveRef);
}

void plPostPipeline::UpdateShaders()
{
	float r, g, b;
	r = g = b = 0.0f;
	hsColorRGBA DistortionClearColor;
	DistortionClearColor.Set(r, g, b, 1.0f);

	fPsShader->SetColor(0, DistortionClearColor);
    //Clear(r, g, b, a);

	float w = float(fVP.w) / float(fRealVP.w),
          h = float(fVP.h) / float(fRealVP.h),
          x = float(fVP.x) / float(fRealVP.w),
          y = float(fVP.y) / float(fRealVP.h);

    float as = float(fVP.w) / float(fVP.h);

    // We are using 1/4 of DistortionCenter offset value here, since it is
    // relative to [-1,1] range that gets mapped to [0, 0.5].
	float lensCenterSet[2] = {x + (w + fDistortion.XCenterOffset * 0.5f)*0.5f, y + h*0.5f};
	fPsShader->SetFloat2(kRiftShaderLensCenter, lensCenterSet);
	
	float screenCenterSet[2] = {x + w*0.5f, y + h*0.5f};
	fPsShader->SetFloat2(kRiftShaderScreenCenter, screenCenterSet);

    // MA: This is more correct but we would need higher-res texture vertically; we should adopt this
    // once we have asymmetric input texture scale.
    float scaleFactor = 1.0f / fDistortion.Scale;
	
	float scaleOutSet[2] = {(w/2) * scaleFactor, (h/2) * scaleFactor * as};
	fPsShader->SetFloat2(kRiftShaderScaleOut, scaleOutSet );
	
	float scaleInSet[2] = {(2/w), (2/h) / as};
	fPsShader->SetFloat2(kRiftShaderScaleIn, scaleInSet);

	float distortionSet[4] = {fDistortion.K[0], fDistortion.K[1], fDistortion.K[2], fDistortion.K[3]};
	fPsShader->SetFloat4(kRiftShaderHmdWarpParam, distortionSet);

	//Vertex shader consts
    hsMatrix44 texm, transpose;
	texm.Reset(false);
	texm.fMap[0][0] = w;
	texm.fMap[0][3] = x;
	texm.fMap[1][1] = h;
	texm.fMap[1][3] = y;
	texm.fMap[2][2] = 0;
	transpose.Reset(false);
	texm.GetTranspose(&transpose);
	//texm.Reset(false);
	fVsShader->SetMatrix44(kRiftShaderTexm, transpose);
	
    /*hsMatrix44 view, vTransposed;
	view.Reset(false);
	view.fMap[0][0] = view.fMap[1][1] = 2;
	view.fMap[0][3] = view.fMap[1][3] = -1;
	view.fMap[2][2] = 0;
	view.Reset(false);
	*/
	//view.GetTranspose(&vTransposed);
	//fVsShader->SetMatrix44(kRiftShaderView, view);
}

plRenderTarget* plPostPipeline::CreatePostRT(uint16_t width, uint16_t height){
	// Create our render target
    const uint16_t flags = plRenderTarget::kIsOffscreen;
    const uint8_t bitDepth(32);
    const uint8_t zDepth(-1);
    const uint8_t stencilDepth(-1);

	fPostRT = new plRenderTarget(flags, (uint16_t)(width * fRenderScale), (uint16_t)(height * fRenderScale), bitDepth, zDepth, stencilDepth);
    fPostRT->SetScreenRenderTarget(true);

	return fPostRT;

    //static int idx=0;
    //plString buff = plString::Format("tRT%d", idx++);
    //hsgResMgr::ResMgr()->NewKey(buff, fPostRT, this->GetKey()->GetUoid().GetLocation());
}

void plPostPipeline::SetViewport(OVR::Util::Render::Viewport vp, bool resetProjection = false)
{
	fVP = vp;

	plViewTransform vt = fPipe->GetViewTransform();
	
	vt.SetViewPort(fVP.x, fVP.y, fVP.x + fVP.w, fVP.y + fVP.h, false);
	
	if(resetProjection){
		vt.SetWidth((float)fVP.w);
		vt.SetHeight((float)fVP.h);
		vt.SetDepth(0.3f, 500.0f);
		vt.ResetProjectionMatrix();
	}
	
    fPipe->SetViewTransform(vt);
	fPipe->SetViewport();
}


void plPostPipeline::EnablePostRT()
{ 
	fPipe->PushRenderTarget(fPostRT); 
};

void plPostPipeline::DisablePostRT()
{ 
	fPipe->PopRenderTarget(); 
};

void plPostPipeline::RenderPostEffects(){
	fPipe->RenderPostScene(fPostRT, fVsShader, fPsShader);
}