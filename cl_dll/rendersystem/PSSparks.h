#ifndef PSSPARKS_H
#define PSSPARKS_H

//-----------------------------------------------------------------------------
// Set of particles, emitting from a point
// Legacy system
//-----------------------------------------------------------------------------
class CPSSparks : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSSparks"; }// XDM3038b
	CPSSparks(void);
	CPSSparks(uint32 maxParticles, const Vector &origin, float scalex, float scaley, float scaledelta, float velocity, float startenergy, byte r, byte g, byte b, float a, float adelta, struct model_s *pTexture, int r_mode, float timetolive);
	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
};

#endif // PSSPARKS_H
