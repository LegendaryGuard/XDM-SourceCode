//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "triggers.h"
#include "trains.h"// trigger_camera has train functionality
#include "gamerules.h"
#include "game.h"
#include "globals.h"


//=================================================================
//
// CTriggerSound
//
//=================================================================
LINK_ENTITY_TO_CLASS(trigger_sound, CTriggerSound);

void CTriggerSound::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "roomtype"))
	{
		if (!pev->health)
			pev->health = atof(pkvd->szValue);
		else
			conprintf(1, "trigger_sound: room type already defined in 'health' keyvalue!\n");

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CTriggerSound::Touch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	if (IsLockedByMaster(pOther))
		return;

	if (pOther->IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		if (pPlayer->UpdateSoundEnvironment(this, pev->health, (Center() - pPlayer->Center()).Length()))
			SUB_UseTargets(pPlayer, USE_TOGGLE, 0);
	}
}

void CTriggerSound::Spawn(void)
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(edict(), STRING(pev->model));
	SetBits(pev->effects, EF_NODRAW);
	// XDM3037:	0 means reset! if (!pev->health)
}








//=================================================================
//
// CTriggerOnSight
// Fires target when something is visible
//
//=================================================================
#define SF_ONSIGHT_NOLOS   0x00001
#define SF_ONSIGHT_NOGLASS 0x00002
#define SF_ONSIGHT_ACTIVE  0x08000
#define SF_ONSIGHT_DEMAND  0x10000

class CTriggerOnSight : public CBaseDelay
{
public:
	virtual void Spawn(void);
	virtual void Think(void);
	virtual int	ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual STATE GetState(void);

	BOOL VisionCheck(void);
	BOOL CanSee(CBaseEntity *pLooker, CBaseEntity *pSeen);
};

LINK_ENTITY_TO_CLASS( trigger_onsight, CTriggerOnSight );

void CTriggerOnSight::Spawn(void)
{
	if (pev->target || pev->noise)
		SetNextThink(1);// if we're going to have to trigger stuff, start thinking
	else// otherwise, just check whenever someone asks about our state.
		SetBits(pev->spawnflags, SF_ONSIGHT_DEMAND);

	if (pev->max_health > 0)
		pev->health = cos(pev->max_health/2 * M_PI/180.0);
}

STATE CTriggerOnSight::GetState(void)
{
	if (FBitSet(pev->spawnflags, SF_ONSIGHT_DEMAND))
		return VisionCheck()?STATE_ON:STATE_OFF;
	else
		return FBitSet(pev->spawnflags, SF_ONSIGHT_ACTIVE)?STATE_ON:STATE_OFF;
}

void CTriggerOnSight::Think(void)
{
	// is this a sensible rate?
	SetNextThink(0.1);

	//if (!UTIL_IsMasterTriggered(m_sMaster, NULL))
	//{
	//	ClearBits(pev->spawnflags, SF_ONSIGHT_ACTIVE);
	//	return;
	//}

	if (VisionCheck())
	{
		if (!FBitSet(pev->spawnflags, SF_ONSIGHT_ACTIVE))
		{
			FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
			FireTargets(STRING(pev->noise1), this, this, USE_ON, 0);
			SetBits(pev->spawnflags, SF_ONSIGHT_ACTIVE);
		}
	}
	else
	{
		if (FBitSet(pev->spawnflags, SF_ONSIGHT_ACTIVE))
		{
			FireTargets(STRING(pev->noise), this, this, USE_TOGGLE, 0);
			FireTargets(STRING(pev->noise1), this, this, USE_OFF, 0);
			ClearBits(pev->spawnflags, SF_ONSIGHT_ACTIVE);
		}
	}
}

BOOL CTriggerOnSight::VisionCheck(void)
{
	CBaseEntity *pLooker;
	if (pev->netname)
	{
		pLooker = UTIL_FindEntityByTargetname(NULL, STRING(pev->netname));
		if (!pLooker)
			return FALSE; // if we can't find the eye entity, give up
	}
	else
	{
		pLooker = UTIL_FindEntityByClassname(NULL, "player");
		if (!pLooker)
		{
			conprintf(1, "trigger_onsight can't find player!?\n");
			return FALSE;
		}
	}

	CBaseEntity *pSeen;
	if (pev->message)
		pSeen = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
	else
		return CanSee(pLooker, this);

	if (!pSeen)
	{
		// must be a classname.
		pSeen = UTIL_FindEntityByClassname(pSeen, STRING(pev->message));
		while (pSeen != NULL)
		{
			if (CanSee(pLooker, pSeen))
				return TRUE;
			pSeen = UTIL_FindEntityByClassname(pSeen, STRING(pev->message));
		}
		return FALSE;
	}
	else
	{
		while (pSeen != NULL)
		{
			if (CanSee(pLooker, pSeen))
				return TRUE;
			pSeen = UTIL_FindEntityByTargetname(pSeen, STRING(pev->message));
		}
		return FALSE;
	}
}

// by the criteria we're using, can the Looker see the Seen entity?
BOOL CTriggerOnSight::CanSee(CBaseEntity *pLooker, CBaseEntity *pSeen)
{
	// out of range?
	if (pev->frags && (pLooker->pev->origin - pSeen->pev->origin).Length() > pev->frags)
		return FALSE;

	// check FOV if appropriate
	if (pev->max_health < 360)
	{
		// copied from CBaseMonster's FInViewCone function
		Vector2D	vec2LOS;
		float	flDot;
		float flComp = pev->health;
		UTIL_MakeVectors ( pLooker->pev->angles );
		vec2LOS = ( pSeen->pev->origin - pLooker->pev->origin ).Make2D();
		vec2LOS.NormalizeSelf();
		flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );
		//conprintf(2, "flDot is %f\n", flDot);

		if ( pev->max_health == -1 )
		{
			CBaseMonster *pMonst = pLooker->MyMonsterPointer();
			if (pMonst)
				flComp = pMonst->m_flFieldOfView;
			else
				return FALSE; // not a monster, can't use M-M-M-MonsterVision
		}

		// outside field of view
		if (flDot <= flComp)
			return FALSE;
	}

	// check LOS if appropriate
	if (!FBitSet(pev->spawnflags, SF_ONSIGHT_NOLOS))
	{
		TraceResult tr;
		if (FBitSet(pev->spawnflags, SF_ONSIGHT_NOGLASS))
			UTIL_TraceLine(pLooker->EyePosition(), pSeen->pev->origin, ignore_monsters, ignore_glass, pLooker->edict(), &tr);
		else
			UTIL_TraceLine(pLooker->EyePosition(), pSeen->pev->origin, ignore_monsters, dont_ignore_glass, pLooker->edict(), &tr);
		if (tr.flFraction < 1.0 && tr.pHit != pSeen->edict())
			return FALSE;
	}

	return TRUE;
}




//=================================================================
//
// CTriggerSetPatrol
// Sets monster's state to patrol
//
//=================================================================
class CTriggerSetPatrol : public CBaseDelay
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	int		m_iszPath;
	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( trigger_startpatrol, CTriggerSetPatrol );

TYPEDESCRIPTION	CTriggerSetPatrol::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerSetPatrol, m_iszPath, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerSetPatrol,CBaseDelay);

void CTriggerSetPatrol::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszPath"))
	{
		m_iszPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerSetPatrol::Spawn(void)
{
	// point entity! if (showtriggers.value <= 0)
	SetBits(pev->effects, EF_NODRAW);
	pev->renderamt = 0;
}

void CTriggerSetPatrol::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target )/*, pActivator */);
	CBaseEntity *pPath = UTIL_FindEntityByTargetname( NULL, STRING( m_iszPath )/*, pActivator */);
	if (pTarget && pPath)
	{
		CBaseMonster *pMonster = pTarget->MyMonsterPointer();
		if (pMonster) pMonster->StartPatrol(pPath);
	}
}







//-----------------------------------------------------------------------------
// Purpose: center of gravity, useful for pushables (incl. monsters and players)
// 'gravity' - gravity scalar force
// 'health' - radius
//-----------------------------------------------------------------------------
// obsolete: use 'state' KV. #define SF_GRAVITY_START_ON			0x0001
#define SF_GRAVITY_GRADIENT			0x0002// clients as far as radius are almost not affected
#define SF_GRAVITY_NOCLIENTS		0x0004
#define SF_GRAVITY_NOMONSTERS		0x0008
#define SF_GRAVITY_CLEARVELOCITY	0x0010
#define SF_GRAVITY_IGNOREVIS		0x0020

class CEnvGravityPoint : public CBaseDelay
{
public:
	CEnvGravityPoint(void);
	virtual void Spawn(void);
	virtual void Think(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(env_gravitypoint, CEnvGravityPoint);

CEnvGravityPoint::CEnvGravityPoint() : CBaseDelay()
{
	SetState(STATE_OFF);// this one starts off unless directly specified
}

void CEnvGravityPoint::Spawn(void)
{
	pev->effects = EF_NODRAW;
	CBaseDelay::Spawn();
}

void CEnvGravityPoint::Think(void)
{
	//DBG_PRINT_ENT_THINK(Think);
	if (GetState() == STATE_ON)
	{
		CBaseEntity *pEntity = NULL;
		TraceResult	tr;
		Vector vecDelta, vecEntity;
		vec_t dist, force;

		while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, pev->health)) != NULL)
		{
			if (pEntity == this)
				continue;
			if (!pEntity->IsPushable())
				continue;
			if (FBitSet(pEntity->pev->effects, EF_NODRAW))
				continue;
			if (pEntity->pev->movetype == MOVETYPE_NONE || pEntity->pev->movetype == MOVETYPE_NOCLIP)
				continue;
			if (pEntity->IsPlayer() && FBitSet(pev->spawnflags, SF_GRAVITY_NOCLIENTS))
				continue;
			if (pEntity->IsMonster() && FBitSet(pev->spawnflags, SF_GRAVITY_NOMONSTERS))
				continue;

			vecEntity = pEntity->Center();
			if (FBitSet(pev->spawnflags, SF_GRAVITY_IGNOREVIS))
				tr.flFraction = 1.0f;
			else
				UTIL_TraceLine(pev->origin, vecEntity, ignore_monsters, dont_ignore_glass, edict(), &tr);

			if (tr.flFraction == 1.0f || tr.pHit == pEntity->edict())// the entity is blocking trace line itself
			{
				force = pev->gravity;
				vecDelta = pev->origin; vecDelta -= vecEntity;
				dist = vecDelta.Length();
				if (dist != 0.0f)
					vecDelta /= dist;// 1-unit vector

				if (FBitSet(pev->spawnflags, SF_GRAVITY_GRADIENT))
					force *= (1.0f - dist/pev->health);

				if (FBitSet(pev->spawnflags, SF_GRAVITY_CLEARVELOCITY))
					pEntity->pev->velocity.Clear();

				if (dist < pEntity->pev->maxs.Length() * 1.5f)// otherwise entities will fly through and return...
				{
					pEntity->pev->velocity.Clear();
					//pEntity->pev->gravity = 0;
					UTIL_SetOrigin(pEntity, pev->origin);
				}
				else
					pEntity->pev->velocity += vecDelta * force;
			}
		}
		CBaseEntity::Think();// XDM3037: allow SetThink()
		SetNextThink(0.1);
	}
}

// This thing is so universal it should be moved to CBaseDelay()
void CEnvGravityPoint::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (IsLockedByMaster(pActivator))
		return;

	if (!ShouldToggle(useType, (GetState() == STATE_ON)?1:0))
		return;

	if (GetState() == STATE_OFF)
	{
		SetState(STATE_ON);
		SetNextThink(0.0);
	}
	else
	{
		SetState(STATE_OFF);
		DontThink();// XDM3038a
	}
}




//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_secret, CTriggerSecret);

void CTriggerSecret::Spawn(void)
{
	//Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	if (g_pWorld)// we depend on the fact that world spawns before everything else
		g_pWorld->m_iNumSecrets++;
}

void CTriggerSecret::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (pActivator && pActivator->IsPlayer())
	{
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
			if (pPlayer)
			{
				if (pPlayer == pActivator)
				{
					pPlayer->m_Stats[STAT_SECRETS_COUNT]++;
					ClientPrint(pPlayer->pev, HUD_PRINTHUD, "#SECRET_LOCAL\n", FStringNull(pev->message)?"":STRING(pev->message));
					if (g_pGameRules)
						g_pGameRules->OnPlayerFoundSecret(pPlayer, this);
				}
				else
					ClientPrint(pPlayer->pev, HUD_PRINTHUD, "#SECRET_OTHER\n", STRING(pActivator->pev->netname), FStringNull(pev->message)?"":STRING(pev->message));
			}
		}
		gpGlobals->found_secrets++;
	}
	else
		ClientPrint(NULL, HUD_PRINTHUD, "#SECRET_FAIL\n", FStringNull(pev->message)?"":STRING(pev->message));

	SUB_UseTargets(pActivator, useType, value);
	Destroy();// secrets are single use only
}
