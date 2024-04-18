//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "globals.h"
#include "game.h"


LINK_ENTITY_TO_CLASS(l_grenade, CLGrenade);


CLGrenade *CLGrenade::CreateGrenade(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, float fTime, bool contact)
{
	CLGrenade *pNew = GetClassPtr((CLGrenade *)NULL, "l_grenade");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;
		pNew->pev->velocity = vecVel;
		pNew->pev->skin = contact?0:1;
		pNew->pev->dmg = fDamage;// XDM3038c
		pNew->pev->dmgtime = gpGlobals->time + fTime;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		if (contact)
		{
			pNew->pev->skin = 0;
			pNew->SetThink(&CGrenade::DangerSoundThink);
			pNew->SetTouch(&CLGrenade::ExplodeTouch);
		}
		else
		{
			pNew->pev->skin = 1;
			pNew->SetThink(&CLGrenade::TimeThink);
			pNew->SetTouch(&CGrenade::BounceTouch);
		}
	}
	return pNew;
}

/*CLGrenade *CLGrenade::ShootTimed(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float time)
{
	CLGrenade *pNew = GetClassPtr((CLGrenade *)NULL, "l_grenade");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;
		pNew->pev->velocity = vecVel;
		pNew->pev->skin = 1;
		pNew->pev->dmgtime = gpGlobals->time + time;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		pNew->SetThink(&CLGrenade::TimeThink);
		pNew->SetTouch(&CGrenade::BounceTouch);
	}
	return pNew;
}*/

void CLGrenade::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_SLIDEBOX;
	//pev->renderfx = kRenderFxGlowShell;
	pev->gravity *= 0.5;// XDM3038
	pev->friction *= 0.8;// XDM3038
	pev->scale = 1.0f;
	//pev->framerate = 1.0;
	//pev->impulse = 1;
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgGrenadeLauncher;

	pev->avelocity.x = -80.0;

	pev->mins.Set(-2,-2,-2);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set( 2, 2, 2);
	CBaseProjectile::Spawn();// starts animation

	m_fRegisteredSound = FALSE;
	//if (pev->waterlevel < 3)
		PLAYBACK_EVENT_FULL(0, edict(), g_usTrail, 0, pev->origin, pev->angles, 16, 0, entindex(), (pev->skin == 0)?1:0, 0, 0);

	SetNextThink(0);
}

void CLGrenade::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/lgrenade.mdl");// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	PRECACHE_SOUND("weapons/lgrenade_hit1.wav");
	PRECACHE_SOUND("weapons/lgrenade_hit2.wav");
}

void CLGrenade::TimeThink(void)
{
	if (!IsInWorld())
	{
		Destroy();
		return;
	}

	StudioFrameAdvance();
	SetNextThink(0.1);// XDM3038a
	if (pev->avelocity.x < 0.0f)
		pev->avelocity.x += 0.2f;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		pev->sequence = 1;
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
	}

	if (pev->dmgtime <= gpGlobals->time)
		Detonate();

	if (pev->waterlevel != WATERLEVEL_NONE)
	{
		pev->velocity *= 0.5f;
		pev->framerate = 0.2f;
	}
}

void CLGrenade::BounceSound(void)
{
	switch (RANDOM_LONG(0,1))
	{
	case 0:	EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/lgrenade_hit1.wav", RANDOM_FLOAT(0.75, 1.0), ATTN_STATIC, 0, RANDOM_LONG(95,105)); break;
	case 1:	EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/lgrenade_hit2.wav", RANDOM_FLOAT(0.75, 1.0), ATTN_STATIC, 0, RANDOM_LONG(95,105)); break;
	}
}

int CLGrenade::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	Detonate();
	return 1;
}

void CLGrenade::ExplodeTouch(CBaseEntity *pOther)
{
	//FIXME: Grenade may touch a brush inside sky!!
	//	if (POINT_CONTENTS(pev->origin) == CONTENTS_SKY)
	//		return;// touched the sky
	//	if (pOther == m_hOwner && m_fLaunchTime < (gpGlobals->time + 0.25f))// XDM3037
	//		return;

	Detonate();
}

void CLGrenade::Detonate(void)
{
	TraceResult tr;
	Vector v(0,0,0);
	if (pev->velocity.IsZero())// XDM3035: otherwise tr.fAllSolid and the grenade won't detonate (test on CTF_ProjectX2 conveyor)
		v.z = -1.0f;
	else
		v = pev->velocity.Normalize()*16.0f;

	UTIL_TraceLine(pev->origin - v, pev->origin + v, ignore_monsters, edict(), &tr);
	if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (pev->dmg * GREN_DAMAGE_TO_RADIUS * 0.1f));

	//bool bSubgrenades = pev->impulse > 0;
	//tr.vecPlaneNormal.x *= -1.0f;
	//tr.vecPlaneNormal.y *= -1.0f;
	//SQB?VectorAngles(tr.vecPlaneNormal, pev->angles);// XDM3037a: explosion code calculates decal direction
	Explode(pev->origin, DMG_BLAST, g_iModelIndexBigExplo1, 24, g_iModelIndexWExplosion, 24, 1.5f);

	if (pev->skin == 1 && sv_overdrive.value > 0.0f)
	{
		for (short i=0; i<3; ++i)// create grenades after Explode() so they won't be damaged
			CARGrenade::CreateGrenade(pev->origin + UTIL_RandomBloodVector()*3.0f, pev->angles, (tr.vecPlaneNormal + RandomVector(VECTOR_CONE_45DEGREES)) * RANDOM_LONG(300, 400), m_hOwner, this, 0.0f);
	}
}
