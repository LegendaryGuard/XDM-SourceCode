//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef PROJECTILE_H
#define PROJECTILE_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

// When a player shoots grenade towards his feet, let him get away during this period
#define PROJECTILE_IGNORE_OWNER_TIME	0.6f

// Base class for all projectiles
// Has the ability to do proper collisions even with stupid HL/Quake bugs and restrictions
class CBaseProjectile : public CBaseAnimating
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual int Classify(void) { return CLASS_GRENADE; }
	virtual bool IsProjectile(void) const { return true; }
	virtual bool IsPushable(void) { return true; }
	virtual bool ShouldRespawn(void) const { return false; }
	virtual int	BloodColor(void) { return DONT_BLEED; }
	virtual void EXPORT Think(void);
	virtual void EXPORT Touch(CBaseEntity *pOther);
	virtual void EXPORT Blocked(CBaseEntity *pOther);
	virtual void SetObjectCollisionBox(void);
	virtual int ShouldCollide(CBaseEntity *pOther);
	//virtual CBaseEntity *GetDamageAttacker(void);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	//TODO?	Can't simply implement it in CBaseEntity because of weapons code virtual void SetOwner(CBaseEntity *pEntity);
	void SetIgnoreEnt(CBaseEntity *pEntity);

protected:
	float m_fLaunchTime;
	float m_fEntIgnoreTimeInterval;// time interval in seconds
	CBaseEntity *m_pLastTouched;

	static TYPEDESCRIPTION m_SaveData[];
};

#endif // PROJECTILE_H
