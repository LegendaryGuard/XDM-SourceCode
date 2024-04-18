//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef TELEPORTER_H
#define TELEPORTER_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32


#define TELEPORTER_RADIUS			3// 4 is too huge! Crouching players will never get a shot!

#define TELEPORTER_FLY_SPEED1		400
#define TELEPORTER_FLY_SPEED2		320

#define TELEPORTER_LIFE				5.0f

#define TELEPORTER_EFFECT_RADIUS1	64.0f
#define TELEPORTER_EFFECT_RADIUS2	48.0f
#define TELEPORTER_SEARCH_RADIUS	400.0f
#define TELEPORTER_BLAST_RADIUS		256.0f

#define TELEPORTER_SPRITE_BALL		"sprites/exit1.spr"
#define TELEPORTER_SPRITE_FLASHBALL	"sprites/animspark.spr"
#define TELEPORTER_SPRITE_FLASHBEAM	"sprites/flash.spr"


class CTeleporter : public CBaseProjectile
{
public:
	static CTeleporter *CreateTeleporter(const Vector &vecOrigin, const Vector &vecVelocity, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, CBaseEntity *pEnemy, float fDamage, float fLife, int iType);
	virtual int Restore(CRestore &restore);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual bool IsPushable(void) {return FALSE;}
	//virtual void TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	//virtual int ObjectCaps(void) { return FCAP_ACROSS_TRANSITION | FCAP_MUST_SPAWN; }
	virtual int ShouldCollide(CBaseEntity *pOther);// XDM3035

	//void EXPORT CollideThink(void);
	void EXPORT TeleportThink(void);
	void EXPORT TeleportTouch(CBaseEntity *pOther);
	void MovetoTarget(const Vector &vecTarget);
	void Blast(short type);

	int m_iFlash;// don't save model indexes!

private:
	unsigned short m_usTeleporter;
	unsigned short m_usTeleporterHit;
};

/*class CTeleporterSpiral : public CBaseEntity
{
	virtual void Spawn(void);
	virtual int ObjectCaps(void) {return FCAP_DONT_SAVE;}
	void EXPORT InitThink(void);
	void EXPORT SpiralThink(void);
	void EXPORT SpiralTouch(CBaseEntity *pOther);
};*/

#endif // TELEPORTER_H
