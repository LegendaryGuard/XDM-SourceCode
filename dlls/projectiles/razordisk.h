#ifndef RAZORDISK_H
#define RAZORDISK_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

#define RAZORDISK_AIR_VELOCITY		1920
#define RAZORDISK_WATER_VELOCITY	1600
//#define RAZORDISK_MAX_BOUNCES		12
#define RAZORDISK_LIFE_TIME			10.0f

class CRazorDisk : public CBaseProjectile
{
public:
	static CRazorDisk *DiskCreate(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, int mode);

	virtual void Spawn(void);
	virtual void Precache(void);

	void EXPORT DiskThink(void);
	void EXPORT DiskTouch(CBaseEntity *pOther);

protected:
	unsigned short m_usDiskHit;
};

#endif // RAZORDISK_H
