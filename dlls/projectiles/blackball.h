//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef BLACKBALL_H
#define BLACKBALL_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

#define BLACKBALL_RADIUS			3// projectile size. 4 is too huge! Crouching players will never get a shot!

#define BLACKBALL_FLY_SPEED			320.0f
#define BLACKBALL_MAX_LIFE			5.0f
#define BLACKBALL_MAX_RADIUS		1024.0f
#define BLACKBALL_DAMAGE_TO_RADIUS	10.0f

#define BLACKBALL_SPRITE			"sprites/black_ball.spr"

class CBlackBall : public CBaseProjectile
{
public:
	static CBlackBall *CreateBlackBall(CBaseEntity *pOwner, CBaseEntity *pEntIgnore, const Vector &vecSrc, const Vector &vecDir, float fSpeed, float fLife, float fScale, float fDamage);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool IsPushable(void) { return false; }
	//virtual int ObjectCaps(void) { return FCAP_ACROSS_TRANSITION | FCAP_MUST_SPAWN; }
	virtual int ShouldCollide(CBaseEntity *pOther);
	void Detonate(void);
	void EXPORT BlackHoleThink(void);
	void EXPORT BBThink(void);
	void EXPORT BBTouch(CBaseEntity *pOther);

private:
	unsigned short m_usBlackBall;
};


#endif // BLACKBALL_H
