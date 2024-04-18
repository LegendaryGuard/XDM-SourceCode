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
#include "gamerules.h"
#include "decals.h"
#include "blackball.h"
#include "game.h"

LINK_ENTITY_TO_CLASS(blackball, CBlackBall);

CBlackBall *CBlackBall::CreateBlackBall(CBaseEntity *pOwner, CBaseEntity *pEntIgnore, const Vector &vecSrc, const Vector &vecDir, float fSpeed, float fLife, float fScale, float fDamage)
{
	//DBG_PRINTF("CBlackBall::CreateBlackBall(%d, %d, src, dir, spd %g, life %g, scale %g, dmg %g)\n", pOwner?pOwner->entindex():0, pEntIgnore?pEntIgnore->entindex():0, fSpeed, fLife, fScale, fDamage);
	CBlackBall *pNew = GetClassPtr((CBlackBall *)NULL, "blackball");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->movedir = vecDir;
		pNew->pev->speed = fSpeed;
		pNew->pev->dmg = fDamage;
		pNew->pev->scale = fScale;
		pNew->pev->fuser2 = fLife;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;
		}
		pNew->SetIgnoreEnt(pEntIgnore);
		pNew->Spawn();
		//pNew->pev->basevelocity = pNew->pev->velocity = vecDir * fSpeed;
	}
	return pNew;
}

int CBlackBall::Restore(CRestore &restore)
{
	int status = CBaseProjectile::Restore(restore);
	/* uncomment if any savedata is added
	if (status == 0)
		return 0;

	status = restore.ReadFields("CBlackBall", this, m_SaveData, ARRAYSIZE(m_SaveData));*/
	if (status != 0)
	{
		m_nFrames = MODEL_FRAMES(pev->modelindex);
		pev->impulse = 0;// do effect initialization
	}
	return status;
}

void CBlackBall::Spawn(void)
{
	Precache();
	//pev->movetype = MOVETYPE_FLY;
	//pev->solid = SOLID_SLIDEBOX;// other types ignore monsters or clients SOLID_TRIGGER fails!!
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_SLIDEBOX;// XDM3038a: crash prevention// XDM3037: relies on MOVETYPE_FLYMISSILE. It's purely magic.
	pev->flags |= FL_FLY;
	pev->takedamage = DAMAGE_NO;
	pev->gravity = 0;
	pev->friction = 0;
	pev->impulse = 0;// do effect initialization
	//ASSERT(pev->dmg > 0.0f);
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgBHG;

	if (pev->waterlevel == WATERLEVEL_NONE && POINT_CONTENTS(pev->origin) > CONTENTS_WATER)
	{
		SetTouch(&CBlackBall::BBTouch);
		SetThink(&CBlackBall::BBThink);
	}
	else
	{
		SetTouchNull();
		SetThinkNull();
		pev->flags = FL_KILLME;//Destroy();
		return;
	}
	pev->mins.Set(-2,-2,-2);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(2,2,2);
	//projectile	SET_MODEL(edict(), PLASMABALL_SPRITE);
	//animating	m_nFrames = MODEL_FRAMES(pev->modelindex);
	//	UTIL_SetSize(this, Vector(-1, -1, -1), Vector(1, 1, 1));
	//pev->speed = BB_SPEED;
	pev->rendermode = kRenderTransAlpha;// ?
	pev->renderamt = 255;
	pev->rendercolor.Set(255,255,255);
	// TEST pev->effects |= EF_BRIGHTFIELD|EF_BRIGHTLIGHT;
	CBaseProjectile::Spawn();
	pev->renderfx = kRenderFxPulseFast;
	pev->fuser3 = pev->scale;// fuser3 is the base component for final scale
	//pev->teleport_time = gpGlobals->time;// remember start time
	//pev->animtime = gpGlobals->time;
	pev->air_finished = gpGlobals->time + pev->fuser2;//life;
	SetNextThink(0.05);
}

void CBlackBall::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING(BLACKBALL_SPRITE);// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	PRECACHE_SOUND("weapons/blackball_blast.wav");
	m_usBlackBall = PRECACHE_EVENT(1, "events/fx/blackball.sc");
}

void CBlackBall::BBTouch(CBaseEntity *pOther)
{
	if (pOther->IsProjectile() && pOther->pev->modelindex == pev->modelindex)
	{
		Detonate();// XDM3037
		return;
	}
	// XDM3037
	//if (pev->owner == pOther->edict())
	//if (pOther == m_hOwner && (gpGlobals->time - m_fLaunchTime) > 1.0f)
	//	return;

	//if (POINT_CONTENTS(pev->origin) <= CONTENTS_WATER)
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		pev->health = 0;
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}

	Vector vecForward;
	AngleVectors(pev->angles, vecForward, NULL, NULL);
	pev->origin -= vecForward*8.0f;// don't get stuck inside something we touched
	//UTIL_SetOrigin(this, pev->origin);
	Detonate();
}

void CBlackBall::BBThink(void)
{
	if (pev->waterlevel > WATERLEVEL_NONE)// || POINT_CONTENTS(pev->origin) <= CONTENTS_WATER)
	{
		pev->velocity.Clear();
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
	}
	else
	{
		if (pev->impulse == 0)
		{
			PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), m_usBlackBall, 0, pev->origin, pev->angles, 80.0f, 0.0f, g_iModelIndexLaser, g_iModelIndexColdball2, 0, UTIL_LiquidContents(pev->origin));
			pev->impulse = 1;
		}
		else if (pev->air_finished <= gpGlobals->time)
		{
			Detonate();
			return;
		}
		/*pev->basevelocity = */pev->velocity = pev->movedir;// * pev->speed;// don't stop!
		pev->velocity *= pev->speed;
		if (pev->fuser2 > 0.0f)
			pev->fuser3 = (pev->air_finished - gpGlobals->time)/pev->fuser2;

		pev->scale = 1.0f - pev->fuser3 + RANDOM_FLOAT(0.1, 0.2);
		UpdateFrame();
		SetNextThink(0.05);
	}
}

void CBlackBall::Detonate(void)
{
	pev->renderfx = kRenderFxNone;
	SetTouchNull();
	Vector dir(pev->velocity);
	dir.NormalizeSelf();// velocity may change?
	Vector src(pev->origin - dir*2.0f);
	Vector end(pev->origin + /*pev->movedir*/dir*8.0f);
	//pev->dmg = 0.0f;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->deadflag = DEAD_DYING;
	//pev->effects = EF_NODRAW;
	pev->framerate = 0.0f;
	//pev->animtime = 0.0f;
	pev->owner = NULL;
	pev->velocity.Clear();
	SetThink(&CBlackBall::BlackHoleThink);
	SetNextThink(0);
	if (pev->dmg <= 0.0f)// it might be drained by some other ball
	{
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.1);// allow event to work
	}
	else// search for other balls and merge with them
	{
		pev->impulse = 2;
		/*CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, pev->dmg * GREN_DAMAGE_TO_RADIUS_GRAVITY)) != NULL)
		{
			if (pEntity->IsProjectile() && pEntity->pev->impulse < 2)// found a black hole which is not exploding yet
			{
				if (FClassnameIs(pEntity->pev, STRING(pev->classname)))
				{
					pev->dmg += pEntity->pev->dmg;
					pEntity->pev->air_finished = gpGlobals->time;// make it shut down immediately
				}
			}
		}*/
	}
	//EMIT_SOUND_DYN(edict(), CHAN_VOICE, "ambience/particle_suck1.wav", VOL_NORM, ATTN_NORM, 0, 90);
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), m_usBlackBall, 0, pev->origin, pev->angles, pev->dmg, 0.0f, 0, g_iModelIndexColdball2, 1, UTIL_LiquidContents(pev->origin));
	pev->dmgtime = gpGlobals->time + GREN_TELEPORT_AFTERTIME;
}

// This is called almost every frame
void CBlackBall::BlackHoleThink(void)
{
	Vector vecSpot(pev->origin);
	if (pev->flags & FL_ONGROUND)// for RadiusDamage
		vecSpot.z += 2.0f;

	float flDamage = pev->dmg * ((pev->dmgtime - gpGlobals->time)/GREN_TELEPORT_AFTERTIME);
	if (flDamage > 0)
	{
		BlackHoleImplosion(vecSpot, this, GetDamageAttacker(), pev->dmg, flDamage * BLACKBALL_DAMAGE_TO_RADIUS, false);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 0.25f);
	}

	if (gpGlobals->time >= pev->dmgtime)
	{
		pev->deadflag = DEAD_DEAD;
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.1);
	}
	else
	{
		if (pev->scale > 0.1f)
			pev->scale *= 0.8f;

		//if (pev->dmg -= (whatever/GREN_TELEPORT_AFTERTIME)//pev->dmg *= 0.75f;// decrease damage and radius
		//if (pev->renderamt < 250)
		//	pev->renderamt += 4;
		SetNextThink(0.2);
	}
}

int CBlackBall::ShouldCollide(CBaseEntity *pOther)
{
	//if (pOther->pev->modelindex == pev->modelindex)// BAD
	//	return 0;
	/*if (pOther->pev->solid == SOLID_NOT)
		return 0;
	if (pOther->pev->effects == EF_NODRAW)
		return 0;*/
	if (pOther->IsProjectile())
		return 0;
	if (pOther->IsPlayer() && !pOther->IsAlive())// some disintegrating players block entities
		return 0;

	return CBaseProjectile::ShouldCollide(pOther);
}
