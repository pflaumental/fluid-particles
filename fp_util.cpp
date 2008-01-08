#include "DXUT.h"
#include "SDKmisc.h"
#pragma warning(disable:4530)
#pragma warning(disable:4995)
#include <sstream>
#pragma warning(default:4995)
#pragma warning(default:4530)
#include "fp_util.h"

float fp_Util::GetRandomMinMax( float fMin, float fMax ) {
    float fRandNum = (float)rand () / RAND_MAX;
    return fMin + (fMax - fMin) * fRandNum;
}

D3DXVECTOR3 fp_Util::GetRandomVector( void ) {
	D3DXVECTOR3 vVector;

    // Pick a random Z between -1.0f and 1.0f.
    vVector.z = fp_Util::GetRandomMinMax( -1.0f, 1.0f );
    
    // Get radius of this circle
    float radius = (float)sqrt(1 - vVector.z * vVector.z);
    
    // Pick a random point on a circle.
    float t = fp_Util::GetRandomMinMax( -D3DX_PI, D3DX_PI );

    // Compute matching X and Y for our Z.
    vVector.x = (float)cosf(t) * radius;
    vVector.y = (float)sinf(t) * radius;

	return vVector;
}

ID3D10Effect* fp_Util::LoadEffect(
		ID3D10Device* d3dDevice, 
		const LPCWSTR Filename, 
		const D3D10_SHADER_MACRO *ShaderMacros) {
	HRESULT hr;
	ID3D10Effect* effect10 = NULL;
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
	#if defined( DEBUG ) || defined( _DEBUG )
	dwShaderFlags |= D3D10_SHADER_DEBUG;
	#endif
	WCHAR str[MAX_PATH];
	hr = DXUTFindDXSDKMediaFileCch(str, MAX_PATH, Filename);
	if (FAILED(hr)) {
		std::wstringstream s;
		s << L"Shader Load error: Could not locate \"" << Filename << L"\".";			
		MessageBoxW(NULL, s.str().c_str(), L"Shader Load Error", MB_OK);
		return NULL;
	}
	ID3D10Blob *errors;
	hr = D3DX10CreateEffectFromFile(str, ShaderMacros, NULL, "fx_4_0", dwShaderFlags, 0,
			d3dDevice, NULL, NULL, &effect10, &errors, NULL );
	if (FAILED(hr)) {
		std::wstringstream s;
		if (errors == NULL) {
			s << L"Unknown error loading shader: \"" << str << L"\".";	
		} else {
			s << L"Error loading shader \"" << str << "\"\n " << (CHAR*)errors->GetBufferPointer();	
		}
		MessageBoxW(NULL, s.str().c_str(), L"Shader Load Error", MB_OK);
		return NULL;
	}
	return effect10;
}