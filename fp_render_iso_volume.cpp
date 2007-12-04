#include "DXUT.h"
//#include "SDKmisc.h"
#include "fp_render_iso_volume.h"
//#include "fp_util.h"

fp_VolumeIndex operator+(
        const fp_VolumeIndex& A,
        const fp_VolumeIndex& B) {
    fp_VolumeIndex result;
    result.X = A.X + B.X;
    result.Y = A.Y + B.Y;
    result.Z = A.Z + B.Z;
    return result;
}

fp_IsoVolume::fp_IsoVolume(fp_Fluid* Fluid, float VoxelSize, float IsoVolumeBorder) 
        :
        m_Fluid(Fluid),
        m_NumValues(0),
        m_NumValuesYZ(0),
        m_NumValuesX(0),
        m_NumValuesY(0),
        m_NumValuesZ(0),        
        m_IsoValues(),
        m_StampRowLengths(NULL),
        m_StampRowStartOffsets(NULL),
        m_StampRowValueStarts(NULL),
        m_StampValues(NULL),
        m_IsoVolumeBorder(IsoVolumeBorder) {
    m_VoxelSize = -1.0f; // Needed in SetSmoothingLength()
    UpdateSmoothingLength();
    SetVoxelSize(VoxelSize);
    m_IsoValues.reserve(FP_INITIAL_ISOVOLUME_CAPACITY);
}

void fp_IsoVolume::UpdateSmoothingLength() {
    if(m_VoxelSize > 0.0f) {
        if(m_StampValues != NULL)
            DestroyStamp();
        CreateStamp();
    }
}

void fp_IsoVolume::SetVoxelSize(float VoxelSize) {
    m_VoxelSize = VoxelSize;
    m_HalfVoxelSize = VoxelSize / 2.0f;
    if(m_StampValues != NULL)
        DestroyStamp();
    CreateStamp();
}

void fp_IsoVolume::ConstructFromFluid() {
    float MinX, MaxX, MinY, MaxY, MinZ, MaxZ;
    m_Fluid->GetParticleMinsAndMaxs(MinX, MaxX, MinY, MaxY, MinZ, MaxZ);
    MinX -= m_IsoVolumeBorder;
    MinY -= m_IsoVolumeBorder;
    MinZ -= m_IsoVolumeBorder;
    MaxX += m_IsoVolumeBorder;
    MaxY += m_IsoVolumeBorder;
    MaxZ += m_IsoVolumeBorder;
    m_VolumeStart = D3DXVECTOR3(MinX + m_HalfVoxelSize, MinY + m_HalfVoxelSize, MinZ + m_HalfVoxelSize);
    m_VolumeCellOffset = D3DXVECTOR3(m_VoxelSize, m_VoxelSize, m_VoxelSize);
    m_NumValuesX = (int)((MaxX - MinX + m_VoxelSize) / m_VoxelSize);
    m_NumValuesY = (int)((MaxY - MinY + m_VoxelSize) / m_VoxelSize);
    m_NumValuesZ = (int)((MaxZ - MinZ + m_VoxelSize) / m_VoxelSize);
    m_NumValuesYZ = m_NumValuesY * m_NumValuesZ;
    m_NumValues = m_NumValuesYZ * m_NumValuesX;
    m_IsoValues.clear();
    m_IsoValues.resize(m_NumValues, 0.0f);
    int NumParticles = m_Fluid->m_NumParticles;
    fp_FluidParticle* Particles = m_Fluid->m_Particles;
    float* Densities = m_Fluid->GetDensities();
    float ParticleMass = m_Fluid->m_ParticleMass;
    for (int i = 0; i < NumParticles; i++) {
        D3DXVECTOR3 particlePosition = Particles[i].m_Position;
        float particleDensity = Densities[i];
        DistributeParticle(particlePosition, particleDensity, ParticleMass, MinX, MinY,
                MinZ);
    }
}

inline void fp_IsoVolume::DistributeParticle(
        D3DXVECTOR3 ParticlePosition,
        float ParticleDensity,
        float ParticleMass,
        float MinX,
        float MinY,
        float MinZ) {
    fp_VolumeIndex particleVolumeIndex;
    particleVolumeIndex.X = (int)((ParticlePosition.x - MinX) / m_VoxelSize);
    particleVolumeIndex.Y = (int)((ParticlePosition.y - MinY) / m_VoxelSize);
    particleVolumeIndex.Z = (int)((ParticlePosition.z - MinZ) / m_VoxelSize);

    for (int stampRowIndex = 0; stampRowIndex < m_NumStampRows; stampRowIndex++) {
        fp_VolumeIndex destStart = particleVolumeIndex
                + m_StampRowStartOffsets[stampRowIndex];
        if(destStart.X < 0 || destStart.X >= m_NumValuesX
                || destStart.Y < 0 || destStart.Y >= m_NumValuesY)
            continue;

        int destIndexXY = m_NumValuesYZ * destStart.X
            + m_NumValuesZ * destStart.Y;
        int destIndex = destStart.Z < 0 ? destIndexXY : destIndexXY + destStart.Z;
        int zEnd = destStart.Z + m_StampRowLengths[stampRowIndex];
        if(zEnd > m_NumValuesZ)
            zEnd = m_NumValuesZ;
        int destEndIndex = destIndexXY + zEnd;

        int stampValueIndex = m_StampRowValueStarts[stampRowIndex];

        for(; destIndex < destEndIndex; destIndex++) {
            float additionalIsoValue = ParticleMass * m_StampValues[stampValueIndex++]
                    / ParticleDensity;
            m_IsoValues[destIndex] += additionalIsoValue;
        }
    }
}

void fp_IsoVolume::CreateStamp() {
    int stampRadius = (int)ceil((m_Fluid->m_SmoothingLength - m_HalfVoxelSize) 
            / m_VoxelSize);
    int stampSideLength = 1 + 2 * stampRadius;
    int stampSideLengthSq = stampSideLength * stampSideLength;    
    
    float halfLength = stampSideLength * m_HalfVoxelSize;
    D3DXVECTOR3 mid(halfLength, halfLength, halfLength);
    
    float* cubeStampDistancesSq = new float[stampSideLength * stampSideLength
            * stampSideLength];
    fp_VolumeIndex* cubeStampRowStarts = new fp_VolumeIndex[stampSideLengthSq];
    int* cubeStampRowLengths = new int[stampSideLengthSq];
    m_NumStampValues = 0;
    m_NumStampRows = 0;

    for (int stampX = 0; stampX < stampSideLength; stampX++) {
        for (int stampY = 0; stampY < stampSideLength; stampY++) {
            int stampIndexXY = stampX * stampSideLengthSq + stampY * stampSideLength;
            int cubeStampRowIndex = stampX * stampSideLength + stampY;
            cubeStampRowLengths[cubeStampRowIndex] = 0;
            for (int stampZ = 0; stampZ < stampSideLength; stampZ++) {
                D3DXVECTOR3 coord(
                        stampX * m_VoxelSize + m_HalfVoxelSize,
                        stampY * m_VoxelSize + m_HalfVoxelSize,
                        stampZ * m_VoxelSize + m_HalfVoxelSize);
                D3DXVECTOR3 toMid = mid - coord;
                float distSq = D3DXVec3LengthSq(&toMid);
                if(distSq < m_Fluid->m_SmoothingLengthSq) {
                    int stampIndex = stampIndexXY + stampZ;
                    cubeStampDistancesSq[stampIndex] = distSq;
                    if(cubeStampRowLengths[cubeStampRowIndex] == 0) {
                        m_NumStampRows++;
                        fp_VolumeIndex rowStart;
                        rowStart.X = stampX;
                        rowStart.Y = stampY;
                        rowStart.Z = stampZ;
                        cubeStampRowStarts[cubeStampRowIndex] = rowStart;
                    }
                    m_NumStampValues++;
                    cubeStampRowLengths[cubeStampRowIndex]++;
                } else {
                    cubeStampDistancesSq[stampIndexXY + stampZ] = -1.0f;
                }
            }
        }
    }
    m_StampRowLengths = new int[m_NumStampRows];
    m_StampRowStartOffsets = new fp_VolumeIndex[m_NumStampRows];
    m_StampRowValueStarts = new int[m_NumStampRows];
    m_StampValues = new float[m_NumStampValues];
    int stampRowIndex = 0;
    int stampValueIndex = 0;
    for (int cubeRow = 0; cubeRow < stampSideLengthSq; cubeRow++) {
        int rowLength = cubeStampRowLengths[cubeRow];
        if(rowLength == 0)
            continue;
        m_StampRowLengths[stampRowIndex] = rowLength;
        fp_VolumeIndex cubeStampVolumeIndex = cubeStampRowStarts[cubeRow];
        fp_VolumeIndex rowStartOffset;
        rowStartOffset.X = cubeStampVolumeIndex.X - stampRadius;
        rowStartOffset.Y = cubeStampVolumeIndex.Y - stampRadius;
        rowStartOffset.Z = cubeStampVolumeIndex.Z - stampRadius;
        m_StampRowStartOffsets[stampRowIndex] = rowStartOffset;
        m_StampRowValueStarts[stampRowIndex] = stampValueIndex;
        int cubeStampIndex = cubeStampVolumeIndex.X * stampSideLengthSq
                + cubeStampVolumeIndex.Y * stampSideLength
                + stampRadius - (rowLength - 1) / 2;
        int cubeStampEnd = cubeStampIndex + rowLength;
        for (; cubeStampIndex < cubeStampEnd; cubeStampIndex++) {            
            m_StampValues[stampValueIndex]
                    = m_Fluid->WPoly6(cubeStampDistancesSq[cubeStampIndex]);
            stampValueIndex++;
        }
        stampRowIndex++;
    }
}

void fp_IsoVolume::DestroyStamp() {
    delete[] m_StampRowLengths;
    delete[] m_StampRowStartOffsets;
    delete[] m_StampRowValueStarts;
    delete[] m_StampValues;
}

fp_RenderIsoVolume::fp_RenderIsoVolume(fp_IsoVolume* IsoVolume, float IsoLevel) :
        m_IsoVolume(IsoVolume),
        m_IsoLevel(IsoLevel) {
    m_Material.Diffuse.r = m_Material.Diffuse.g = m_Material.Diffuse.b = 0.8f;
    m_Material.Diffuse.a = 1.0f;
    m_Material.Ambient = m_Material.Diffuse;
    m_Material.Emissive = m_Material.Diffuse;
    m_Material.Specular = m_Material.Diffuse;
}

fp_RenderIsoVolume::~fp_RenderIsoVolume() {
    OnLostDevice();
    OnDetroyDevice();
}

HRESULT fp_RenderIsoVolume::OnCreateDevice(                                 
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* pBackBufferSurfaceDesc ) {
    HRESULT hr;

    return S_OK;
}

HRESULT fp_RenderIsoVolume::OnResetDevice(
        IDirect3DDevice9* d3dDevice,
        const D3DSURFACE_DESC* BackBufferSurfaceDesc ) {
    d3dDevice->CreateVertexBuffer( FP_MC_MAX_VETICES * sizeof(fp_MCVertex), 
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 
            fp_MCVertex::FVF_Flags, D3DPOOL_DEFAULT, &m_VertexBuffer, NULL );
    d3dDevice->CreateIndexBuffer(FP_MC_MAX_TRIANGLES * 3,
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
            D3DFMT_INDEX32, D3DPOOL_DEFAULT, &m_IndexBuffer, NULL );
    return S_OK;
}

void fp_RenderIsoVolume::ConstructMesh() {
    m_IsoLevel;
    m_NumVertices = 0;
    int indexIndex = 0;
    m_NumTriangles = 0;

    // Build cube
    int NumValuesX = m_IsoVolume->m_NumValuesX;
    int NumValuesY = m_IsoVolume->m_NumValuesY;
    int NumValuesZ = m_IsoVolume->m_NumValuesZ;
    int NumValuesYZ = m_IsoVolume->m_NumValuesYZ;
    int xEnd = NumValuesX - 1;
    int yEnd = NumValuesY - 1;
    int zEnd = NumValuesZ - 1;
    D3DXVECTOR3 volumeStart = m_IsoVolume->m_VolumeStart;
    float voxelSize = m_IsoVolume->m_VoxelSize;
    D3DXVECTOR3 volumeOffset = D3DXVECTOR3(voxelSize, voxelSize, voxelSize);
    std::vector<float>* isoValues = &m_IsoVolume->m_IsoValues;

    fp_MCVertex *mcVertices;
    m_VertexBuffer->Lock( 0, FP_MC_MAX_VETICES * sizeof(fp_MCVertex),
        (void**)&mcVertices, D3DLOCK_DISCARD );
    int* mcIndizes;
    m_IndexBuffer->Lock(0, FP_MC_MAX_TRIANGLES * 3, (void**)&mcIndizes, D3DLOCK_DISCARD);
    
    for(int cubeX=0; cubeX < xEnd; cubeX++) {
        for(int cubeY=0; cubeY < yEnd; cubeY++) {
            int isoVolumeIndex4 = cubeX * NumValuesYZ + cubeY * NumValuesZ; // x,y,z
            int isoVolumeIndex5 = isoVolumeIndex4 + NumValuesYZ; // x+1,y,z
            int isoVolumeIndex6 = isoVolumeIndex5 + NumValuesZ; // x+1,y+1,z
            int isoVolumeIndex7 = isoVolumeIndex4 + NumValuesZ; // x,y+1,z
            float isoValue0, isoValue1, isoValue2, isoValue3;
            float isoValue4 = (*isoValues)[isoVolumeIndex4++];
            float isoValue5 = (*isoValues)[isoVolumeIndex5++];
            float isoValue6 = (*isoValues)[isoVolumeIndex6++];
            float isoValue7 = (*isoValues)[isoVolumeIndex7++];
            int vertexIndizes[12] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
            D3DXVECTOR3 corner0(volumeStart + D3DXVECTOR3(volumeOffset.x * cubeX, 
                    volumeOffset.y * cubeY, 0.0f ));
            for(int cubeZ=0; cubeZ < zEnd; cubeZ++) {                
                isoValue0 = isoValue4;
                isoValue1 = isoValue5;
                isoValue2 = isoValue6;
                isoValue3 = isoValue7;

                isoValue4 = (*isoValues)[isoVolumeIndex4++];
                isoValue5 = (*isoValues)[isoVolumeIndex5++];
                isoValue6 = (*isoValues)[isoVolumeIndex6++];
                isoValue7 = (*isoValues)[isoVolumeIndex7++];

                byte cubeType=0;
                if(isoValue0 < m_IsoLevel) cubeType |= 1;
                if(isoValue1 < m_IsoLevel) cubeType |= 2;
                if(isoValue2 < m_IsoLevel) cubeType |= 4;
                if(isoValue3 < m_IsoLevel) cubeType |= 8;
                if(isoValue4 < m_IsoLevel) cubeType |= 16;
                if(isoValue5 < m_IsoLevel) cubeType |= 32;
                if(isoValue6 < m_IsoLevel) cubeType |= 64;
                if(isoValue7 < m_IsoLevel) cubeType |= 128;

                int edgeValue = s_EdgeTable[cubeType];
                if(edgeValue == 0)
                    continue;

                // Vertices:                
                vertexIndizes[0] = vertexIndizes[4];
                vertexIndizes[1] = vertexIndizes[5];
                vertexIndizes[2] = vertexIndizes[6];
                vertexIndizes[3] = vertexIndizes[7];

                assert(!((edgeValue & 1) && (vertexIndizes[0] == -1)));
                assert(!((edgeValue & 2) && (vertexIndizes[1] == -1)));
                assert(!((edgeValue & 4) && (vertexIndizes[2] == -1)));
                assert(!((edgeValue & 8) && (vertexIndizes[3] == -1)));

                if(edgeValue & 16) // Edge 4 between corner 4 and 5
                    mcVertices[vertexIndizes[4] = m_NumVertices++] = D3DXVECTOR3(corner0.x
                            + (m_IsoLevel - isoValue4) / (isoValue5 - isoValue4)
                            * volumeOffset.x, corner0.y, corner0.z + volumeOffset.z);
                if(edgeValue & 32) // Edge 5 between corner 5 and 6
                    mcVertices[vertexIndizes[5] = m_NumVertices++] = D3DXVECTOR3(corner0.x
                            + volumeOffset.x, corner0.y + (m_IsoLevel - isoValue5)
                            / (isoValue6 - isoValue5) * volumeOffset.y, corner0.z
                            + volumeOffset.z);
                if(edgeValue & 64) // Edge 6 between corner 7 and 6
                    mcVertices[vertexIndizes[6] = m_NumVertices++] = D3DXVECTOR3(corner0.x
                            + (m_IsoLevel - isoValue7) / (isoValue6 - isoValue7)
                            * volumeOffset.x, corner0.y + volumeOffset.y, corner0.z
                            + volumeOffset.z);
                if(edgeValue & 128) // Edge 7 between corner 4 and 7
                    mcVertices[vertexIndizes[7] = m_NumVertices++] = D3DXVECTOR3(corner0.x,
                            corner0.y + (m_IsoLevel - isoValue4) / (isoValue7 - isoValue4)
                            * volumeOffset.y, corner0.z + volumeOffset.z);
                if(edgeValue & 256) // Edge 8 between corner 0 and 4
                    mcVertices[vertexIndizes[8] = m_NumVertices++] = D3DXVECTOR3(corner0.x,
                            corner0.y, corner0.z + (m_IsoLevel - isoValue0) / (isoValue4
                            - isoValue0) * volumeOffset.z);
                if(edgeValue & 512) // Edge 9 between corner 1 and 5
                    mcVertices[vertexIndizes[9] = m_NumVertices++] = D3DXVECTOR3(corner0.x
                            + volumeOffset.x, corner0.y, corner0.z + (m_IsoLevel
                            - isoValue1) / (isoValue5 - isoValue1) * volumeOffset.z);
                if(edgeValue & 1024) // Edge 10 between corner 2 and 6
                    mcVertices[vertexIndizes[10] = m_NumVertices++] = D3DXVECTOR3(corner0.x
                            + volumeOffset.x, corner0.y + volumeOffset.y, corner0.z
                            + (m_IsoLevel - isoValue2) / (isoValue6 - isoValue2)
                            * volumeOffset.z);
                if(edgeValue & 2048) // Edge 11 between corner 3 and 7
                    mcVertices[vertexIndizes[11] = m_NumVertices++] = D3DXVECTOR3(corner0.x,
                            corner0.y + volumeOffset.y, corner0.z + (m_IsoLevel
                            - isoValue3) / (isoValue7 - isoValue3) * volumeOffset.z);

                // Triangles:
                int* triTableEntry = s_TriTable[cubeType];
                for(int i=0; triTableEntry[i] != -1; ) {
                    mcIndizes[indexIndex++] = vertexIndizes[triTableEntry[i++]];
                    mcIndizes[indexIndex++] = vertexIndizes[triTableEntry[i++]];
                    mcIndizes[indexIndex++] = vertexIndizes[triTableEntry[i++]];
                }

                corner0.z += volumeOffset.z;
            }     
        }        
    }
    m_NumTriangles = indexIndex / 3;
    m_IndexBuffer->Unlock();
    m_VertexBuffer->Unlock();
}

void fp_RenderIsoVolume::OnFrameRender(
        IDirect3DDevice9* d3dDevice,
        double Time,
        float ElapsedTime ) {  
    d3dDevice->SetStreamSource(0, m_VertexBuffer, 0, sizeof(fp_MCVertex));
    d3dDevice->SetIndices(m_IndexBuffer);
    d3dDevice->SetFVF(fp_MCVertex.FVF_Flags);
    d3dDevice->SetMaterial(&m_Material);
    d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    d3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_NumVertices, 0, m_NumTriangles);
    d3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
}

void fp_RenderIsoVolume::OnDetroyDevice() {

}

void fp_RenderIsoVolume::OnLostDevice() {
    if( m_VertexBuffer != NULL )        
        m_VertexBuffer->Release();
    if( m_IndexBuffer != NULL )        
        m_IndexBuffer->Release();
}

int fp_RenderIsoVolume::s_EdgeTable[256] = {
        0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
        0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
        0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
        0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
        0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
        0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
        0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
        0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
        0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
        0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
        0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
        0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
        0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
        0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
        0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
        0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
        0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
        0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
        0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
        0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
        0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
        0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
        0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
        0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
        0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
        0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
        0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
        0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
        0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
        0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
        0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
        0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0 };

int fp_RenderIsoVolume::s_TriTable[256][16] = {
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
        {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
        {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
        {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
        {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
        {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
        {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
        {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
        {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
        {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
        {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
        {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
        {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
        {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
        {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
        {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
        {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
        {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
        {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
        {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
        {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
        {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
        {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
        {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
        {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
        {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
        {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
        {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
        {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
        {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
        {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
        {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
        {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
        {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
        {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
        {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
        {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
        {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
        {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
        {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
        {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
        {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
        {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
        {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
        {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
        {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
        {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
        {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
        {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
        {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
        {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
        {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
        {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
        {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
        {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
        {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
        {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
        {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
        {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
        {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
        {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
        {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
        {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
        {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
        {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
        {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
        {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
        {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
        {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
        {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
        {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
        {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
        {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
        {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
        {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
        {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
        {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
        {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
        {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
        {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
        {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
        {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
        {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
        {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
        {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
        {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
        {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
        {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
        {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
        {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
        {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
        {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
        {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
        {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
        {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
        {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
        {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
        {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
        {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
        {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
        {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
        {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
        {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
        {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
        {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
        {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
        {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
        {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
        {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
        {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
        {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
        {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
        {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
        {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
        {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
        {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
        {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
        {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
        {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
        {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
        {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
        {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
        {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
        {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
        {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
        {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
        {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
        {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
        {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
        {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
        {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
        {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
        {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
        {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
        {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
        {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
        {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
        {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
        {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
        {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
        {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
        {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
        {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
        {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
        {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
        {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
        {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
        {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
        {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
        {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
        {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
        {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
        {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
        {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
        {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
        {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
        {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
        {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
        {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
        {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
        {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
        {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
        {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
        {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
        {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
        {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
        {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
        {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
        {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
        {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
        {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
        {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
        {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
        {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
