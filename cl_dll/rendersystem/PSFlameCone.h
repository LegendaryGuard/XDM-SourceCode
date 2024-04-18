#ifndef PSFLAMECONE_H
#define PSFLAMECONE_H

//-----------------------------------------------------------------------------
// Creates a fountain-like spray of particles, can be directed or spheric
// Legacy system
//-----------------------------------------------------------------------------
class CPSFlameCone : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSFlameCone"; }// XDM3038b
	CPSFlameCone(void);
	CPSFlameCone(uint32 maxParticles, const Vector &origin, const Vector &direction, const Vector &spread, float velocity, struct model_s *pTexture, int r_mode, float a, float adelta, float scale, float scaledelta, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	//virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
	//virtual bool Update(const float &time, const double &elapsedTime);
};

#endif // PSFLAMECONE_H
