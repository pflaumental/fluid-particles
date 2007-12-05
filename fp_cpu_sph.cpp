#include "DXUT.h"
#include "fp_cpu_sph.h"
#include "fp_util.h"
#include <float.h>

fp_Grid::fp_Grid(int InitialCapacity, float CellWidth)
        :
        m_Cells(),
        m_NumParticles(0),
        m_NumCells(0),
        m_NumCellsX(0),
        m_NumCellsY(0),
        m_NumCellsZ(0),
        m_CellWidth(CellWidth),
        m_MinX(FLT_MAX),
        m_MinY(FLT_MAX),
        m_MinZ(FLT_MAX),
        m_MaxX(FLT_MIN),
        m_MaxY(FLT_MIN),
        m_MaxZ(FLT_MIN) {
    m_Cells.reserve(FP_INITIAL_GRID_CAPACITY);
}

fp_Grid::fp_Grid(const fp_Grid& Other)
        :
        m_NumParticles(Other.m_NumParticles),
        m_NumCells(Other.m_NumCells),
        m_NumCellsX(Other.m_NumCellsX),
        m_NumCellsY(Other.m_NumCellsY),
        m_NumCellsZ(Other.m_NumCellsZ),
        m_CellWidth(Other.m_CellWidth),
        m_MinX(Other.m_MinX),
        m_MinY(Other.m_MinY),
        m_MinZ(Other.m_MinZ),
        m_MaxX(Other.m_MaxX),
        m_MaxY(Other.m_MaxY),
        m_MaxZ(Other.m_MaxZ) {
    for (int i = 0; i < m_NumCells; i++) {
        fp_GridCell* otherCell = Other.m_Cells[i];
        if(otherCell != NULL) {
            m_Cells[i] = new fp_GridCell(*otherCell);
        }
    }
}

fp_Grid::~fp_Grid() {
    for (int i = 0; i < m_NumCells; i++) {
        if(m_Cells[i] != NULL) {
            delete m_Cells[i];
            m_Cells[i] = NULL;
        }
    }
}

void fp_Grid::FillAndPrepare(fp_FluidParticle *Particles, int NumParticles) {
    for (int i = 0; i < m_NumCells; i++) {
        if(m_Cells[i] != NULL) {
            delete m_Cells[i];
            m_Cells[i] = NULL;
        }
    }

    m_NumParticles = NumParticles;
    SetBounds(Particles, NumParticles);    
    m_NumCellsX = (int)((m_MaxX - m_MinX + m_CellWidth) / m_CellWidth);
    m_NumCellsY = (int)((m_MaxY - m_MinY + m_CellWidth) / m_CellWidth);
    m_NumCellsZ = (int)((m_MaxZ - m_MinZ + m_CellWidth) / m_CellWidth);
    m_NumCells = m_NumCellsX * m_NumCellsY * m_NumCellsZ;
    m_Cells.resize(m_NumCells, NULL);

    for (int i = 0; i < NumParticles; i++) {
        D3DXVECTOR3 pos = Particles[i].m_Position;
        int cellIndexX = (int)((pos.x - m_MinX) / m_CellWidth);
        if(cellIndexX < 0 || cellIndexX >= m_NumCellsX)
            continue;
        int cellIndexY = (int)((pos.y - m_MinY) / m_CellWidth);
        if(cellIndexY < 0 || cellIndexY >= m_NumCellsY)
            continue;
        int cellIndexZ = (int)((pos.z - m_MinZ) / m_CellWidth);
        if(cellIndexZ < 0 || cellIndexZ >= m_NumCellsZ)
            continue;
        int cellIndex = cellIndexX + cellIndexY * m_NumCellsX
                + cellIndexZ * m_NumCellsX * m_NumCellsY;
        fp_GridCell* cell = m_Cells[cellIndex];
        if(cell == NULL) {
            m_Cells[cellIndex] = cell = new fp_GridCell();
            cell->reserve(FP_DEFAULT_INITIAL_CELL_CAPACITY);
        }
        cell->push_back(Particles[i]);
    }
}

inline void fp_Grid::SetBounds(fp_FluidParticle *Particles, int NumParticles){
    m_MinX = m_MinY = m_MinZ = FLT_MAX;
    m_MaxX = m_MaxY = m_MaxZ = FLT_MIN;    
    for (int i = 0; i < NumParticles; i++)
    {
        D3DXVECTOR3 pos = Particles[i].m_Position;
        if(pos.x < -FP_FLUID_MAX_POS) continue;
        if(pos.x > FP_FLUID_MAX_POS) continue;
        if(pos.y < -FP_FLUID_MAX_POS) continue;
        if(pos.y > FP_FLUID_MAX_POS) continue;
        if(pos.z < -FP_FLUID_MAX_POS) continue;
        if(pos.z > FP_FLUID_MAX_POS) continue;
        if(pos.x < m_MinX)
            m_MinX = pos.x;
        if(pos.y < m_MinY)
            m_MinY = pos.y;
        if(pos.z < m_MinZ)
            m_MinZ = pos.z;
        if(pos.x > m_MaxX)
            m_MaxX = pos.x;
        if(pos.y > m_MaxY)
            m_MaxY = pos.y;
        if(pos.z > m_MaxZ)
            m_MaxZ = pos.z;
    }
}

fp_Fluid::fp_Fluid(
        int NumParticlesX,
        int NumParticlesY,
        int NumParticlesZ,
        float SpacingX,
        float SpacingY,
        float SpacingZ,
        D3DXVECTOR3 Center,
        float SmoothingLenght,
        float GasConstantK,
        float Viscosity,
        float SurfaceTension,
        float GradientColorFieldThreshold,
        float ParticleMass,
        float RestDensityCoefficient,
        float DampingCoefficient)
        :
        m_NumParticles(NumParticlesX * NumParticlesY * NumParticlesZ),        
        m_Particles(new fp_FluidParticle[NumParticlesX * NumParticlesY * NumParticlesZ]),
        m_GasConstantK(GasConstantK),
        m_Viscosity(Viscosity),
        m_SurfaceTension(SurfaceTension),
        m_RestDensityCoefficient(RestDensityCoefficient),
        m_GradientColorFieldThresholdSq(GradientColorFieldThreshold
                * GradientColorFieldThreshold),
        m_DampingCoefficient(DampingCoefficient) {
    m_ParticleMass = ParticleMass; // Needed in SetSmoothingLength(...)
    m_Grid = NULL;
    SetSmoothingLength(SmoothingLenght);
    SetParticleMass(ParticleMass);
    m_Grid = new fp_Grid(FP_INITIAL_GRID_CAPACITY, SmoothingLenght);
    m_PressureAndViscosityForces = new D3DXVECTOR3[m_NumParticles];
    m_GradientColorField = new D3DXVECTOR3[m_NumParticles];
    m_LaplacianColorField = new float[m_NumParticles];
    m_OldDensities = new float[m_NumParticles];
    m_NewDensities = new float[m_NumParticles];
    float startX = Center.x - 0.5f * (NumParticlesX - 1) * SpacingX;
    float startY = Center.y - 0.5f * (NumParticlesY - 1) * SpacingY;
    float startZ = Center.z - 0.5f * (NumParticlesZ - 1) * SpacingZ;     

    int i = 0;
    for(int iZ = 0; iZ < NumParticlesZ; iZ++ ) {
        for (int iY = 0; iY < NumParticlesY; iY++) {
            for (int iX = 0; iX < NumParticlesX; iX++) {                
                m_Particles[i].m_Position = D3DXVECTOR3(iX * SpacingX + startX,
                        iY * SpacingY + startY, iZ * SpacingZ + startZ);
                m_Particles[i].m_Velocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                m_Particles[i].m_Index = i;
                m_PressureAndViscosityForces[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                m_GradientColorField[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                m_LaplacianColorField[i] = 0.0f;
                m_OldDensities[i] = m_InitialDensity;
                m_NewDensities[i] = m_InitialDensity;
                i++;
            }
        }
    }  
    m_Grid->FillAndPrepare(m_Particles, m_NumParticles);
    // Calculate initial densities
    Update(0.0f);
    float* tmpDensities = m_OldDensities;
    m_OldDensities = m_NewDensities;
    for(int i=0; i<m_NumParticles; i++) {
        tmpDensities[i] = m_InitialDensity;
    }
    m_NewDensities = tmpDensities;
}

void fp_Fluid::SetSmoothingLength(float SmoothingLength) {
    m_SmoothingLength = SmoothingLength;
    m_SearchRadius = SmoothingLength;
    m_SmoothingLengthPow3Inv = 1.0f / pow(SmoothingLength, 3);
    m_SmoothingLengthSq = SmoothingLength * SmoothingLength;
    m_SmoothingLengthSqInv = 1.0f / m_SmoothingLengthSq;
    m_WPoly6Coefficient = 315.0f / (64.0f * D3DX_PI * pow(SmoothingLength, 9));
    m_GradientWPoly6Coefficient = -945.0f / (32.0f * D3DX_PI * pow(SmoothingLength,9));
    m_LaplacianWPoly6Coefficient = 945.0f / (8.0f * D3DX_PI * pow(SmoothingLength,9));
    m_GradientWSpikyCoefficient = -45.0f / (D3DX_PI * pow(SmoothingLength, 6));
    m_LaplacianWViscosityCoefficient = 45.0f / (D3DX_PI * pow(SmoothingLength, 5));
    m_InitialDensity = m_ParticleMass * WPoly6(m_SmoothingLengthSq);
    m_RestDensity = m_RestDensityCoefficient * m_InitialDensity;
    if(m_Grid != NULL) {
        m_Grid->m_CellWidth = SmoothingLength;
    }
}

void fp_Fluid::SetParticleMass(float ParticleMass) {
    m_ParticleMass = ParticleMass;
    m_InitialDensity = ParticleMass * WPoly6(m_SmoothingLengthSq);
    m_RestDensity = m_RestDensityCoefficient * m_InitialDensity;
}

float* fp_Fluid::GetDensities() {
    return m_OldDensities;
}

void fp_Fluid::GetParticleMinsAndMaxs(
        float& MinX, 
        float& MaxX, 
        float& MinY, 
        float& MaxY, 
        float& MinZ, 
        float& MaxZ) {
    MinX = m_Grid->m_MinX;
    MaxX = m_Grid->m_MaxX;    
    MinY = m_Grid->m_MinY;
    MaxY = m_Grid->m_MaxY;    
    MinZ = m_Grid->m_MinZ;
    MaxZ = m_Grid->m_MaxZ;        
}

void fp_Fluid::Update(float ElapsedTime) {
    // Calculate new densities, pressure forces and viscosity forces

    // For each cell
    for (int cellIndexZ = 0; cellIndexZ < m_Grid->m_NumCellsZ; cellIndexZ++) {
        for (int cellIndexY = 0; cellIndexY < m_Grid->m_NumCellsY; cellIndexY++) {
            for (int cellIndexX = 0; cellIndexX < m_Grid->m_NumCellsX; cellIndexX++) {
                int cellIndex = cellIndexX + cellIndexY * m_Grid->m_NumCellsX
                        + cellIndexZ * m_Grid->m_NumCellsX * m_Grid->m_NumCellsY;
                fp_GridCell* cell = m_Grid->m_Cells[cellIndex];
                if(cell == NULL)
                    continue;
                fp_GridCellSize cellSize = cell->size();               
                // For each following neighbour-cell
                for (int iN = 0; iN < 1; iN++) {
                    int neighbourCellIndexZ = cellIndexZ + iN;
                    if(neighbourCellIndexZ >= m_Grid->m_NumCellsZ)
                        continue;
                    for (int jN = 0; jN < 1; jN++) {
                        int neighbourCellIndexY = cellIndexY + jN;
                        if(neighbourCellIndexY >= m_Grid->m_NumCellsY)
                            continue;
                        for (int kN = 0; kN < 1; kN++) {
                            int neighbourCellIndexX = cellIndexX + kN;
                            if(neighbourCellIndexX >= m_Grid->m_NumCellsX)
                                continue;
                            int neighbourCellIndex = neighbourCellIndexX
                                    + neighbourCellIndexY * m_Grid->m_NumCellsX
                                    + neighbourCellIndexZ * m_Grid->m_NumCellsX
                                    * m_Grid->m_NumCellsY;
                            fp_GridCell* neighbourCell
                                    = m_Grid->m_Cells[neighbourCellIndex];
                            if(neighbourCell == NULL)
                                continue;
                            fp_GridCellSize neighbourCellSize = neighbourCell->size();

                            // Pairwise process all particles from cell with all particles
                            // from neighbour cell

                            // For each particle in cell
                            for (fp_GridCellSize iCellParticle = 0; iCellParticle < cellSize;
                                    iCellParticle++) {
                                fp_FluidParticle* particle = &(*cell)[iCellParticle];

                                // For each particle in neighbour-cell
                                for (fp_GridCellSize iNeighbourCellParticle = 0;
                                        iNeighbourCellParticle < neighbourCellSize;
                                        iNeighbourCellParticle++) {

                                    // Do not process a particle with itself, otherwise
                                    // it would get evalutated twice
                                    if(iCellParticle == iNeighbourCellParticle
                                            && cellIndex == neighbourCellIndex)
                                        continue;
                                    
                                    fp_FluidParticle* neighbourParticle =
                                            &(*neighbourCell)[iNeighbourCellParticle];
                                    D3DXVECTOR3 toNeighbour = neighbourParticle->m_Position
                                            - particle->m_Position;
                                    float distSq = D3DXVec3LengthSq(&toNeighbour);
                                    if(distSq < FP_DEFAULT_FLUID_SMOOTHING_LENGTH_SQ) {                                        
                                        ProcessParticlePair(particle, neighbourParticle,
                                                distSq);
                                    }
                                }
                            }
                        }                            
                    }                        
                }
            }
        }        
    }

    // Move particles and clear fields
    for (int i = 0; i < m_NumParticles; i++) {
        D3DXVECTOR3 oldVelocity = m_Particles[i].m_Velocity;
        D3DXVECTOR3 oldVelocityContribution = oldVelocity
                * pow(m_DampingCoefficient, ElapsedTime);
        D3DXVECTOR3 totalForce = m_PressureAndViscosityForces[i];
        D3DXVECTOR3 gradColorField = m_GradientColorField[i];
        float gradColorFieldLenSq = D3DXVec3LengthSq(&gradColorField);
        if(gradColorFieldLenSq >= m_GradientColorFieldThresholdSq) {
            D3DXVECTOR3 surfaceTensionForce = (-m_SurfaceTension *
                    m_LaplacianColorField[i]/sqrt(gradColorFieldLenSq)) * gradColorField;
            totalForce += surfaceTensionForce;
        }
        D3DXVECTOR3 newVelocity = oldVelocityContribution 
                + totalForce * ElapsedTime / m_OldDensities[i];        
        m_Particles[i].m_Velocity = newVelocity;
        m_Particles[i].m_Position += 0.5f * ElapsedTime
                * (oldVelocityContribution + newVelocity);

        m_PressureAndViscosityForces[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);        
        m_GradientColorField[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_LaplacianColorField[i] = 0.0f;
        m_OldDensities[i] = m_RestDensity;
    }

    // Update Grid
    m_Grid->FillAndPrepare(m_Particles, m_NumParticles);

    // Prepare densities
    float* tmpDensities = m_OldDensities;
    m_OldDensities = m_NewDensities;
    m_NewDensities = tmpDensities;
}

inline void fp_Fluid::ProcessParticlePair(
        fp_FluidParticle* Particle1, 
        fp_FluidParticle* Particle2,
        float DistanceSq){
    float dist = sqrt(DistanceSq);
    float hSq_lenRSq = m_SmoothingLengthSq - DistanceSq;
    D3DXVECTOR3 r1 = Particle1->m_Position - Particle2->m_Position;
    int particle1Index = Particle1->m_Index;
    int particle2Index = Particle2->m_Index;

    // Density
    float particle1Density = m_OldDensities[particle1Index];
    float particle2Density = m_OldDensities[particle2Index];
    float wPoly6Value = WPoly6(hSq_lenRSq);
    float AdditionalDensity = m_ParticleMass * wPoly6Value;
    m_NewDensities[particle1Index] += AdditionalDensity;
    m_NewDensities[particle2Index] += AdditionalDensity;

    // Pressure forces
    float PressureAt1 = m_GasConstantK * (particle1Density - m_RestDensity);
    float PressureAt2 = m_GasConstantK * (particle2Density - m_RestDensity);
    float PressureSum = PressureAt1 + PressureAt2;
    D3DXVECTOR3 gradWSpikyValue1 = GradientWSpiky(&r1, dist);
    D3DXVECTOR3 commonPressureTerm1 = - m_ParticleMass * PressureSum / 2.0f
            * gradWSpikyValue1;
    D3DXVECTOR3 pressureForce1 = commonPressureTerm1 / particle2Density;
    D3DXVECTOR3 pressureForce2 = - commonPressureTerm1 / particle1Density;
    
    #if defined(DEBUG) || defined(_DEBUG)

    float pressureForce1LenSq = D3DXVec3LengthSq(&pressureForce1);
    D3DXVECTOR3 pressureForce1Normalized, r1Normalized;
    D3DXVec3Normalize(&pressureForce1Normalized, &pressureForce1);
    D3DXVec3Normalize(&r1Normalized, &r1);
    float testDot = D3DXVec3Dot(&pressureForce1Normalized, &r1Normalized);
    assert(pressureForce1LenSq < 0.001f || pressureForce1LenSq > -0.001f
            || testDot > 0.99f);
    assert(m_OldDensities[particle1Index] >= m_InitialDensity);
    assert(pressureForce1LenSq < FP_DEBUG_MAX_FORCE_SQ);

    #endif

    // Viscosity forces
    D3DXVECTOR3 velocityDifference1 = Particle2->m_Velocity - Particle1->m_Velocity;
    float laplacianWViskosityValue1 = LaplacianWViscosity(dist);
    D3DXVECTOR3 commonViscosityTerm1 = m_ParticleMass * m_Viscosity * velocityDifference1
            * laplacianWViskosityValue1;
    D3DXVECTOR3 viscosityForce1 = commonViscosityTerm1 / particle2Density;
    D3DXVECTOR3 viscosityForce2 = -commonViscosityTerm1 / particle1Density;

    #if defined(DEBUG) || defined(_DEBUG)
    float viscosityForce1LenSq = D3DXVec3LengthSq(&viscosityForce1);
    assert(viscosityForce1LenSq < FP_DEBUG_MAX_FORCE_SQ);
    #endif

    // Surface tension
    D3DXVECTOR3 gradWPoly6Value1 = GradientWPoly6(&r1, hSq_lenRSq);
    D3DXVECTOR3 commonGradientColorFieldTerm1 = m_ParticleMass * gradWPoly6Value1;
    m_GradientColorField[particle1Index] += commonGradientColorFieldTerm1
            / particle2Density;
    m_GradientColorField[particle2Index] -= commonGradientColorFieldTerm1
            / particle1Density;
    float laplacianWPoly6Value1 = LaplacianWPoly6(DistanceSq, hSq_lenRSq);
    float commonLaplacianColorFieldTerm1 = m_ParticleMass * laplacianWPoly6Value1;
    m_LaplacianColorField[particle1Index] = commonLaplacianColorFieldTerm1
            / particle2Density;
    m_LaplacianColorField[particle2Index] = commonLaplacianColorFieldTerm1
            / particle1Density;

    // Total forces
    m_PressureAndViscosityForces[particle1Index] += pressureForce1 + viscosityForce1;
    m_PressureAndViscosityForces[particle2Index] += pressureForce2 + viscosityForce2;

}

inline float fp_Fluid::WPoly6(float HSq_LenRSq) {
    return m_WPoly6Coefficient * pow(HSq_LenRSq, 3);
}

inline D3DXVECTOR3 fp_Fluid::GradientWPoly6(const D3DXVECTOR3* R, float HSq_LenRSq) {
    return (*R) * m_GradientWPoly6Coefficient * pow(HSq_LenRSq, 2);
}

inline float fp_Fluid::LaplacianWPoly6(
        float LenRSq, 
        float HSq_LenRSq) {
    return m_LaplacianWPoly6Coefficient * HSq_LenRSq * (LenRSq - 0.75f * HSq_LenRSq);
}

inline D3DXVECTOR3 fp_Fluid::GradientWSpiky(const D3DXVECTOR3* R, float LenR) {
    return (*R) * (m_GradientWSpikyCoefficient / LenR) * pow(m_SmoothingLength - LenR, 2);
}


inline float fp_Fluid::LaplacianWViscosity(float LenR) {
    return m_LaplacianWViscosityCoefficient * (1.0f - LenR / m_SmoothingLength);
}

fp_Fluid::~fp_Fluid() {
    delete[] m_Particles;
    delete m_Grid;
}

