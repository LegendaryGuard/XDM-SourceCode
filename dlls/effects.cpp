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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "pm_materials.h"
#include "func_break.h"
#include "shake.h"
#include "studio.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "globals.h"
#include "sound.h"

// This file contains a proud hack collection written by VALVe

// UNDONE: beams with pev->target should act as triggers when tripped

// XDM: rewritten
#define SF_BUBBLES_START_OFF		0x0001

class CBubbling : public CBaseEntity
{
public:
	virtual void Spawn(void);
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	virtual int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void EXPORT FizzThink(void);
	static TYPEDESCRIPTION m_SaveData[];
	int m_density;
	int m_frequency;
	int m_state;
};

LINK_ENTITY_TO_CLASS(env_bubbles, CBubbling);

TYPEDESCRIPTION	CBubbling::m_SaveData[] =
{
	DEFINE_FIELD( CBubbling, m_density, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_frequency, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_state, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE(CBubbling, CBaseEntity);

void CBubbling::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_density = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		m_frequency = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "current"))
	{
		pev->speed = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CBubbling::Spawn(void)
{
	Precache();
	SET_MODEL(edict(), STRING(pev->model));	// Set size
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->renderamt = 0;// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;
	if (!FBitSet(pev->spawnflags, SF_BUBBLES_START_OFF))
	{
		SetThink(&CBubbling::FizzThink);
		SetNextThink(2.0);
		m_state = 1;
	}
	else
		m_state = 0;
}

void CBubbling::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (g_pCvarDeveloper && g_pCvarDeveloper->value <= 0.0f)
		return;

	if (ShouldToggle(useType, (m_state > 0)))
		m_state = !m_state;

	if (m_state)
	{
		SetThink(&CBubbling::FizzThink);
		SetNextThink(0.1);
	}
	else
	{
		SetThinkNull();
		DontThink();// XDM3038a
	}
}

void CBubbling::FizzThink(void)
{
	Vector vecBottomCenter((pev->absmax.x+pev->absmin.x)*0.5f, (pev->absmax.y+pev->absmin.y)*0.5f, pev->absmin.z+4.0f);// HACK: magic number??
	//Vector vecBottomCenter(Center());
	//TEST	UTIL_ShowBox(pev->origin, pev->mins, pev->maxs, 10, 0,255,0);
	//TEST	UTIL_ShowLine(vecBottomCenter, vecBottomCenter + Vector(0,0,16), 2.0, 0,255,0);
	Vector vecQVolume((pev->absmax.x-pev->absmin.x)*0.5f, (pev->absmax.y-pev->absmin.y)*0.5f, 0.5f);// resulting thickness is x2
	FX_BubblesBox(vecBottomCenter, vecQVolume, m_density*3);// XDM3035c: HL bubbles only spawn at bottom
	//OBSOLETE	UTIL_ShowBox(vecBottomCenter, -vecQVolume, vecQVolume, 2.0, 255,0,0);

	if (m_frequency > 19)
		SetNextThink(0.5);
	else
		SetNextThink(2.5 - (0.1f * m_frequency));
}









// --------------------------------------------------
//
// Beams
// This is a CUSTOM ENTITY which means that it uses entvars in a completely different way (hack)
// This code is so hacky ugly and buggy that you should not attempt to touch it!
//
// --------------------------------------------------

LINK_ENTITY_TO_CLASS( beam, CBeam );

void CBeam::Spawn(void)
{
//#if defined (_DEBUG)
//	if (!FStringNull(pev->target))// XDM3038c
//		conprintf(1, "%s[%d] \"%s\" has target \"%s\"\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->target));
//#endif

	pev->solid = SOLID_NOT; // Remove model & collisions
	Precache();
}

void CBeam::Precache(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	if (pev->owner)
		SetStartEntity(ENTINDEX(pev->owner));
	if (pev->aiment)
		SetEndEntity(ENTINDEX(pev->aiment));
}

int CBeam::ObjectCaps(void)
{
	return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | (FBitSet(pev->spawnflags, SF_BEAM_TEMPORARY)?FCAP_DONT_SAVE:0);
}

void CBeam::SetStartEntity(int entityIndex)
{
	pev->sequence = (entityIndex & 0x0FFF) | ((pev->sequence&0xF000)<<12);
	pev->owner = INDEXENT(entityIndex);
}

void CBeam::SetEndEntity(int entityIndex)
{
	pev->skin = (entityIndex & 0x0FFF) | ((pev->skin&0xF000)<<12);
	pev->aiment = INDEXENT(entityIndex);
}

// These don't take attachments into account
const Vector &CBeam::GetStartPos(void)
{
	if (GetType() == BEAM_ENTS)
	{
		edict_t *pent = INDEXENT(GetStartEntity());
		if (pent)
			return pent->v.origin;
	}
	return pev->origin;
}

const Vector &CBeam::GetEndPos(void)
{
	int type = GetType();
	if (type == BEAM_POINTS || type == BEAM_HOSE)
		return pev->angles;

	edict_t *pent = INDEXENT(GetEndEntity());
	if (pent)
		return pent->v.origin;

	return pev->angles;
}

CBeam *CBeam::BeamCreate(const char *pSpriteName, int width)
{
	// Create a new entity with CBeam private data
	// ? CBeam *pBeam = Create("beam", g_vecZero, g_vecZero);
	CBeam *pBeam = GetClassPtr((CBeam *)NULL, "beam");// XDM3037a
	if (pBeam)
		pBeam->BeamInit(pSpriteName, width);

	return pBeam;
}

// WARNING: requires ALLOCATED/STATIC string in argument!
void CBeam::BeamInit(const char *pSpriteName, int width)
{
	pev->flags |= FL_CUSTOMENTITY;
	SetColor(255, 255, 255);
	SetBrightness(255);
	SetNoise(0);
	SetFrame(0);
	SetScrollRate(0);
	if (pSpriteName)
	{
		pev->model = MAKE_STRING(pSpriteName);
		SetTexture(PRECACHE_MODEL(STRINGV(pev->model)));// XDM3038: <- pev->modelindex
	}
	SetWidth(width);
	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
}

void CBeam::PointsInit( const Vector &start, const Vector &end )
{
	SetType( BEAM_POINTS );
	SetStartPos( start );
	SetEndPos( end );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::HoseInit( const Vector &start, const Vector &direction )
{
	SetType( BEAM_HOSE );
	SetStartPos( start );
	SetEndPos( direction );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::PointEntInit( const Vector &start, int endIndex )
{
	SetType( BEAM_ENTPOINT );
	SetStartPos( start );
	SetEndEntity( endIndex );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::EntPointInit( int endIndex, const Vector &end )// XDM: TODO: this does not seem to work
{
	SetType(BEAM_ENTPOINT);
	SetStartEntity( endIndex );
	SetEndPos( end );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::EntsInit( int startIndex, int endIndex )
{
	SetType( BEAM_ENTS );
	SetStartEntity( startIndex );
	SetEndEntity( endIndex );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::RelinkBeam(void)
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();
	pev->mins.x = min( startPos.x, endPos.x );
	pev->mins.y = min( startPos.y, endPos.y );
	pev->mins.z = min( startPos.z, endPos.z );
	pev->maxs.x = max( startPos.x, endPos.x );
	pev->maxs.y = max( startPos.y, endPos.y );
	pev->maxs.z = max( startPos.z, endPos.z );
	pev->mins = pev->mins - pev->origin;
	pev->maxs = pev->maxs - pev->origin;
	UTIL_SetSize(this, pev->mins, pev->maxs);
	UTIL_SetOrigin(this, pev->origin);
}

#if 0
void CBeam::SetObjectCollisionBox(void)
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	pev->absmin.x = min( startPos.x, endPos.x );
	pev->absmin.y = min( startPos.y, endPos.y );
	pev->absmin.z = min( startPos.z, endPos.z );
	pev->absmax.x = max( startPos.x, endPos.x );
	pev->absmax.y = max( startPos.y, endPos.y );
	pev->absmax.z = max( startPos.z, endPos.z );
}
#endif

/* unused void CBeam::TriggerTouch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(TriggerTouch);
	if ( pOther->pev->flags & (FL_CLIENT | FL_MONSTER) )
	{
		if ( pev->owner )
		{
			CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
			if (pOwner)
				pOwner->Use( pOther, this, USE_TOGGLE, 0 );
		}
		conprintf(2, "CBeam::TriggerTouch() Firing targets!!!\n");
	}
}*/

CBaseEntity *CBeam::RandomTargetname(const char *szName)
{
	int total = 0;
	CBaseEntity *pEntity = NULL;
	CBaseEntity *pNewEntity = NULL;
	while ((pNewEntity = UTIL_FindEntityByTargetname(pNewEntity, szName)) != NULL)
	{
		++total;
		if (RANDOM_LONG(0,total-1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}

void CBeam::DoSparks(const Vector &start, const Vector &end)
{
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// XDM3035
	{
		if (FBitSet(pev->spawnflags, SF_BEAM_SPARKSTART))
			UTIL_Sparks(start);

		if (FBitSet(pev->spawnflags, SF_BEAM_SPARKEND))
			UTIL_Sparks(end);
	}
}

void CBeam::BeamDamage(TraceResult *ptr)// XDM3035
{
	RelinkBeam();
	if (pev->dmg > 0.0f && ptr->flFraction != 1.0 && ptr->pHit != NULL)// XDM3038c
	{
		CBaseEntity *pHit = CBaseEntity::Instance(ptr->pHit);
		if (pHit)
		{
			CBaseEntity *pAttacker = GetDamageAttacker();// XDM3037
			ClearMultiDamage();
			pHit->TraceAttack(pAttacker, pev->dmg * (gpGlobals->time - pev->dmgtime), (ptr->vecEndPos - pev->origin).Normalize(), ptr, DMG_ENERGYBEAM );
			ApplyMultiDamage(this, pAttacker);
			if (FBitSet(pev->spawnflags, SF_BEAM_DECALS) && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))
			{
				if (pHit->IsBSPModel())
					UTIL_DecalTrace(ptr, DECAL_BIGSHOT1 + RANDOM_LONG(0,4));
			}
		}
	}
	pev->dmgtime = gpGlobals->time;
}

// XDM
void CBeam::Expand(float scaleSpeed, float fadeSpeed)
{
	pev->speed = scaleSpeed;
	pev->health = fadeSpeed;
	SetThink(&CBeam::ExpandThink);
	SetNextThink(0);// XDM3038a: now
	pev->ltime = gpGlobals->time;
}

void CBeam::ExpandThink(void)
{
	float frametime = gpGlobals->time - pev->ltime;
	//if (pev->body < 255)
		pev->body++;

	pev->scale += pev->speed * frametime;// width
	pev->renderamt -= pev->health * frametime;

	if (pev->renderamt <= 0)
	{
		pev->renderamt = 0;
		if (FBitSet(pev->flags, FL_GODMODE))// XDM3034
		{
			SetBits(pev->effects, EF_NODRAW);
			SetThinkNull();
			DontThink();
		}
		else
		{
			Destroy();
			return;
		}
	}
	else
	{
		SetNextThink(0.1);
		pev->ltime = gpGlobals->time;
	}
}

/*CBaseEntity *CBeam::GetTripEntity(TraceResult *ptr)
{
	CBaseEntity *pTrip = NULL;

	if (ptr->flFraction == 1.0 || ptr->pHit == NULL)
		return NULL;

	pTrip = CBaseEntity::Instance(ptr->pHit);
	if (pTrip == NULL)
		return NULL;

	if (FStringNull(pev->netname))
	{
		if (pTrip->pev->flags & (FL_CLIENT | FL_MONSTER))
			return pTrip;
		else
			return NULL;
	}
	else if (FClassnameIs(pTrip->pev, STRING(pev->netname)))
		return pTrip;
	else if (FStrEq(STRING(pTrip->pev->targetname), STRING(pev->netname)))
		return pTrip;
	else
		return NULL;
}*/



LINK_ENTITY_TO_CLASS( env_lightning, CLightning );
LINK_ENTITY_TO_CLASS( env_beam, CLightning );

TYPEDESCRIPTION	CLightning::m_SaveData[] =
{
	DEFINE_FIELD( CLightning, m_active, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_iszStartEntity, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_iszEndEntity, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_life, FIELD_FLOAT ),
	DEFINE_FIELD( CLightning, m_boltWidth, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_noiseAmplitude, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_brightness, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_speed, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_restrike, FIELD_FLOAT ),
	// XDM3038c: FIX: model index! DEFINE_FIELD( CLightning, m_spriteTexture, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_iszSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_frameStart, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_radius, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CLightning, CBeam );

void CLightning::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "LightningStart"))
	{
		m_iszStartEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "LightningEnd"))
	{
		m_iszEndEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "life"))
	{
		m_life = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "BoltWidth"))
	{
		m_boltWidth = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		m_noiseAmplitude = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TextureScroll"))
	{
		m_speed = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "StrikeTime"))
	{
		m_restrike = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "framestart"))
	{
		m_frameStart = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Radius"))
	{
		m_radius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBeam::KeyValue( pkvd );
}

void CLightning::Spawn(void)
{
	if (FStringNull(m_iszSpriteName))
	{
		conprintf(0, "Design error: %s[%d] \"%s\" at (%g %g %g) without sprite!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		SetThink(&CLightning::SUB_Remove);
		SetNextThink(0);
		return;
	}

	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	CBeam::Spawn();
	//pev->solid = SOLID_NOT;							// Remove model & collisions
	//Precache();
	pev->dmgtime = gpGlobals->time;
	// XDM3038: TESTME: all code moved to Activate()
}

void CLightning::Precache(void)
{
	m_spriteTexture = PRECACHE_MODEL(STRINGV(m_iszSpriteName));
	CBeam::Precache();
}

void CLightning::Activate(void)
{
	if (ServerSide())
	{
		BeamUpdateVars();
		if (pev->dmg > 0 || !FStringNull(pev->targetname))// XDM3038c: TODO: TESTME: FIX? There was "pev->target" here! A typo?
		{
			SetThink(&CLightning::UpdateThink);
			SetNextThink(0.1);
		}
		else
			SetThinkNull();

		if (!FStringNull(pev->targetname))
		{
			if (FBitSet(pev->spawnflags, SF_BEAM_STARTON))
			{
				m_active = 1;
			}
			else
			{
				pev->effects = EF_NODRAW;
				m_active = 0;
				DontThink();// XDM3038a
			}
			SetUse(&CLightning::ToggleUse);
		}
	}
	else
	{
		m_active = 0;
		if (!FStringNull(pev->targetname))
		{
			SetUse(&CLightning::StrikeUse);
		}
		if (FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_BEAM_STARTON))
		{
			SetThink(&CLightning::StrikeThink);
			SetNextThink(3.0);
		}
	}
	//if ( ServerSide() )
	//	BeamUpdateVars();
}

// BUGBUG!!! Every toggle creates additional beam!!!
void CLightning::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	DBG_PRINT_ENT_USE(ToggleUse);
	if (!ShouldToggle(useType, (m_active > 0)))
		return;

	if (m_active)
	{
		m_active = 0;
		SetBits(pev->effects, EF_NODRAW);
		DontThink();// XDM3038a
	}
	else
	{
		if (pActivator)// XDM3035
			m_hOwner = pActivator;
		else
			m_hOwner = NULL;

		m_active = 1;
		ClearBits(pev->effects, EF_NODRAW);
		DoSparks(GetStartPos(), GetEndPos());
		if (pev->dmg > 0)
		{
			SetNextThink(0);// XDM3038a: now
			pev->dmgtime = gpGlobals->time;
		}
	}
}

void CLightning::StrikeUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	DBG_PRINT_ENT_USE(StrikeUse);
	if (!ShouldToggle(useType, (m_active > 0)))
		return;

	if (m_active)
	{
		m_active = 0;
		SetThinkNull();
		DontThink();// XDM3038a
		SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);// XDM3038: everyone should be notified of the change
	}
	else
	{
		if (pActivator)// XDM3035
			m_hOwner = pActivator;
		else
			m_hOwner = NULL;

		// Update inside
		SetThink(&CLightning::StrikeThink);
		SetNextThink(0.1);
	}

	if ( !FBitSet( pev->spawnflags, SF_BEAM_TOGGLE ) )
		SetUseNull();
}

void CLightning::StrikeThink(void)
{
	/*if (!g_ServerActive)// XDM3038: prevent firing during loading
	{
		pev->nextthink += 1.0f;
		return;
	}*/
	/*if (pev->nextthink > gpGlobals->time)// FIXME BUGBUG somehow, after Restore() nextthink == 0 and it causes StrikeThink to run too soon 
	{
		conprintf(0, "DBG: CLightning(%s %s %s)::StrikeThink() THINKING AHEAD OF TIME!!\n", STRING(pev->targetname), STRING(m_iszStartEntity), STRING(m_iszEndEntity));// XDM
		return;
	}*/
	if ( m_life != 0 )
	{
		if (FBitSet(pev->spawnflags, SF_BEAM_RANDOM))
			SetNextThink(m_life + RANDOM_FLOAT(0, m_restrike));
		else
			SetNextThink(m_life + m_restrike);
	}
	m_active = 1;

	if (!FStringNull(pev->target))// XDM3038c
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
		// TODO: derive CBeam from CBaseDelay UseTargets(pev->message, m_iszKillTarget, m_hActivator.Get()?(CBaseEntity *)m_hActivator:this, pCaller, USE_TOGGLE, 0);// this includes killtarget

	if (FStringNull(m_iszEndEntity))
	{
		if (FStringNull(m_iszStartEntity))
		{
			RandomArea();
		}
		else
		{
			CBaseEntity *pStart = RandomTargetname( STRING(m_iszStartEntity) );
			if (pStart != NULL)
				RandomPoint( pStart->pev->origin );
			else
				conprintf(0, "%s[%d] %s: unknown start entity: \"%s\"!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(m_iszStartEntity));
		}
		return;
	}

	CBaseEntity *pStart = RandomTargetname(STRING(m_iszStartEntity));
	CBaseEntity *pEnd = RandomTargetname(STRING(m_iszEndEntity));
	if (pStart != NULL && pEnd != NULL)
	{
		if (IsPointEntity(pStart) || IsPointEntity(pEnd))
		{
			if (FBitSet(pev->spawnflags, SF_BEAM_RING))
				return;// don't work
		}
		SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);// XDM3038
		DoSparks(pStart->pev->origin, pEnd->pev->origin);
		if (pev->dmg > 0)
		{
			TraceResult tr;
			UTIL_TraceLine(pStart->pev->origin, pEnd->pev->origin, dont_ignore_monsters, NULL, &tr);
			pev->dmgtime = gpGlobals->time - 1.0f;// do full damage now
			BeamDamage(&tr);// XDM3037
		}
	}
}

void CLightning::UpdateThink(void)
{
	if (pev->dmg > 0)
		DamageThink();

	// TODO: trip beam -> target
}

void CLightning::DamageThink(void)
{
	SetNextThink(0.1);
	TraceResult tr;
	UTIL_TraceLine(GetStartPos(), GetEndPos(), dont_ignore_monsters, NULL, &tr);
	BeamDamage(&tr);
}

// This is called from RandomArea() and RandomPoint() only. Simple beam doesn't send any MSGs.
void CLightning::Zap(const Vector &vecSrc, const Vector &vecDest)
{
	//conprintf(0, "DBG: CLightning::Zap()\n");// XDM
#if 1
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(vecSrc.x);
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		WRITE_COORD(vecDest.x);
		WRITE_COORD(vecDest.y);
		WRITE_COORD(vecDest.z);
		WRITE_SHORT(m_spriteTexture);
		WRITE_BYTE(m_frameStart); // framestart
		WRITE_BYTE((int)pev->framerate); // framerate
		WRITE_BYTE((int)(m_life*10.0f)); // life
		WRITE_BYTE(m_boltWidth);  // width
		WRITE_BYTE(m_noiseAmplitude);   // noise
		WRITE_BYTE((int)pev->rendercolor.x);	// r, g, b
		WRITE_BYTE((int)pev->rendercolor.y);	// r, g, b
		WRITE_BYTE((int)pev->rendercolor.z);	// r, g, b
		WRITE_BYTE(pev->renderamt);	// brightness
		WRITE_BYTE(m_speed);		// speed
	MESSAGE_END();
#else
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_LIGHTNING);
		WRITE_COORD(vecSrc.x);
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		WRITE_COORD(vecDest.x);
		WRITE_COORD(vecDest.y);
		WRITE_COORD(vecDest.z);
		WRITE_BYTE((int)(m_life*10.0));
		WRITE_BYTE(m_boltWidth);
		WRITE_BYTE(m_noiseAmplitude);
		WRITE_SHORT(m_spriteTexture);
	MESSAGE_END();
#endif
	DoSparks( vecSrc, vecDest );
}

bool CLightning::ServerSide(void)
{
	if (m_life == 0 && !FBitSet(pev->spawnflags, SF_BEAM_RING))
		return true;

	return false;
// test	return true;
}

void CLightning::RandomArea(void)
{
	size_t iLoops = 0;
	Vector vecSrc(pev->origin);
	TraceResult tr1;
	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecDir1( RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		vecDir1.NormalizeSelf();
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, edict(), &tr1 );

		if (tr1.flFraction == 1.0)
			continue;

		Vector vecDir2;
		do {
			vecDir2.Set(RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		} while (DotProduct(vecDir1, vecDir2) > 0);
		vecDir2.NormalizeSelf();
		TraceResult tr2;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir2 * m_radius, ignore_monsters, edict(), &tr2 );

		if (tr2.flFraction == 1.0)
			continue;

		if ((tr1.vecEndPos - tr2.vecEndPos).Length() < m_radius * 0.1)
			continue;

		UTIL_TraceLine( tr1.vecEndPos, tr2.vecEndPos, ignore_monsters, edict(), &tr2 );

		if (tr2.flFraction != 1.0)
			continue;

		Zap( tr1.vecEndPos, tr2.vecEndPos );
		break;
	}
}

void CLightning::RandomPoint( Vector &vecSrc )
{
	size_t iLoops = 0;
	TraceResult tr1;
	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecDir1( RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		vecDir1.NormalizeSelf();
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, edict(), &tr1 );

		if ((tr1.vecEndPos - vecSrc).Length() < m_radius * 0.1)
			continue;

		if (tr1.flFraction == 1.0)
			continue;

		Zap( vecSrc, tr1.vecEndPos );
		break;
	}
}

void CLightning::BeamUpdateVars(void)
{
	CBaseEntity *pStart = UTIL_FindEntityByTargetname ( NULL, STRING(m_iszStartEntity) );
	CBaseEntity *pEnd   = UTIL_FindEntityByTargetname ( NULL, STRING(m_iszEndEntity) );
	if (!pStart || !pEnd) return;
	int pointStart, pointEnd;
	pointStart = IsPointEntity( pStart );
	pointEnd = IsPointEntity( pEnd );
	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
	pev->flags |= FL_CUSTOMENTITY;
	pev->model = m_iszSpriteName;
	SetTexture( m_spriteTexture );
	int beamType = BEAM_ENTS;
	if ( pointStart || pointEnd )
	{
		if ( !pointStart )	// One point entity must be in pStart
		{
			CBaseEntity *pTemp;
			// Swap start & end
			pTemp = pStart;
			pStart = pEnd;
			pEnd = pTemp;
			int swap = pointStart;
			pointStart = pointEnd;
			pointEnd = swap;
		}
		if ( !pointEnd )
			beamType = BEAM_ENTPOINT;
		else
			beamType = BEAM_POINTS;
	}

	SetType( beamType );
	if ( beamType == BEAM_POINTS || beamType == BEAM_ENTPOINT || beamType == BEAM_HOSE )
	{
		SetStartPos( pStart->pev->origin );
		if ( beamType == BEAM_POINTS || beamType == BEAM_HOSE )
			SetEndPos( pEnd->pev->origin );
		else
			SetEndEntity( ENTINDEX(ENT(pEnd->pev)) );
	}
	else
	{
		SetStartEntity( ENTINDEX(ENT(pStart->pev)) );
		SetEndEntity( ENTINDEX(ENT(pEnd->pev)) );
	}

	RelinkBeam();

	SetWidth( m_boltWidth );
	SetNoise( m_noiseAmplitude );
	SetFrame( m_frameStart );
	SetScrollRate( m_speed );
	if (FBitSet(pev->spawnflags, SF_BEAM_SHADEIN))
		SetFlags( BEAM_FSHADEIN );
	else if (FBitSet(pev->spawnflags, SF_BEAM_SHADEOUT))
		SetFlags( BEAM_FSHADEOUT );
	else if (FBitSet(pev->spawnflags, SF_BEAM_SOLID))
		SetFlags( BEAM_FSOLID );
}

// XDM3038: new mechanism survives save/restore/join/leave
int CLightning::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (!pClient && msgtype == MSG_ONE)// a client has connected and needs an update
		return 0;

	if (!m_active || ServerSide())
		return 0;

	CBaseEntity *pStart = RandomTargetname(STRING(m_iszStartEntity));
	CBaseEntity *pEnd = RandomTargetname(STRING(m_iszEndEntity));
	if (pStart != NULL && pEnd != NULL)
	{
		if (IsPointEntity(pStart) || IsPointEntity(pEnd))
		{
			if (FBitSet(pev->spawnflags, SF_BEAM_RING))
				return 0;// don't work
		}
		Vector vecMiddle(pEnd->pev->origin);
		vecMiddle -= pStart->pev->origin;
		MESSAGE_BEGIN(msgtype, svc_temp_entity, vecMiddle, (pClient == NULL)?NULL : pClient->edict());
			if (IsPointEntity(pStart) || IsPointEntity(pEnd))
			{
				if (!IsPointEntity(pEnd))// One point entity must be in pEnd
				{
					CBaseEntity *pTemp = pStart;
					pStart = pEnd;
					pEnd = pTemp;
				}
				if (!IsPointEntity(pStart))// One sided
				{
					WRITE_BYTE(TE_BEAMENTPOINT);
					WRITE_SHORT(pStart->entindex());
					WRITE_COORD(pEnd->pev->origin.x);
					WRITE_COORD(pEnd->pev->origin.y);
					WRITE_COORD(pEnd->pev->origin.z);
				}
				else
				{
					WRITE_BYTE(TE_BEAMPOINTS);
					WRITE_COORD(pStart->pev->origin.x);
					WRITE_COORD(pStart->pev->origin.y);
					WRITE_COORD(pStart->pev->origin.z);
					WRITE_COORD(pEnd->pev->origin.x);
					WRITE_COORD(pEnd->pev->origin.y);
					WRITE_COORD(pEnd->pev->origin.z);
				}
			}
			else
			{
				if (FBitSet(pev->spawnflags, SF_BEAM_RING))
					WRITE_BYTE(TE_BEAMRING);
				else
					WRITE_BYTE(TE_BEAMENTS);

				WRITE_SHORT(pStart->entindex());   
				WRITE_SHORT(pEnd->entindex());
			}
			WRITE_SHORT(m_spriteTexture);
			WRITE_BYTE(m_frameStart);			// framestart
			WRITE_BYTE((int)pev->framerate);		// framerate
			int remlife;// x10 in msg units
			if (m_life <= 0.0f)
				remlife = 0;
			else
				remlife = (int)(m_life * 10.0f);
//			else if (sendcase == SCD_CLIENTUPDATEREQUEST || sendcase == SCD_CLIENTRESTORE)
// numbers are wrong				remlife = (int)((pev->nextthink - gpGlobals->time)*10.0f);// send actual remaining "on" life

			WRITE_BYTE(remlife);	// life
			WRITE_BYTE(m_boltWidth);			// width
			WRITE_BYTE(m_noiseAmplitude);		// noise
			WRITE_BYTE((int)pev->rendercolor.x);	// r, g, b
			WRITE_BYTE((int)pev->rendercolor.y);	// r, g, b
			WRITE_BYTE((int)pev->rendercolor.z);	// r, g, b
			WRITE_BYTE(pev->renderamt);			// brightness
			WRITE_BYTE(m_speed);				// speed
		MESSAGE_END();
		return 1;
	}
	return 0;
}













LINK_ENTITY_TO_CLASS( env_laser, CLaser );

TYPEDESCRIPTION	CLaser::m_SaveData[] =
{
	DEFINE_FIELD( CLaser, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CLaser, m_iszSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLaser, m_firePosition, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CLaser, CBeam );

void CLaser::Spawn(void)
{
	if (FStringNull(pev->model))
	{
		SetThink(&CLaser::SUB_Remove);
		return;
	}
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();

	SetThink(&CLaser::StrikeThink);
	pev->flags |= FL_CUSTOMENTITY;

	PointsInit( pev->origin, pev->origin );

	if (!m_pSprite && m_iszSpriteName)
		m_pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteName), pev->origin, TRUE );
	else
		m_pSprite = NULL;

	if (m_pSprite)
	{
		m_pSprite->SetTransparency( kRenderGlow, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx );
		m_pSprite->pev->framerate = pev->framerate;// XDM3035
	}

	if (!FStringNull(pev->targetname) && !FBitSet(pev->spawnflags, SF_BEAM_STARTON))
		TurnOff();
	else
		TurnOn();
}

void CLaser::Precache(void)
{
	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	if (m_iszSpriteName)
		PRECACHE_MODEL(STRINGV(m_iszSpriteName));
}

void CLaser::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "LaserTarget"))
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "width"))
	{
		SetWidth( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		SetNoise( atoi(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TextureScroll"))
	{
		SetScrollRate( atoi(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "EndSprite"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "framestart"))
	{
		pev->frame = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBeam::KeyValue( pkvd );
}

void CLaser::UpdateOnRemove(void)// XDM3035a: remove child sprite along with the beam
{
	if (m_pSprite)
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = NULL;
	}
	CBeam::UpdateOnRemove();// XDM3037: this is just right
}

int CLaser::IsOn(void)
{
	if (FBitSet(pev->effects, EF_NODRAW))
		return 0;

	return 1;
}

void CLaser::TurnOff(void)
{
	SetBits(pev->effects, EF_NODRAW);
	DontThink();// XDM3038a

	if (m_pSprite)
		m_pSprite->TurnOff();
}

void CLaser::TurnOn(void)
{
	ClearBits(pev->effects, EF_NODRAW);
	if (m_pSprite)
		m_pSprite->TurnOn();

	pev->dmgtime = gpGlobals->time;
	SetNextThink(0);// XDM3038a: now
}

void CLaser::TurnOffThink(void)// XDM3035
{
	SetThinkNull();
	DontThink();// XDM3038a

	if (IsOn())
		TurnOff();
}

void CLaser::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	int active = IsOn();
	if (!ShouldToggle(useType, (active > 0)))
		return;

	if (pActivator)// XDM3037
		m_hOwner = pActivator;
	else
		m_hOwner = NULL;

	if (active)
		TurnOff();
	else
		TurnOn();
}

void CLaser::FireAtPoint(TraceResult &tr)
{
	SetEndPos(tr.vecEndPos);
	if (m_pSprite)
		UTIL_SetOrigin(m_pSprite, tr.vecEndPos);

	BeamDamage(&tr);
	DoSparks(GetStartPos(), tr.vecEndPos);
}

void CLaser::StrikeThink(void)
{
	CBaseEntity *pEnd = RandomTargetname(STRING(pev->message));
	if (pEnd)
		m_firePosition = pEnd->pev->origin;

	TraceResult tr;
	UTIL_TraceLine(pev->origin, m_firePosition, dont_ignore_monsters, NULL, &tr);
	FireAtPoint(tr);// XDM3037
	SetNextThink(0.1);
}



//=================================================================
// CSprite - a general purpose sprite
// pev->impulse - ON/OFF
//=================================================================
LINK_ENTITY_TO_CLASS( env_sprite, CSprite );

TYPEDESCRIPTION	CSprite::m_SaveData[] =
{
	DEFINE_FIELD( CSprite, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CSprite, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSprite, CPointEntity );

// WARNING: Use only for dynamic sprites, not to place sprites in the world!
CSprite *CSprite::SpriteCreate(const char *pSpriteName, const Vector &origin, bool animate)
{
	CSprite *pSprite = GetClassPtr((CSprite *)NULL, "env_sprite");// XDM
	if (pSprite)
	{
		pSprite->pev->button = 1;// XDM3035a: IMPORTANT: mark as dynamically created sprite
		pSprite->pev->model = MAKE_STRING(pSpriteName);
		pSprite->pev->origin = origin;
		pSprite->pev->solid = SOLID_NOT;
		pSprite->pev->movetype = MOVETYPE_NOCLIP;
		pSprite->Spawn();
		if (animate)
			pSprite->TurnOn();
	}
	return pSprite;
}

bool CSprite::ValidateModel(void)// XDM3037a
{
	if (FStringNull(pev->model))
	{
		conprintf(0, "Design error: %s[%d] \"%s\" at (%g %g %g) without sprite!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		return false;
	}
	if (!UTIL_FileExtensionIs(STRING(pev->model), ".spr"))
	{
		conprintf(0, "Design error: %s[%d] \"%s\": \"%s\" is not a sprite!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->model));
		//test		CBaseEntity::CreateCopy(MAKE_STRING("env_static"), this, pev->spawnflags);
		return false;
	}
	return true;
}

void CSprite::KeyValue(KeyValueData *pkvd)// XDM
{
	if (FStrEq(pkvd->szKeyName, "life"))
	{
		pev->dmgtime = gpGlobals->time + atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CSprite::Precache(void)
{
	if (pev->button == 0)// placed by designer
		ClearBits(pev->spawnflags, SF_SPRITE_TEMPORARY|SF_SPRITE_CLIENTONLY);// prevent level designers from setting these flags

	if (pev->button || FBitSet(pev->spawnflags, SF_SPRITE_TEMPORARY) || sv_serverstaticents.value > 0.0f || !FStringNull(pev->targetname) || pev->rendermode == kRenderGlow)// XDM3037: glow is impossible to make
	{
		ClearBits(pev->spawnflags, SF_SPRITE_CLIENTONLY);
		//CPointEntity::Precache();
	}
	else
		SetBits(pev->spawnflags, SF_SPRITE_CLIENTONLY);

	//if (!FBitSet(pev->spawnflags, SF_SPRITE_CLIENTONLY))// UNDONE: client-only sprites
	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));// XDM3037a: model already validated

	// Reset attachment after save/restore
	if (pev->aiment)
		SetAttachment(pev->aiment, pev->body);
	else
	{
		// Clear attachment
		pev->skin = 0;
		pev->body = 0;
	}
}

// NOT called during respawn!
// pev->button == 1 means a dynamically created sprite (not part of map design)
void CSprite::Spawn(void)
{
	if (!ValidateModel())
	{
		pev->flags = FL_KILLME;
		return;
	}

	Precache();
	pev->movetype		= MOVETYPE_NONE;
	pev->solid			= SOLID_NOT;
	//pev->frame			= 0;
	pev->effects		= EF_NOINTERP;
	UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);// XDM3035a: IMPORTANT!
	pev->health			= 65535.0;
	pev->takedamage		= DAMAGE_NO;
	pev->deadflag		= DEAD_NO;

	if (!FBitSet(pev->spawnflags, SF_SPRITE_CLIENTONLY))
	{
		SET_MODEL(edict(), STRING(pev->model));
		m_maxFrame = (float)MODEL_FRAMES(pev->modelindex)-1;
	}
	else
		SetBits(pev->effects, EF_NODRAW);

	if (StartOn())
		TurnOn();
	else
		TurnOff();

	// HACK: Worldcraft only sets y rotation, copy to Z
	if (pev->angles.y != 0 && pev->angles.z == 0)
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
}

bool CSprite::StartOn(void)// XDM3038c
{
	if (FBitSet(pev->spawnflags, SF_SPRITE_STARTON) || FStringNull(pev->targetname))
		return true;

	return false;
}

int CSprite::ObjectCaps(void)
{
	return (CPointEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | ((FBitSet(pev->spawnflags, SF_SPRITE_TEMPORARY)?FCAP_DONT_SAVE:0));
}

void CSprite::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	int on = pev->impulse;//!(pev->effects & EF_NODRAW);
	if (ShouldToggle(useType, (on > 0)))
	{
		if (on > 0)
			TurnOff();
		else
			TurnOn();

		SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
	}
}

// XDM3035a: awesome traffic economy on some maps!
// Called by clients connecting to the game
int CSprite::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	if (!FBitSet(pev->spawnflags, SF_SPRITE_CLIENTONLY))// server entity mode
		return 0;

	if (msgtype == MSG_ONE)// a client has connected and needs an update
	{
		if (pClient == NULL)
			return 0;

		if (pClient->IsBot())// bots don't need sprites =)
			return 0;
	}
	else if (msgtype == MSG_BROADCAST)
		msgtype = MSG_ALL;// we need this fix in case someone will try to put this update into unreliable message stream

#if defined (_DEBUG)
	//if (msgtype == MSG_ALL && sendcase != SCD_GLOBALUPDATE)
		conprintf(2, "%s[%d] %s: s2c(%hd) -> \"%s\"[%d] @(%g %g %g) (%g %g %g)\n", STRING(pev->classname), entindex(), STRING(pev->targetname), sendcase, STRING(pev->model), pev->modelindex, pev->origin.x, pev->origin.y, pev->origin.z, pev->angles.x, pev->angles.y, pev->angles.z);
#endif
	MESSAGE_BEGIN(msgtype, gmsgStaticSprite, pev->origin, (pClient == NULL)?NULL : pClient->edict());
		WRITE_SHORT(entindex());
		if (IsRemoving())// XDM3037 // SCD_ENTREMOVE
			WRITE_SHORT(0);
		else if (pev->modelindex == 0)// XDM3038b: not precached
			WRITE_SHORT(1);
		else
			WRITE_SHORT(pev->modelindex);

		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_ANGLE(pev->angles.x);
		WRITE_ANGLE(pev->angles.y);
		WRITE_ANGLE(pev->angles.z);
		WRITE_BYTE(pev->rendermode);
		WRITE_BYTE(pev->renderfx);
		WRITE_BYTE(pev->rendercolor.x);
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
		WRITE_BYTE(pev->renderamt);
		// XDM3037a: new method		WRITE_BYTE(pev->effects);
	if (pev->impulse > 0)// XDM3037a: ON
		WRITE_COORD(pev->scale);
	else
		WRITE_COORD(0.0f);// OFF

		WRITE_BYTE((int)pev->framerate);

	if (pev->pContainingEntity->num_leafs > 0)// just send one because it's a point entity
		WRITE_SHORT(pev->pContainingEntity->leafnums[0]);
	else
		WRITE_SHORT(0);

	MESSAGE_END();
	return 1;
}

// XDM3035a: hacked entity
void CSprite::TurnOn(void)
{
	pev->frame = 0;
	pev->animtime = gpGlobals->time;
	pev->impulse = 1;

	if (FBitSet(pev->spawnflags, SF_SPRITE_CLIENTONLY))// XDM3035a: client static entity
	{
		SetBits(pev->effects, EF_NODRAW);// keep invisible, don't send over network!
	}
	else
	{
		ClearBits(pev->effects, EF_NODRAW);
		if ((pev->framerate && m_maxFrame > 1.0f) || FBitSet(pev->spawnflags, SF_SPRITE_ONCE))
		{
			SetThink(&CSprite::AnimateThink);
			SetNextThink(0);// XDM3038a
			m_lastTime = gpGlobals->time;
		}
	}
	SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

void CSprite::TurnOff(void)
{
	SetBits(pev->effects, EF_NODRAW);
	pev->animtime = 0.0;
	pev->impulse = 0;
	DontThink();// XDM3038a
	SetThinkNull();// XDM3035a: TESTME
	SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
}

void CSprite::AnimateThink(void)
{
	Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	if (pev->impulse > 0)// XDM3035a: TurnOff() may unset nextthink
		SetNextThink(0.1);

	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead(void)
{
	if (gpGlobals->time > pev->dmgtime)
	{
		if (FBitSet(pev->flags, FL_GODMODE))// XDM3034
		{
			SetBits(pev->effects, EF_NODRAW);
			SetThinkNull();
			DontThink();
		}
		else
		{
			Destroy();
			return;
		}
	}
	else
	{
		AnimateThink();
		SetNextThink(0);// XDM3038a: now
	}
}

void CSprite::Expand(float scaleSpeed, float fadeSpeed)
{
	pev->speed = scaleSpeed;
	pev->health = fadeSpeed;
	SetThink(&CSprite::ExpandThink);
	SetNextThink(0);// XDM3038a: now
	m_lastTime = gpGlobals->time;
}

void CSprite::ExpandThink(void)
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt -= pev->health * frametime;
	if (pev->renderamt <= 0 || (pev->movetype == MOVETYPE_FOLLOW && FNullEnt(pev->aiment)))
	{
		pev->renderamt = 0;
		if (FBitSet(pev->flags, FL_GODMODE))// XDM3034
		{
			SetBits(pev->effects, EF_NODRAW);
			SetThinkNull();
			DontThink();
		}
		else
		{
			Destroy();
			return;
		}
	}
	else
	{
		pev->nextthink		= gpGlobals->time + 0.1;
		m_lastTime			= gpGlobals->time;
	}
}

void CSprite::Animate(float frames)
{
	pev->frame += frames;
	if (pev->frame > m_maxFrame)
	{
		if (FBitSet(pev->spawnflags, SF_SPRITE_ONCE))
		{
			TurnOff();
		}
		else
		{
			if (m_maxFrame > 0)
				pev->frame = fmod(pev->frame, m_maxFrame);
		}
	}
}

void CSprite::AnimateAndDie(const float &framerate)
{
	SetThink(&CSprite::AnimateUntilDead);
	pev->framerate = framerate;
	pev->dmgtime = gpGlobals->time + (m_maxFrame / framerate);
	SetNextThink(0);// XDM3038a: now
}

void CSprite::SetAttachment(edict_t *pEntity, int attachment)
{
	if (pEntity)
	{
		pev->movetype = MOVETYPE_FOLLOW;
		pev->skin = ENTINDEX(pEntity);
		pev->body = attachment;
		pev->aiment = pEntity;
	}
	else// XDM3038c
	{
		pev->movetype = MOVETYPE_NONE;
		pev->skin = 0;
		pev->body = 0;
		pev->aiment = 0;
	}
}




// XDM: CGlow is now public CSprite
LINK_ENTITY_TO_CLASS(env_glow, CGlow);

bool CGlow::StartOn(void)// XDM3038c
{
	if (!FBitSet(pev->spawnflags, SF_GLOW_START_OFF) || FStringNull(pev->targetname))
		return true;

	return false;
}




TYPEDESCRIPTION CGibShooter::m_SaveData[] =
{
	DEFINE_FIELD( CGibShooter, m_iGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibCapacity, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibMaterial, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibModelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_flGibVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter, m_flVariance, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter, m_flGibLife, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGibShooter, CBaseDelay );

LINK_ENTITY_TO_CLASS( gibshooter, CGibShooter );
LINK_ENTITY_TO_CLASS( env_shooter, CGibShooter );// XDM3035a


void CGibShooter::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iGibs"))
	{
		m_iGibs = m_iGibCapacity = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVelocity"))
	{
		m_flGibVelocity = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVariance"))
	{
		m_flVariance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flGibLife"))
	{
		m_flGibLife = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "shootmodel"))// XDM: env_shooter
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "shootsounds"))// XDM: env_shooter
	{
		int iNoise = atoi(pkvd->szValue);
		switch (iNoise)
		{
		case 0:
			m_iGibMaterial = matGlass;
			break;
		case 1:
			m_iGibMaterial = matWood;
			break;
		case 2:
			m_iGibMaterial = matMetal;
			break;
		case 3:
			m_iGibMaterial = matFlesh;
			break;
		case 4:
			m_iGibMaterial = matRocks;
			break;
		default:
		//case -1:
			m_iGibMaterial = matNone;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CGibShooter::Precache(void)
{
	if (FStringNull(pev->model))
	{
		pev->model = ALLOC_STRING(g_szDefaultGibsHuman);
		m_iGibMaterial = matFlesh;
	}
	m_iGibModelIndex = PRECACHE_MODEL(STRINGV(pev->model));

	MaterialSoundPrecache((Materials)m_iGibMaterial);

	if (pev->scale >= 50.0f)// XDM: HACK! For gibs on c1a4* maps TODO: make check optional??
	{
		conprintf(1, "Design error: %s[%d] %s: scale set too large: %g!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->scale);
		pev->scale = 1.0f;
	}
}

void CGibShooter::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if (m_flDelay == 0)
		m_flDelay = 0.1;

	if (m_flGibLife == 0)
		m_flGibLife = 25;

	SetMovedir(pev);
	pev->body = MODEL_FRAMES(m_iGibModelIndex);
}

void CGibShooter::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	m_hActivator = pActivator;// XDM3037
	SetThink(&CGibShooter::ShootThink);
	SetNextThink(0);// XDM3038a: now
}

CGib *CGibShooter::CreateGib(void)
{
	CGib *pGib = GetClassPtr((CGib *)NULL, "gib");
	if (pGib)
	{
		pGib->SpawnGib(STRING(pev->model));
		int bodyPart = 0;
		if (pev->body > 1)
			bodyPart = RANDOM_LONG(0, pev->body-1);
		//pGib->pev->body = RANDOM_LONG(0, pev->body - 1);// GTFO // avoid throwing random amounts of the 0th gib. (skull).

		pGib->pev->body = bodyPart;
		pGib->m_material = m_iGibMaterial;

		if (m_iGibMaterial == matFlesh)
			pGib->m_bloodColor = BLOOD_COLOR_RED;
		else
			pGib->m_bloodColor = DONT_BLEED;

		pGib->pev->rendermode = pev->rendermode;
		pGib->pev->renderamt = pev->renderamt;
		pGib->pev->rendercolor = pev->rendercolor;
		pGib->pev->renderfx = pev->renderfx;
	//if (strstr(STRING(pev->model), ".spr") != NULL) XDM
		pGib->pev->scale = pev->scale;
		pGib->pev->skin = pev->skin;
	}
	return pGib;
}

void CGibShooter::ShootThink(void)
{
	Vector vecShootDir(pev->movedir);
	vecShootDir += gpGlobals->v_right * RANDOM_FLOAT(-1, 1) * m_flVariance;
	vecShootDir += gpGlobals->v_forward * RANDOM_FLOAT(-1, 1) * m_flVariance;
	vecShootDir += gpGlobals->v_up * RANDOM_FLOAT(-1, 1) * m_flVariance;
	vecShootDir.NormalizeSelf();

	if (sv_clientgibs.value > 0.0f)// XDM3035
	{
		if (UTIL_FileExtensionIs(STRING(pev->model), ".mdl"))// XDM3037a: more flexibility for model gibs
		{
		// TODO: create replacement for this
		MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_BREAKMODEL);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(0);//size.x
			WRITE_COORD(0);//size.y
			WRITE_COORD(0);//size.z
			WRITE_COORD(vecShootDir.x*m_flGibVelocity);//velocity.x
			WRITE_COORD(vecShootDir.y*m_flGibVelocity);//velocity.y
			WRITE_COORD(vecShootDir.z*m_flGibVelocity);//velocity.z
			WRITE_BYTE((int)(m_flGibVelocity*0.1f));//random velocity 0.1
			WRITE_SHORT(m_iGibModelIndex);
			WRITE_BYTE(m_iGibCapacity);//count: Shoot all gibs in this version!
			WRITE_BYTE((int)(m_flGibLife * RANDOM_FLOAT(9.5, 10.5)));//life 0.1
			int flags = 0;
			if (m_iGibMaterial == matGlass)
			{
				flags |= BREAK_GLASS;
				flags |= BREAK_TRANS;// kRenderTransTexture
			}
			else if (m_iGibMaterial == matMetal)
				flags |= BREAK_METAL;
			else if (m_iGibMaterial == matComputer)
				flags |= BREAK_METAL;
			else if (m_iGibMaterial == matFlesh)
				flags |= BREAK_FLESH;
			else if (m_iGibMaterial == matWood)
				flags |= BREAK_WOOD;
			else if (m_iGibMaterial == matCinderBlock)
				flags |= BREAK_CONCRETE;
			else if (m_iGibMaterial == matRocks)
				flags |= BREAK_CONCRETE;

			WRITE_BYTE(flags);//flags
		MESSAGE_END();
		}
		else// HACK: TE_BREAKMODEL does not specify render mode
		{
		MESSAGE_BEGIN(MSG_PAS, svc_temp_entity, pev->origin);
			WRITE_BYTE(TE_SPRAY);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(vecShootDir.x);//direction.x
			WRITE_COORD(vecShootDir.y);//direction.y
			WRITE_COORD(vecShootDir.z);//direction.z
			WRITE_SHORT(m_iGibModelIndex);
			WRITE_BYTE(m_iGibCapacity);//count
			WRITE_BYTE((int)(m_flGibVelocity*0.1f));//speed
			WRITE_BYTE(80);// noise (client will divide by 100)
			WRITE_BYTE(pev->rendermode);// rendermode
		MESSAGE_END();
		}

		m_iGibs -= m_iGibCapacity;// use all gibs
	}
	else
	{
		CGib *pGib = CreateGib();
		if (pGib)
		{
			pGib->pev->origin = pev->origin;
			pGib->pev->velocity = vecShootDir * m_flGibVelocity;
			pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
			pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);
			float thinkTime = pGib->pev->nextthink - gpGlobals->time;
			pGib->m_lifeTime = (m_flGibLife * RANDOM_FLOAT(0.95, 1.05));	// +/- 5%
			if (pGib->m_lifeTime < thinkTime)
			{
				pGib->SetNextThink(pGib->m_lifeTime);
				pGib->m_lifeTime = 0;
			}
		}
		--m_iGibs;
	}

	if (m_iGibs <= 0)
	{
		if (FBitSet(pev->spawnflags, SF_GIBSHOOTER_REPEATABLE))
		{
			m_iGibs = m_iGibCapacity;
			SetThinkNull();
			SetNextThink(0);// XDM3038a: now
		}
		else
		{
			SetThink(&CGibShooter::SUB_Remove);
			SetNextThink(0);// XDM3038a: now
		}
	}
	else
		SetNextThink(m_flDelay);
}




#if defined (_DEBUG)
class CTestEffect : public CBaseDelay
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	//virtual void KeyValue(KeyValueData *pkvd);
	void EXPORT TestThink(void);
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iLoop;
	int m_iBeam;
	CBeam *m_pBeam[24];
	float m_flBeamTime[24];
	float m_flStartTime;
};

LINK_ENTITY_TO_CLASS( test_effect, CTestEffect );

void CTestEffect::Spawn(void)
{
	Precache();
}

void CTestEffect::Precache(void)
{
	PRECACHE_MODEL("sprites/lgtning.spr");
}

void CTestEffect::TestThink(void)
{
	int i;
	float t = (gpGlobals->time - m_flStartTime);

	if (m_iBeam < 24)
	{
		CBeam *pbeam = CBeam::BeamCreate( "sprites/lgtning.spr", 100 );
		TraceResult tr;
		Vector vecSrc(pev->origin);
		Vector vecDir( RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		vecDir.NormalizeSelf();
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 128, ignore_monsters, edict(), &tr);
		pbeam->PointsInit( vecSrc, tr.vecEndPos );
		//pbeam->SetColor( 80, 100, 255 );
		pbeam->SetColor( 255, 180, 100 );
		pbeam->SetWidth( 100 );
		pbeam->SetScrollRate( 12 );
		m_flBeamTime[m_iBeam] = gpGlobals->time;
		m_pBeam[m_iBeam] = pbeam;
		m_iBeam++;

#if 0
		Vector vecMid = (vecSrc + tr.vecEndPos) * 0.5;
		MESSAGE_BEGIN( MSG_BROADCAST, svc_temp_entity );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(vecMid.x);	// X
			WRITE_COORD(vecMid.y);	// Y
			WRITE_COORD(vecMid.z);	// Z
			WRITE_BYTE( 20 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 100 );		// b
			WRITE_BYTE( 20 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END();
#endif
	}

	if (t < 3.0)
	{
		for (i = 0; i < m_iBeam; i++)
		{
			t = (gpGlobals->time - m_flBeamTime[i]) / ( 3 + m_flStartTime - m_flBeamTime[i]);
			m_pBeam[i]->SetBrightness( 255 * t );
			// m_pBeam[i]->SetScrollRate( 20 * t );
		}
		SetNextThink(0.1);
	}
	else
	{
		for (i = 0; i < m_iBeam; i++)
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
		m_flStartTime = gpGlobals->time;
		m_iBeam = 0;
		// SetNextThink(0);// XDM3038a: now
		SetThinkNull();
	}
}

void CTestEffect::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	DBG_PRINT_ENT_USE(Use);
	SetThink(&CTestEffect::TestThink);
	SetNextThink(0.1);
	m_flStartTime = gpGlobals->time;
}
#endif // _DEBUG



// Blood effects
LINK_ENTITY_TO_CLASS( env_blood, CEnvBlood );

void CEnvBlood::Spawn(void)
{
	CPointEntity::Spawn();
	SetMovedir(pev);
}

void CEnvBlood::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "color"))
	{
		int color = atoi(pkvd->szValue);
		switch (color)
		{
		case 0: SetColor(BLOOD_COLOR_RED); break;
		case 1: SetColor(BLOOD_COLOR_YELLOW); break;
		default: SetColor(color); break;// XDM3035a: more freedom
		}

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "amount"))
	{
		SetBloodAmount(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

Vector CEnvBlood::Direction(void)
{
	if (FBitSet(pev->spawnflags, SF_BLOOD_RANDOM))
		return UTIL_RandomBloodVector();

	return pev->movedir;
}

Vector CEnvBlood::BloodPosition( CBaseEntity *pActivator )
{
	if (FBitSet(pev->spawnflags, SF_BLOOD_PLAYER))
	{
		CBasePlayer *pPlayer = NULL;
		if (pActivator && pActivator->IsPlayer())
			pPlayer = (CBasePlayer *)pActivator;
		else
			pPlayer = UTIL_ClientByIndex(1);//INDEXENT(1);

		if (pPlayer)
			return (pPlayer->pev->origin + pPlayer->pev->view_ofs) + Vector(RANDOM_FLOAT(-10,10), RANDOM_FLOAT(-10,10), RANDOM_FLOAT(-10,10));
	}
	return pev->origin;
}

void CEnvBlood::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (FBitSet(pev->spawnflags, SF_BLOOD_STREAM))
		UTIL_BloodStream( BloodPosition(pActivator), Direction(), Color(), BloodAmount() );// XDM
	else
		UTIL_BloodDrips( BloodPosition(pActivator), Direction(), Color(), BloodAmount() );

	if (FBitSet(pev->spawnflags, SF_BLOOD_DECAL))
	{
		TraceResult tr;
		Vector start(BloodPosition(pActivator));
		UTIL_TraceLine(start, start + Direction() * BloodAmount() * 2, ignore_monsters, NULL, &tr);
		if (tr.flFraction != 1.0)
			UTIL_BloodDecalTrace(&tr, Color());
	}
}




// Screen shake
LINK_ENTITY_TO_CLASS( env_shake, CEnvShake );

// pev->scale is amplitude
// pev->dmg_save is frequency
// pev->dmg_take is duration
// pev->dmg is radius
// radius of 0 means all players
// NOTE: UTIL_ScreenShake() will only shake players who are on the ground
void CEnvShake::Spawn(void)
{
	CPointEntity::Spawn();
	if (FBitSet(pev->spawnflags, SF_SHAKE_EVERYONE))
		pev->dmg = 0;

	if (FBitSet(pev->spawnflags, SF_SHAKE_STARTON))
		pev->impulse = 1;

	//if (Duration() == 0)
	//	SetNextThink(1.0);
}

void CEnvShake::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "amplitude"))
	{
		SetAmplitude(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		SetFrequency(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		SetRadius(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CEnvShake::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (Duration() == 0)// XDM3038c: toggle mode
	{
		if (ShouldToggle(useType, pev->impulse > 0))
		{
			if (pev->impulse > 0)
			{
				pev->impulse = 0;
				DontThink();
			}
			else
			{
				pev->impulse = 1;
				SetThink(&CEnvShake::ShakeThink);
				SetNextThink(0.0);
			}
		}
	}
	else
		UTIL_ScreenShake(pev->origin, Amplitude(), Frequency(), Duration(), Radius());
}

// XDM3038c: this may be called during the game, so leave initialization code here
void CEnvShake::SetDuration(const float &duration)
{
	pev->dmg_take = duration;
	if (duration == 0)
	{
		SetThink(&CEnvShake::ShakeThink);
		SetNextThink(1.0);
	}
	else
		DontThink();
}

// XDM3038c: new
void CEnvShake::ShakeThink(void)
{
	if (pev->impulse > 0)
	{
		UTIL_ScreenShake(pev->origin, Amplitude(), Frequency(), 2.5, Radius());
		SetNextThink(2.0);
	}
}





LINK_ENTITY_TO_CLASS( env_fade, CEnvFade );

// pev->dmg_take is duration
// pev->dmg_save is hold duration
void CEnvFade::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	CPointEntity::Spawn();
}

void CEnvFade::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime( atof(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))// XDM3035a
	{
		pev->armorvalue = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CEnvFade::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	int fadeFlags = 0;

	if (!FBitSet(pev->spawnflags, SF_FADE_IN))
		fadeFlags |= FFADE_OUT;

	if (FBitSet(pev->spawnflags, SF_FADE_MODULATE))
		fadeFlags |= FFADE_MODULATE;

	if (FBitSet(pev->spawnflags, SF_FADE_STAYOUT))// XDM
		fadeFlags |= FFADE_STAYOUT;

	if (FBitSet(pev->spawnflags, SF_FADE_ONLYONE))
	{
		if (pActivator && pActivator->IsNetClient())
			UTIL_ScreenFade( pActivator, pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags );
	}
	else if (!FBitSet(pev->spawnflags, SF_FADE_DIRECTVISIBLE | SF_FADE_FACING))
	{
		if (pActivator && IsMultiplayer())// XDM3037: for activator only
			UTIL_ScreenFade( pActivator, pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags );
		else
			UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags );
	}
	else
	{
		// search through all clients
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
			if (pPlayer)
			{
				if (FBitSet(pev->spawnflags, SF_FADE_DIRECTVISIBLE) && !FVisible(pPlayer->EyePosition()))// XDM3035c
					continue;

				if (FBitSet(pev->spawnflags, SF_FADE_FACING) && !IsFacing(pPlayer->EyePosition(), pPlayer->pev->v_angle, pev->origin))// XDM3035c
					continue;

				UTIL_ScreenFade(pPlayer, pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags);
			}
		}
	}
	SUB_UseTargets( this, USE_TOGGLE, 0 );
}





LINK_ENTITY_TO_CLASS( env_message, CEnvMessage );

void CEnvMessage::Spawn(void)
{
	CPointEntity::Spawn();
	switch (pev->impulse)
	{
	case 1: // Medium radius
		pev->speed = ATTN_STATIC;
		break;

	case 2:	// Large radius
		pev->speed = ATTN_NORM;
		break;

	case 3:	//EVERYWHERE
		pev->speed = ATTN_NONE;
		break;

	default:
	case 0: // Small radius
		pev->speed = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if (pev->scale <= 0)
		pev->scale = 1.0;
}

void CEnvMessage::Precache(void)
{
	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(STRINGV(pev->noise));

	if (g_pGameRules && g_pGameRules->IsCoOp())// XDM3038
		SetBits(pev->spawnflags, SF_MESSAGE_ALL);
}

void CEnvMessage::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "messagesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messagevolume"))
	{
		pev->scale = atof(pkvd->szValue) * 0.1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messageattenuation"))
	{
		pev->impulse = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

// XDM3037: TODO: UNDONE: implement properly via SendClientData()!
void CEnvMessage::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	CBaseEntity *pPlayer = NULL;

	if (FBitSet(pev->spawnflags, SF_MESSAGE_ALL))
		ClientPrint(NULL, HUD_PRINTHUD, STRING(pev->message));
	else
	{
		if (pActivator && pActivator->IsNetClient())
			pPlayer = pActivator;
		else
			pPlayer = UTIL_ClientByIndex(1);

		if (pPlayer)
			ClientPrint(pPlayer->pev, HUD_PRINTHUD, STRING(pev->message));
	}
	if (!FStringNull(pev->noise))
		EMIT_SOUND(edict(), CHAN_BODY, STRING(pev->noise), pev->scale, pev->speed);

	SUB_UseTargets(this, USE_TOGGLE, 0);

	if (FBitSet(pev->spawnflags, SF_MESSAGE_ONCE))
		Destroy();// XDM3037: MUST BE LAST!
}




//=========================================================
// FunnelEffect
//=========================================================
LINK_ENTITY_TO_CLASS( env_funnel, CEnvFunnel );

void CEnvFunnel::Precache(void)
{
	if (!FStringNull(pev->model))// XDM: custom model
		m_iSprite = PRECACHE_MODEL(STRINGV(pev->model));
	else
		m_iSprite = PRECACHE_MODEL("sprites/flare6.spr");
}

void CEnvFunnel::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

void CEnvFunnel::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	m_hActivator = pActivator;// XDM3037
	if (m_iSprite != 0)// XDM3038c
	{
	MESSAGE_BEGIN(MSG_BROADCAST, svc_temp_entity);
		WRITE_BYTE(TE_LARGEFUNNEL);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(m_iSprite);
		WRITE_SHORT(FBitSet(pev->spawnflags, SF_FUNNEL_REVERSE)?1:0);// funnel flows in reverse?
	MESSAGE_END();
	}
	if (!FBitSet(pev->spawnflags, SF_FUNNEL_REPEATABLE))
	{
		SetThink(&CEnvFunnel::SUB_Remove);
		SetNextThink(0);// XDM3038a: now
	}
}






//----------------------------------------------------------------
// Spark
//----------------------------------------------------------------
LINK_ENTITY_TO_CLASS(env_spark, CEnvSpark);
LINK_ENTITY_TO_CLASS(env_debris, CEnvSpark);

void CEnvSpark::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "MaxDelay"))
	{
		m_flDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CEnvSpark::Spawn(void)
{
	Precache();

	if (m_flDelay <= 0)
		m_flDelay = 1.5;

	if (FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_ENVSPARK_STARTON))
	{
		SetThink(&CEnvSpark::SparkThink);
		SetNextThink(1.0);
	}
}

void CEnvSpark::Precache(void)
{
// buttons/spark*.wav in ClientPrecache()
}

void CEnvSpark::SparkThink(void)
{
	UTIL_Sparks(Center());
	EMIT_SOUND_DYN(edict(), CHAN_STATIC, gSoundsSparks[RANDOM_LONG(0,NUM_SPARK_SOUNDS-1)], RANDOM_FLOAT(0.05, 0.5), ATTN_STATIC, 0, RANDOM_LONG(95,105));

	if (FBitSet(pev->spawnflags, SF_ENVSPARK_LIGHT) && (g_pGameRules == NULL || g_pGameRules->FAllowEffects()))// XDM3037
		DynamicLight(pev->origin, 127, 255,239,207, 2, RANDOM_LONG(90,110));

	SUB_UseTargets(m_hActivator, USE_ON, 1.0);// XDM3037: can be used with broken lamps
	SetNextThink(RANDOM_FLOAT(0.1, m_flDelay));
}

void CEnvSpark::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (!ShouldToggle(useType, (pev->impulse > 0)))
		return;

	if (FBitSet(pev->spawnflags, SF_ENVSPARK_TOGGLE) && pev->impulse > 0)
	{
		pev->impulse = 0;
		SetThinkNull();
		DontThink();// XDM3038a
	}
	else
	{
		m_hActivator = pActivator;
		pev->impulse = 1;
		SetThink(&CEnvSpark::SparkThink);
		SetNextThink(0.1);
	}
}

/*void CEnvSpark::SparkStart(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse(&CEnvSpark::SparkStop);
	SetThink(&CEnvSpark::SparkThink);
	SetNextThink(0.1 + RANDOM_FLOAT(0, m_flDelay));
}

void CEnvSpark::SparkStop(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse(&CEnvSpark::SparkStart);
	SetThinkNull();
}*/

//-----------------------------------------------------------------------------
//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( env_render, CRenderFxManager );

void CRenderFxManager::Spawn(void)
{
	//Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;// XDM3035a
}

void CRenderFxManager::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (FStringNull(pev->target))// XDM3037: apply to caller if no target specified
	{
		if (pCaller)
			Apply(pCaller->pev);
	}
	else
	{
		edict_t *pentTarget = NULL;
		while ((pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target))) != NULL)
		{
			if (FNullEnt(pentTarget))
				break;

			Apply(VARS(pentTarget));
		}
	}
}

void CRenderFxManager::Apply(entvars_t *pevTarget)
{
	if (!FBitSet(pev->spawnflags, SF_RENDER_MASKFX))
		pevTarget->renderfx = pev->renderfx;
	if (!FBitSet(pev->spawnflags, SF_RENDER_MASKAMT))
		pevTarget->renderamt = pev->renderamt;
	if (!FBitSet(pev->spawnflags, SF_RENDER_MASKMODE))
		pevTarget->rendermode = pev->rendermode;
	if (!FBitSet(pev->spawnflags, SF_RENDER_MASKCOLOR))
		pevTarget->rendercolor = pev->rendercolor;
}
