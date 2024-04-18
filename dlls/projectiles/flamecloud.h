//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef	FLAMECLOUD_H
#define	FLAMECLOUD_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

#define	FLAMECLOUD_UPDATETIME	0.05f// 20 FPS
#define	FLAMECLOUD_SCALE		0.1f
#define	FLAMECLOUD_SCALEADD		0.05f
#define	FLAMECLOUD_ADELTA		20
#define	FLAMECLOUD_VELMULT		16.0f// per second! == 20FPS*0.8 // 0.8f every 0.05 seconds

class CFlameCloud : public CBaseProjectile
{
public:
	virtual void Spawn(void);
	//virtual void Spawn(byte restore);
	virtual void Precache(void);
	virtual bool IsPushable(void) { return true; }// XDM3035c
	//virtual int ObjectCaps(void) { return FCAP_ACROSS_TRANSITION | FCAP_DONT_SAVE; } // FCAP_MUST_SPAWN; }
	virtual int ShouldCollide(CBaseEntity *pOther);// XDM3035
	void EXPORT FlameThink(void);
	void EXPORT FlameTouch(CBaseEntity *pOther);

	static CFlameCloud *CreateFlame(const Vector &vecOrigin, const Vector &vecVelocity, int &spr_idx, float scale, float scale_add, float velmult, float damage, int brightness, int br_delta, int effects, CBaseEntity *pOwner, CBaseEntity *pEntIgnore);
};

#endif // FLAMECLOUD_H
