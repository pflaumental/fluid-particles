#pragma once
#ifndef FP_TEST_PARTICLE_SIM_H
#define FP_TEST_PARTICLE_SIM_H

#include "DXUT.h"

struct fp_Particle {
    D3DXVECTOR3 m_Position;
    D3DXVECTOR3 m_Velocity;
	D3DCOLOR    m_Color;
};

class fp_TestSim {
public:
    fp_Particle* m_Particles;
    const int m_NumParticles;

    fp_TestSim(int NumParticles);
    ~fp_TestSim();
    void Update(double fTime, float fElapsedTime);
private:
    double m_LastResetTime;
};

#endif
