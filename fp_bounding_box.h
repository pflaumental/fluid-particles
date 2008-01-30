#pragma once
#ifndef FP_BOUNDING_BOX_H
#define FP_BOUNDING_BOX_H

#include "DXUT.h"


class fp_BoundingBox {
public:
	fp_BoundingBox(D3DXVECTOR2 vDimX = D3DXVECTOR2(-1,1),  D3DXVECTOR2 vDimY = D3DXVECTOR2(-1,1),  D3DXVECTOR2 dimZ = D3DXVECTOR2(-1,1));
	virtual ~fp_BoundingBox();

	HRESULT CreateDevice( ID3D10Device* pd3dDevice, ID3D10EffectTechnique* pTechniqueRender);
	void	FrameRender( ID3D10Device* pd3dDevice, bool bDoSetup=true);
	void	FrameRenderSolid( ID3D10Device* pd3dDevice, bool bDoSetup=true);
	void    DestroyDevice();

	void SetLayout(ID3D10Device* pd3dDevice, ID3D10Buffer* pVB);

	D3DXVECTOR3 PosMin();
	D3DXVECTOR3 PosMax();
	D3DXVECTOR3 Size();
	D3DXVECTOR3 Center();

	D3DXVECTOR2 m_vDimX;
	D3DXVECTOR2 m_vDimY;
	D3DXVECTOR2 m_vDimZ;

protected:
	ID3D10Buffer*				m_pVBBoundingBox;
	ID3D10Buffer*				m_pVBBoundingBoxInd;
	ID3D10Buffer*				m_pVBBoundingBoxSolidInd;
	ID3D10InputLayout*			m_pVertexLayout;
};

#endif
