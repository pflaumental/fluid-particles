#include "DXUT.h"
#include "fp_cpu_sph.h"
#include "fp_util.h"
#include <float.h>

fp_Grid::fp_Grid(int InitialCapacity, float CellWidth, float InitialParticleDensity)
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
    m_Cells.reserve(FP_DEFAULT_INITIAL_GRID_CAPACITY);
}

fp_Grid::fp_Grid(const fp_Grid& Other)
        :
        m_Cells(Other.m_Cells),
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
        int cellIndexX = (pos.x - m_MinX) / m_CellWidth;
        int cellIndexY = (pos.y - m_MinY) / m_CellWidth;
        int cellIndexZ = (pos.z - m_MinZ) / m_CellWidth;
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

void fp_Grid::Extract(fp_FluidParticle* OutParticles) {
    int iParticle = 0;
    // For each cell
    for (int cellIndexZ = 0; cellIndexZ < m_NumCellsZ; cellIndexZ++) {
        for (int cellIndexY = 0; cellIndexY < m_NumCellsY; cellIndexY++) {
            for (int cellIndexX = 0; cellIndexX < m_NumCellsX; cellIndexX++) {
                int cellIndex = cellIndexX + cellIndexY * m_NumCellsX
                        + cellIndexZ * m_NumCellsX * m_NumCellsY;
                fp_GridCell* cell = m_Cells[cellIndex];
                if(cell == NULL)
                    continue;
                int cellSize = cell->size();
                // For each particle in cell
                for (int iCellParticle = 0; iCellParticle < cellSize; iCellParticle++) {
                    OutParticles[iParticle++] = (*cell)[iCellParticle];
                }
            }
        }
    }
}

void fp_Grid::SetDensities(float Density) {
    for (int cellIndexZ = 0; cellIndexZ < m_NumCellsZ; cellIndexZ++) {
        for (int cellIndexY = 0; cellIndexY < m_NumCellsY; cellIndexY++) {
            for (int cellIndexX = 0; cellIndexX < m_NumCellsX; cellIndexX++) {
                int cellIndex = cellIndexX + cellIndexY * m_NumCellsX
                        + cellIndexZ * m_NumCellsX * m_NumCellsY;
                fp_GridCell* cell = m_Cells[cellIndex];
                if(cell == NULL)
                    continue;
                int cellSize = cell->size();
                // For each particle in cell
                for (int iCellParticle = 0; iCellParticle < cellSize; iCellParticle++) {
                    (*cell)[iCellParticle].m_Density = Density;
                }
            }
        }
    }
}

void fp_Grid::SetBounds(fp_FluidParticle *Particles, int NumParticles){
    m_MinX = m_MinY = m_MinZ = FLT_MAX;
    m_MaxX = m_MaxY = m_MaxZ = FLT_MIN;    
    for (int i = 0; i < NumParticles; i++)
    {
        D3DXVECTOR3 pos = Particles[i].m_Position;
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
        float Viscosity,
        float ParticleMass,
        float RestDensityCoefficient,
        float Stiffness,
        float SearchRadius,
        int InitialGridCapacity)
        :
        m_NumParticles(NumParticlesX * NumParticlesY * NumParticlesZ),
        m_Grid(new fp_Grid(InitialGridCapacity, SearchRadius, ParticleMass 
                * WPoly6(0.0f))),
        m_Particles(new fp_FluidParticle[NumParticlesX * NumParticlesY * NumParticlesZ]),
        m_GasConstantK(1.0f),
        m_SmoothingLength(SmoothingLenght),
        m_SmoothingLengthSq(SmoothingLenght * SmoothingLenght),        
        m_Viscosity(Viscosity),
        m_ParticleMass(ParticleMass),
        //m_Stiffness(fStiffness),
        m_SearchRadius(SearchRadius),
        m_WPoly6Coefficient(315.0f / (64.0f * D3DX_PI * pow(SmoothingLenght, 9))),
        //m_GradientWPoly6Coefficient(0.0f),
        //m_LaplaceWPoly6Coefficient(0.0f),
        m_GradientWSpikyCoefficient(-45.0f / (D3DX_PI * pow(SmoothingLenght, 6))),
        m_LaplacianWViscosityCoefficient(45.0f / (4.0f * D3DX_PI
                * pow(SmoothingLenght, 3))) {
    m_SmoothingLengthPow3Inv = 1.0f / (m_SmoothingLengthSq * SmoothingLenght);
    m_RestDensity = RestDensityCoefficient * ParticleMass * WPoly6(0.0f);
    float startX = Center.x - 0.5f * (NumParticlesX - 1) * SpacingX;
    float startY = Center.y - 0.5f * (NumParticlesY - 1) * SpacingY;
    float startZ = Center.z - 0.5f * (NumParticlesZ - 1) * SpacingZ;     

    for(int iZ = 0; iZ < NumParticlesZ; iZ++ ) {
        for (int iY = 0; iY < NumParticlesY; iY++) {
            for (int iX = 0; iX < NumParticlesX; iX++) {
                int i = iX + iY * NumParticlesY + iZ * NumParticlesY * NumParticlesZ;
                m_Particles[i].m_Position = D3DXVECTOR3(iX * SpacingX + startX,
                        iY * SpacingY + startY, iZ * SpacingZ + startZ);
                m_Particles[i].m_Velocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                m_Particles[i].m_Density = m_RestDensity;
            }
        }
    }  
    m_Grid->FillAndPrepare(m_Particles, m_NumParticles);
    m_NewGrid = new fp_Grid(*m_Grid);
    // Calculate initial densities
    Update(0.0f, 0.0f);
    // Copy updated densities to m_Particles and m_Grid
    m_NewGrid->Extract(m_Particles);
    fp_Grid* tmpGrid = m_NewGrid;
    m_NewGrid = m_Grid;
    m_Grid = tmpGrid;
}

void fp_Fluid::Update(double Time, float ElapsedTime) {

    int iParticle = 0;
    // For each cell
    for (int cellIndexZ = 0; cellIndexZ < m_Grid->m_NumCellsZ; cellIndexZ++) {
        for (int cellIndexY = 0; cellIndexY < m_Grid->m_NumCellsY; cellIndexY++) {
            for (int cellIndexX = 0; cellIndexX < m_Grid->m_NumCellsX; cellIndexX++) {
                int cellIndex = cellIndexX + cellIndexY * m_Grid->m_NumCellsX
                        + cellIndexZ * m_Grid->m_NumCellsX * m_Grid->m_NumCellsY;
                fp_GridCell* cell = m_Grid->m_Cells[cellIndex];
                if(cell == NULL)
                    continue;
                fp_GridCell* newCell = m_NewGrid->m_Cells[cellIndex];
                int cellSize = cell->size();               
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
                            fp_GridCell* newNeighbourCell
                                    = m_Grid->m_Cells[neighbourCellIndex];
                            int neighbourCellSize = neighbourCell->size();

                            // Pairwise process all particles from cell with all particles
                            // from neighbour cell

                            // For each particle in cell
                            for (int iCellParticle = 0; iCellParticle < cellSize;
                                    iCellParticle++) {
                                fp_FluidParticle* particle = &(*cell)[iCellParticle];

                                // For each particle in neighbour-cell
                                for (int iNeighbourCellParticle = 0;
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
                                        // Select particles in m_NewGrid as output
                                        fp_FluidParticle* newParticle
                                                = &(*m_NewGrid->m_Cells
                                                [cellIndex])[iCellParticle];
                                        fp_FluidParticle* newNeighbourParticle
                                                = &(*m_NewGrid->m_Cells
                                                [neighbourCellIndex])
                                                [iNeighbourCellParticle];
                                        ProcessParticlePair(particle, neighbourParticle,
                                                newParticle, newNeighbourParticle, distSq,
                                                ElapsedTime);
                                    }
                                }
                            }
                        }                            
                    }                        
                }
            }
        }        
    }
    // m_Grid must contain the calculated densities
    // wheras m_NewGrid must contain m_RestDensity
    m_NewGrid->Extract(m_Particles);    
    m_Grid->FillAndPrepare(m_Particles, m_NumParticles);
    delete m_NewGrid;
    m_NewGrid = new fp_Grid(*m_Grid); 
    m_NewGrid->SetDensities(m_RestDensity);
}

inline void fp_Fluid::ProcessParticlePair(
        fp_FluidParticle* Particle1, 
        fp_FluidParticle* Particle2,
        fp_FluidParticle* OutNewParticle1, 
        fp_FluidParticle* OutNewParticle2,
        float DistanceSq,
        float ElapsedTime){
    // Calculate 

    // TODO: schauen was man aus den Schleifen rausziehen kann
    float dist = sqrt(DistanceSq);    
    D3DXVECTOR3 r1 = Particle1->m_Position - Particle2->m_Position;  

    assert(dist <= m_SmoothingLength);

    // Density
    float wPoly6Value = WPoly6(DistanceSq);
    float AdditionalDensity = m_ParticleMass * wPoly6Value;

    // Pressure forces
    float PressureAt1 = m_GasConstantK * (Particle1->m_Density - m_RestDensity);
    float PressureAt2 = m_GasConstantK * (Particle2->m_Density - m_RestDensity);    

    float PressureSum = PressureAt1 + PressureAt2;
    D3DXVECTOR3 gradWSpikyValue1 = GradientWSpiky(r1, dist);
    D3DXVECTOR3 commonPressureTerm1 = - m_ParticleMass * PressureSum / 2.0f
            * gradWSpikyValue1;
    D3DXVECTOR3 pressureForce1 = commonPressureTerm1 / Particle2->m_Density;
    D3DXVECTOR3 pressureForce2 = - commonPressureTerm1 / Particle1->m_Density;
    
    #if defined(DEBUG) || defined(_DEBUG)

    float pressureTest = D3DXVec3LengthSq(&pressureForce1);
    D3DXVECTOR3 pressureForce1Normalized, r1Normalized;
    D3DXVec3Normalize(&pressureForce1Normalized, &pressureForce1);
    D3DXVec3Normalize(&r1Normalized, &r1);
    float testDot = D3DXVec3Dot(&pressureForce1Normalized, &r1Normalized);
    assert(pressureTest < 0.001f || pressureTest > -0.001f || testDot > 0.99f);
    assert(Particle2->m_Density > 0.0f);

    #endif

    // Viscosity forces
    D3DXVECTOR3 velocityDifference1 = Particle2->m_Velocity - Particle1->m_Velocity;
    float laplacianWViskosityValue1 = LaplacianWViscosity(r1, dist);
    D3DXVECTOR3 commonViscosityTerm1 = m_Viscosity * velocityDifference1
            * laplacianWViskosityValue1;
    D3DXVECTOR3 viscosityForce1 = D3DXVECTOR3(0.0f, 0.0f, 0.0f); //commonViscosityTerm1 / Particle2->m_Density;
    D3DXVECTOR3 viscosityForce2 = D3DXVECTOR3(0.0f, 0.0f, 0.0f); //-commonViscosityTerm1 / Particle1->m_Density;

    // Total forces
    D3DXVECTOR3 totalForce1 = pressureForce1 + viscosityForce1;
    D3DXVECTOR3 totalForce2 = pressureForce2 + viscosityForce2;

    assert(totalForce1.x < 10.0f);

    // Write out

    // Densities
    OutNewParticle1->m_Density += AdditionalDensity;        
    OutNewParticle1->m_Velocity += (totalForce1 * ElapsedTime / Particle1->m_Density);    
    OutNewParticle1->m_Position += 0.5f * ElapsedTime
            * (Particle1->m_Velocity + OutNewParticle1->m_Velocity);
    
    OutNewParticle2->m_Velocity -= (totalForce2 * ElapsedTime / Particle2->m_Density);
    OutNewParticle2->m_Density += AdditionalDensity;  
    OutNewParticle2->m_Position += 0.5f * ElapsedTime
            * (Particle2->m_Velocity + OutNewParticle2->m_Velocity);
}

inline float fp_Fluid::WPoly6(float LenRSq) {
    return m_WPoly6Coefficient * pow(m_SmoothingLengthSq - LenRSq, 3);
}

inline D3DXVECTOR3 fp_Fluid::GradientWSpiky(D3DXVECTOR3 R, float LenR) {
    return R * (m_GradientWSpikyCoefficient / LenR) * pow(m_SmoothingLength - LenR, 2);
}


inline float fp_Fluid::LaplacianWViscosity(D3DXVECTOR3 R, float LenR) {
    return (R.x + R.y + R.z) * (m_LaplacianWViscosityCoefficient / LenR)
            * (m_SmoothingLengthPow3Inv + m_SmoothingLength / pow(LenR, 4));
}

fp_Fluid::~fp_Fluid() {
    delete[] m_Particles;
    delete m_Grid;
    delete m_NewGrid;
}

