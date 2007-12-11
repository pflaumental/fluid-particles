#pragma once
#ifndef FP_CPU_SPH_H
#define FP_CPU_SPH_H

#include "DXUT.h"
#pragma warning(disable:4995)
#include <vector>
#pragma warning(default:4995)

#include "fp_global.h"



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

class fp_Fluid {
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
    float m_CollisionRadius;
    float m_CollisionRadiusSq;
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
    float m_GlassRadiusMinusCollisionRadius;
    float m_GlassRadiusMinusCollisionRadiusSq;
    float m_GlassFloor;
    float m_GlassFloorPlusCollisionRadius;
    D3DXVECTOR3 m_GlassPosition;
    
    D3DXVECTOR3 m_Gravity;

    fp_Fluid(
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
        float CollisionRadius = FP_DEFAULT_FLUID_COLLISION_RADIUS,
        float GasConstantK = FP_DEFAULT_FLUID_GAS_CONSTANT_K,        
        float Viscosity = FP_DEFAULT_FLUID_VISCOSITY,
        float SurfaceTension = FP_DEFAULT_FLUID_SURFACE_TENSION,
        float GradientColorFieldThreshold = FP_DEFAULT_FLUID_GRADIENT_COLORFIELD_THRESHOLD,
        float ParticleMass = FP_DEFAULT_FLUID_PARTICLE_MASS,
        float RestDensityCoefficient = FP_DEFAULT_FLUID_REST_DENSITY_COEFFICIENT, 
        float DampingCoefficient = FP_DEFAULT_FLUID_DAMPING_COEFFICIENT);
    ~fp_Fluid();
    void Update(float ElapsedTime);
    void SetCollisionRadius(float CollisionRadius);
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
    fp_Grid* m_Grid;
    float* m_OldDensities;
    float* m_NewDensities;
    D3DXVECTOR3* m_PressureAndViscosityForces;
    D3DXVECTOR3* m_GradientColorField;
    float* m_LaplacianColorField;

    inline void HandleGlassCollision(fp_FluidParticle* Particle);
    inline void ProcessParticlePair(
            fp_FluidParticle* Particle1, 
            fp_FluidParticle* Particle2,
            float DistanceSq);
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
