/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== bmodels.cpp ========================================================

  spawn, think, and use functions for entities that use brush models

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "pm_materials.h"
#include "pm_shared.h"
#include "func_break.h"
#include "decals.h"
#include "weapons.h"// XDM
#include "explode.h"
#include "game.h"
#include "sound.h"
#include "soundent.h"
#include "gamerules.h"
#include "globals.h"


// This shit is obsolete and should eventually disappear
const char *CBreakable::pSpawnObjects[] =
{
	NULL,					// 0
	"item_battery",
	"item_healthkit",
	"weapon_9mmhandgun",
	"ammo_9mmclip",
	"weapon_9mmAR",			// 5
	"ammo_9mmAR",
	"ammo_ARgrenades",
	"weapon_shotgun",
	"ammo_buckshot",
	"weapon_crossbow",		// 10
	"ammo_crossbow",
	"weapon_357",
	"ammo_357",
	"weapon_rpg",
	"ammo_rpgclip",			// 15
	"ammo_gaussclip",
	"weapon_handgrenade",
	"weapon_tripmine",
	"weapon_satchel",
	"weapon_snark",			// 20
	"weapon_hornetgun",
	"item_airtank",
	"item_antidote",
	"weapon_sword",
	"weapon_strtarget",		// 25
	"weapon_chemgun",
	"ammo_lightp",
	"weapon_alauncher",
	"ammo_alauncher",
	"weapon_glauncher",		// 30
	"ammo_glauncher",
	"weapon_plasma",
	"weapon_flame",
	"ammo_fueltank",
	"weapon_displacer",		// 35
	"weapon_beamrifle",
	"weapon_bhg",
	"weapon_dlauncher"
};

//
// func_breakable - bmodel that breaks into pieces after taking damage
//
LINK_ENTITY_TO_CLASS(func_breakable, CBreakable);

TYPEDESCRIPTION CBreakable::m_SaveData[] =
{
	DEFINE_FIELD(CBreakable, m_Material, FIELD_INTEGER),
	DEFINE_FIELD(CBreakable, m_iExplosion, FIELD_INTEGER),
	//DEFINE_FIELD(CBreakable, m_idShard, FIELD_INTEGER),// Don't need to save/restore these because we precache after restore
	DEFINE_FIELD(CBreakable, m_angle, FIELD_FLOAT),
	DEFINE_FIELD(CBreakable, m_iszGibModel, FIELD_MODELNAME),
	DEFINE_FIELD(CBreakable, m_iszSpawnObject, FIELD_STRING),
	//DEFINE_FIELD(CBreakable, m_iSpawnObject, FIELD_INTEGER),// XDM3035c: index in pSpawnObjects array
	DEFINE_FIELD(CBreakable, m_iszWhenHit, FIELD_STRING),
	// Explosion magnitude is stored in pev->impulse
};

IMPLEMENT_SAVERESTORE(CBreakable, CBaseEntity);

void CBreakable::KeyValue(KeyValueData *pkvd)
{
	// UNDONE_WC: explicitly ignoring these fields, but they shouldn't be in the map file!
	if (FStrEq(pkvd->szKeyName, "explosion"))
	{
		if (!_stricmp(pkvd->szValue, "directed"))
			m_iExplosion = EXPLOSION_DIRECTED;
		//else if (!_stricmp(pkvd->szValue, "random"))
		//	m_iExplosion = EXPLOSION_RANDOM;
		else
			m_iExplosion = EXPLOSION_RANDOM;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "material"))
	{
		int i = atoi(pkvd->szValue);
		if ((i < 0) || (i >= matLastMaterial))
			m_Material = matNone;// XDM3037: TESTME?
		else
			m_Material = (Materials)i;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "deadmodel"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "shards"))
	{
		//m_iShards = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "gibmodel"))
	{
		m_iszGibModel = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spawnobject"))
	{
		if (pkvd->szValue)
		{
			if (isalpha(*pkvd->szValue))
				m_iszSpawnObject = ALLOC_STRING(pkvd->szValue);// XDM
			else
			{
				int iObject = atoi(pkvd->szValue);
				if (iObject > 0)
				{
					if (iObject < ARRAYSIZE(pSpawnObjects))
						m_iszSpawnObject = MAKE_STRING(pSpawnObjects[iObject]);// XDM3038c: these strings are static anyway
					else
					{
						conprintf(0, "Design error: %s[%d] has bad %s - \"%s\"!\n", STRING(pev->classname), entindex(), pkvd->szKeyName, pkvd->szValue);
						m_iszSpawnObject = iStringNull;
					}
				}
			}
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "explodemagnitude"))
	{
		pev->impulse = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	//else if (FStrEq(pkvd->szKeyName, "lip"))
	//	pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "whenhit"))// SHL
	{
		m_iszWhenHit = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CBreakable::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars! TESTME
    Precache();

	if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY))
		pev->takedamage	= DAMAGE_NO;
	else
	{
		pev->takedamage	= DAMAGE_YES;
		SetThink(&CBreakable::ExistThink);
		SetNextThink(0.5);// XDM3038a // XDM3035c: need to think so CheckEnvironment() works.
	}

	if (UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))
	{
		pev->solid		= SOLID_BBOX;
		pev->movetype	= MOVETYPE_TOSS;
	}
	else
	{
		pev->solid		= SOLID_BSP;
		pev->movetype	= MOVETYPE_PUSH;
	}
    m_angle			= pev->angles.y;
	pev->angles.y	= 0;
	pev->max_health = pev->health;// XDM3037

	// HACK:  matGlass can receive decals, we need the client to know about this
	//  so use class to store the material flag
	if (m_Material == matGlass || m_Material == matUnbreakableGlass)// XDM: ?
		pev->playerclass = 1;

	SET_MODEL(edict(), STRING(pev->model));//set size and link into world.

	if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY))// Only break on trigger
		SetTouchNull();
	else
		SetTouch(&CBreakable::BreakTouch);

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if (!IsBreakable() && pev->rendermode == kRenderNormal)// XDM: I want to shoot through the glass!!!
		SetBits(pev->flags, FL_WORLDBRUSH);
}

void CBreakable::Precache(void)
{
	char *pGibModelName = NULL;
	if (!FStringNull(m_iszGibModel))
		pGibModelName = STRINGV(m_iszGibModel);
	else
		pGibModelName = (char *)gBreakModels[m_Material];// XDM3038c

	size_t i = 0;// XDM: don't precache all sounds!
	for (i = 0; i < NUM_BREAK_SOUNDS; ++i)
		PRECACHE_SOUND((char *)gBreakSounds[m_Material][i]);

	//MaterialSoundPrecache(m_Material);
	for (i = 0; i < NUM_SHARD_SOUNDS; ++i)
		PRECACHE_SOUND((char *)gShardSounds[m_Material][i]);

	if (pGibModelName)
		m_idShard = PRECACHE_MODEL(pGibModelName);
	else
		m_idShard = 0;

	// Precache the spawn item's data
	if (!FStringNull(m_iszSpawnObject))
		UTIL_PrecacheOther(STRING(m_iszSpawnObject));//UTIL_PrecacheOther(pSpawnObjects[m_iSpawnObject]);
}

void CBreakable::ExistThink(void)
{
	if (pev->takedamage != DAMAGE_NO)
		pev->nextthink = pev->ltime + 0.5f;
}

bool CBreakable::IsIgnitable(void)// XDM3037
{
	//if (m_Material == matCinderBlock || m_Material == matUnbreakableGlass || m_Material == matRocks)
	//	return false;
	if (m_Material == matWood || m_Material == matCeilingTile || m_Material == matComputer)
		return true;
	else if (Explodable())//if (m_Material == matMetal)
	{
		if (pev->health < pev->max_health)// only damaged (leaking) barrels will be set on fire
			return true;
	}
	return false;
}

// XDM3038c: now for real
void CBreakable::DamageSound(float flVol)
{
	if (m_Material == matNone)
		return;

	int material;
	if (m_Material == matComputer && RANDOM_LONG(0,1))// 50% switch between metal and glass sounds
		material = matMetal;
	else
		material = m_Material;

	if (gShardSounds[material])
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, gShardSounds[material][RANDOM_LONG(0,NUM_SHARD_SOUNDS-1)], min(flVol,VOL_NORM), ATTN_NORM, 0, RANDOM_LONG(95,110));
}

void CBreakable::BreakTouch(CBaseEntity *pOther)
{
	// only players can break these right now
	if (!pOther->IsPlayer() || !IsBreakable())// XDM3037: TODO: consider including monsters here?
        return;
	if (pOther->pev->movetype == MOVETYPE_NOCLIP)// XDM3038c: avoid noclipping players
		return;

	float flDamage;
	if (FBitSet(pev->spawnflags, SF_BREAK_TOUCH))// can be broken when run into
	{
		flDamage = pOther->pev->velocity.Length() * 0.01f;
		if (flDamage >= pev->health)
		{
			SetTouchNull();
			TakeDamage(pOther, pOther, flDamage, DMG_CRUSH);
			// do a little damage to player if we broke glass or computer
			pOther->TakeDamage(this, m_hActivator?(CBaseEntity *)m_hActivator:this, flDamage/4, DMG_SLASH);// XDM3035: m_hActivator
		}
	}

	if (FBitSet(pev->spawnflags, SF_BREAK_PRESSURE) && pOther->pev->absmin.z >= pev->maxs.z - 2.0f)// can be broken when stood upon
	{
		m_hActivator = pOther;// XDM3038c
		// play creaking sound here.
		DamageSound(VOL_NORM);
		SetThink(&CBreakable::Die);
		SetTouchNull();
		if (m_flDelay == 0)
		{// !!!BUGBUG - why doesn't zero delay work?
			m_flDelay = 0.1;
		}
		pev->nextthink = pev->ltime + m_flDelay;
	}
}

// Break when triggered
void CBreakable::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (IsBreakable())
	{
		pev->angles.y = m_angle;
		UTIL_MakeVectors(pev->angles);
		g_vecAttackDir = gpGlobals->v_forward;
		Killed(pActivator, pActivator, GIB_NORMAL);// XDM3038a //Die();
	}
}

void CBreakable::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	// random spark if this is a 'computer' object
	if (m_Material == matComputer)// XDM: optimization
	{
		if (RANDOM_LONG(0,1))
		{
			UTIL_Sparks(ptr->vecEndPos);
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, gSoundsSparks[RANDOM_LONG(0,NUM_SPARK_SOUNDS-1)], RANDOM_FLOAT(0.7, 1.0), ATTN_NORM, 0, RANDOM_LONG(95,105));
		}
	}
	else if (m_Material == matGlass)
	{
		UTIL_DecalTrace(ptr, DECAL_GLASSBREAK1 + RANDOM_LONG(0, 2));
	}
	else if (m_Material == matUnbreakableGlass)
	{
		if (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH))
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(0.5, 1.5));
		// client UTIL_DecalTrace(ptr, DECAL_BPROOF1);
	}
	CBaseDelay::TraceAttack(pAttacker, flDamage, vecDir, ptr, bitsDamageType);

	if (!FStringNull(m_iszWhenHit))
	{
		FireTargets(STRING(m_iszWhenHit), pAttacker, this, USE_ON, 1.0);
		// once? FREE_STRING(m_iszWhenHit);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Filter damage by material
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CBreakable::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (!IsBreakable())
		return 0;

	int bitsDamageBase = bitsDamageType & ~DMGM_MODIFIERS;// extract base damage types (not modifiers like GIBALWAYS, etc.)
	//Vector vecTemp;
	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin).
	if (pInflictor)
	{
		//vecTemp = Center() - pInflictor->pev->origin;//( pev->absmin + ( pev->size * 0.5 ) );
		// this global is still used for glass and other non-monster killables, along with decals.
		g_vecAttackDir = Center();//vecTemp;
		g_vecAttackDir -= pInflictor->Center();
		g_vecAttackDir.NormalizeSelf();
		if (pAttacker == pInflictor)
		{
			//vecTemp = pInflictor->pev->origin - ( pev->absmin + ( pev->size * 0.5 ) );
			// if a client hit the breakable with a crowbar, and breakable is crowbar-sensitive, break it now.
			if (pAttacker && pAttacker->IsPlayer() && FBitSet(pev->spawnflags, SF_BREAK_CROWBAR) && (bitsDamageType & DMG_CLUB))
				flDamage = pev->health;
		}
	}

	// Breakables take double damage from the crowbar
	if (bitsDamageBase & DMG_CLUB)
		flDamage *= 1.5f;// XDM: here was 2

	// Boxes / glass / etc. don't take much poison damage, just the impact of the dart - consider that 10%
	if (!FBitSet(bitsDamageType, DMGM_BREAK))// XDM: if no explicit breaking force was specified, ignore some damage types
	{
		if (bitsDamageBase == DMG_RADIATION || bitsDamageBase == DMG_FREEZE || bitsDamageBase == DMG_NERVEGAS)// XDM3037: '=='! Not '&'!
		{
			if (m_Material == matFlesh)
				flDamage = 1.0f;
			else
				return 0;
		}
		else if ((bitsDamageBase == DMG_ACID) || (bitsDamageBase == DMG_POISON))
		{
			if (m_Material == matGlass || m_Material == matUnbreakableGlass)
				return 0;
			else
				flDamage *= 0.1f;
		}
		else if (bitsDamageBase == DMG_SHOCK)
		{
			if (m_Material != matFlesh && m_Material != matComputer)
				return 0;
		}
		else if (bitsDamageBase & DMGM_COLD)
		{
			if (m_Material != matGlass && m_Material != matFlesh)
				return 0;
		}
		else if (bitsDamageBase & DMGM_FIRE)
		{
			if (IsIgnitable())
			{
				if (m_Material != matWood && m_Material != matCeilingTile)
					flDamage *= 0.25f;// probably some explodable barrel
			}
			else
				return 0;
		}
	}
	bool makenoise = true;
	if (IsIgnitable())//if (m_Material == matWood || m_Material == matComputer)
	{
		if (bitsDamageType & DMGM_FIRE)
		{
			EMIT_SOUND_DYN(edict(), CHAN_BODY, "weapons/flame_burn.wav", VOL_NORM, ATTN_IDLE, 0, RANDOM_LONG(95, 105));
			makenoise = false;
			if ((g_pGameRules == NULL || g_pGameRules->FAllowEffects()) && (m_flBurnTime == 0.0f || RANDOM_LONG(0,3) == 0))// XDM3038: always smoke first time, after that - randomize
			{
				MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, Center());
					WRITE_BYTE(TE_SMOKE);
					WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
					WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
					WRITE_COORD(RANDOM_FLOAT(pev->absmin.z, pev->absmax.z));
					WRITE_SHORT(g_iModelIndexSmoke);
					WRITE_BYTE(RANDOM_LONG(10,20));// scale * 10
					WRITE_BYTE(RANDOM_LONG(10,14));// framerate
				MESSAGE_END();
			}
		}
		if (bitsDamageType & DMG_IGNITE)// XDM3037: WARNING! only react to DMG_IGNITE to prevent recursion!!
		{
			if (m_flBurnTime == 0.0f)// was not burning, start.
			{
				m_flBurnTime = gpGlobals->time;
				m_hTBDAttacker = pAttacker;// XDM3037
			}
			m_flBurnTime += flDamage*0.2f;// XDM3037: continue burning for N seconds
		}
	}

	//pev->dmg = flDamage;// XDM: remember here, after ignoring

	// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		//m_hActivator = pAttacker;// XDM3035c: TESTME
		if (bitsDamageType & DMGM_FIRE)// XDM3038b: HACK to make gibs smoke
			m_flBurnTime = gpGlobals->time+0.1f;

		Killed(pInflictor, pAttacker, GIB_NORMAL);// IMPORTANT: order
	}
	else
	{
		// Make a shard noise each time func breakable is hit.
		// Don't play shard noise if cbreakable actually died.
		if (makenoise && pev->max_health > 0)// XDM
			DamageSound(flDamage/pev->max_health + 0.1f);// VOL_NORM == 1.0
	}
	return (int)flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Entity was killed as a result of TakeDamage() (most likely)
// Input  : *pInflictor - weapon/projectile
//			*pAttacker - weapon owner
//			iGib - GIBMODE
//-----------------------------------------------------------------------------
void CBreakable::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	int soundbits = NULL;// XDM
	int pitch = 95 + RANDOM_LONG(0,29);
	int cFlag = 0;
	int iVirtualVolume = 120;// XDM3038a: for AI

	if (pitch > 97 && pitch < 103)// ???
		pitch = PITCH_NORM;

	// The more negative pev->health, the louder the sound should be
	float fvol = 0.75f + 0.25f * (abs(pev->health)/pev->max_health);// pev->health can go lower than -pev->max_health!
	if (fvol > VOL_NORM)
		fvol = VOL_NORM;

	EMIT_SOUND_DYN(edict(), CHAN_VOICE, gBreakSounds[m_Material][RANDOM_LONG(0,NUM_BREAK_SOUNDS-1)], fvol, ATTN_NORM, 0, pitch);// XDM3038a
	switch (m_Material)
	{
	case matGlass:
		cFlag = BREAK_GLASS;
		soundbits = bits_SOUND_WORLD;
		iVirtualVolume = fvol*600;
		break;
	case matWood:
		cFlag = BREAK_WOOD;
		soundbits = bits_SOUND_WORLD;
		iVirtualVolume = fvol*480;
		break;
	case matComputer:
	case matMetal:
		cFlag = BREAK_METAL;
		soundbits = bits_SOUND_WORLD;
		iVirtualVolume = fvol*580;
		break;
	case matFlesh:
		cFlag = BREAK_FLESH;
		soundbits = bits_SOUND_WORLD|bits_SOUND_MEAT;
		iVirtualVolume = fvol*300;
		break;
	case matRocks:
	case matCinderBlock:
		cFlag = BREAK_CONCRETE;
		soundbits = bits_SOUND_WORLD;
		iVirtualVolume = fvol*500;
		break;
	case matCeilingTile:
		cFlag = BREAK_WOOD;// XDM3038c: was concrete
		soundbits = bits_SOUND_WORLD;
		iVirtualVolume = fvol*400;
		break;
	}

	if (m_idShard > 0)
	{
		Vector vecSpot;// shard origin
		Vector vecVelocity;// shard velocity

		if (m_iExplosion == EXPLOSION_DIRECTED)
			vecVelocity = g_vecAttackDir * clamp(pev->dmg * -10, -1024, 1024);// XDM 200;
		else
			vecVelocity = RandomVector() * clamp(pev->dmg * 10, 1, 240);// XDM: really nice randomization!

		if (m_Material == matWood)// XDM
			vecVelocity *= 2.0;

		vecSpot = VecBModelOrigin(pev);// XDM3038a //vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

		if (m_flBurnTime > gpGlobals->time)// XDM3038a
			cFlag |= BREAK_SMOKE;

		//CGib::SpawnModelGibs(pev, vecSpot, pev->mins, pev->maxs, vecVelocity, limit(pev->dmg, 10, 50), m_idShard, 20/*-count*/, 2.5, m_Material);
		MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecSpot);
			WRITE_BYTE(TE_BREAKMODEL);
			WRITE_COORD(vecSpot.x);// position
			WRITE_COORD(vecSpot.y);
			WRITE_COORD(vecSpot.z);
			WRITE_COORD(pev->size.x);// size
			WRITE_COORD(pev->size.y);
			WRITE_COORD(pev->size.z);
			WRITE_COORD(vecVelocity.x);// velocity
			WRITE_COORD(vecVelocity.y);
			WRITE_COORD(vecVelocity.z);
			WRITE_BYTE(clamp(pev->dmg, 10, 50));// velocity randomization
			WRITE_SHORT(m_idShard);//model id#
			WRITE_BYTE(0);// # of shards, let client decide
			// duration
			if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
				WRITE_BYTE(50);// XDM3038a: extended time
			else
				WRITE_BYTE(25);// 2.5 seconds
			
			WRITE_BYTE(cFlag);// flags
		MESSAGE_END();
	}

	m_hActivator = pAttacker;// XDM3038a: do we need this anymore?
	pev->takedamage = DAMAGE_NO;// IMPORTANT! Prevents self-destructive recursion!
	pev->deadflag = DEAD_DEAD;

	if (soundbits && iVirtualVolume > 0)
		CSoundEnt::InsertSound(soundbits, pev->origin, iVirtualVolume, 2.0);// XDM

	vec_t size = pev->size.x;
	if (size < pev->size.y)
		size = pev->size.y;
	if (size < pev->size.z)
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins(pev->absmin);
	Vector maxs(pev->absmax);
	mins.z = pev->absmax.z;
	maxs.z += 8;

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity *pList[256];
	size_t count = UTIL_EntitiesInBox(pList, 256, mins, maxs, FL_ONGROUND);
	if (count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			if (pList[i]->pev->groundentity == edict())// XDM3038c
			{
				ClearBits(pList[i]->pev->flags, FL_ONGROUND);
				pList[i]->pev->groundentity = NULL;
			}
		}
	}

	// Don't fire something that could fire myself
	pev->movetype = MOVETYPE_NONE;// XDM3038
	pev->solid = SOLID_NOT;
	pev->targetname = iStringNull;
	pev->health = 0;// XDM
	// Fire targets on break
	SUB_UseTargets(pAttacker, USE_TOGGLE, 0);// XDM3035c: must keep pev->target to use this!

	// replaced by CBaseDelay::Killed
	//SetThink(&CBaseEntity::SUB_Remove);// clears pev->target!
	//pev->nextthink = pev->ltime + 0.1;
	UTIL_SetOrigin(this, pev->origin);// XDM3038: this forces the engine to refresh this entity solidness which allows proper RadiusDamage()

	if (!FStringNull(m_iszSpawnObject))//(m_iSpawnObject > 0)
		Create(STRING(m_iszSpawnObject), VecBModelOrigin(pev), pev->angles, pev->velocity, edict(), SF_NORESPAWN);

	if (Explodable())
		ExplosionCreate(Center(), pev->angles, pAttacker, this, pev->impulse, 0, 0.0f);

	CBaseDelay::Killed(pInflictor, pAttacker, iGib);
}

// Think() callback to delay explosion
void CBreakable::Die(void)
{
	DontThink();
	Killed(m_hActivator, m_hActivator, GIB_NORMAL);// XDM3038a
}

bool CBreakable::IsBreakable(void) const
{
	return (m_Material != matUnbreakableGlass);
}

bool CBreakable::Explodable(void) const
{
	return (pev->impulse > 0);
}

//-----------------------------------------------------------------------------
// Purpose: Which decal should be drawn on this entity for specified damage type
// Input  : &bitsDamageType - 
// Output : Server decal index
//-----------------------------------------------------------------------------
int	CBreakable::DamageDecal(const int &bitsDamageType)
{
	if (m_Material == matGlass)
		return DECAL_GLASSBREAK1 + RANDOM_LONG(0,2);

	if (m_Material == matUnbreakableGlass)
		return DECAL_BPROOF1;

	if (m_Material == matWood)// XDM
		return DECAL_WOODBREAK1 + RANDOM_LONG(0,2);

	if (m_Material == matComputer)// XDM
		return DECAL_LARGESHOT1 + RANDOM_LONG(0,4);

	return CBaseEntity::DamageDecal(bitsDamageType);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: how should shock wave interact with this entity
// Input  : &bitsDamageType - DMG_GENERIC
//			&vecSrc - source point of explosion
// Output : short - DMG_INTERACTION
//-----------------------------------------------------------------------------
short CBreakable::DamageInteraction(const int &bitsDamageType, const Vector &vecSrc)
{
	if (m_Material == matRocks || m_Material == matCinderBlock)
		return DMGINT_NONE;// even radiation

	if (Explodable() && (bitsDamageType & DMG_BLAST))
		return DMGINT_ABSORB;

	if ((m_Material == matWood || m_Material == matCeilingTile) && (bitsDamageType & (DMGM_FIRE|DMG_IGNITE)))
		return DMGINT_ABSORB;

	if ((m_Material == matWood || m_Material == matFlesh) && (bitsDamageType & DMGM_POISON))
		return DMGINT_ABSORB;

	return CBaseEntity::DamageInteraction(bitsDamageType, vecSrc);
}




// WARNING: pev->friction is used wrong here!

TYPEDESCRIPTION	CPushable::m_SaveData[] =
{
	//DEFINE_FIELD(CPushable, m_maxSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CPushable, m_soundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CPushable, CBreakable);

LINK_ENTITY_TO_CLASS(func_pushable, CPushable);

void CPushable::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "size"))
	{
		int bbox = atoi(pkvd->szValue);
		switch (bbox)
		{
		case 0:	// Point
			UTIL_SetSize(this, Vector(-8, -8, -8), Vector(8, 8, 8));
			break;

		case 2: // Big Hull!?!? !!!BUGBUG Figure out what this hull really is
			UTIL_SetSize(this, VEC_DUCK_HULL_MIN*2, VEC_DUCK_HULL_MAX*2);
			break;

		case 3: // Player duck
			UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
			break;

		default:
		case 1: // Player
			UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "buoyancy"))
	{
		pev->skin = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBreakable::KeyValue(pkvd);
}

void CPushable::Spawn(void)
{
	if (FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE))
		CBreakable::Spawn();
	else
		Precache();

	pev->movetype	= MOVETYPE_PUSHSTEP;
	pev->solid		= SOLID_BBOX;
	SET_MODEL(edict(), STRING(pev->model));

	if (pev->friction > 399)
		pev->friction = 399;

	//m_maxSpeed = 400 - pev->friction;// HACK!
	SetBits(pev->flags, FL_FLOAT);
	pev->friction = 0;
	pev->origin.z += 1;	// Pick up off of the floor
	UTIL_SetOrigin(this, pev->origin);
	// Multiply by area of the box's cross-section (assume 1000 units^3 standard volume)
	pev->skin = (pev->skin * (pev->maxs.x - pev->mins.x) * (pev->maxs.y - pev->mins.y)) * 0.0005f;// buoyancy
	m_soundTime = 0;
}

void CPushable::Precache(void)
{
	if (m_Material == matNone || m_Material == matLastMaterial)
		return;

	int i = 0;// XDM: don't precache all sounds!
	for(i = 0; i < NUM_PUSH_SOUNDS; i++)
		PRECACHE_SOUND((char *)gPushSounds[m_Material][i]);

	if (FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE))
		CBreakable::Precache();
}

// Pull the func_pushable
void CPushable::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pActivator && pActivator == pCaller && pActivator->IsPlayer())// used directly by the player
	{
		if (!pActivator->pev->velocity.IsZero())
			Move(pActivator, 0);
	}
	else// if (!pActivator || !pActivator->IsPlayer())
	{
		if (FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE))
			CBreakable::Use(pActivator, pCaller, useType, value);

		//return;
	}
}

void CPushable::Touch(CBaseEntity *pOther)
{
	if (pOther->IsBSPModel())// XDM: worldspawn is BSP model too
	{
		if (FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE) && (pev->velocity.z <= -1024))
		{
			// Didn't do 'was in air' check. Let's allow breaking when sliding too fast?
			//ALERT(at_console, "velocity = %f %f %f\n", pev->velocity.x, pev->velocity.y, pev->velocity.z);
			this->CBreakable::Use(pOther, pOther, USE_ON, 1);
			return;
		}

		if (!pOther->IsMovingBSP())// but not moving...
			return;
	}

	if (pOther->IsProjectile())// XDM
		return;

	Move(pOther, 1);
}

void CPushable::Move(CBaseEntity *pOther, int push)
{
	entvars_t *pevToucher = pOther->pev;
	int playerTouch = 0;

	// Is entity standing on this pushable ?
	if (FBitSet(pevToucher->flags,FL_ONGROUND) && pevToucher->groundentity && VARS(pevToucher->groundentity) == pev)
	{
		// Only push if floating
		if (pev->waterlevel > WATERLEVEL_NONE)
			pev->velocity.z += pevToucher->velocity.z * 0.1f;

		return;
	}

	if (pOther->IsPlayer())
	{
		if (push && !FBitSet(pevToucher->button, IN_FORWARD|IN_USE))	// Don't push unless the player is pushing forward and NOT use (pull)
			return;
		playerTouch = 1;
	}

	float factor;
	if (playerTouch)
	{
		if (!FBitSet(pevToucher->flags, FL_ONGROUND))// Don't push away from jumping/falling players unless in water
		{
			if (pev->waterlevel == WATERLEVEL_NONE)
				return;
			else
				factor = 0.1f;
		}
		else
			factor = 1.0f;
	}
	else
		factor = 0.25f;

	pev->velocity.x += pevToucher->velocity.x * factor;
	pev->velocity.y += pevToucher->velocity.y * factor;

	vec_t length = pev->velocity.Length2D();//sqrt( pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y );
	if (push)// && (length > MaxSpeed()) )
	{
		float maxspeed = 400 - pev->friction;// VALVEHACK!
		if (length > maxspeed)
		{
			float k = maxspeed/length;
			pev->velocity.x *= k;
			pev->velocity.y *= k;
		}
	}
	if (playerTouch)
	{
		pevToucher->velocity.x = pev->velocity.x;
		pevToucher->velocity.y = pev->velocity.y;
		if ((gpGlobals->time - m_soundTime) > 0.7)
		{
			m_soundTime = gpGlobals->time;
			if (length > 0 && FBitSet(pev->flags, FL_ONGROUND))
			{
				m_lastSound = RANDOM_LONG(0,2);
				PlayMatPushSound(m_lastSound);// XDM
			}
			else
				StopMatPushSound(m_lastSound);// XDM
		}
	}
}

#if 0
void CPushable::StopSound(void)
{
	Vector dist = pev->oldorigin - pev->origin;
	if (dist.Length() <= 0)
		STOP_SOUND(edict(), CHAN_WEAPON, m_soundNames[m_lastSound]);
}
#endif

int CPushable::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (FBitSet(pev->spawnflags, SF_PUSH_BREAKABLE))
		return CBreakable::TakeDamage(pInflictor, pAttacker, flDamage, bitsDamageType);

	return 1;
}

void CPushable::PlayMatPushSound(int m_lastSound)
{
	if (m_Material == matNone/* || m_Material == matLastMaterial*/)
		return;

	CSoundEnt::InsertSound(bits_SOUND_WORLD, pev->origin, 128, 0.5);// XDM
	EMIT_SOUND(edict(), CHAN_BODY, gPushSounds[m_Material][m_lastSound], 0.5f, ATTN_NORM);
}

void CPushable::StopMatPushSound(int m_lastSound)
{
	if (m_Material == matNone/* || m_Material == matLastMaterial*/)
		return;

	STOP_SOUND(edict(), CHAN_BODY, gPushSounds[m_Material][m_lastSound]);
}

// XDM3038c
float CPushable::DamageForce(const float &damage)
{
	if (gMaterialWeight[m_Material] == 0.0f)
		return 0;
	else
		return CBreakable::DamageForce(damage) / gMaterialWeight[m_Material];
}





LINK_ENTITY_TO_CLASS(func_breakable_model, CBreakableModel);

void CBreakableModel::Spawn(void)
{
	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	CBreakable::Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	SET_MODEL(edict(), STRING(pev->model));

	/*if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY))
		pev->takedamage	= DAMAGE_NO;
	else
		pev->takedamage	= DAMAGE_YES;

	if (FBitSet(pev->spawnflags, SF_BREAK_TRIGGER_ONLY))
		SetTouchNull();
	else
		SetTouch(&CBreakable::BreakTouch);*/

	if (pev->health <= 0)
		pev->health = 20;

	m_angle = pev->angles.y;
}
