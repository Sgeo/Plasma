#include <stdio.h>
#include <d3dx9.h>
#include <Windows.h>
#include <math.h>
#include <string>

#ifndef INC_D3DUTILS_H
#define INC_D3DUTILS_H

// These first two macros are taken from the
// VStudio help files - necessary to convert the
// __FUNCTION__ symbol from char to wchar_t.
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

// Only the first of these macro's should be used. The _INTERNAL
// one is so that the sp##id part generates "sp1234" type identifiers
// instead of always "sp__LINE__"...
#define PROFILE_BLOCK PROFILE_BLOCK_INTERNAL( __LINE__ )
#define PROFILE_BLOCK_INTERNAL(id) D3DUtils::ScopeProfiler sp##id ( WIDEN(__FUNCTION__), __LINE__ );

// To avoid polluting the global namespace,
// all D3D utility functions/classes are wrapped
// up in the D3DUtils namespace.
namespace D3DUtils
{
	class ScopeProfiler
	{
		public:
			ScopeProfiler( WCHAR *Name, int Line );
			~ScopeProfiler( );

		private:
			ScopeProfiler( );
	};

	void SetMarker( WCHAR *Name);
}

#endif