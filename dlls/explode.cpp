//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "explode.h"


// Use this function to create explosions
CEnvExplosion *ExplosionCreate(const Vector &center, const Vector &angles, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, int magnitude, int flags, float delay)
{
	CEnvExplosion *pExplosion = CEnvExplosion::CreateExplosion(center, angles, pOwner, pEntIgnore, magnitude, flags);
	if (pExplosion)
	{
		if (delay > 0.0)
		{
			pExplosion->SetThink(&CBaseEntity::SUB_CallUseToggle);
			pExplosion->SetNextThink(delay);
		}
		else
			pExplosion->Use(NULL, pOwner, USE_ON, 1.0);
	}
	return pExplosion;
}


//-----------------------------------------------------------------------------
// XDM: CEnvExplosion now works as CGrenade and has SAME SPAWNFLAGS
//-----------------------------------------------------------------------------
TYPEDESCRIPTION	CEnvExplosion::m_SaveData[] =
{
	DEFINE_FIELD(CEnvExplosion, m_iMagnitude, FIELD_INTEGER),
	DEFINE_FIELD(CEnvExplosion, m_fSpriteScale, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CEnvExplosion, CGrenade);//CBaseMonster);

LINK_ENTITY_TO_CLASS(env_explosion, CEnvExplosion);

CEnvExplosion *CEnvExplosion::CreateExplosion(const Vector &origin, const Vector &angles, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, int magnitude, int flags)
{
	CEnvExplosion *pExp = GetClassPtr((CEnvExplosion *)NULL, "env_explosion");
	if (pExp)
	{
		pExp->pev->spawnflags = flags;
		pExp->pev->origin = origin;
		pExp->pev->angles = angles;
		pExp->m_hOwner = pOwner;// XDM3037
		pExp->m_iMagnitude = magnitude;
		pExp->SetIgnoreEnt(pEntIgnore);// XDM3037
		pExp->Spawn();
	}
	return pExp;
}

void CEnvExplosion::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))// don't use pev->dmg
	{
		m_iMagnitude = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritescale"))// XDM
	{
		m_fSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sprite"))// XDM
	{
		pev->message = ALLOC_STRING(pkvd->szValue);// XDM3035c
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		pev->weapons = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvExplosion::Precache(void)
{
	if (FStringNull(pev->message))
		pev->modelindex = 0;
	else
		pev->modelindex = PRECACHE_MODEL(STRINGV(pev->message));
}

void CEnvExplosion::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	Precache();// XDM3038
	pev->effects = EF_NODRAW;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;//intangible
	pev->model = iStringNull;//invisible
	UTIL_SetOrigin(this, pev->origin);
	SetThinkNull();
	SetTouchNull();
	//checked in CGrenade::Explode()	if (m_fSpriteScale <= 0.0f)// XDM
	//	m_fSpriteScale = max(min(m_iMagnitude * 0.02f, 100.0f), 0.1f);
}

void CEnvExplosion::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	m_hActivator = pActivator;// XDM3035b
	if (pev->weapons == 0)
		pev->weapons = DMG_BLAST;

	if (FBitSet(pev->spawnflags, SF_NUCLEAR))
	{
		pev->oldbuttons = 0;// counter
		SetThink(&CGrenade::NuclearExplodeThink);
		SetNextThink(0.1);
	}
	else
	{
		pev->dmg = m_iMagnitude;// IMPORTANT: pev->dmg is changed by DoDamageInit()
		if (pev->modelindex > 0)
			Explode(pev->origin, pev->weapons, pev->modelindex, 0, pev->modelindex, 0, m_fSpriteScale, NULL);
		else
			Explode(pev->origin, pev->weapons, g_iModelIndexFireball, 0, g_iModelIndexWExplosion, 0, m_fSpriteScale, NULL);
	}
}


//-----------------------------------------------------------------------------
// XDM: OBSOLETE! Keep for compatibility
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(spark_shower, CShower);

void CShower::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->solid = SOLID_NOT;
	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model));// XDM3038c: this modifies pev->mins/maxs!

	UTIL_SetSize(this, g_vecZero, g_vecZero);
	UTIL_SetOrigin(this, pev->origin);// XDM3038b
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 159;
	pev->scale = 0.2f;
	pev->speed = RANDOM_FLOAT(0.5, 1.5);
	pev->angles.Clear();
	pev->velocity = RANDOM_FLOAT(200, 300) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT(-100.f,100.f);
	pev->velocity.y += RANDOM_FLOAT(-100.f,100.f);
	if (pev->velocity.z >= 0)
		pev->velocity.z += 200.0f;
	else
		pev->velocity.z -= 200.0f;

	SetNextThink(0.1f);
}

void CShower::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("sprites/zeroflare.spr");

	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	//pev->modelindex = g_iModelIndexZeroFlare;
}

void CShower::Think(void)
{
	UTIL_Sparks(pev->origin);
	pev->speed -= 0.1f;
	if (pev->renderamt > 0)// XDM
	{
		--pev->renderamt;
		SetBits(pev->effects, EF_MUZZLEFLASH);
	}

	if (pev->speed > 0)
	{
		ClearBits(pev->flags, FL_ONGROUND);
		SetNextThink(0.1f);
	}
	else
		Destroy();

}

void CShower::Touch(CBaseEntity *pOther)
{
	if (FBitSet(pev->flags, FL_ONGROUND))
		pev->velocity *= 0.1f;
	else
		pev->velocity *= 0.6f;

	if ((pev->velocity.x*pev->velocity.x+pev->velocity.y*pev->velocity.y) < 10.0f)
		pev->speed = 0;
}
