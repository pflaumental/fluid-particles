#include "DXUT.h"
#include "fp_util.h"

float fp_Util::GetRandomMinMax( float fMin, float fMax )
{
    float fRandNum = (float)rand () / RAND_MAX;
    return fMin + (fMax - fMin) * fRandNum;
}

D3DXVECTOR3 fp_Util::GetRandomVector( void )
{
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