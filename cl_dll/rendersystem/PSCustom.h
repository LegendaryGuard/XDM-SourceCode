#ifndef PSCUSTOM_H
#define PSCUSTOM_H

class CParticle;
class CPSCustom;

//#define PARTICLETYPE_NAME_MAX					32
#define WATERCIRCLE_SIZE_DELTA					96.0

#define PARTICLE_FWATERCIRCLE					PARTICLE_FLAG1
#define PARTICLE_FHITSURFACE					PARTICLE_FLAG2

// These flags indicate that some values were defined by the user
#define PSCUSTOM_DEF_COLORMAX					(1 << 0)
#define PSCUSTOM_DEF_RADIUSRANGE				(1 << 1)
#define PSCUSTOM_DEF_BOXRANGE					(1 << 2)

// These flags define when a particle should not be drawn parallel to screen surface
#define PSCUSTOM_PFLAG_ORIENTED_START			(1 << 0)
#define PSCUSTOM_PFLAG_ORIENTED_HIT_SURFACE		(1 << 1)
#define PSCUSTOM_PFLAG_ORIENTED_HIT_LIQUID		(1 << 2)
#define PSCUSTOM_PFLAG_LOOPFRAMES_HIT_SURFACE	(1 << 3)
#define PSCUSTOM_PFLAG_LOOPFRAMES_HIT_LIQUID	(1 << 4)
#define PSCUSTOM_PFLAG_HIT_FADEOUT				(1 << 5)

//-----------------------------------------------------------------------------
// Custom system
// Can be initialized by ParseKeyValue (from a file) or by setting values directly
//-----------------------------------------------------------------------------
class CPSCustom : public CParticleSystem
{
	typedef CParticleSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CPSCustom"; }// XDM3038b
	CPSCustom(void);
	CPSCustom(uint32 maxParticles, const Vector &origin, int emitterindex, int attachment, struct model_s *pTexture, int rendermode, float timetolive);
	virtual ~CPSCustom(void);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void OnSystemRemoved(const CRenderSystem *pSystem);// XDM3038c

	int m_iEmitterIndex;// index of my env_entity
	uint32 m_iParticleFlags;
	::Color m_ColorMax;
	Vector m_vAccelMin;
	Vector m_vAccelMax;
	Vector m_vSinVel;
	Vector m_vCosVel;
	Vector m_vSinAccel;
	Vector m_vCosAccel;
	//char m_szName[PARTSYSTEM_NAME_MAX];
	uint32 m_iDefinedFlags;// little hack
	model_t *m_pTextureHitLiquid;
	model_t *m_pTextureHitSurface;
	//short m_StartInsideEntity;
	CPSCustom *m_pSystemOnHitLiquid;// this system will be used to InitializeParticle() when it hits liquid
	CPSCustom *m_pSystemOnHitSurface;
};

#endif // PSCUSTOM_H
