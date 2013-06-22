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
#include "pnKeyedObject/hsKeyedObject.h"
#include "pnKeyedObject/plKey.h"
#include "pnKeyedObject/plFixedKey.h"
#include "pnKeyedObject/plUoid.h"

plRiftCamera::plRiftCamera(){
	//Rift init
	YawInitial = 3.141592f;
	EyePos = Vector3f(0.0f, 1.6f, -5.0f),
    EyeYaw = YawInitial;
	EyePitch = 0.0f;
	EyeRoll = 0.0f;
    LastSensorYaw = 0.0f;
	UpVector = Vector3f(0.0f, 0.0f, 1.0f);
	ForwardVector = Vector3f(0.0f, 1.0f, 0.0f);
	RightVector = Vector3f(1.0f, 0.0f, 0.0f);
}

plRiftCamera::~plRiftCamera(){
}

void plRiftCamera::initRift(){

	System::Init(Log::ConfigureDefaultLog(LogMask_All));

	pManager = *DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
	SFusion.SetPredictionEnabled(true);

	SConfig.SetStereoMode(Util::Render::Stereo_LeftRight_Multipass);
	SConfig.SetDistortionFitPointVP(-1.0f, 0.0f);

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

	createDistortionPlate();
}

bool plRiftCamera::MsgReceive(plMessage* msg)
{
	return true;
}

void plRiftCamera::CalculateRiftCameraOrientation(hsPoint3 camPosition){
	//Matrix4f hmdMat(hmdOrient);

	Quatf riftOrientation = SFusion.GetOrientation();

	Matrix4f tester = Matrix4f(riftOrientation.Inverted());
	Vector3f camPos(camPosition.fX, camPosition.fY, camPosition.fZ);
	//Vector3f poa(1, 0, 0);
	
	Vector3f upp(1, 0, 0);
	
	tester *= Matrix4f::RotationX(3.1415926 * 0.5);
	tester *= Matrix4f::RotationY(3.1415926);
	tester *= Matrix4f::RotationZ(3.1415926 * 0.5);
	tester *= Matrix4f::Translation(-camPos);
	//tester *= Matrix4f::Translation(-camPos) * eyeConfig.ViewAdjust;
	
	hsMatrix44 testerAlt;
	
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			testerAlt.fMap[i][j] = tester.M[i][j];
		}
	}
	
	//std::stringstream riftCons;
	//riftCons << riftOrientation.x << ", " << riftOrientation.y << ", " << riftOrientation.z << ", " << riftOrientation.w;
	//fConsole->AddLine(riftCons.str().c_str());

	fVirtualCam->SetRiftOverrideMatrix(testerAlt);
	fVirtualCam->SetRiftOverridePOA(hsVector3(riftOrientation.x, riftOrientation.y, riftOrientation.z));
	fVirtualCam->SetRiftOverrideUp(hsVector3(0, 0, riftOrientation.w));
}

void plRiftCamera::createDistortionPlate(){

	plLayer         *layer;
    hsGMaterial     *material;
    plString        keyName;

	//Create the plate
	fRiftDistortionPlate = nil;
	plPlateManager::Instance().CreatePlate( &fRiftDistortionPlate, 0, 0, 1.0, 1.0 );
	fRiftDistortionPlate->SetVisible( true );
	fRiftDistortionPlate->SetDepth(3);

	/*
	//material override TEST
	plMipmap* texture = fRiftDistortionPlate->CreateMaterial(512,512,true);
	int x, y;
	for( y = 0; y < texture->GetHeight(); y++ )
    {
        uint32_t  *pixels = texture->GetAddr32( 0, y );
        for( x = 0; x < texture->GetWidth(); x++ )
            pixels[ x ] = 0x55555555;
    }*/

	/// Create a new bitmap
    plMipmap* texture = new plMipmap( 512, 512, plMipmap::kRGB32Config, 1 );
	memset( texture->GetImage(), 0xff, texture->GetHeight() * texture->GetRowBytes() );
    keyName = plString::Format( "PlateBitmap#%d", 27 );
    hsgResMgr::ResMgr()->NewKey( keyName, texture, plLocation::kGlobalFixedLoc );
    texture->SetFlags( texture->GetFlags() | plMipmap::kDontThrowAwayImage );

	//Create Vertex shader
	fRiftPixelShader = new plShader;

	const plKey keyObj = GetKey();
	const char * key = GetKey()->GetName().c_str();

	plString buff = plString::Format("%s_RiftVertexShader", GetKey()->GetName().c_str());
	hsgResMgr::ResMgr()->NewKey(buff, fRiftPixelShader, GetKey()->GetUoid().GetLocation());
	
	fRiftPixelShader->SetIsPixelShader(false);
	fRiftPixelShader->SetNumConsts(2);
	fRiftPixelShader->SetInputFormat(1);
	fRiftPixelShader->SetOutputFormat(0);
	fRiftPixelShader->SetDecl(plShaderTable::Decl(vs_RiftDistortAssembly));

	hsgResMgr::ResMgr()->SendRef(fRiftPixelShader->GetKey(), new plGenRefMsg(GetKey(), plRefMsg::kOnRequest, 0, 0), plRefFlags::kActiveRef);
	

	//Create Pixel shader
	fRiftPixelShader = new plShader;

	buff = plString::Format("%s_RiftPixelShader", GetKey()->GetName().c_str());
	hsgResMgr::ResMgr()->NewKey(buff, fRiftPixelShader, GetKey()->GetUoid().GetLocation());
	
	fRiftPixelShader->SetIsPixelShader(true);
	fRiftPixelShader->SetNumConsts(6);
	fRiftPixelShader->SetInputFormat(0);
	fRiftPixelShader->SetOutputFormat(0);

	fRiftPixelShader->SetDecl(plShaderTable::Decl(vs_RiftDistortAssembly));

	hsgResMgr::ResMgr()->SendRef(fRiftPixelShader->GetKey(), new plGenRefMsg(GetKey(), plRefMsg::kOnRequest, 0, 0), plRefFlags::kActiveRef);
	 
	/// Create material for layer
    material = new hsGMaterial();
    keyName = plString::Format( "RiftPlate" );
    hsgResMgr::ResMgr()->NewKey( keyName, material, plLocation::kGlobalFixedLoc );
    layer = material->MakeBaseLayer();
    layer->SetShadeFlags( layer->GetShadeFlags() | hsGMatState::kShadeNoShade | hsGMatState::kShadeWhite | hsGMatState::kShadeReallyNoFog );
    layer->SetZFlags( layer->GetZFlags() | hsGMatState::kZNoZRead );
    layer->SetBlendFlags( layer->GetBlendFlags() | hsGMatState::kBlendAlpha );
	layer->SetPixelShader(fRiftPixelShader);
	layer->SetVertexShader(fRiftPixelShader);
	layer->SetOpacity( 1.0f );

	hsgResMgr::ResMgr()->AddViaNotify( texture->GetKey(), new plLayRefMsg( layer->GetKey(), plRefMsg::kOnCreate, 0, plLayRefMsg::kTexture ), plRefFlags::kActiveRef );

    // Set up a ref to these. Since we don't have a key, we use the
    // generic RefObject() (and matching UnRefObject() when we're done).
    // If we had a key, we would use myKey->AddViaNotify(otherKey) and myKey->Release(otherKey).
    material->GetKey()->RefObject();

	fRiftDistortionPlate->SetMaterial(material);
	fRiftDistortionPlate->SetTexture(texture);





	
}



//LEFTOVER TESTING CODE - Is this useful for richard?
/*

	//Rift update
	/*std::stringstream riftConsole;
	Quatf    hmdOrient = SFusion.GetOrientation();
	float    yaw = 0.0f;

	hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &EyePitch, &EyeRoll);

	EyeYaw += (yaw - LastSensorYaw);
	LastSensorYaw = yaw;

	Matrix4f rollPitchYaw = Matrix4f::RotationY(EyeRoll) * Matrix4f::RotationX(-EyePitch) * Matrix4f::RotationZ(-EyeYaw);
    Vector3f up = rollPitchYaw.Transform(UpVector);
    Vector3f forward = rollPitchYaw.Transform(ForwardVector);

	hsMatrix44 convertMat;
	
	Vector3f camPos(fNewCamera->GetCameraPos().fX, fNewCamera->GetCameraPos().fY, fNewCamera->GetCameraPos().fZ);
	Matrix4f viewMat = Matrix4f::LookAtLH(camPos, camPos + forward, up);
	viewMat.Scaling(-1.0f, 0.0f,0.0f);
	//Matrix4f viewMat = Matrix4f::RotationY(EyeYaw) * Matrix4f::RotationX(-EyePitch) * Matrix4f::RotationZ(EyeRoll) * Matrix4f::Translation(camPos);


	//Matrix4f orientMat = hmdOrient;
	//orientMat.Translation(camPos);
	//orientMat.AxisConversion( WorldAxes(Axis_Right, Axis_In, Axis_Up ), WorldAxes(Axis_Left, Axis_In, Axis_Up));

	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			convertMat.fMap[i][j] = viewMat.M[i][j];
		}
	}

	

	//const hsPoint3 camForward = fNewCamera->GetCameraPos() + hsVector3(forward.x, forward.y, forward.z);
	//const hsVector3 camUp = hsVector3(up.x, up.y, up.z);

	//convertMat.MakeCamera(&fNewCamera->GetCameraPos(), &camForward, &camUp);
	fNewCamera->SetRiftOverrideMatrix(convertMat);

	//fNewCamera->SetRiftOverrideY(yaw);
	//fNewCamera->SetRiftOverrideX(EyePitch);

	//riftConsole << "Yaw: " << EyeYaw << " Pitch:" << EyePitch << " Roll:" << EyeRoll;
	//fConsole->AddLine(riftConsole.str().c_str());
	*/

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