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

===== mortar.cpp ========================================================

  the "LaBuznik" mortar device

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "weapons.h"
#include "decals.h"
#include "soundent.h"
#include "skill.h"// XDM
#include "game.h"
#include "gamerules.h"

class CFuncMortarField : public CBaseToggle
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void KeyValue(KeyValueData *pkvd);
	// Bmodels don't go across transitions
	virtual int	ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
	void EXPORT FieldUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iszXController;
	int m_iszYController;
	float m_flSpread;
	float m_flDelay;
	int m_iCount;
	int m_fControl;
};

LINK_ENTITY_TO_CLASS( func_mortar_field, CFuncMortarField );

TYPEDESCRIPTION	CFuncMortarField::m_SaveData[] =
{
	DEFINE_FIELD( CFuncMortarField, m_iszXController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_iszYController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_flSpread, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMortarField, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMortarField, m_iCount, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncMortarField, m_fControl, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncMortarField, CBaseToggle );

void CFuncMortarField::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszXController"))
	{
		m_iszXController = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszYController"))
	{
		m_iszYController = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flSpread"))
	{
		m_flSpread = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fControl"))
	{
		m_fControl = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iCount"))
	{
		m_iCount = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else// XDM3038: WOW! What a bug!!
		CBaseToggle::KeyValue(pkvd);
}

// Drop bombs from above
void CFuncMortarField::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;// XDM3038c
	SET_MODEL(edict(), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetBits(pev->effects, EF_NODRAW);
	SetUse(&CFuncMortarField::FieldUse);
	Precache();
}

void CFuncMortarField::Precache(void)
{
	if (FStringNull(pev->noise))
		pev->noise = MAKE_STRING("weapons/mortar.wav");// custom fall sound

	PRECACHE_SOUND(STRINGV(pev->noise));

	if (!FStringNull(pev->noise1))// XDM3038c: custom explosion sound
		PRECACHE_SOUND(STRINGV(pev->noise1));

	UTIL_PrecacheOther("monster_mortar");// XDM: we can't override precached default sound in there
}

// If connected to a table, then use the table controllers, else hit where the trigger is.
void CFuncMortarField::FieldUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Vector vecStart(RANDOM_FLOAT(pev->mins.x, pev->maxs.x), RANDOM_FLOAT(pev->mins.y, pev->maxs.y), pev->maxs.z);

	if (m_fControl == 1)// Trigger Activator
	{
		if (pActivator != NULL)
		{
			vecStart.x = pActivator->pev->origin.x;
			vecStart.y = pActivator->pev->origin.y;
		}
	}
	else if (m_fControl == 2)// table
	{
		CBaseEntity *pController;

		if (!FStringNull(m_iszXController))
		{
			pController = UTIL_FindEntityByTargetname( NULL, STRING(m_iszXController));
			if (pController != NULL)
			{
				vecStart.x = pev->mins.x + pController->pev->ideal_yaw * (pev->size.x);
			}
		}
		if (!FStringNull(m_iszYController))
		{
			pController = UTIL_FindEntityByTargetname( NULL, STRING(m_iszYController));
			if (pController != NULL)
			{
				vecStart.y = pev->mins.y + pController->pev->ideal_yaw * (pev->size.y);
			}
		}
	}

	EMIT_SOUND_DYN(edict(), CHAN_VOICE, STRING(pev->noise), VOL_NORM, ATTN_NONE, 0, RANDOM_LONG(95,124));

	float t = 2.5;
	TraceResult tr;
	for (int i = 0; i < m_iCount; i++)
	{
		Vector vecSpot(vecStart);
		vecSpot.x += RANDOM_FLOAT(-m_flSpread, m_flSpread);
		vecSpot.y += RANDOM_FLOAT(-m_flSpread, m_flSpread);

		UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -1) * 4096, ignore_monsters, edict(), &tr);
		edict_t *pentOwner = NULL;
		if (pActivator)
			pentOwner = pActivator->edict();

		CBaseEntity *pMortar = Create("monster_mortar", tr.vecEndPos, g_vecZero, pentOwner);
		if (pMortar)
		{
			if (!FStringNull(pev->noise1))// XDM3038c: custom explosion sound
				pMortar->pev->noise = pev->noise1;

			pMortar->SetNextThink(t);// XDM3038a
			t += RANDOM_FLOAT(0.2, 0.5);
		}
		if (i == 0)
			CSoundEnt::InsertSound(bits_SOUND_DANGER, tr.vecEndPos, 400, 0.3);
	}
}


class CMortar : public CGrenade
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	void EXPORT MortarExplode(void);
};

LINK_ENTITY_TO_CLASS( monster_mortar, CMortar );

void CMortar::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	if (pev->dmg == 0)
		pev->dmg	 = gSkillData.DmgMortar;// XDM3038c

	SetThink(&CMortar::MortarExplode);
	DontThink();// XDM3038a
	SetTouchNull();// XDM3038c
	Precache();
}

void CMortar::Precache(void)
{
	if (FStringNull(pev->noise))
		pev->noise = MAKE_STRING("weapons/mortarhit.wav");// custom explosion sound

	PRECACHE_SOUND(STRINGV(pev->noise));
}

void CMortar::MortarExplode(void)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0.0f, 0.0f, g_psv_zmax->value), dont_ignore_monsters, edict(), &tr);
	// mortar beam
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(tr.vecEndPos.x);
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_SHORT(g_iModelIndexShockWave);
		WRITE_BYTE(0); // framerate
		WRITE_BYTE(10); // framerate
		WRITE_BYTE(1); // life
		WRITE_BYTE(40); // width
		WRITE_BYTE(0); // noise
		WRITE_BYTE(255); // r
		WRITE_BYTE(191); // g
		WRITE_BYTE(127); // b
		WRITE_BYTE(95); // brightness
		WRITE_BYTE(0); // speed
	MESSAGE_END();
	// blast circle
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BEAMTORUS);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 32);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 32 + pev->dmg * 2 / .2); // reach damage radius over .3 seconds
		WRITE_SHORT(g_iModelIndexShockWave);
		WRITE_BYTE(0); // startframe
		WRITE_BYTE(10); // framerate
		WRITE_BYTE(2); // life
		WRITE_BYTE(12); // width
		WRITE_BYTE(0); // noise
		WRITE_BYTE(255); // r
		WRITE_BYTE(159); // g
		WRITE_BYTE(127); // b
		WRITE_BYTE(127); // brightness
		WRITE_BYTE(10); // speed
	MESSAGE_END();
	}
	SetBits(pev->spawnflags, SF_NODECAL);// XDM: don't draw regular explosion decal
	UTIL_TraceLine(pev->origin + Vector(0.0f,0.0f,pev->dmg*2.0f), pev->origin - Vector(0.0f, 0.0f, g_psv_zmax->value), dont_ignore_monsters, edict(), &tr);
	if (tr.flFraction < 1.0f)// XDM3037: pull out of the wall a bit
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * 8.0f);

	// SQB?
	//tr.vecPlaneNormal.x *= -1.0f;
	//tr.vecPlaneNormal.y *= -1.0f;
	VectorAngles(tr.vecPlaneNormal, pev->angles);// XDM3037a: explosion code calculates decal direction
	Explode(pev->origin, DMG_BLAST, g_iModelIndexBigExplo3, 40, g_iModelIndexBigExplo3, 32, 8.0f, STRING(pev->noise));// XDM
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
	{
		//UTIL_ScreenShake(tr.vecEndPos, 25.0, 150.0, 1.0, 800);// additional
		UTIL_DecalTrace(&tr, DECAL_BLOW);
	}
	pev->health = 0;
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0.1);// XDM3038a
}

// XDM3038c: in case this gets called directly
void CMortar::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	pev->health = 0;
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(0);
}
