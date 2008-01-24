#pragma once
#ifndef FP_UTIL_H
#define FP_UTIL_H

#include "DXUT.h"

template<class T>
struct fp_Vec3 {
    int x;
    int y;
    int z;

    fp_Vec3<T>() : x(0), y(0), z(0) {}
    fp_Vec3<T>(int X, int Y, int Z) : x(X), y(Y), z(Z) {}
    fp_Vec3<T>(const fp_Vec3& Other) : x(Other.x), y(Other.y), z(Other.z) {}
};

template<class T> fp_Vec3<T> operator+(const fp_Vec3<T>& A, const fp_Vec3<T>& B) {
    fp_Vec3<T> result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;
    return result;
}

typedef fp_Vec3<int> fp_VolumeIndex;

// Helper functions
class fp_Util {
public:
    // Stuff a FLOAT into a DWORD argument
    static inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }

    // Gets a random number between min/max boundaries
    static float GetRandomMinMax( float fMin, float fMax );

    // Generates a random vector where X,Y, and Z components are between -1.0 and 1.0
    static D3DXVECTOR3 GetRandomVector();

	static ID3D10Effect* LoadEffect(
			ID3D10Device* d3dDevice, 
			const LPCWSTR Filename, 
			const D3D10_SHADER_MACRO *ShaderMacros = NULL);
};

#endif
