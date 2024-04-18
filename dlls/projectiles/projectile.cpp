//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
//
// Purpose: Base class for all projectiles
//
// IMPORTANT: 1) derived classes MUST NOT override Think() and Touch()
// callbacks directly. Use normal SetThink() to custom exports.
// 2) derived classes MUST call CBaseProjectile::Spawn() from their
// own Spawn() functions.
//
// The major purpose of this class is to override HL/Quake hack
// which causes pev->owner chaos in TraceLines and Touches
// It also disables collision with owner (player) for a small amount of time,
// but not forever so players may fairly shoot themselves.
//
// Size of sprite projectiles is somewhat buggy. They collide with world
// as if they were of human size. So either that or 0. Zero is +-OK.
//
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "projectile.h"
#include "game.h"
#include "globals.h"

TYPEDESCRIPTION	CBaseProjectile::m_SaveData[] = 
{
	DEFINE_FIELD(CBaseProjectile, m_fLaunchTime, FIELD_TIME),
	DEFINE_FIELD(CBaseProjectile, m_fEntIgnoreTimeInterval, FIELD_FLOAT)
};

IMPLEMENT_SAVERESTORE(CBaseProjectile, CBaseAnimating);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProjectile::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "EntIgnoreTimeInterval"))
	{
		m_fEntIgnoreTimeInterval = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: MANDATORY: Required initialization.
// Require: pev->mins/maxs - valid size; pev->model is a valid model name;
//          pev->scale (TODO)
//
// Call this from derived classes INSTEAD OF setting model/size manually.
// Call this BEFORE issuing client messages/events.
//
//-----------------------------------------------------------------------------
void CBaseProjectile::Spawn(void)
{
// entities are still bugged #if !defined (SV_NO_PITCH_CORRECTION)
	pev->angles[PITCH] = -pev->angles[PITCH];
//#endif

	SetBits(pev->spawnflags, SF_NORESPAWN);// XDM3038c: force
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// Set these to prevent engine from distorting entvars!
	//pev->owner = m_hOwner.Get();// HACK: have to use this to avoid colliding with owner for a while
	// we could set movetype, solid, flags, and other things here, but we won't
	if (m_fEntIgnoreTimeInterval <= 0.0f)// right now we think user forgot to set it, but later it can be set to -1 or 0
		m_fEntIgnoreTimeInterval = PROJECTILE_IGNORE_OWNER_TIME;

	m_fLaunchTime = gpGlobals->time;
	m_pLastTouched = NULL;
	if (g_pWorld)// invalid during transitions!
	{
		// WARNING: these should be non-zero!
		pev->gravity *= g_pWorld->pev->gravity;// XDM3038: should we make it more generic?
		pev->friction *= g_pWorld->pev->friction;
	}
	Vector vecCustomMinS(pev->mins);// save user-defined values!
	Vector vecCustomMaxS(pev->maxs);
	//if (!FStringNull(pev->model))
	//	SET_MODEL(edict(), STRING(pev->model));// XDM3037: this modifies pev->mins/maxs!
 	CBaseAnimating::Spawn();// XDM3038c: sets model, starts animation (requires model set)
	//if (UTIL_FileExtensionIs(STRING(pev->model), ".spr"))
	//{
		pev->mins = vecCustomMinS;// restore user-defined values!
		pev->maxs = vecCustomMaxS;
		//if (pev->scale != 0.0f){
		//pev->mins *= pev->scale;// *test1.value;
		//pev->maxs *= pev->scale;
	//}
	UTIL_SetSize(this, pev->mins, pev->maxs);
	//UTIL_SetOrigin(this, pev->origin);// in CBaseEntity::Spawn()
	//don't need to	SetBlocked()
	//don't need to	SetThink()
	//ASSERT(pev->dmg > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: COMPLETE OVERRIDE:
// Warning: derived class must think!
// Note   : safe to use with Disintegrate() or SetThink()
//-----------------------------------------------------------------------------
void CBaseProjectile::Think(void)
{
	if (pev->owner && m_fEntIgnoreTimeInterval >= 0.0f)
	{
		if ((gpGlobals->time - m_fLaunchTime) > m_fEntIgnoreTimeInterval)
			SetIgnoreEnt(NULL);//pev->owner = NULL;// start colliding with owner
	}
	/*works, but is it good for all?	if (pev->owner)// XDM3037: owner has gone far enough and is hopefully not touching me, forget him (become solid)
	{
		if ((pev->origin - pev->owner->v.origin).Length() > (pev->size.Length() + pev->owner->v.size.Length()))
			SetIgnoreEnt(NULL);
	}*/

	CBaseAnimating::Think();// this calls if (m_pfnThink) (this->*m_pfnThink)();
}

//-----------------------------------------------------------------------------
// Purpose: COMPLETE OVERRIDE: proxy layer
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBaseProjectile::Blocked(CBaseEntity *pOther)
{
	if (pOther->IsPlayer() || pOther->IsMonster())
	{
		pev->velocity *= 0.5f;
		pev->velocity += pOther->pev->velocity;
	}
	CBaseAnimating::Blocked(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: COMPLETE OVERRIDE: Ignore owner for a while
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBaseProjectile::Touch(CBaseEntity *pOther)
{
	if (m_hOwner.Get() && pOther == m_hOwner)
	{
		if (m_fEntIgnoreTimeInterval >= 0.0f && (gpGlobals->time - m_fLaunchTime) <= m_fEntIgnoreTimeInterval)
			return;
	}
	/* NO! Stuck in owner's feet!	else if (m_hOwner.Get() == pev->owner)// we were ignoring our owner
	{
		SetIgnoreEnt(NULL);// we touched something else anyway, forget about "ignore entity"
	}*/
	CBaseAnimating::Touch(pOther);// if (m_pfnTouch) (this->*m_pfnTouch)(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: tried to fix size...
//-----------------------------------------------------------------------------
void CBaseProjectile::SetObjectCollisionBox(void)
{
	pev->absmin = pev->origin;
	pev->absmin += pev->mins;
	pev->absmax = pev->origin;
	pev->absmax += pev->maxs;
	//pev->size = pev->maxs;
	//pev->size -= pev->mins;
}

//-----------------------------------------------------------------------------
// Purpose: Avoid touching owner when launched, but DO touch him again after a while
// Note   : This is NOT called for Touch(), but only for traces!!
// Input  : *pOther - 
// Output : int - 1/0 do/don't
//-----------------------------------------------------------------------------
int CBaseProjectile::ShouldCollide(CBaseEntity *pOther)
{
	if (pOther->pev->effects == EF_NODRAW)
		return 0;
	// we could avoid other projectiles, but we won't	if (pOther->pev->modelindex == pev->modelindex && pOther->pev->movetype == pev->movetype)// fast enough, but maybe we should compare classnames?
	//		return 0;

	if (m_hOwner.Get())
	{
		if (pOther == (CBaseEntity *)m_hOwner)
		{
			if (m_fEntIgnoreTimeInterval >= 0.0f && (gpGlobals->time - m_fLaunchTime) <= m_fEntIgnoreTimeInterval)
				return 0;
		}
		//NO! ShouldCollide() is called for absolutely random things!		else if (m_hOwner.Get() == pev->owner)// we were ignoring our owner
		//SetIgnoreEnt(NULL);// we touched something else anyway, forget about "ignore entity"

		/*if (m_pLastTouched == m_hOwner)
		{
			if (m_fLaunchTime < (gpGlobals->time + 0.25f))
				return 0;
		}*/
	}
	//m_pLastTouched = pOther;
	return CBaseAnimating::ShouldCollide(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: This entity will be transparent to projectile
// Input  : *pEnt - entity to NOT to collide with
//-----------------------------------------------------------------------------
void CBaseProjectile::SetIgnoreEnt(CBaseEntity *pEntity)
{
	if (pEntity)
		pev->owner = pEntity->edict();// Half-Life/Quake engine hack
	else
		pev->owner = NULL;
}
