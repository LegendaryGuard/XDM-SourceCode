//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "projectile.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"
#include "customprojectile.h"
#include "game.h"

// UNDONE: !!!

LINK_ENTITY_TO_CLASS(env_projectile, CEnvProjectile);


//-----------------------------------------------------------------------------
// Purpose: A really tricky way to create an instance from an instance
// Input  : *targetname - 
//			&vecSrc - 
//			&vecAng - 
//			&vecVel - 
//			*pOwner - 
// Output : CEnvProjectile
//-----------------------------------------------------------------------------
CEnvProjectile *CreateProjectile(const char *targetname, const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore)
{
	CBaseEntity *pEntity = UTIL_FindEntityByTargetname(NULL, targetname);
	if (pEntity == NULL)
		return NULL;
	if (!FClassnameIs(pEntity->pev, "env_projectile"))// important!
	{
		conprintf(1, "CreateProjectile(%s): Error: entity %s[%d] is not a projectile!\n", targetname, STRING(pEntity->pev->classname), pEntity->entindex());
		return NULL;
	}
	//CEnvProjectile *pSource = (CEnvProjectile *)pEntity;// use RTTI?
	CEnvProjectile *pNew = (CEnvProjectile *)CBaseEntity::CreateCopy(STRING(pEntity->pev->classname), pEntity, pEntity->pev->spawnflags | SF_NORESPAWN, false);
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;
		pNew->pev->bInDuck = true;
		pNew->pev->targetname = iStringNull;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;// XDM3037
		}
		pNew->SetIgnoreEnt(pEntIgnore);// XDM3037
		pNew->Spawn();
		pNew->pev->velocity = vecVel;
		pNew->pev->velocity *= pNew->pev->speed;
	}
	return pNew;
}


TYPEDESCRIPTION	CEnvProjectile::m_SaveData[] =
{
	DEFINE_FIELD(CEnvProjectile, m_iszSpriteTrail, FIELD_STRING),
	DEFINE_FIELD(CEnvProjectile, m_iszSpriteHitTarget, FIELD_STRING),
	DEFINE_FIELD(CEnvProjectile, m_iszSpriteHitWorld, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE(CEnvProjectile, CBaseAnimating);


void CEnvProjectile::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		pev->weapons = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lifetime"))
	{
		m_fLifeTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "decal"))
	{
		m_iDecal = DECAL_INDEX(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritetrail"))
	{
		m_iszSpriteTrail = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritehit"))
	{
		m_iszSpriteHitTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritehitworld"))
	{
		m_iszSpriteHitWorld = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseProjectile::KeyValue(pkvd);
}

void CEnvProjectile::Spawn(void)
{
	if (pev->bInDuck == false)// not shot, world reference. Must be kept all the time.
	{
		if (FStringNull(pev->model) || FStringNull(pev->targetname))// somebody have created me using CBaseEntity::Create(), WRONG!
		{
			conprintf(2, "%s[%d] %s: was created with wrong method! removed.\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
			pev->flags |= FL_KILLME;
			return;
		}
		Precache();
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
		pev->effects = EF_NODRAW;
		pev->modelindex = 0;
		DontThink();// XDM3037
		return;
	}
	if (pev->movetype == MOVETYPE_NONE)// XDM3038b: allow custom
		pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_NO;

	if (pev->movetype == MOVETYPE_FLY)// XDM3038b
		pev->flags = FL_FLY;

	if (pev->dmg_save == 0)// XDM3038b: radius
		pev->dmg_save = pev->dmg * DAMAGE_TO_RADIUS_DEFAULT;

	//custom	pev->gravity = 0.0;
	//	UTIL_SetOrigin(this, pev->origin);
	//	UTIL_SetSize(this, Vector(-1, -1, -1), Vector(1, 1, 1));
	//	UTIL_SetSize(this, 1.0f);
	//	SET_MODEL(edict(), STRING(pev->model));
	if (pev->mins.IsZero())
		pev->mins.Set(-1,-1,-1);// XDM3037: 3 lines of required projectile initialization: mins, maxs, model (in Precache)

	if (pev->maxs.IsZero())
		pev->maxs.Set(1,1,1);

	if (pev->scale == 0.0f)
		pev->scale = 1.0f;

	//if (pev->speed == 0.0f)// XDM3038c: velocity will be multiplied by it
	//	pev->speed = 1.0f;

	CBaseProjectile::Spawn();// XDM3037

	if (FBitSet(pev->spawnflags, SF_ENVPROJECTILE_LIGHT))
		pev->effects |= EF_DIMLIGHT;

	if (m_iSpriteTrail > 0)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
			WRITE_BYTE(TE_BEAMFOLLOW);
			WRITE_SHORT(entindex() + 0x1000);
			WRITE_SHORT(m_iSpriteTrail);
			WRITE_BYTE(1);// life
			WRITE_BYTE(4);// width
			WRITE_BYTE(pev->rendercolor.x);// r
			WRITE_BYTE(pev->rendercolor.y);// g
			WRITE_BYTE(pev->rendercolor.z);// b
			WRITE_BYTE(pev->renderamt);// a
		MESSAGE_END();
	}

	SetThink(&CEnvProjectile::FlyThink);
	//PLAYBACK_EVENT_FULL(0, edict(), m_usProjectileStart, 0.05f, pev->origin, pev->angles, 1.6f/*beamwidth*/, 0.1f/*glowscale*/, m_iTrail, m_iFlare, pev->skin, 0);
	//ResetSequenceInfo();
	pev->sequence = 0;
	pev->animtime = gpGlobals->time;
	pev->teleport_time = gpGlobals->time + m_fLifeTime;// XDM3038b
	SetNextThink(0);
}

void CEnvProjectile::Precache(void)
{
	if (FStringNull(pev->model))
	{
		conprintf(1, "CEnvProjectile(%s) Error: projectile without model!\n", STRING(pev->targetname));
		pev->model = MAKE_STRING(g_szDefaultStudioModel);
	}
	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));

	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(STRINGV(pev->noise));
	if (!FStringNull(pev->noise1))
		PRECACHE_SOUND(STRINGV(pev->noise1));

	if (!FStringNull(m_iszSpriteTrail))
		m_iSpriteTrail = PRECACHE_MODEL(STRINGV(m_iszSpriteTrail));
	if (!FStringNull(m_iszSpriteHitTarget))
		m_iSpriteHitTarget = PRECACHE_MODEL(STRINGV(m_iszSpriteHitTarget));
	if (!FStringNull(m_iszSpriteHitWorld))
		m_iSpriteHitWorld = PRECACHE_MODEL(STRINGV(m_iszSpriteHitWorld));

	//m_usProjectileHit = PRECACHE_EVENT(1, "events/fx/lightphit.sc");
	//m_usProjectileStart = PRECACHE_EVENT(1, "events/fx/lightpstart.sc");
}

void CEnvProjectile::Touch(CBaseEntity *pOther)
{
	if (pOther->pev->modelindex == pev->modelindex)
		return;

	// XDM3037	if (pev->owner == pOther->edict())
	//		return;

	if (pev->waterlevel > WATERLEVEL_NONE && !FBitSet(pev->spawnflags, SF_ENVPROJECTILE_UNDERWATER))
	{
		KillFX();
		pev->health = 0;
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}
	//CBaseEntity *pOwner = NULL;
	//if (pev->owner)
	//	pOwner = CBaseEntity::Instance(pev->owner);
	pev->owner = NULL; // can't traceline attack owner if this is set

	//UTIL_MakeVectors(pev->angles);
	//TraceResult tr = UTIL_GetGlobalTrace();
	TraceResult tr;// XDM3035a: less hacky?
	Vector forward;
	AngleVectors(pev->angles, forward, NULL, NULL);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 8.0f, dont_ignore_monsters, edict(), &tr);
	CBaseEntity *pAttacker = GetDamageAttacker();// XDM3037
	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		EMIT_SOUND(edict(), CHAN_VOICE, STRING(pev->noise), VOL_NORM, ATTN_NORM);
		Explode(m_iSpriteHitWorld);
	}
	else
	{
		ClearMultiDamage();
		gMultiDamage.type = DMG_GENERIC;
		pOther->TraceAttack(pAttacker, pev->dmg, pev->velocity.Normalize(), &tr, pev->weapons);
		ApplyMultiDamage(this, pAttacker);
		EMIT_SOUND(edict(), CHAN_VOICE, STRING(pev->noise1), VOL_NORM, ATTN_NORM);
		Explode(m_iSpriteHitTarget);
	}
}

void CEnvProjectile::FlyThink(void)
{
	//StudioFrameAdvance();
	if (m_fLifeTime > 0 && pev->teleport_time <= gpGlobals->time)
	{
		Explode(m_iSpriteHitWorld);
	}
	else if ((pev->movetype != MOVETYPE_TOSS && pev->velocity.IsZero()/* XDM3035 hung */) ||
		(pev->waterlevel > WATERLEVEL_NONE && !FBitSet(pev->spawnflags, SF_ENVPROJECTILE_UNDERWATER)))// || POINT_CONTENTS(pev->origin) <= CONTENTS_WATER)
	{
		pev->velocity.Clear();
		KillFX();
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);// XDM3038a
	}
	else
		SetNextThink(0.05);// XDM3038a
}

bool CEnvProjectile::IsProjectile(void) const
{
	if (pev->bInDuck)
		return true;

	return false;
}

int CEnvProjectile::ShouldCollide(CBaseEntity *pOther)
{
	if (FStrEq(pOther->pev->classname, pev->classname))
		return 0;

	return CBaseProjectile::ShouldCollide(pOther);
}

void CEnvProjectile::Explode(int iSpr)
{
	SetTouchNull();
	SetThinkNull();
	pev->effects |= EF_NODRAW;// KillFX(): don't send TE_KILLBEAM
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;// !
	SetTouchNull();
	SetThink(&CBaseEntity::SUB_Remove);
	::RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg, pev->dmg_save, CLASS_NONE, pev->weapons);
	KillFX();
	Vector src(pev->origin);
	Vector dir(pev->velocity);
	dir.NormalizeSelf();
	Vector end(pev->origin);
	end += dir*8.0f;

	if (pev->skin > 0)
		src -= dir*2.0f;

	if (iSpr > 0)
	{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(iSpr);
		WRITE_BYTE((int)(pev->scale * 10.0f)); // scale * 10
		WRITE_BYTE((int)pev->renderamt);
	MESSAGE_END();
	}
	//Vector end = pev->origin + pev->velocity.Normalize()*8.0;
	//PLAYBACK_EVENT_FULL(FEV_RELIABLE, edict(), m_usLightHit, 0.0f, src, angles, 0.0f, 0.0f, pev->modelindex, iSpr, pev->skin, 0);
	pev->velocity.Clear();

	if (!FBitSet(pev->spawnflags, SF_ENVPROJECTILE_SILENT))
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, DIM_GUN_FLASH, 0.2);

	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0);
}

void CEnvProjectile::KillFX(void)
{
	if (!FBitSet(pev->effects, EF_NODRAW))
	{
		MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);// do we need this?
			WRITE_BYTE(TE_KILLBEAM);
			WRITE_SHORT(entindex());
		MESSAGE_END();
	}
	pev->effects = EF_NODRAW;
	pev->renderfx = kRenderFxNone;
}

/*bool CEnvProjectile::IsWorldReference(void)
{
	return (pev->bInDuck == false);
}*/
