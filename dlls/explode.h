/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef EXPLODE_H
#define EXPLODE_H

#define	SF_ENVEXPLOSION_NODAMAGE	SF_NODAMAGE// XDM3034

// Spark Shower
class CShower : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual int ObjectCaps(void) {return FCAP_DONT_SAVE;}
};


class CEnvExplosion : public CGrenade//CBaseMonster
{
public:
	static CEnvExplosion *CreateExplosion(const Vector &origin, const Vector &angles, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, int magnitude, int flags);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool IsPushable(void) { return false; }// XDM
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual bool IsProjectile(void) const { return false; }// XDM3038: WARNING: even though our parent class IS a projectile, we don't want this to be treated like one!
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	int m_iMagnitude;// how large is the fireball? how much damage?
	float m_fSpriteScale;// what's the exact fireball sprite scale? 
//protected:
//	int m_iCustomSprite;// XDM
};

CEnvExplosion *ExplosionCreate(const Vector &center, const Vector &angles, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, int magnitude, int flags, float delay);

#endif			//EXPLODE_H
