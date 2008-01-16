#include "DXUT.h"
#include "fp_cpu_sph.h"
#include "fp_util.h"
#include <float.h>

void fp_FluidCalculateFluidStateMTWrapper(void* Data) {
    fp_FluidMTHelperData* mtData
            = (fp_FluidMTHelperData*) Data;
    mtData->m_Fluid->CalculateFluidStateMT(mtData->m_ThreadIdx);
}

void fp_FluidCalculateGlassFluidStateChangeMTWrapper(void* Data) {
    fp_FluidMTHelperData* mtData
        = (fp_FluidMTHelperData*) Data;
    mtData->m_Fluid->CalculateGlassFluidStateChangeMT(mtData->m_ThreadIdx);
}

void fp_FluidMoveParticlesMTWrapper(void* Data) {
    fp_FluidMTHelperData* mtData
        = (fp_FluidMTHelperData*) Data;
    mtData->m_Fluid->MoveParticlesMT(mtData->m_ThreadIdx);
}

void fp_FluidDummyFunc(void* Data) {
    ;
}

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
    m_NumCellsYZ = m_NumCellsY * m_NumCellsZ;
    m_NumCells = m_NumCellsX * m_NumCellsYZ;
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
        int cellIndex = cellIndexX * m_NumCellsYZ + cellIndexY * m_NumCellsZ
                + cellIndexZ;
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
        D3DXVECTOR3 Gravity,
        float SmoothingLenght,
        float GlassEnforceDistance,
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
        m_CurrentGlassPosition(Center),
        m_LastGlassPosition(Center),
        m_LastGlassVelocity(0.0f, 0.0f, 0.0f),
        m_GlassVelocityChange(0.0f, 0.0f, 0.0f),
        m_GlassRadius(GlassRadius),
        m_GlassFloor(GlassFloor),
        m_Gravity(Gravity),
        m_GasConstantK(GasConstantK),
        m_Viscosity(Viscosity),
        m_SurfaceTension(SurfaceTension),
        m_RestDensityCoefficient(RestDensityCoefficient),
        m_GradientColorFieldThresholdSq(GradientColorFieldThreshold
                * GradientColorFieldThreshold),
        m_DampingCoefficient(DampingCoefficient) {
    m_WorkerThreadMgr = WorkerThreadMgr;
    int numWorkerThreads = m_WorkerThreadMgr->m_NumWorkerThreads;
    m_MTData = new fp_FluidMTHelperData[numWorkerThreads];
    for(int iWorker=0; iWorker < numWorkerThreads; iWorker++) {
        m_MTData[iWorker].m_Fluid = this;
        m_MTData[iWorker].m_ThreadIdx = iWorker;
    }

    SetGlassEnforceDistance(GlassEnforceDistance);
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
                ((D3DXVECTOR3*)m_PressureAndViscosityForces)[i]
                        = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                ((D3DXVECTOR3*)m_GradientColorField)[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
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
    volatile float* tmpDensities = m_OldDensities;
    m_OldDensities = (float*)m_NewDensities;
    for(int i=0; i<m_NumParticles; i++) {
        tmpDensities[i] = m_InitialDensity;
    }
    m_NewDensities = tmpDensities;
}

fp_Fluid::~fp_Fluid() {
    m_WorkerThreadMgr->DoJobOnAllThreads(fp_FluidDummyFunc, NULL, 0);
    m_WorkerThreadMgr->WaitTillJobFinishedOnAllThreads();
    delete[] m_Particles;
    delete[] m_MTData;
    delete m_Grid;
}

void fp_Fluid::SetGlassEnforceDistance(float GlassEnforceDistance) {
    float glassEnforceDistanceSq = GlassEnforceDistance * GlassEnforceDistance;
    m_GlassRadiusPlusEnforceDistance = m_GlassRadius + GlassEnforceDistance;
    m_GlassRadiusPlusEnforceDistanceSq = m_GlassRadiusPlusEnforceDistance
        * m_GlassRadiusPlusEnforceDistance ;
    m_GlassFloorPlusEnforceDistance = m_GlassFloor + GlassEnforceDistance;
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
    m_CurrentElapsedTime = ElapsedTime;

    // Calculate new densities, pressure forces and viscosity forces
    m_WorkerThreadMgr->DoJobOnAllThreads(fp_FluidCalculateFluidStateMTWrapper, m_MTData,
            sizeof(fp_FluidMTHelperData));
    m_WorkerThreadMgr->WaitTillJobFinishedOnAllThreads();

    // For glass collision:
    D3DXVECTOR3 glassVelocity = (m_CurrentGlassPosition - m_LastGlassPosition) / ElapsedTime;
    m_GlassVelocityChange = glassVelocity - m_LastGlassVelocity;
    m_LastGlassVelocity = glassVelocity;
    m_LastGlassPosition = m_CurrentGlassPosition;
    
    // Calculate densities changes, pressure forces and viscosity forces produced by
    // glass
    m_CurrentGlassFloorY = m_GlassFloor + m_CurrentGlassPosition.y;
    m_WorkerThreadMgr->DoJobOnAllThreads(
            fp_FluidCalculateGlassFluidStateChangeMTWrapper, m_MTData,
            sizeof(fp_FluidMTHelperData));
    m_WorkerThreadMgr->WaitTillJobFinishedOnAllThreads();

    // Move particles and clear fields    
    m_CurrentGlassEnforceMinY = m_GlassFloorPlusEnforceDistance + m_CurrentGlassPosition.y;
    m_WorkerThreadMgr->DoJobOnAllThreads(fp_FluidMoveParticlesMTWrapper, m_MTData,
        sizeof(fp_FluidMTHelperData));
    m_WorkerThreadMgr->WaitTillJobFinishedOnAllThreads();

    // Update Grid
    m_Grid->FillAndPrepare(m_Particles, m_NumParticles);

    // Prepare densities
    float* tmpDensities = m_OldDensities;
    m_OldDensities = (float*)m_NewDensities;
    m_NewDensities = tmpDensities;
}

void fp_Fluid::CalculateFluidStateMT(int ThreadIdx) {
    int NumCellsX = m_Grid->m_NumCellsX, NumCellsY = m_Grid->m_NumCellsY, NumCellsZ
            = m_Grid->m_NumCellsZ, NumCellsYZ = NumCellsY * NumCellsZ;

    // For each cell
    for (int cellIndexX = 0; cellIndexX < NumCellsX; cellIndexX++) {
        for (int cellIndexY = 0; cellIndexY < NumCellsY; cellIndexY++) {
            for (int cellIndexZ = 0; cellIndexZ < NumCellsZ; cellIndexZ++) {                    
                int cellIndex = cellIndexX * NumCellsYZ + cellIndexY * NumCellsZ
                    + cellIndexZ;
                // For MT:
                if(cellIndex % m_WorkerThreadMgr->m_NumWorkerThreads != ThreadIdx)
                    continue;

                fp_GridCell* cell = m_Grid->m_Cells[cellIndex];
                if(cell == NULL)
                    continue;
                fp_GridCellSize cellSize = cell->size();
                // For each particle in cell
                for (fp_GridCellSize iCellParticle = 0; iCellParticle < cellSize;
                    iCellParticle++) {
                        fp_FluidParticle* particle = &(*cell)[iCellParticle];
                        // For each following neighbor-cell
                        for (int xN = 0; xN <= 1; xN++) {
                            int neighborCellIndexX = cellIndexX + xN;
                            if(neighborCellIndexX >= NumCellsX)
                                continue;
                            for (int yN = 0; yN <= 1; yN++) {
                                int neighborCellIndexY = cellIndexY + yN;
                                if(neighborCellIndexY >= NumCellsY)
                                    continue;
                                for (int zN = 0; zN <= 1; zN++) {
                                    int neighborCellIndexZ = cellIndexZ + zN;
                                    if(neighborCellIndexZ >= NumCellsZ)
                                        continue;
                                    int neighborCellIndex = neighborCellIndexX * NumCellsYZ
                                        + neighborCellIndexY * NumCellsZ
                                        + neighborCellIndexZ;
                                    fp_GridCell* neighborCell
                                        = m_Grid->m_Cells[neighborCellIndex];
                                    if(neighborCell == NULL)
                                        continue;
                                    fp_GridCellSize neighborCellSize = neighborCell->size();

                                    // Pairwise process all particles from cell with all particles
                                    // from neighbor cell

                                    // For each particle in neighbor-cell
                                    for (fp_GridCellSize iNeighbourCellParticle = 0;
                                            iNeighbourCellParticle < neighborCellSize;
                                            iNeighbourCellParticle++) {
                                        // Process only "following" particles, otherwise particles
                                        // would get evaluated twice
                                        if(cellIndex == neighborCellIndex
                                                && iCellParticle <= iNeighbourCellParticle)
                                            continue;
                                        fp_FluidParticle* neighborParticle =
                                        &(*neighborCell)[iNeighbourCellParticle];
                                        D3DXVECTOR3 toNeighbour = neighborParticle->m_Position
                                            - particle->m_Position;
                                        float distSq = D3DXVec3LengthSq(&toNeighbour);
                                        assert(distSq > 0.0f);
                                        if(distSq < m_SmoothingLengthSq) {                                        
                                            ProcessParticlePair(particle, neighborParticle,
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
}

// Simulates the glass-fluid interaction similarly to particle-particle interaction
void fp_Fluid::CalculateGlassFluidStateChangeMT(int ThreadIdx) {
    int numParticlesPerThread = (int)ceil((double)m_NumParticles
        / m_WorkerThreadMgr->m_NumWorkerThreads);
    int startIndex = ThreadIdx * numParticlesPerThread;
    int endIndex = startIndex + numParticlesPerThread;
    if(endIndex > m_NumParticles) endIndex = m_NumParticles;
    for (int i = startIndex; i < endIndex; i++) {
        fp_FluidParticle* Particle = &m_Particles[i];
        D3DXVECTOR3 particlePosition = Particle->m_Position;
        int particleIndex = Particle->m_Index;

        // Handle Collision with floor 
        float lenR = particlePosition.y - m_CurrentGlassFloorY;
        if(lenR < FP_DEFAULT_FLUID_GLASS_PUSHBACK_DISTANCE)
            lenR = FP_DEFAULT_FLUID_GLASS_PUSHBACK_DISTANCE;
        if(lenR < m_SmoothingLength) {
            float lenRSq = lenR * lenR;
            D3DXVECTOR3 r = D3DXVECTOR3(0.0f, lenR, 0.0f);

            // Density
            float hSq_lenRSq = m_SmoothingLengthSq - lenRSq;
            float particleDensity = m_OldDensities[particleIndex];
            float wPoly6Value = WPoly6(hSq_lenRSq);
            float AdditionalDensity = m_ParticleMass * wPoly6Value;
            m_NewDensities[particleIndex] += AdditionalDensity;

            // Pressure force
            float pressure = m_GasConstantK * (particleDensity - m_RestDensity);
            D3DXVECTOR3 gradWSpikyValue = GradientWSpiky(&r, lenR);
            D3DXVECTOR3 pressureTerm = - m_ParticleMass * pressure * gradWSpikyValue;
            D3DXVECTOR3 pressureForce = pressureTerm / particleDensity;

            // Viscosity force
            D3DXVECTOR3 velocityDifference = m_LastGlassVelocity - Particle->m_Velocity;
            float laplacianWViskosityValue = LaplacianWViscosity(lenR);
            D3DXVECTOR3 viscosityTerm = m_ParticleMass * m_Viscosity * velocityDifference
                * laplacianWViskosityValue;
            D3DXVECTOR3 viscosityForce = viscosityTerm / particleDensity;

            ((D3DXVECTOR3*)m_PressureAndViscosityForces)[particleIndex] += pressureForce
                + viscosityForce;
        }

        // Handle collision with side
        D3DXVECTOR3 particleToCenter = m_CurrentGlassPosition - particlePosition;
        particleToCenter.y = 0.0f;
        float particleToCenterLen = D3DXVec3Length(&particleToCenter);
        float particleToCenterLenInv = 1.0f / particleToCenterLen;
        lenR = m_GlassRadius - particleToCenterLen;  
        if(lenR < FP_DEFAULT_FLUID_GLASS_PUSHBACK_DISTANCE) {
            lenR = FP_DEFAULT_FLUID_GLASS_PUSHBACK_DISTANCE;
            particleToCenterLen = m_GlassRadius - FP_DEFAULT_FLUID_GLASS_PUSHBACK_DISTANCE;
            particleToCenter *= particleToCenterLenInv * particleToCenterLen;
            particleToCenterLenInv = 1.0f / particleToCenterLen;
        }
        if(lenR < m_SmoothingLength) {        
            float lenRSq = lenR * lenR;
            D3DXVECTOR3 r = particleToCenter * particleToCenterLenInv * lenR;

            // Density
            float hSq_lenRSq = m_SmoothingLengthSq - lenRSq;
            float particleDensity = m_OldDensities[particleIndex];
            float wPoly6Value = WPoly6(hSq_lenRSq);
            float AdditionalDensity = m_ParticleMass * wPoly6Value;
            m_NewDensities[particleIndex] += AdditionalDensity;

            // Pressure force
            float pressure = m_GasConstantK * (particleDensity - m_RestDensity);
            D3DXVECTOR3 gradWSpikyValue = GradientWSpiky(&r, lenRSq);
            D3DXVECTOR3 pressureTerm = - m_ParticleMass * pressure * gradWSpikyValue;
            D3DXVECTOR3 pressureForce = pressureTerm / particleDensity;        

            // Viscosity force
            D3DXVECTOR3 velocityDifference = m_LastGlassVelocity - Particle->m_Velocity;
            float laplacianWViskosityValue = LaplacianWViscosity(lenR);
            D3DXVECTOR3 viscosityTerm = m_ParticleMass * m_Viscosity * velocityDifference
                * laplacianWViskosityValue;
            D3DXVECTOR3 viscosityForce = viscosityTerm / particleDensity;

            ((D3DXVECTOR3*)m_PressureAndViscosityForces)[particleIndex] += pressureForce
                + viscosityForce;
        }
    }
}

void fp_Fluid::MoveParticlesMT(int ThreadIdx) {
    // Move particles and clear fields
    int numParticlesPerThread = (int)ceil((double)m_NumParticles
            / m_WorkerThreadMgr->m_NumWorkerThreads);
    int startIndex = ThreadIdx * numParticlesPerThread;
    int endIndex = startIndex + numParticlesPerThread;
    if(endIndex > m_NumParticles) endIndex = m_NumParticles;
    for (int i = startIndex; i < endIndex; i++) {
        D3DXVECTOR3 oldVelocity = m_Particles[i].m_Velocity;
        D3DXVECTOR3 oldVelocityContribution = oldVelocity
            * pow(m_DampingCoefficient, m_CurrentElapsedTime);
        D3DXVECTOR3 totalForce = ((D3DXVECTOR3*)m_PressureAndViscosityForces)[i];
        D3DXVECTOR3 gradColorField = ((D3DXVECTOR3*)m_GradientColorField)[i];
        float gradColorFieldLenSq = D3DXVec3LengthSq(&gradColorField);
        if(gradColorFieldLenSq >= m_GradientColorFieldThresholdSq) {
            D3DXVECTOR3 surfaceTensionForce = (-m_SurfaceTension *
                m_LaplacianColorField[i]/sqrt(gradColorFieldLenSq)) * gradColorField;
            totalForce += surfaceTensionForce;
        }
        D3DXVECTOR3 newVelocity = oldVelocityContribution 
            + (totalForce / m_OldDensities[i] + m_Gravity) * m_CurrentElapsedTime;        
        m_Particles[i].m_Velocity = newVelocity;
        m_Particles[i].m_Position += 0.5f * m_CurrentElapsedTime
            * (oldVelocityContribution + newVelocity);        

        EnforceGlass(&m_Particles[i]);

        ((D3DXVECTOR3*)m_PressureAndViscosityForces)[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        ((D3DXVECTOR3*)m_GradientColorField)[i] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_LaplacianColorField[i] = 0.0f;
        m_OldDensities[i] = m_RestDensity;
    }
}

inline void fp_Fluid::EnforceGlass(fp_FluidParticle* Particle) {
    D3DXVECTOR3 particlePosition = Particle->m_Position;

    // Use "manual" model to enforce the particles to stay in the glass
    
    // Handle Collision with floor    
    if(particlePosition.y < m_CurrentGlassEnforceMinY) {
        // Position particle on floor
        Particle->m_Position.y = m_CurrentGlassEnforceMinY;
        // Invert it's velocity along y-Axis
        Particle->m_Velocity.y = -Particle->m_Velocity.y + m_GlassVelocityChange.y;
    }

    // Handle collision with side
    D3DXVECTOR3 particleToCenter = m_CurrentGlassPosition - particlePosition;
    particleToCenter.y = 0.0f;
    float particleToCenterLenSq = D3DXVec3LengthSq(&particleToCenter);
    if(particleToCenterLenSq > m_GlassRadiusPlusEnforceDistanceSq) {
        // Position particle on side
        float particleToCenterLen = sqrt(particleToCenterLenSq);
        float scale = (particleToCenterLen - m_GlassRadiusPlusEnforceDistance)
                / particleToCenterLen;
        Particle->m_Position += scale * particleToCenter;
        // Invert it's velocity along the sides normal
        D3DXVECTOR3 normal;
        D3DXVec3Normalize(&normal, &particleToCenter);
        // R = -2 * (N*V) * N + V
        Particle->m_Velocity += -2.0f * D3DXVec3Dot(&normal, &Particle->m_Velocity)
                * normal;
        Particle->m_Velocity.x += m_GlassVelocityChange.x * abs(normal.x);
        Particle->m_Velocity.z += m_GlassVelocityChange.z * abs(normal.z);
    }
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
    //float testDot = D3DXVec3Dot(&pressureForce1Normalized, &r1Normalized);
    //assert(pressureForce1LenSq < 0.1f || pressureForce1LenSq > -0.1f
    //        || testDot > 0.99f);
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
    ((D3DXVECTOR3*)m_GradientColorField)[particle1Index]
            += commonGradientColorFieldTerm1 / particle2Density;
    ((D3DXVECTOR3*)m_GradientColorField)[particle2Index]
            -= commonGradientColorFieldTerm1 / particle1Density;
    float laplacianWPoly6Value1 = LaplacianWPoly6(DistanceSq, hSq_lenRSq);
    float commonLaplacianColorFieldTerm1 = m_ParticleMass * laplacianWPoly6Value1;
    m_LaplacianColorField[particle1Index] += commonLaplacianColorFieldTerm1
            / particle2Density;
    m_LaplacianColorField[particle2Index] += commonLaplacianColorFieldTerm1
            / particle1Density;

    // Total forces
    ((D3DXVECTOR3*)m_PressureAndViscosityForces)[particle1Index] += pressureForce1 + viscosityForce1;
    ((D3DXVECTOR3*)m_PressureAndViscosityForces)[particle2Index] += pressureForce2 + viscosityForce2;

}

