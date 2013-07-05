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
#include "plSurface/plLayer.h"
#include "plSurface/plShader.h"
#include "plSurface/plShaderTable.h"
#include "plSurface/hsGMaterial.h"
#include "plGImage/plMipmap.h"
#include "plResMgr/plResManager.h"
#include "plResMgr/plKeyFinder.h"
#include "plMessage/plLayRefMsg.h"


plPostPipeline::plPostPipeline(){
}

plPostPipeline::~plPostPipeline(){
}


bool plPostPipeline::MsgReceive(plMessage* msg)
{
	return true;
}


void plPostPipeline::EnablePostRT()
{ 
	//fPipe->EndWorldRender();
	fPipe->PushRenderTarget(fPostRT); 
};

void plPostPipeline::DisablePostRT()
{ 
	fPipe->PushRenderTarget(nil); 
	//fPipe->BeginPostScene();
};

void plPostPipeline::CreatePostRT(uint16_t width, uint16_t height){
	// Create our render target
    const uint16_t flags = plRenderTarget::kIsOffscreen;
    const uint8_t bitDepth(32);
    const uint8_t zDepth(-1);
    const uint8_t stencilDepth(-1);
   fPostRT = new plRenderTarget(flags, width, height, bitDepth, zDepth, stencilDepth);

    static int idx=0;
    plString buff = plString::Format("tRT%d", idx++);
    hsgResMgr::ResMgr()->NewKey(buff, fPostRT, this->GetKey()->GetUoid().GetLocation());
}

void plPostPipeline::RenderPostEffects(){
	fPipe->ClearBackbuffer();
}


void plPostPipeline::CreatePostSurface(){

	plLayer         *layer;
    hsGMaterial     *material;
    plString        keyName;
	plPlate			*postPlate;

	//Create the plate
	postPlate = nil;
	plPlateManager::Instance().CreatePlate( &postPlate, 0, 0, 1.0, 1.0 );
	postPlate->SetVisible( true );
	postPlate->SetDepth(3);

	//material override TEST
	plMipmap* texture = postPlate->CreateMaterial(512,512,true);
	int x, y;
	for( y = 0; y < texture->GetHeight(); y++ )
    {
        uint32_t  *pixels = texture->GetAddr32( 0, y );
        for( x = 0; x < texture->GetWidth(); x++ )
            pixels[ x ] = 0x55555555;
    }

	/// Create a new bitmap
	/*
    plMipmap* texture = new plMipmap( 512, 512, plMipmap::kRGB32Config, 1 );
	memset( texture->GetImage(), 0xff, texture->GetHeight() * texture->GetRowBytes() );
    keyName = plString::Format( "PlateBitmap#%d", 27 );
    hsgResMgr::ResMgr()->NewKey( keyName, texture, plLocation::kGlobalFixedLoc );
    texture->SetFlags( texture->GetFlags() | plMipmap::kDontThrowAwayImage );

	//Create Vertex shader
	plShader *vertShader = new plShader;

	const plKey keyObj = GetKey();
	const char * key = GetKey()->GetName().c_str();

	plString buff = plString::Format("%s_PostVertexShader", GetKey()->GetName().c_str());
	hsgResMgr::ResMgr()->NewKey(buff, vertShader, GetKey()->GetUoid().GetLocation());
	
	vertShader->SetIsPixelShader(false);
	vertShader->SetNumConsts(2);
	vertShader->SetInputFormat(1);
	vertShader->SetOutputFormat(0);
	vertShader->SetDecl(plShaderTable::Decl(plShaderID::vs_RiftDistortAssembly));

	hsgResMgr::ResMgr()->SendRef(vertShader->GetKey(), new plGenRefMsg(GetKey(), plRefMsg::kOnRequest, 0, 0), plRefFlags::kActiveRef);
	

	//Create Pixel shader
	plShader *pixShader = new plShader;

	buff = plString::Format("%s_PostPixelShader", GetKey()->GetName().c_str());
	hsgResMgr::ResMgr()->NewKey(buff, vertShader, GetKey()->GetUoid().GetLocation());
	
	pixShader->SetIsPixelShader(true);
	pixShader->SetNumConsts(6);
	pixShader->SetInputFormat(0);
	pixShader->SetOutputFormat(0);

	pixShader->SetDecl(plShaderTable::Decl(plShaderID::vs_RiftDistortAssembly));

	hsgResMgr::ResMgr()->SendRef(vertShader->GetKey(), new plGenRefMsg(GetKey(), plRefMsg::kOnRequest, 0, 0), plRefFlags::kActiveRef);
	 
	/// Create material for layer
    material = new hsGMaterial();
	buff = plString::Format("%s_RiftPlate", GetKey()->GetName().c_str());
    hsgResMgr::ResMgr()->NewKey( buff, material, plLocation::kGlobalFixedLoc );
    layer = material->MakeBaseLayer();
    layer->SetShadeFlags( layer->GetShadeFlags() | hsGMatState::kShadeNoShade | hsGMatState::kShadeWhite | hsGMatState::kShadeReallyNoFog );
    layer->SetZFlags( layer->GetZFlags() | hsGMatState::kZNoZRead );
    layer->SetBlendFlags( layer->GetBlendFlags() | hsGMatState::kBlendAlpha );
	layer->SetPixelShader(pixShader);
	layer->SetVertexShader(vertShader);
	layer->SetOpacity( 1.0f );

	hsgResMgr::ResMgr()->AddViaNotify( texture->GetKey(), new plLayRefMsg( layer->GetKey(), plRefMsg::kOnCreate, 0, plLayRefMsg::kTexture ), plRefFlags::kActiveRef );

    // Set up a ref to these. Since we don't have a key, we use the
    // generic RefObject() (and matching UnRefObject() when we're done).
    // If we had a key, we would use myKey->AddViaNotify(otherKey) and myKey->Release(otherKey).
    material->GetKey()->RefObject();

	postPlate->SetMaterial(material);
	postPlate->SetTexture(texture);
	*/
}