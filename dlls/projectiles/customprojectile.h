//====================================================================
// X-Half-Life
// Copyright (c) 2001-2017
//
// Purpose: Custom projectile for mappers
//
//====================================================================
#ifndef CUSTOMPROJECTILE_H
#define CUSTOMPROJECTILE_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32


#define SF_ENVPROJECTILE_UNDERWATER		0x0001
#define SF_ENVPROJECTILE_LIGHT			0x0002
#define SF_ENVPROJECTILE_SILENT			0x0004


class CEnvProjectile : public CBaseProjectile
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual bool IsProjectile(void) const;
	virtual int ShouldCollide(CBaseEntity *pOther);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	void EXPORT FlyThink(void);
	void Explode(int iSpr);
	void KillFX(void);
//	bool IsWorldReference(void);

	static TYPEDESCRIPTION m_SaveData[];
// need to be saved, entvar used instead	bool m_bEnabled;
protected:
	string_t m_iszSpriteTrail;
	string_t m_iszSpriteHitTarget;
	string_t m_iszSpriteHitWorld;
	float m_fLifeTime;

	int m_iSpriteTrail;
	int m_iSpriteHitWorld;
	int m_iSpriteHitTarget;
	int m_iDecal;
//	unsigned short m_usProjectileStart;
//	unsigned short m_usProjectileHit;
};

// Creates a new projectile as a copy of specified entity
CEnvProjectile *CreateProjectile(const char *targetname, const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore);

#endif // CUSTOMPROJECTILE_H
