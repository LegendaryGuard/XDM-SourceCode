#ifndef PSSPIRALEFFECT_H
#define PSSPIRALEFFECT_H

//-----------------------------------------------------------------------------
// Sequential spiral of particles
//-----------------------------------------------------------------------------
class CPSSpiralEffect : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSSpiralEffect"; }// XDM3038b
	CPSSpiralEffect(void);
	CPSSpiralEffect(uint32 maxParticles, const Vector &origin, float radius, float radiusdelta, model_t *pSprite, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive);
	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);
};

#endif // PSSPIRALEFFECT_H
