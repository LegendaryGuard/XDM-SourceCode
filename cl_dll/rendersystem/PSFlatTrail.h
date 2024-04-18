#ifndef PSFLATTRAIL_H
#define PSFLATTRAIL_H

//-----------------------------------------------------------------------------
// Trail made of world-oriented (non-rotating) particles
//-----------------------------------------------------------------------------
class CPSFlatTrail : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSFlatTrail"; }// XDM3038b
	CPSFlatTrail(void);
	CPSFlatTrail(const Vector &start, const Vector &end, struct model_s *pTexture, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float dist_delta, float timetolive);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
	virtual void Render(void);

protected:
	Vector m_vDelta;
};

#endif // PSFLATTRAIL_H
