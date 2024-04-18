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
#include "plasmaball.h"
#include "game.h"

LINK_ENTITY_TO_CLASS(plasmaball, CPlasmaBall);

CPlasmaBall *CPlasmaBall::CreatePB(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, float fSpeed, float fDamage, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, int iType)
{
	CPlasmaBall *pNew = GetClassPtr((CPlasmaBall *)NULL, "plasmaball");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;//UTIL_VecToAngles(vecDir); allow user to customize
		pNew->pev->movedir = vecDir;
		pNew->pev->speed = fSpeed;
		pNew->pev->skin = iType;
		pNew->pev->dmg = fDamage;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;// XDM3037
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		/*pNew->pev->basevelocity = */pNew->pev->velocity = vecDir * fSpeed;
	}
	return pNew;
}

void CPlasmaBall::Spawn(void)
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
	if (pev->dmg == 0)// XDM3038c: !
		pev->dmg = gSkillData.DmgPlasmaBall;

	if (pev->waterlevel == WATERLEVEL_NONE && POINT_CONTENTS(pev->origin) > CONTENTS_WATER)
	{
		SetTouch(&CPlasmaBall::PBTouch);
		SetThink(&CPlasmaBall::PBThink);
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
	//animating	m_nFrames = MODEL_FRAMES(pev->modelindex);// XDM
	pev->scale = 0.4f;
	//UTIL_SetSize(this, Vector(-1, -1, -1), Vector(1, 1, 1));
	//projectile	UTIL_SetSize(this, 1.0f);
	//projectile	UTIL_SetOrigin(this, pev->origin);
	//pev->speed = PB_SPEED;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->rendercolor.Set(255,255,255);
	pev->effects |= EF_DIMLIGHT;
	CBaseProjectile::Spawn();// XDM3037

	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
		pev->renderfx = kRenderFxPulseFast;
		if (sv_effects.value > 1.0)// XDM3037: overkill?
		{
			MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
				WRITE_BYTE(TE_BEAMFOLLOW);
				WRITE_SHORT(entindex() + 0x1000);
				WRITE_SHORT(g_iModelIndexLaser);
				WRITE_BYTE(1);// life
				WRITE_BYTE(8);// width
				WRITE_BYTE(pev->rendercolor.x);// r
				WRITE_BYTE(pev->rendercolor.y);// g
				WRITE_BYTE(pev->rendercolor.z);// b
				WRITE_BYTE(159);// a
			MESSAGE_END();
		}
	}
	//pev->animtime = gpGlobals->time;
	SetNextThink(0.05);// XDM3038a
}

void CPlasmaBall::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING(PLASMABALL_SPRITE);// XDM3037

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	//m_iSpriteHit = PRECACHE_MODEL(PLASMABALL_SPRITE_HIT);
	PRECACHE_SOUND("weapons/pb_exp.wav");
	m_usHit = PRECACHE_EVENT(1, "events/fx/plasmaball.sc");
}

void CPlasmaBall::PBTouch(CBaseEntity *pOther)
{
	// XDM3037: TESTME	if (pOther->IsProjectile())// huh?
	//	return;

	if (pOther->IsProjectile() && pOther->pev->modelindex == pev->modelindex)
	{
		//if (FStrEq(STRING(pOther->pev->classname), STRING(pev->classname)))
		{
			Explode(PLASMABALL_EV_HIT);// XDM3037
			return;
		}
	}

	// XDM3037
	//if (pev->owner == pOther->edict())
	//if (pOther == m_hOwner && (gpGlobals->time - m_fLaunchTime) > 1.0f)
	//	return;

	//if (POINT_CONTENTS(pev->origin) <= CONTENTS_WATER)
	if (pev->waterlevel > WATERLEVEL_NONE)
	{
		Explode(PLASMABALL_EV_HITWATER);
		return;
	}

	/*CBaseEntity *pOwner = NULL;
	if (pev->owner)
	{
		pOwner = CBaseEntity::Instance(pev->owner);
		pev->owner = NULL;// can't traceline attack owner if this is set
	}*/

	//UTIL_MakeVectors(pev->angles);
	//UTIL_GetGlobalTrace();
	//UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 16, dont_ignore_monsters, edict(), &tr);
	Vector vecForward;
	AngleVectors(pev->angles, vecForward, NULL, NULL);// XDM3037: fixes many bugs
	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		//UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2));
		::RadiusDamage(pev->origin - vecForward*2.0f, this, m_hOwner, pev->dmg, pev->dmg*3.0, CLASS_NONE, DMG_ENERGYBEAM | DMG_NEVERGIB | DMG_RADIUS_MAX);
		//if (pOwner)
		//	pev->owner = pOwner->edict();
		Explode(PLASMABALL_EV_HITSOLID);
	}
	else
	{
		TraceResult tr;
		Vector end(pev->origin);
		end += vecForward;
		UTIL_TraceLine(pev->origin, end, dont_ignore_monsters, edict(), &tr);
		ClearMultiDamage();
		gMultiDamage.type = DMG_ENERGYBEAM;
		pOther->TraceAttack(m_hOwner, pev->dmg, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM | DMG_NEVERGIB);
		ApplyMultiDamage(this, m_hOwner);
		//if (pOwner)
		//	pev->owner = pOwner->edict();
		Explode(PLASMABALL_EV_HIT);
	}
}

void CPlasmaBall::PBThink(void)
{
	/*pev->basevelocity = */pev->velocity = pev->movedir;// * pev->speed;// don't stop!
	pev->velocity *= pev->speed;

	if (pev->waterlevel > WATERLEVEL_NONE)// || POINT_CONTENTS(pev->origin) <= CONTENTS_WATER)
	{
		Explode(PLASMABALL_EV_HITWATER);
	}
	else
	{
		UpdateFrame();
		SetNextThink(0.05);// XDM3038a
	}
}

void CPlasmaBall::Explode(int mode)
{
	pev->renderfx = kRenderFxNone;
	SetTouchNull();
	SetThinkNull();
	Vector src;//, end;
	if (mode == PLASMABALL_EV_HITWATER)
	{
		src = pev->origin;
		//end = pev->origin;
	}
	else
	{
		Vector dir(pev->velocity);
		dir.NormalizeSelf();// velocity may change?
		//src(pev->origin - dir*2.0f);
		src = dir; src *= -2.0f; src += pev->origin;
		////end(pev->origin + /*pev->movedir*/dir*8.0f);
		//end = dir; end *= 8.0f; end += pev->origin;
	}
	//UTIL_DebugAngles(src, pev->angles, 2, 16);
	PLAYBACK_EVENT_FULL(0, edict(), m_usHit, 0, src, pev->angles/*end*/, pev->dmg, 0, pev->modelindex, 0/*m_iSpriteHit*/, pev->skin, mode);
	pev->dmg = 0.0f;
	pev->effects = EF_NODRAW;
	pev->velocity.Clear();
	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, DIM_GUN_FLASH, 0.2);
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0);
}
