//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
// TODO: rewrite, emulate. eats too much bandwidth.
// NOTE: these flames interacto with the world (pushable by the wind, etc)
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "weapons.h"
#include "flamecloud.h"
#include "game.h"
#include "gamerules.h"
#include "globals.h"
#include "skill.h"

LINK_ENTITY_TO_CLASS(flame_cloud, CFlameCloud);

CFlameCloud *CFlameCloud::CreateFlame(const Vector &vecOrigin, const Vector &vecVelocity, int &spr_idx, float scale, float scale_add, float velmult, float damage, int brightness, int br_delta, int effects, CBaseEntity *pOwner, CBaseEntity *pEntIgnore)
{
	CFlameCloud *pCloud = GetClassPtr((CFlameCloud *)NULL, "flame_cloud");
	if (pCloud)
	{
		//pCloud->pev->classname = MAKE_STRING("flame_cloud");
		//pCloud->pev->solid = SOLID_NOT;
		pCloud->pev->origin = vecOrigin;
		pCloud->pev->velocity = vecVelocity;
		pCloud->pev->modelindex = spr_idx;
		pCloud->pev->dmg = damage;
		pCloud->pev->renderamt = brightness;
		pCloud->pev->oldbuttons = br_delta;
		pCloud->pev->scale = scale;
		pCloud->pev->frags = scale_add;
		pCloud->pev->fuser1 = velmult;// XDM3038b
		if (pOwner)
		{
			pCloud->m_hOwner = pOwner;
			pCloud->pev->team = pOwner->pev->team;// XDM3037
		}
		pCloud->SetIgnoreEnt(pEntIgnore);// XDM3037
		pCloud->Spawn();
		//if (dyn_light && sv_flame_effect.value > 0.0f)
			pCloud->pev->effects |= effects;
	}
	return pCloud;
}

/*void CFlameCloud::Spawn(byte restore)
{
	Spawn();
}*/

void CFlameCloud::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_TRIGGER;// XDM3037: relies on MOVETYPE_FLYMISSILE. It's purely magic.
	pev->flags |= FL_FLY;
	pev->takedamage = DAMAGE_NO;
	pev->gravity = -1.0f;// float upwards
	pev->friction = 0;
	if (pev->dmg == 0)// XDM3038c: !
		pev->dmg = gSkillData.DmgFlame;

	//UTIL_SetOrigin(this, pev->origin);
	//UTIL_SetSize(this, 2.0f);
	//UTIL_SetSize(this, Vector(-1,-1,-1), Vector(1,1,1));
	//UTIL_SetSize(this, g_vecZero, g_vecZero);
	//pev->angles.Clear();
	//pev->angles = UTIL_VecToAngles(pev->velocity);// XDM3035
	VectorAngles(pev->velocity, pev->angles);// XDM3038

	//if (sv_serverflame.value <= 0.0f)
	//	pev->effects = EF_NODRAW;// XDM3037

	pev->rendercolor.Set(95,79,255);
	pev->rendermode = kRenderTransAdd;
	pev->health = 1.0f;

	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		pev->renderfx = kRenderFxPulseFast;

	pev->mins.Set(-2,-2,-2);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(2,2,2);
	m_fEntIgnoreTimeInterval = 0.5f;// tune this
	CBaseProjectile::Spawn();// XDM3037
	//XDM3037	pev->impulse = MODEL_FRAMES(pev->modelindex);
	pev->frame = RANDOM_LONG(0, m_nFrames-1);//pev->impulse - 1); XDM3038: fixed bad frame

	if (!FBitSet(pev->effects, EF_NODRAW))
	{
		//conprintf(1, "CFlameCloud::draw\n");
		PLAYBACK_EVENT_FULL(/*FEV_RELIABLE*/0, edict(), g_usFlameTrail, 0.0f, pev->origin, pev->angles, 2.0f, 0.0f, pev->modelindex, g_iModelIndexSmoke, RANDOM_LONG(0,1), 0);
	}
	//else
	//	conprintf(1, "CFlameCloud::NOdraw\n");

	//SetThink(Think);
	SetTouch(&CFlameCloud::FlameTouch);
	pev->framerate = 1.0f;
	pev->dmgtime = gpGlobals->time;
	SetThink(&CFlameCloud::FlameThink);
	SetNextThink(0.02);// XDM3038a
}

void CFlameCloud::Precache(void)
{
	if (pev->modelindex == 0)
		pev->modelindex = g_iModelIndexFlameFire;
	//pev->model = model by index???

	//pev->model = MAKE_STRING("sprites/flamefire.spr");// XDM3037
	//pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}

// XDM3038a: Think() is bad
void CFlameCloud::FlameThink(void)
{
	if (pev->waterlevel > 1)
	{
		Destroy();
		return;
	}
	if (m_nFrames > 1)
	{
		if (pev->frame < m_nFrames - 1)
			pev->frame += 1.0f;
		else
			pev->frame = 0;
	}
	if (pev->renderamt > pev->oldbuttons - 1)
	{
		pev->renderamt -= pev->oldbuttons;
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
			pev->rendercolor.x = clamp(pev->rendercolor.x + 32.0f, 0, 255);
			pev->rendercolor.y = clamp(pev->rendercolor.y + 30.0f, 0, 255);
		}
		pev->velocity *= pev->fuser1 * FLAMECLOUD_UPDATETIME;//0.85f;
	}
	else
	{
		Destroy();
		return;
	}
	if (pev->scale >= 1.0f)
		pev->rendermode = kRenderTransAdd;

	pev->scale += pev->frags;
	SetNextThink(FLAMECLOUD_UPDATETIME);// XDM3038a
}

void CFlameCloud::FlameTouch(CBaseEntity *pOther)
{
	if (pOther->IsProjectile() && pOther->pev->modelindex == pev->modelindex)
		return;

	if (pOther->IsBSPModel())
	{
		if ((g_pGameRules == NULL || g_pGameRules->FAllowEffects()) && sv_flame_effect.value > 1.0f)
			UTIL_DecalPoints(pev->origin, pev->origin + pev->velocity.Normalize(), edict(), DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2));

		if (pOther->IsMovingBSP())
		{
			pev->velocity *= 0.5f;
			pev->velocity += pOther->pev->velocity;
		}
		else
			pev->velocity.Clear();
	}
	else if (!pOther->IsProjectile() && pOther->pev->solid > SOLID_TRIGGER)
	{
		pev->velocity *= 0.5f;
		pev->velocity += pOther->pev->velocity;
		if (pev->dmgtime <= gpGlobals->time)// don't expand too fast (and too big)
			pev->scale += pev->frags;
	}

	if (pOther->pev->takedamage && pev->dmg > 0)
	{
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		// XDM3038a: :-/		pev->modelindex = g_iModelIndexFlameFire2;
		m_nFrames = MODEL_FRAMES(pev->modelindex);
		pev->frame = 0;
		if (pev->renderamt < 128)
			pev->dmg *= 0.5f;
		//pev->dmg *= pev->renderamt / 255;
		pev->rendercolor.Set(255,255,255);
		pev->owner = NULL;
		if (pev->dmgtime > 0 && pev->dmgtime <= gpGlobals->time)
		{
			CBaseEntity *pAttacker = GetDamageAttacker();// XDM3037
			TraceResult tr;
			Vector dir = pev->origin + pev->velocity.Normalize();
			UTIL_TraceLine(pev->origin, dir*2.0f, dont_ignore_monsters, edict(), &tr);
			ClearMultiDamage();
			pOther->TraceAttack(pAttacker, pev->dmg, dir, &tr, DMG_BURN|DMG_NEVERGIB|DMG_NOHITPOINT|DMG_IGNITE|DMG_DONT_BLEED);
			if (ApplyMultiDamage(this, pAttacker) > 0)
			{
				if (!FBitSet(pev->effects, EF_NODRAW))// XDM3037: delay added so similar events may cache
					PLAYBACK_EVENT_FULL(FEV_UPDATE, pOther->edict(), g_usFlame, 0.1f, pev->origin, pev->angles, 2.0f, 0.0f, pev->modelindex, g_iModelIndexSmoke, RANDOM_LONG(0,1), 0);
			}
			// XDM3035:	client effects are sufficient, stay invisible.		pev->effects = 0;

			/*if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
			{
				DynamicLight(pev->origin, pev->dmg * 2, 255,200,100, 10, 100);
				UTIL_Sparks(pev->origin);
			}*/
		}
		/* XDM3037: event effects are OK. This makes too much lag.		if (pOther->IsPlayer() || pOther->IsMonster())
		{
			UTIL_SetOrigin(this, pOther->Center());
			pev->velocity = pOther->pev->velocity;
			pev->aiment = pOther->edict();
			pev->movetype = MOVETYPE_FOLLOW;
			pev->scale *= 2.0f;
		}
		if (IsMultiplayer())*/
			SetThink(&CBaseEntity::SUB_Remove);// XDM3035c

		SetNextThink(0.0);// XDM3038b
		pev->dmg = 0.0f;// XDM3038b
	}
	else if (pOther->IsBSPModel())// this BSP model soesn't take any damage, so do a little radius damage
	{
		if (pev->dmgtime > 0 && pev->dmgtime <= gpGlobals->time)
		{
			pev->owner = NULL;
			::RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg, pev->dmg * 4.0f, CLASS_NONE, DMG_BURN|DMG_SLOWBURN|DMG_NEVERGIB|DMG_NOHITPOINT|DMG_IGNITE|DMG_DONT_BLEED);// XDM3035: DMG_SLOWBURN
		}
	}
	pev->dmgtime = gpGlobals->time + 0.2f;
}

int CFlameCloud::ShouldCollide(CBaseEntity *pOther)// XDM3035
{
	if (pOther->IsProjectile())//pev->modelindex == pev->modelindex)// fast enough, but maybe we should compare classnames?
	{
		//if (pev->modelindex != pev->modelindex)
		//	conprintf(0, "CFlameCloud::ShouldCollide() 0\n");
		return 0;
	}
	return CBaseProjectile::ShouldCollide(pOther);
}
