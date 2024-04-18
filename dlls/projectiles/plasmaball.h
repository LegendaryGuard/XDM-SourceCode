//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef PLASMABALL_H
#define PLASMABALL_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32


#define PLASMABALL_SPEED		1200

#define PLASMABALL_SPRITE		"sprites/pball.spr"
#define PLASMABALL_SPRITE_HIT	"sprites/pball_hit.spr"


class CPlasmaBall : public CBaseProjectile
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool IsProjectile(void) const { return true; }
	virtual bool IsPushable(void)		{ return FALSE; }
	void EXPORT PBTouch(CBaseEntity *pOther);
	void EXPORT PBThink(void);
	void Explode(int mode);

	static CPlasmaBall *CreatePB(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, float fSpeed, float fDamage, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, int iType);

protected:
	//int m_iSpriteHit;
	unsigned short m_usHit;
};

#endif // PLASMABALL_H
