//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef LIGHTP_H
#define LIGHTP_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32


#define LIGHTP_SPEED1			4000
#define LIGHTP_SPEED2			3200

#define LIGHTP_SPRITE_TRAIL		"sprites/laserbeam.spr"
/*
#define LIGHTP_SPRITE_FLARE		"sprites/lightp_flare.spr"
#define LIGHTP_SPRITE_SPARK		"sprites/lightp_hit.spr"
#define LIGHTP_SPRITE_RING		"sprites/lightp_ring.spr"
*/

class CLightProjectile : public CBaseProjectile
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool IsPushable(void) { return false; }
	virtual int ShouldCollide(CBaseEntity *pOther);// XDM3035

	void EXPORT LightTouch(CBaseEntity *pOther);
	void EXPORT LightThink(void);
	void Explode(int mode);
	//void KillFX(void);

	static CLightProjectile *CreateLP(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, int iType);

protected:
	int m_iTrail;
	/* XDM3038	int m_iFlare;
	int m_iSpark;
	int m_iRing;*/

	unsigned short m_usLightP;
	//unsigned short m_usLightHit;
	//unsigned short m_usLightStart;
};

#endif // LIGHTP_H
