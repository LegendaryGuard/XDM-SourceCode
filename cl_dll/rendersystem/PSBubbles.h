#ifndef PSBUBBLES_H
#define PSBUBBLES_H

#define BUBLES_DEFAULT_DENSITY		8// one bubble every N units

//-----------------------------------------------------------------------------
// Bubbles
//-----------------------------------------------------------------------------
class CPSBubbles : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSBubbles"; }// XDM3038b
	CPSBubbles(void);
	CPSBubbles(uint32 maxParticles, uint32 type, const Vector &vector1, const Vector &vector2, float velocity, struct model_s *pTexture, int r_mode, float a, float adelta, float scale, float scaledelta, float timetolive);
	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
};

#endif // PSBUBBLES_H
