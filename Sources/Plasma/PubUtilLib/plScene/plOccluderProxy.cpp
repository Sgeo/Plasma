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

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/

#include "hsTypes.h"
#include "plOccluderProxy.h"
#include "plOccluder.h"
#include "plDrawable/plDrawableSpans.h"
#include "plDrawable/plDrawableGenerator.h"
#include "pnMessage/plProxyDrawMsg.h"

plOccluderProxy::plOccluderProxy()
:	plProxyGen(hsColorRGBA().Set(0.2f,0.2f,0.8f,1.f), hsColorRGBA().Set(1.f,0.5f,0.5f,1.f), 0.5f),
	fOwner(nil)
{
}

plOccluderProxy::~plOccluderProxy()
{
}

hsBool plOccluderProxy::Init(plOccluder* occluder)
{
	plProxyGen::Init(occluder);

	fOwner = occluder;
	fProxyMsgType = plProxyDrawMsg::kOccluder;

	return fOwner != nil;
}

plKey plOccluderProxy::IGetNode() const 
{ 
	return fOwner ? fOwner->GetSceneNode() : nil; 
}

plDrawableSpans* plOccluderProxy::ICreateProxy(hsGMaterial* mat, hsTArray<UInt32>& idx, plDrawableSpans* addTo)
{
	if( fOwner )
	{
		return fOwner->CreateProxy(mat, idx, addTo);
	}
	return nil;
}