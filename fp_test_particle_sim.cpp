#include "DXUT.h"
#include "fp_test_particle_sim.h"
#include "fp_util.h"

fp_TestSim::fp_TestSim(int NumParticles)
        : m_NumParticles(NumParticles) {
    m_Particles = new fp_Particle[NumParticles];
    
    for( int i = 0; i < m_NumParticles; i++ )
    {
        m_Particles[i].m_Position = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		m_Particles[i].m_Velocity = fp_Util::GetRandomVector() * fp_Util::GetRandomMinMax( 0.5f, 5.0f );
        m_Particles[i].m_Color = D3DCOLOR_COLORVALUE( fp_Util::GetRandomMinMax( 0.0f, 1.0f ), 
                                                       fp_Util::GetRandomMinMax( 0.0f, 1.0f ), 
                                                       fp_Util::GetRandomMinMax( 0.0f, 1.0f ), 
                                                       1.0f );
	}
}

fp_TestSim::~fp_TestSim() {
    delete[] m_Particles;
}

void fp_TestSim::Update(double fTime, float fElapsedTime) {

    float fTimeElapsedSinceLastReset = (float)(fTime - m_LastResetTime);

    if( fTimeElapsedSinceLastReset >= 5.0f )
	{
		for( int i = 0; i < m_NumParticles; i++ )
			m_Particles[i].m_Position = D3DXVECTOR3(0.0f,0.0f,0.0f);

		m_LastResetTime = fTime;
	}
		
	for( int i = 0; i < m_NumParticles; i++ )
        m_Particles[i].m_Position += m_Particles[i].m_Velocity * fElapsedTime;
}
