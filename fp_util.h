#pragma once
#ifndef FP_UTIL_H
#define FP_UTIL_H

#include "DXUT.h"

// Helper functions

class fp_Util {
public:
    // Stuff a FLOAT into a DWORD argument
    static inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }

    // Gets a random number between min/max boundaries
    static float GetRandomMinMax( float fMin, float fMax );

    // Generates a random vector where X,Y, and Z components are between -1.0 and 1.0
    static D3DXVECTOR3 GetRandomVector();
};

#endif
