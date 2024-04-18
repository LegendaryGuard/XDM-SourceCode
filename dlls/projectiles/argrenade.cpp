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

LINK_ENTITY_TO_CLASS(ar_grenade, CARGrenade);

CARGrenade *CARGrenade::CreateGrenade(const Vector &vecSrc, const Vector &vecAng, const Vector &vecVel, CBaseEntity *pOwner, CBaseEntity *pEntIgnore, float fDamage)
{
	CARGrenade *pNew = GetClassPtr((CARGrenade *)NULL, "ar_grenade");
	if (pNew)
	{
		pNew->pev->origin = vecSrc;
		pNew->pev->angles = vecAng;
		pNew->pev->velocity = vecVel;
		pNew->pev->dmg = fDamage;
		if (pOwner)
		{
			pNew->m_hOwner = pOwner;
			pNew->pev->team = pOwner->pev->team;
		}
		pNew->SetIgnoreEnt(pEntIgnore);
		pNew->Spawn();
	}
	return pNew;
}

void CARGrenade::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_SLIDEBOX;
	pev->gravity *= 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
	pev->friction *= 0.8;
	pev->scale = 1.0f;
	if (pev->dmg == 0)
		pev->dmg = gSkillData.DmgM203Grenade;

	pev->mins.Set(-1,-1,-1);// XDM3037a: 3 lines of required projectile initialization: mins, maxs, model (in Precache)
	pev->maxs.Set(1,1,1);
	CBaseProjectile::Spawn();// starts animation
	pev->avelocity.x = RANDOM_FLOAT(100.0f, 500.0f);
	m_fRegisteredSound = FALSE;
	SetTouch(&CARGrenade::ExplodeTouch);
	SetThink(&CARGrenade::DangerSoundThink);
	SetNextThink(0);
}

void CARGrenade::Precache(void)
{
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/grenade.mdl");

	CBaseProjectile::Precache();
}
