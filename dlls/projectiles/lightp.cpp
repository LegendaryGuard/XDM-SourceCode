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
#include "lightp.h"
#include "game.h"

LINK_ENTITY_TO_CLASS(lightp, CLightProjectile);

CLightProjectile *CLightProjectile::CreateLP(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, int iType)
{
	CLightProjectile *pNew = GetClassPtr((CLightProjectile *)NULL, "lightp");
	if (pNew)
	{
		pNew->pev->skin = iType;
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;//UTIL_VecToAngles(vecDir);
		pNew->pev->movedir = vecDir;
		pNew->pev->dmg = fDamage;// XDM3038c
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		/*if (iType == 0)
			pNew->pev->basevelocity = pNew->pev->velocity = vecDir * LIGHTP_SPEED1;
		else
			pNew->pev->basevelocity = pNew->pev->velocity = vecDir * LIGHTP_SPEED2;*/
		//pNew->pev->avelocity.z = 10;
	}
	return pNew;
}

void CLightProjectile::Spawn(void)
{
	Precache();
	//pev->movetype = MOVETYPE_FLY;
	//pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_SLIDEBOX;// XDM3038a: crash prevention// XDM3037: relies on MOVETYPE_FLYMISSILE. It's purely magic.
	pev->flags |= FL_FLY;
	pev->takedamage = DAMAGE_NO;
	pev->gravity = 0;
	pev->friction = 0;
	//pev->light_level = 255;
	// XDM3037	UTIL_SetOrigin(this, pev->origin);
	//UTIL_SetSize(this, Vector(-1, -1, -1), Vector(1, 1, 1));

	if (pev->skin == 0)
	{
		//UTIL_SetSize(this, 1.0f);
		pev->mins.Set(-1,-1,-1);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
		pev->maxs.Set(1,1,1);
	}
	else
	{
		//UTIL_SetSize(this, 2.0f);
		pev->mins.Set(-2,-2,-2);
		pev->maxs.Set(2,2,2);
	}

	if (pev->waterlevel == WATERLEVEL_NONE && POINT_CONTENTS(pev->origin) > CONTENTS_WATER)
	{
		SetTouch(&CLightProjectile::LightTouch);
		SetThink(&CLightProjectile::LightThink);
	}
	else
	{
		SetTouchNull();
		SetThinkNull();
		pev->flags = FL_KILLME;//Destroy();
		return;
	}

	if (pev->skin == 0)
	{
		pev->rendercolor.Set(0,255,0);
		pev->speed = LIGHTP_SPEED1;
	}
	else
	{
		pev->skin = 1;
		pev->rendercolor.Set(0,0,255);
		pev->speed = LIGHTP_SPEED2;
	}
	pev->body = pev->skin;
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 207;
	pev->renderfx = kRenderFxFullBright;
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgChemgun;

	m_fEntIgnoreTimeInterval = 0.25f;// fast projectile
	CBaseProjectile::Spawn();// XDM3037

	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		pev->effects |= EF_DIMLIGHT;

	/*pev->basevelocity = */pev->velocity = pev->movedir * pev->speed;// XDM3037: moved inside
	PLAYBACK_EVENT_FULL(0, edict(), m_usLightP, 0.05f, pev->origin, pev->angles, 1.6f/*beamwidth*/, 0.1f/*glowscale*/, m_iTrail, 0/*m_iFlare*/, pev->skin, LIGHTP_EV_START);
	//ResetSequenceInfo(); inside CBaseProjectile::Spawn()
	//pev->sequence = 0;
	//pev->animtime = gpGlobals->time;
	SetNextThink(0);// XDM3038a: now
}

void CLightProjectile::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/lightp.mdl");// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	// XDM3038: client precache	PRECACHE_SOUND("weapons/lp_exp.wav");
	m_iTrail = PRECACHE_MODEL(LIGHTP_SPRITE_TRAIL);
	//m_iFlare = PRECACHE_MODEL(LIGHTP_SPRITE_FLARE);
	//m_iSpark = PRECACHE_MODEL(LIGHTP_SPRITE_SPARK);
	//m_iRing = PRECACHE_MODEL(LIGHTP_SPRITE_RING);
	//m_usLightHit = PRECACHE_EVENT(1, "events/fx/lightphit.sc");
	//m_usLightStart = PRECACHE_EVENT(1, "events/fx/lightpstart.sc");
	m_usLightP = PRECACHE_EVENT(1, "events/fx/lightp.sc");
}

void CLightProjectile::LightTouch(CBaseEntity *pOther)
{
	if (pOther->IsProjectile() && pOther->pev->modelindex == pev->modelindex)
		return;

	//if (POINT_CONTENTS(pev->origin) <= CONTENTS_WATER)
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		Explode(LIGHTP_EV_HITWATER);
		return;
	}
	//CBaseEntity *pOwner = NULL;
	//if (pev->owner)
	//	pOwner = CBaseEntity::Instance(pev->owner);

	//UTIL_MakeVectors(pev->angles);
	//UTIL_GetGlobalTrace();

	TraceResult tr;// XDM3035a: less hacky?
	Vector forward;
	AngleVectors(pev->angles, forward, NULL, NULL);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 8.0f, dont_ignore_monsters, edict(), &tr);

	CBaseEntity *pAttacker = GetDamageAttacker();
	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		pev->owner = NULL; // can't traceline attack owner if this is set
		::RadiusDamage(pev->origin, this, pAttacker, pev->dmg, 24.0f, CLASS_NONE, DMG_ENERGYBEAM | DMG_NEVERGIB | DMG_RADIUS_MAX);

		//if (pOwner)
		//	pev->owner = pOwner->edict();

		Explode(LIGHTP_EV_HITSOLID);// XDM3038a
	}
	else
	{
		ClearMultiDamage();
		gMultiDamage.type = DMG_ENERGYBEAM;
		pOther->TraceAttack(pAttacker, pev->dmg, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM | DMG_NEVERGIB);
		ApplyMultiDamage(this, pAttacker);

		/*if (pev->skin == 1)// fly through?
		{
			pev->solid = SOLID_NOT;
			pev->velocity = pev->basevelocity;
		}
		else*/
			Explode(LIGHTP_EV_HIT);
	}
}

void CLightProjectile::LightThink(void)
{
	//StudioFrameAdvance();
	if (pev->waterlevel > WATERLEVEL_NONE || fabs(pev->velocity.Volume()) < 100.0f/* XDM3035 hung */)// || POINT_CONTENTS(pev->origin) <= CONTENTS_WATER)// Length() is more accurate, but we don't care and Volume() is a lot faster.
	{
		Explode(LIGHTP_EV_HITWATER);
	}
	else
	{
		if (sv_overdrive.value > 0.0f)
			pev->velocity += RandomVector(10.0f);

		SetNextThink(0.05);// XDM3038a
	}
}

void CLightProjectile::Explode(int mode)
{
	SetTouchNull();
	SetThinkNull();
	//KillFX();
	Vector src;//, end;
	if (mode == LIGHTP_EV_HITWATER)
	{
		src = pev->origin;
		//end = pev->origin;
	}
	else
	{
		Vector dir(pev->velocity);
		dir.NormalizeSelf();// velocity may change?
		//src(pev->origin - dir*2.0f);
		if (pev->skin > 0)
		{
			src = dir; src *= -2.0f;
		}
		else
			src.Clear();

		src += pev->origin;
		////end(pev->origin + /*pev->movedir*/dir*8.0f);
		//end = dir; end *= 8.0f; end += pev->origin;
	}
	PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), m_usLightP, 0.0f, src, pev->angles/*end*/, 0.0f, 1.0f, pev->modelindex, 0, pev->skin, mode);
	pev->velocity.Clear();
	pev->effects = EF_NODRAW;// KillFX(): don't send TE_KILLBEAM
	pev->renderfx = kRenderFxNone;
	pev->dmg = 0;
	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, DIM_GUN_FLASH, 0.2);
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0);// XDM3038a: now
}

/*void CLightProjectile::KillFX(void)
{
	if (!FBitSet(pev->effects, EF_NODRAW))
	{
		if (sv_reliability.value > 0)
		{
			MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);// do we need this?
				WRITE_BYTE(TE_KILLBEAM);
				WRITE_SHORT(entindex());
			MESSAGE_END();
		}
	}
	pev->effects = EF_NODRAW;
	pev->renderfx = kRenderFxNone;
}*/

int CLightProjectile::ShouldCollide(CBaseEntity *pOther)// XDM3035
{
	if (pOther->IsProjectile() && pOther->pev->modelindex == pev->modelindex)// fast enough, but maybe we should compare classnames?
		return 0;
	if (pOther->IsPlayer() && pOther->pev->deadflag == DEAD_DYING)// XDM3038a: hack to prevent the annoying effect
		return 0;

	return CBaseProjectile::ShouldCollide(pOther);
}
