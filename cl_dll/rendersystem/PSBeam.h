#ifndef PSBEAM_H
#define PSBEAM_H

//-----------------------------------------------------------------------------
// Beam made of particles
//-----------------------------------------------------------------------------
class CPSBeam : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSBeam"; }// XDM3038b
	CPSBeam(void);
	CPSBeam(uint32 maxParticles, const Vector &origin, const Vector &end, struct model_s *pTexture, int r_mode, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);

private:
	Vector m_vecEnd;
};

#endif // PSBEAM_H
