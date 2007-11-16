#pragma once
#ifndef FP_CPU_SPH_H
#define FP_CPU_SPH_H

#include "DXUT.h"
#pragma warning(disable:4995)
#include <vector>
#pragma warning(default:4995)

#include "fp_global.h"

#define FP_DEFAULT_FLUID_SMOOTHING_LENGTH 4.0f//0.01f
#define FP_DEFAULT_FLUID_SMOOTHING_LENGTH_SQ FP_DEFAULT_FLUID_SMOOTHING_LENGTH\
        * FP_DEFAULT_FLUID_SMOOTHING_LENGTH
#define FP_DEFAULT_FLUID_GAS_CONSTANT_K 15.0f
#define FP_DEFAULT_FLUID_VISCOSITY 0.001f
#define FP_DEFAULT_FLUID_PARTICLE_MASS 0.00020543f
#define FP_DEFAULT_FLUID_REST_DENSITY_COEFFICIENT 35.0f // TODO: tune
#define FP_DEFAULT_FLUID_DAMPING_COEFFICIENT 0.85f // TODO: tune
#define FP_DEFAULT_FLUID_STIFFNESS 1.5f // TODO: what todo with it?
#define FP_DEFAULT_FLUID_SEARCHRADIUS FP_DEFAULT_FLUID_SMOOTHING_LENGTH
#define FP_DEFAULT_INITIAL_GRID_SIDELENGTH 50
#define FP_DEFAULT_INITIAL_GRID_CAPACITY FP_DEFAULT_INITIAL_GRID_SIDELENGTH * \
        FP_DEFAULT_INITIAL_GRID_SIDELENGTH * FP_DEFAULT_INITIAL_GRID_SIDELENGTH
#define FP_DEFAULT_INITIAL_CELL_CAPACITY 10
#define FP_FLUID_MAX_POS 150.0f

// For Debug:
#define FP_DEBUG_MAX_FORCE 0.5f
#define FP_DEBUG_MAX_FORCE_SQ FP_DEBUG_MAX_FORCE * FP_DEBUG_MAX_FORCE

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
    float m_CellWidth;
    float m_MinX;
    float m_MaxX;
    float m_MinY;
    float m_MaxY;
    float m_MinZ;
    float m_MaxZ;

    fp_Grid(
            int InitialCapacity,
            float CellWidth,
            float InitialParticleDensity);
    // Copy constructor
    fp_Grid(const fp_Grid& Other);
    ~fp_Grid();

    void FillAndPrepare(fp_FluidParticle* Particles, int NumParticles);

private:
    void SetBounds(fp_FluidParticle* Particles, int NumParticles);
};

class fp_Fluid {
public:
    fp_FluidParticle* m_Particles;
    int m_NumParticles;
    float m_GasConstantK; // TODO: is this viscosity or stiffness?
    float m_Viscosity;
    float m_SmoothingLength;
    float m_SmoothingLengthSq;
    float m_SmoothingLengthSqInv;
    float m_SmoothingLengthPow3Inv;
    float m_InitialDensity;
    float m_RestDensity;
    float m_DampingCoefficient;
    //float m_Stiffness;
    float m_SearchRadius;
    float m_ParticleMass;
    float m_WPoly6Coefficient;
    float m_GradientWPoly6Coefficient;
    float m_LaplacianWPoly6Coefficient;
    float m_GradientWSpikyCoefficient;
    float m_LaplacianWViscosityCoefficient;    
    
    fp_Fluid(
        int NumParticlesX,
        int NumParticlesY,
        int NumParticlesZ,
        float SpacingX,
        float SpacingY,
        float SpacingZ,
        D3DXVECTOR3 Center,
        float SmoothingLenght = FP_DEFAULT_FLUID_SMOOTHING_LENGTH,
        float GasConstantK = FP_DEFAULT_FLUID_GAS_CONSTANT_K,
        float Viscosity = FP_DEFAULT_FLUID_VISCOSITY,
        float ParticleMass = FP_DEFAULT_FLUID_PARTICLE_MASS,
        float RestDensityCoefficient = FP_DEFAULT_FLUID_REST_DENSITY_COEFFICIENT, 
        float DampingCoefficient = FP_DEFAULT_FLUID_DAMPING_COEFFICIENT,
        float Stiffness = FP_DEFAULT_FLUID_STIFFNESS,
        float SearchRadius = FP_DEFAULT_FLUID_SEARCHRADIUS,
        int InitialGridCapacity = FP_DEFAULT_INITIAL_GRID_CAPACITY);
    ~fp_Fluid();
    void Update(float ElapsedTime);
private:
    fp_Grid* m_Grid;
    float* m_OldDensities;
    float* m_NewDensities;
    D3DXVECTOR3* m_Forces; // TODO TODO TODO
    D3DXVECTOR3* m_GradientField;


    inline void ProcessParticlePair(
            fp_FluidParticle* Particle1, 
            fp_FluidParticle* Particle2,
            float DistanceSq);
    inline float WPoly6(float LenRSq);
    inline D3DXVECTOR3 GradientWPoly6(D3DXVECTOR3 R, float LenRSq);
    inline float LaplacianWPoly6(D3DXVECTOR3 R, float LenRSq);
    inline D3DXVECTOR3 GradientWSpiky(D3DXVECTOR3 R, float LenR);
    inline float LaplacianWViscosity(D3DXVECTOR3 R, float LenR);
};

#endif
