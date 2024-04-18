#ifndef HORNET_H
#define HORNET_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32


#define HORNET_LIFE			4.0f
#define HORNET_DETECT_DIST	512
#define HORNET_VELOCITY1	300
#define HORNET_VELOCITY2	1200

class CHornet : public CBaseMonster// CBaseProjectile
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	//virtual int Classify(void);
	virtual int IRelationship (CBaseEntity *pTarget);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int	ObjectCaps(void) { return FCAP_MUST_SPAWN; }// XDM
	virtual bool IsProjectile(void) const { return true; }
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual int ShouldCollide(CBaseEntity *pOther);// XDM3035
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// XDM3038a
	virtual bool ShouldGibMonster(int iGib) const { return false; }// XDM3038c

	void IgniteTrail(void);
	void EXPORT StartTrack(void);
	void EXPORT StartDart(void);
	void EXPORT TrackTarget(void);
	void EXPORT TrackTouch(CBaseEntity *pOther);
	void EXPORT DartTouch(CBaseEntity *pOther);
	void EXPORT DieTouch(CBaseEntity *pOther);

	static CHornet *CreateHornet(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, bool track_target);

protected:
	int m_iHornetTrail;
	/*int m_iHornetPuff;
	int m_iHornetPuffG;// XDM
	int m_iHornetPuffB;
	int m_iHornetPuffY;
	int m_iHornetPuffR;*/
	unsigned short m_usHornet;
};

#endif // HORNET_H
