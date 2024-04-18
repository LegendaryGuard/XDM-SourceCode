#ifndef	SQKGRENADE_H
#define	SQKGRENADE_H
#ifdef _WIN32
#ifndef __MINGW32__
#pragma once
#endif // !__MINGW32__
#endif // _WIN32


#define SQUEAK_DETONATE_DELAY	15.0
#define SQUEAK_LOOK_RADIUS		512

class CSqueakGrenade : public CBaseMonster//CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int Classify(void);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);// XDM3037a
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual bool GibMonster(void);
	//CGrenade:	virtual	bool IsMonster(void) const { return true; }// XDM: TESTME
	virtual bool IsProjectile(void) const { return true; }
	virtual bool IsPushable(void) { return true; }
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035c: always a temporary monster
	virtual int IRelationship(CBaseEntity *pTarget);

	void EXPORT SuperBounceTouch(CBaseEntity *pOther);
	void EXPORT HuntThink(void);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	float m_flNextBounceSoundTime;
	Vector m_posPrev;
};

// IMPORTANT: these must match in p_ and v_ models!
#define SQUEAKBOX_RELEASE_DELAY		3.0
#define SQUEAKBOX_NUM_SQUEAKS		AMMO_SNARKBOX_GIVE

enum psqueak_bodygroups_e
{
	PSQUEAK_BODYGROUP_BOX = 0,
	PSQUEAK_BODYGROUP_BODY,
	PSQUEAK_BODYGROUP_HANDS
};

enum psqueak_body_box_e
{
	PSQUEAK_BODY_BOX_ON = 0,
	PSQUEAK_BODY_BOX_OFF
};

enum psqueak_body_body_e
{
	PSQUEAK_BODY_BODY_OFF = 0,
	PSQUEAK_BODY_BODY_ON
};

enum panim_squeak_e
{
	SQUEAK_BOX_IDLE = 0,
	SQUEAK_BOX_STAYPUT,
	SQUEAK_BOX_OPEN,
	SQUEAK_BOX_OPENED,
	SQUEAK_HAND_IDLE
};

class CSqueakBox : public CBaseAnimating//CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Open(void);
	void EXPORT SqueakBoxThink(void);
	virtual bool IsProjectile(void) const { return true; }
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

#endif // SQKGRENADE_H
