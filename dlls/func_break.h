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
#ifndef FUNC_BREAK_H
#define FUNC_BREAK_H


// func_breakable
#define SF_BREAK_TRIGGER_ONLY	1	// may only be broken by trigger
#define	SF_BREAK_TOUCH			2	// can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE		4	// can be broken by a player standing on it
#define SF_BREAK_FADE_RESPAWN	8	// XDM: fades in gradually when respawned
//
//
//
//
#define SF_BREAK_CROWBAR		256	// instant break if hit with crowbar


#define EXPLOSION_RANDOM	0
#define EXPLOSION_DIRECTED	1

#define	NUM_SHARDS 6 // this many shards spawned when breakable objects break;

class CBreakable : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	// To spark when hit
	virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	// breakables use an overridden takedamage
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);// XDM3038a
	virtual bool IsBSPModel(void) const { return true; }// XDM
	virtual bool IsMovingBSP(void) const { return false; }// XDM
	virtual bool IsBreakable(void) const;
	//virtual bool SparkWhenHit(void);
	virtual bool Explodable(void) const;
	virtual int DamageDecal(const int &bitsDamageType);
	virtual short DamageInteraction(const int &bitsDamageType, const Vector &vecSrc);// XDM3037: pushable too
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	bool IsIgnitable(void);// XDM3037
	void DamageSound(float flVol);

	void EXPORT BreakTouch(CBaseEntity *pOther);
	void EXPORT Die(void);
	void EXPORT ExistThink(void);

public:
	Materials	m_Material;
	int			m_iExplosion;
	int			m_idShard;
	float		m_angle;
	string_t	m_iszGibModel;
	string_t	m_iszSpawnObject;// XDM3038c
	string_t	m_iszWhenHit;// SHL

	static	TYPEDESCRIPTION m_SaveData[];
	static const char *pSpawnObjects[];
};




// func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE		128
#define SF_PUSH_NOPULL			512// XDM

class CPushable : public CBreakable
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int	ObjectCaps(void) { return CBreakable::ObjectCaps() | FCAP_CONTINUOUS_USE; }
	virtual void Touch(CBaseEntity *pOther);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual bool IsBSPModel(void) const { return true; }// XDM
	virtual bool IsMovingBSP(void) const { return true; }// XDM
	virtual bool IsPushable(void) { return true; }// XDM
	virtual float DamageForce(const float &damage);// XDM3038c

	//inline float MaxSpeed(void) { return m_maxSpeed; }
	// breakables use an overridden takedamage
	void PlayMatPushSound(int m_lastSound);
	void StopMatPushSound(int m_lastSound);
	void Move(CBaseEntity *pMover, int push);

public:
	int m_lastSound;// no need to save/restore, just keeps the same sound from playing twice in a row
	//float m_maxSpeed;
	float m_soundTime;
	static TYPEDESCRIPTION m_SaveData[];
};




// XDM: same as CBreakable, but allows studio model
class CBreakableModel : public CBreakable
{
public:
	virtual void Spawn(void);
};


#endif	// FUNC_BREAK_H
