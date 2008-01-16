#pragma once
#ifndef FP_CPU_SPH_H
#define FP_CPU_SPH_H

#include "DXUT.h"
#pragma warning(disable:4995)
#include <vector>
#pragma warning(default:4995)

#include "fp_global.h"
#include "fp_thread.h"

typedef struct {
    D3DXVECTOR3 m_Position;
    D3DXVECTOR3 m_Velocity;
    int m_Index;
} fp_FluidParticle;

typedef std::vector<fp_FluidParticle> fp_GridCell;
typedef fp_GridCell::size_type fp_GridCellSize;

class fp_Grid {
public:
    std::vector<fp_GridCell*> m_Cells;
    int m_NumParticles;
    int m_NumCells;
    int m_NumCellsX;
    int m_NumCellsY;
    int m_NumCellsZ;
    int m_NumCellsYZ;
    float m_CellWidth;
    float m_MinX;
    float m_MaxX;
    float m_MinY;
    float m_MaxY;
    float m_MinZ;
    float m_MaxZ;

    fp_Grid(
            int InitialCapacity,
            float CellWidth);
    // Copy constructor
    fp_Grid(const fp_Grid& Other);
    ~fp_Grid();

    void FillAndPrepare(fp_FluidParticle* Particles, int NumParticles);

private:
    inline void SetBounds(fp_FluidParticle* Particles, int NumParticles);
};

class fp_Fluid;

// Internal structure, don't use elsewhere
typedef struct {
    fp_Fluid* m_Fluid;
    int m_ThreadIdx;
    int m_NumThreads;
} fp_FluidMTHelperData;

class fp_Fluid {
    friend void fp_FluidCalculateFluidStateMTWrapper(void*);
    friend void fp_FluidCalculateGlassFluidStateChangeMTWrapper(void*);    
    friend void fp_FluidMoveParticlesMTWrapper(void*);
    friend void fp_FluidDummyFunc(void*);

public:
    fp_FluidParticle* m_Particles;
    int m_NumParticles;
    float m_GasConstantK; // TODO: is this viscosity or stiffness?
    float m_Viscosity;
    float m_SurfaceTension;
    float m_GradientColorFieldThresholdSq;
    float m_SmoothingLength;
    float m_SmoothingLengthSq;
    float m_SmoothingLengthSqInv;
    float m_SmoothingLengthPow3Inv;
    float m_InitialDensity;
    float m_RestDensity;
    float m_RestDensityCoefficient;
    float m_DampingCoefficient;
    float m_SearchRadius;
    float m_ParticleMass;
    float m_WPoly6Coefficient;
    float m_GradientWPoly6Coefficient;
    float m_LaplacianWPoly6Coefficient;
    float m_GradientWSpikyCoefficient;
    float m_LaplacianWViscosityCoefficient;

    float m_GlassRadius;
    float m_GlassRadiusPlusEnforceDistance;
    float m_GlassRadiusPlusEnforceDistanceSq;
    float m_GlassFloor;
    float m_GlassFloorPlusEnforceDistance;    
    D3DXVECTOR3 m_CurrentGlassPosition;
    float m_CurrentGlassFloorY;
    float m_CurrentGlassEnforceMinY;
    D3DXVECTOR3 m_LastGlassPosition;
    D3DXVECTOR3 m_LastGlassVelocity;
    D3DXVECTOR3 m_GlassVelocityChange;
    
    D3DXVECTOR3 m_Gravity;

    fp_Fluid(
        fp_WorkerThreadManager* WorkerThreadMgr,
        int NumParticlesX,
        int NumParticlesY,
        int NumParticlesZ,
        float SpacingX,
        float SpacingY,
        float SpacingZ,
        D3DXVECTOR3 Center,
        float GlassRadius,
        float GlassFloor,
        D3DXVECTOR3 Gravity = FP_DEFAULT_GRAVITY,
        float SmoothingLenght = FP_DEFAULT_FLUID_SMOOTHING_LENGTH,
        float GlassEnforceDistance = FP_DEFAULT_FLUID_GLASS_ENFORCE_DISTANCE,
        float GasConstantK = FP_DEFAULT_FLUID_GAS_CONSTANT_K,        
        float Viscosity = FP_DEFAULT_FLUID_VISCOSITY,
        float SurfaceTension = FP_DEFAULT_FLUID_SURFACE_TENSION,
        float GradientColorFieldThreshold = FP_DEFAULT_FLUID_GRADIENT_COLORFIELD_THRESHOLD,
        float ParticleMass = FP_DEFAULT_FLUID_PARTICLE_MASS,
        float RestDensityCoefficient = FP_DEFAULT_FLUID_REST_DENSITY_COEFFICIENT, 
        float DampingCoefficient = FP_DEFAULT_FLUID_DAMPING_COEFFICIENT);
    ~fp_Fluid();
    void Update(float ElapsedTime);
    void SetGlassEnforceDistance(float GlassEnforceDistance);
    void SetSmoothingLength(float SmoothingLength);
    void SetParticleMass(float ParticleMass);
    float* GetDensities();
    void GetParticleMinsAndMaxs(
            float& MinX, 
            float& MaxX, 
            float& MinY, 
            float& MaxY, 
            float& MinZ, 
            float& MaxZ);

    inline float WPoly6(float HSq_LenRSq);
    inline D3DXVECTOR3 GradientWPoly6(const D3DXVECTOR3* R, float HSq_LenRSq);
    inline float LaplacianWPoly6(float LenRSq, float HSq_LenRSq);
    inline D3DXVECTOR3 GradientWSpiky(const D3DXVECTOR3* R, float LenR);
    inline float LaplacianWViscosity(float LenR);
private:
    // For multi threadding
    fp_WorkerThreadManager* m_WorkerThreadMgr;
    fp_FluidMTHelperData* m_MTData;
    float m_CurrentElapsedTime;

    fp_Grid* m_Grid;
    float* m_OldDensities;
    volatile float* m_NewDensities;
    volatile D3DXVECTOR3* m_PressureAndViscosityForces;
    volatile D3DXVECTOR3* m_GradientColorField;
    volatile float* m_LaplacianColorField;
    
    inline void EnforceGlass(fp_FluidParticle* Particle);
    inline void ProcessParticlePair(
            fp_FluidParticle* Particle1, 
            fp_FluidParticle* Particle2,
            float DistanceSq);
    
    void CalculateFluidStateMT(int ThreadIdx);
    void CalculateGlassFluidStateChangeMT(int ThreadIdx);
    void MoveParticlesMT(int ThreadIdx);
};

// Inline definitions


inline float fp_Fluid::WPoly6(float HSq_LenRSq) {
	return m_WPoly6Coefficient * pow(HSq_LenRSq, 3);
}

inline D3DXVECTOR3 fp_Fluid::GradientWPoly6(const D3DXVECTOR3* R, float HSq_LenRSq) {
	return (*R) * m_GradientWPoly6Coefficient * pow(HSq_LenRSq, 2);
}

inline float fp_Fluid::LaplacianWPoly6(float LenRSq, float HSq_LenRSq) {
	   return m_LaplacianWPoly6Coefficient * HSq_LenRSq * (LenRSq - 0.75f * HSq_LenRSq);
}

inline D3DXVECTOR3 fp_Fluid::GradientWSpiky(const D3DXVECTOR3* R, float LenR) {
	return (*R) * (m_GradientWSpikyCoefficient / LenR) * pow(m_SmoothingLength - LenR, 2);
}


inline float fp_Fluid::LaplacianWViscosity(float LenR) {
	return m_LaplacianWViscosityCoefficient * (1.0f - LenR / m_SmoothingLength);
}

#endif
