//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "sound.h"
#include "soundent.h"
#include "decals.h"
#include "skill.h"
#include "razordisk.h"
#include "gamerules.h"
#include "game.h"
#include "globals.h"
#include "pm_shared.h"

LINK_ENTITY_TO_CLASS(razordisk, CRazorDisk);

CRazorDisk *CRazorDisk::DiskCreate(const Vector &vecSrc, const Vector &vecAng, const Vector &vecDir, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage, int mode)
{
	CRazorDisk *pNew = GetClassPtr((CRazorDisk *)NULL, "razordisk");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;
		pNew->pev->impulse = mode;
		pNew->pev->bInDuck = true;
		pNew->pev->dmg = fDamage;// XDM3038c
		if (pOwner)
		{
			pNew->pev->team = pOwner->pev->team;// XDM3037
			pNew->m_hOwner = pOwner;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		/*pNew->pev->basevelocity = */pNew->pev->velocity = vecDir * RAZORDISK_AIR_VELOCITY;
	}
	return pNew;
}

void CRazorDisk::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCEMISSILE;
	pev->solid = SOLID_SLIDEBOX;
	pev->flags |= FL_FLY;
	//pev->framerate = 8;
	if (sv_overdrive.value > 0.0f)
		pev->scale = 2.0f;// LOL

	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgRazorDisk;

	pev->mins.Set(-4,-4,-4);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(4,4,4);
	m_fEntIgnoreTimeInterval = 0.25f;// fast projectile
	CBaseProjectile::Spawn();

	SetThink(&CRazorDisk::DiskThink);
	SetTouch(&CRazorDisk::DiskTouch);
	SetNextThink(0.05);

	pev->teleport_time = gpGlobals->time + RAZORDISK_LIFE_TIME;
	pev->iStepLeft = 0;// Number of bounces
	if (pev->bInDuck)// Shot by someone
	{
		pev->weaponanim = 1;// Create start effects
		//BeamFollow,
		//EMIT_SOUND(edict(), CHAN_BODY, "weapons/blade_fly.wav", VOL_NORM, ATTN_IDLE);
	}
}

void CRazorDisk::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/razordisk.mdl");

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	m_usDiskHit = PRECACHE_EVENT(1, "events/fx/razordisk.sc");
	PRECACHE_SOUND("weapons/blade_fly.wav");
	PRECACHE_SOUND("weapons/blade_hit1.wav");
	PRECACHE_SOUND("weapons/blade_hit2.wav");
	PRECACHE_SOUND("weapons/xbow_exp.wav");
}

void CRazorDisk::DiskThink(void)
{
	if (pev->teleport_time != 0.0f && pev->teleport_time <= gpGlobals->time)
	{
		pev->velocity.Clear();
		//KillFX();
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}
	if (pev->weaponanim == 1)
	{
		pev->weaponanim = 0;
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
		MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
			WRITE_BYTE(TE_BEAMFOLLOW);
			WRITE_SHORT(entindex());
			WRITE_SHORT(g_iModelIndexLaser);
			WRITE_BYTE(1);// life
			WRITE_BYTE(6*pev->scale); // width
			WRITE_BYTE(191);// r, g, b
			WRITE_BYTE(159);
			WRITE_BYTE(255);
			WRITE_BYTE(191);// a
		MESSAGE_END();
		}
	}

	if (pev->waterlevel > WATERLEVEL_WAIST)
	{
		pev->speed = RAZORDISK_WATER_VELOCITY;
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
			FX_BubblesLine(pev->origin - pev->velocity * 0.125f, pev->origin, 4);

		SetNextThink(0.125f);
	}
	else
	{
		pev->speed = RAZORDISK_AIR_VELOCITY;
		SetNextThink(0.05f);// 20 times per second should be sufficient
	}
	if (sv_razordisc_max_bounce.value > 0)
	{
		pev->speed *= 1.0f - 0.1f*(pev->iStepLeft/sv_razordisc_max_bounce.value);// decrease speed depending on number of hits
//#if !defined (SV_NO_PITCH_CORRECTION)
/*		pev->angles.x = -pev->angles.x;// un-bugged client angles to bugged entity angles
//#endif
		AngleVectors(pev->angles, gpGlobals->v_forward, NULL, NULL);
		pev->velocity = gpGlobals->v_forward;
		pev->velocity *= pev->speed;*/
		pev->velocity.SetLength(pev->speed);
	}
}

void CRazorDisk::DiskTouch(CBaseEntity *pOther)
{
	int iHitType = RAZORDISK_HIT_SOLID;// hit type: 0 solid, 1 armor, 2 flesh
	Vector vecDir(pev->velocity);
	vecDir.NormalizeSelf();
	Vector vecForward(vecDir);
	vecForward *= 32.0f;
	//AngleVectors(pev->angles, vecForward, NULL, NULL);// XDM3037: fixes many bugs
	Vector end(pev->origin + vecForward);
	//SetBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);
	ClearBits(gpGlobals->trace_flags, FTRACE_SIMPLEBOX);// need hitbox precision
	TraceResult tr;
	UTIL_TraceLine(pev->origin, end, dont_ignore_monsters, edict(), &tr);
	char cTextureType = CHAR_TEX_NULL;
	bool bStuck = false;

	if (pOther->IsBSPModel())
		cTextureType = TEXTURETYPE_Trace(CBaseEntity::Instance(tr.pHit), pev->origin, end);

	if (cTextureType == CHAR_TEX_SKY)//if (tex && _stricmp(tex, "SKY") == 0)
	{
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.01);// unsafe! Destroy();
		return;
	}
	SetBits(pev->effects, EF_MUZZLEFLASH);
	pev->origin -= vecDir;
	//UTIL_SetOrigin(this, pev->origin);
	if (pev->impulse == 0)
	{
		if (cTextureType == CHAR_TEX_FLESH && tr.iHitgroup == HITGROUP_ARMOR)
		{
			iHitType = RAZORDISK_HIT_ARMOR;
		}
		else if (cTextureType == CHAR_TEX_FLESH || pOther->IsMonster() || pOther->IsPlayer())
		{
			iHitType = RAZORDISK_HIT_FLESH;
			bStuck = true;
		}
		else if (cTextureType == CHAR_TEX_DIRT ||
				cTextureType == CHAR_TEX_SLOSH ||
				cTextureType == CHAR_TEX_WOOD ||
				cTextureType == CHAR_TEX_SNOW ||
				cTextureType == CHAR_TEX_GRASS ||
				cTextureType == CHAR_TEX_CEILING)
		{
			iHitType = RAZORDISK_HIT_STUCK;
			bStuck = true;
		}

		if (pOther->pev->takedamage != DAMAGE_NO)
		{
			ClearMultiDamage();
			CBaseEntity *pAttacker = GetDamageAttacker();// XDM3037
			pOther->TraceAttack(pAttacker, pev->dmg, vecDir, &tr, DMG_SLASH);
			ApplyMultiDamage(this, pAttacker);
		}
		float force = 0.5f*pOther->DamageForce(pev->dmg);
		pOther->pev->velocity += vecForward * force;
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, WEAPON_ACTIVITY_VOLUME, 0.5f);
	}
	else
	{
		pev->takedamage = DAMAGE_NO;
		SetIgnoreEnt(NULL);// can't traceline attack owner if this is set
		::RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg, pev->dmg*DAMAGE_TO_RADIUS_DEFAULT, CLASS_NONE, DMG_BLAST);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 1.0f);
	}

	++pev->iStepLeft;
	if (pev->impulse == 0 && !bStuck && pev->iStepLeft < (int)sv_razordisc_max_bounce.value)// reflect velocity, change angles
	{
		if (pOther != g_pWorld)
			SetIgnoreEnt(pOther);// don't hit twice
		/* MOVETYPE_BOUNCEMISSILE takes care of this vecDir.MirrorByVector(tr.vecPlaneNormal);
		pev->origin += vecDir * pev->size.Length();
		pev->velocity = vecDir; pev->velocity *= pev->speed;
		VectorAngles(vecDir, pev->angles);
#if defined (NOSQB)// back to engine's SQB-corrupted value
		pev->angles.x = -pev->angles.x;
#endif*/
		//UTIL_DebugBeam(pev->origin, pev->origin + tr.vecPlaneNormal*64.0f, 2.0f, 0,0,255);
		//UTIL_DebugBeam(pev->origin, pev->origin + vecDir*64.0f, 2.0f, 255,143,31);
	}
	else// overbounce, stuck or exploded
	{
		pev->velocity.Clear();
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		SetBits(pev->effects, EF_NODRAW);
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.05);// allow the event to pass
	}
	PLAYBACK_EVENT_FULL(FEV_UPDATE, edict(), m_usDiskHit, 0.0, pev->origin, pev->angles, pev->dmg, cTextureType, (pev->waterlevel <= 1)?g_iModelIndexBigExplo4:g_iModelIndexWExplosion, RANDOM_LONG(PITCH_NORM-5,PITCH_NORM+5), iHitType, pev->impulse);
}
