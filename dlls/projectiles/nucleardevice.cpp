//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"
#include "skill.h"


#define NDD_POWERUP_TIME	10.0
#define NDD_PITCH_DELTA		5


class CNuclearDevice : public CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	//virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual int ObjectCaps(void) {return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;}
	virtual void Explode(void);
	void EXPORT NDDThink(void);// don't use just Think() - this cannot be disabled
	void EXPORT NDDTouch(CBaseEntity *pOther);// same thing
};

LINK_ENTITY_TO_CLASS(nucdevice, CNuclearDevice);


void CNuclearDevice::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;//DAMAGE_YES;
	pev->sequence = 1;
	pev->scale = UTIL_GetWeaponWorldScale();
	pev->friction = 0.8f;
	pev->health = 20;

	//SET_MODEL(edict(), "models/p_egon.mdl");
	//UTIL_SetSize(this, Vector(-4, -4, 0), Vector(4, 4, 4));
	//UTIL_SetOrigin(this, pev->origin);
	pev->mins.Set(-4,-4,-4);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(4,4,4);
	CBaseProjectile::Spawn();// XDM3037

	SetThink(&CNuclearDevice::NDDThink);
	SetTouch(&CNuclearDevice::NDDTouch);
	pev->button = PITCH_NORM;
	if (pev->dmg == 0)// XDM3038c
		pev->dmg = gSkillData.DmgNuclear;

	pev->dmgtime = gpGlobals->time + NDD_POWERUP_TIME;
	SetNextThink(0.1);
	SetBits(pev->spawnflags, SF_NORESPAWN);// never!
	pev->renderfx = kRenderFxGlowShell;
	pev->renderamt = 127;
	pev->rendercolor.Set(127,127,127);
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/egon_startrun1.wav", VOL_NORM, ATTN_NORM, 0, pev->button);
}

void CNuclearDevice::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/p_egon.mdl");// XDM3037

	CBaseProjectile::Precache();// skip CGrenade
	PRECACHE_SOUND("weapons/egon_startrun1.wav");
	PRECACHE_SOUND("weapons/mine_beep.wav");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
}

void CNuclearDevice::NDDThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}
	if (pev->dmgtime <= gpGlobals->time)
	{
		Explode();
	}
	else
	{
		if (pev->rendercolor.x < 255)
			pev->rendercolor.x += 5;
		if (pev->renderamt < 255)
			pev->renderamt += 5;

		if (sv_overdrive.value > 0.0f && (pev->dmgtime - gpGlobals->time <= 1.0f))
			pev->renderfx = kRenderFxExplode;

		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
		Vector end = pev->origin + UTIL_RandomBloodVector()*256.0f;
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_LIGHTNING);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(end.x);
			WRITE_COORD(end.y);
			WRITE_COORD(end.z);
			//WRITE_BYTE((pev->dmgtime - gpGlobals->time)*10.0f);// life
			WRITE_BYTE(10.0f);// life
			WRITE_BYTE(20);// width
			WRITE_BYTE(200);// amp
			WRITE_SHORT(g_iModelIndexLaser);
		MESSAGE_END();
		}
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/egon_startrun1.wav", VOL_NORM, 0.5f, SND_CHANGE_PITCH, pev->button);
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/mine_beep.wav", VOL_NORM, ATTN_NORM, 0, pev->button);
		pev->button += NDD_PITCH_DELTA;
		SetBits(pev->effects, EF_MUZZLEFLASH);
		SetNextThink(0.5);// XDM3038a
	}
}

void CNuclearDevice::NDDTouch(CBaseEntity *pOther)
{
	if (pOther->IsBSPModel()/* && pev->flags & FL_ONGROUND*/)
	{
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/mine_deploy.wav", VOL_NORM, ATTN_NORM, 0, PITCH_LOW);
		AlignToFloor();//pev->angles.Clear();
		SetTouchNull();
	}
}

void CNuclearDevice::Explode(void)
{
	STOP_SOUND(edict(), CHAN_WEAPON, "weapons/egon_startrun1.wav");
	pev->effects = EF_NODRAW|EF_BRIGHTLIGHT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;// XDM3038c
	if (FBitSet(pev->flags, FL_ONGROUND))
		pev->origin.z += 1;// XDM3038a: HACK

	SetTouchNull();
	SetThink(&CGrenade::NuclearExplodeThink);
	SetNextThink(0.5);// XDM3038a
}

void CNuclearDevice::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Explode();
	/*if (!pActivator->IsPlayer())
		return;

	if (pActivator->edict() == pev->owner)
	{
		CBaseEntity *pEnt = Create("weapon_egon", pev->origin, pev->angles, pev->velocity, NULL);
		pEnt->pev->spawnflags |= SF_NORESPAWN;
		Destroy();
	}*/
}
