#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

class CParticle;

#include "pm_defs.h"

/* const.h
enum
{
	PSSTARTTYPE_POINT = 0,
	PSSTARTTYPE_SPHERE,
	PSSTARTTYPE_BOX,
	PSSTARTTYPE_LINE,
	PSSTARTTYPE_ENTITYBBOX
};

enum psmovtype_e
{
	PSMOVTYPE_DIRECTED = 0,
	PSMOVTYPE_OUTWARDS,// explosion
	PSMOVTYPE_INWARDS,// implosion
	PSMOVTYPE_RANDOM
};*/

//-----------------------------------------------------------------------------
// Base particle system, with spawn point and direction (*velocity)
//-----------------------------------------------------------------------------
class CParticleSystem : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CParticleSystem"; }// XDM3038b
	CParticleSystem(void);
	CParticleSystem(uint32 maxParticles, const Vector &origin, const Vector &direction, struct model_s *pTexture, int r_mode, float timetolive);
	virtual ~CParticleSystem(void);

	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual void InitializeSystem(void);
	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	virtual void InitializeParticle(CParticle *pParticle, const float &fInterpolaitonMult);
	virtual void ApplyForce(const Vector &origin, const Vector &force, float radius, bool point);
	virtual bool CheckFxLevel(void) const;
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);
	virtual void FollowUpdatePosition(void);

	int m_iTraceFlags;// XDM3037: internal Trace() will use this
	uint32 m_iStartType;// XDM3038b
	uint32 m_iMovementType;// XDM3038b
	uint32 m_iEmitRate;// XDM3038c: PPS
	float m_fStartRadiusMin;// XDM3038c
	float m_fStartRadiusMax;// XDM3038c
	float m_fParticleScaleMin;// XDM3038c
	float m_fParticleScaleMax;
	float m_fParticleSpeedMin;// XDM3038b
	float m_fParticleSpeedMax;
	float m_fParticleWeight;// XDM3038c
	float m_fParticleCollisionSize;// XDM3038c
	float m_fParticleLife;// XDM3038c: set by user
	Vector m_vDirection;// must be normalized (length == 1)
	Vector m_vSpread;// XDM3038b
	Vector m_vDestination;// XDM3038b
	Vector m_vStartMins;// XDM3038c: relative to system origin!
	Vector m_vStartMaxs;// XDM3038c: relative to system origin!

	bool (*m_OnParticleInitialize)(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &fInterpolaitonMult);// will be called for each particle when it's initialized (return true to do standard init processing)
	void *m_pOnParticleInitializeData;// will be passed as pData argument into the callback function

	bool (*m_OnParticleUpdate)(CParticleSystem *pSystem, CParticle *pParticle, void *pData, const float &time, const double &elapsedTime);// will be called for each particle when it's updated (return true to do standard update processing)
	void *m_pOnParticleUpdateData;// will be passed as pData argument into the callback function

	bool (*m_OnParticleCollide)(CParticleSystem *pSystem, CParticle *pParticle, void *pData, pmtrace_s *pTrace);// will be called for each particle when it collides with something (return true to do standard collision processing)
	void *m_pOnParticleCollideData;// will be passed as pData argument into the callback function

	// Combined for padding minimizaiton
	bool m_bOnParticleInitializeDataFree;// _delete_ (free) data pointer when system is deleted. DANGEROUS!!!
	bool m_bOnParticleUpdateDataFree;// _delete_ (free) data pointer when system is deleted. DANGEROUS!!!
	bool m_bOnParticleCollideDataFree;// _delete_ (free) data pointer when system is deleted. DANGEROUS!!!

protected:
	virtual bool AllocateParticles(void);
	virtual CParticle *AllocateParticle(uint32 iStartIndex = 0);// XDM3038: made virtual. Potentially dangerous code!
	virtual void RemoveParticle(CParticle *pParticle);// XDM3038: made virtual. Potentially dangerous code!
	uint32 Emit(void);

	float m_fEnergyStart;// start value for particles
	float m_fAvgParticleLife;// internal value for calculations
	uint32 m_iMaxParticles;// maximum number of particles
	uint32 m_iNumParticles;// number of currently active particles
	float m_fAccumulatedEmit;// number of particles left uninitialized, must accumulate fractions across frames

	CParticle *m_pParticleList;
};

#endif // PARTICLESYSTEM_H
