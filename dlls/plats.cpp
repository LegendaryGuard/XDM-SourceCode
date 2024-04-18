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

===== plats.cpp ========================================================

  spawn, think, and touch functions for trains, etc

  NOTE: in XHL there are no start sounds, there are special start-and-loop move sounds
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "movewith.h"
#include "trains.h"
#include "saverestore.h"
#include "basemonster.h"
#include "gamerules.h"
#include "game.h"

// ======== THIS IS AN EXTREMELY UGLY DISGUSTING HACK!!! ========
LINK_ENTITY_TO_CLASS(trigger_platcontrol, CPlatTrigger);

// XDM3038c: screw this! I'm renaming it!!
CPlatTrigger *CreateTriggerForPlat(CFuncPlat *pPlatform)
{
	//GetClassPtr((CPlatTrigger *)NULL)->SpawnInsideTrigger(pPlatform);
	CPlatTrigger *pTrigger = (CPlatTrigger *)CBaseEntity::Create("trigger_platcontrol", pPlatform->pev->origin, pPlatform->pev->angles, pPlatform->edict());// XDM3038c
	if (pTrigger)
		pTrigger->SetPlatform(pPlatform);

	return pTrigger;
}

void CPlatTrigger::SetPlatform(CFuncPlat *pPlatform)
{
	m_pPlatform = pPlatform;
	// Create trigger entity, "point" it at the owning platform, give it a touch method
	pev->solid		= SOLID_TRIGGER;
	pev->movetype	= MOVETYPE_NONE;
	pev->origin = pPlatform->pev->origin;
	// Establish the trigger field's size
	Vector vecTMin = m_pPlatform->pev->mins + Vector ( 25 , 25 , 0 );
	Vector vecTMax = m_pPlatform->pev->maxs + Vector ( 25 , 25 , 8 );
	vecTMin.z = vecTMax.z - ( m_pPlatform->m_vecPosition1.z - m_pPlatform->m_vecPosition2.z + 8.0f );
	if (m_pPlatform->pev->size.x <= 50)
	{
		vecTMin.x = (m_pPlatform->pev->mins.x + m_pPlatform->pev->maxs.x) / 2.0f;
		vecTMax.x = vecTMin.x + 1;
	}
	if (m_pPlatform->pev->size.y <= 50)
	{
		vecTMin.y = (m_pPlatform->pev->mins.y + m_pPlatform->pev->maxs.y) / 2.0f;
		vecTMax.y = vecTMin.y + 1;
	}
	UTIL_SetSize(this, vecTMin, vecTMax);
}

// When the platform's trigger field is touched, the platform ???
void CPlatTrigger::Touch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;
	// Ignore touches by corpses
	if (!pOther->IsAlive())
		return;
	// Make linked platform go up/down.
	if (m_pPlatform->m_toggle_state == TS_AT_BOTTOM)
		m_pPlatform->GoUp();
	else if (m_pPlatform->m_toggle_state == TS_AT_TOP)
		m_pPlatform->pev->nextthink = m_pPlatform->pev->ltime + 1.0;// delay going down
}



/*void FixAngle(float &angle)
{
	while (angle < 0)
		angle += 360;
	while (angle > 360)
		angle -= 360;
	//return angle;
}

static void FixupAngles(Vector &v)
{
	FixAngle(v.x);
	FixAngle(v.y);
	FixAngle(v.z);
}*/




// Predefined sound lists for Half-Life
// XDM: start sounds are replaced with move sounds
const char *CBasePlatTrain::MoveSounds[] =
{
	"common/null.wav",
	"plats/bigmove1.wav",
	"plats/bigmove2.wav",
	"plats/elevmove1.wav",
	"plats/elevmove2.wav",
	"plats/elevmove3.wav",
	"plats/freightmove1.wav",
	"plats/freightmove2.wav",
	"plats/heavymove1.wav",
	"plats/rackmove1.wav",
	"plats/railmove1.wav",
	"plats/squeekmove1.wav",
	"plats/talkmove1.wav",
	"plats/talkmove2.wav",
	"common/null.wav",
};

const char *CBasePlatTrain::StopSounds[] =
{
	"common/null.wav",
	"plats/bigstop1.wav",
	"plats/bigstop2.wav",
	"plats/elevstop1.wav",
	"plats/elevstop2.wav",
	"plats/elevstop3.wav",
	"plats/freightstop1.wav",
	"plats/freightstop2.wav",
	"plats/heavystop2.wav",
	"plats/rackstop1.wav",
	"plats/railstop1.wav",
	"plats/squeekstop1.wav",
	"plats/talkstop1.wav",
	"plats/talkstop2.wav",
	"common/null.wav",
};

TYPEDESCRIPTION	CBasePlatTrain::m_SaveData[] =
{
//	DEFINE_FIELD( CBasePlatTrain, m_bStartSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlatTrain, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlatTrain, m_bStopSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlatTrain, m_volume, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlatTrain, m_flAttenuation, FIELD_FLOAT ),// XDM3038c
};

IMPLEMENT_SAVERESTORE( CBasePlatTrain, CBaseToggle );

void CBasePlatTrain::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_flHeight = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "rotation"))
	{
		m_vecFinalAngle.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	/*else if (FStrEq(pkvd->szKeyName, "startsnd"))
	{
		m_bStartSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	else if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		m_bStopSnd = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "attenuation"))// XDM3038c
	{
		m_flAttenuation = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CBasePlatTrain::Precache(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!

	// XDM3038b: custom sound has first priority
	if (FStringNull(pev->noiseMovement))
	{
		if (m_bMoveSnd >= 0 && m_bMoveSnd <= ARRAYSIZE(MoveSounds))//14)
			pev->noiseMovement = MAKE_STRING(MoveSounds[m_bMoveSnd]);
	}
	if (FStringNull(pev->noiseStopMoving))
	{
		if (m_bStopSnd >= 0 && m_bStopSnd <= ARRAYSIZE(StopSounds))//14)
			pev->noiseStopMoving = MAKE_STRING(StopSounds[m_bStopSnd]);
	}

	if (!FStringNull(pev->noiseMovement))
		PRECACHE_SOUND(STRINGV(pev->noiseMovement));

	if (!FStringNull(pev->noiseStopMoving))
		PRECACHE_SOUND(STRINGV(pev->noiseStopMoving));
}

void CBasePlatTrain::StartSound(void)// XDM
{
	if (m_soundPlaying)
		return;

	// CHANGED this from CHAN_VOICE to CHAN_STATIC around OEM beta time because trains should
	// use CHAN_STATIC for their movement sounds to prevent sound field problems.
	// this is not a hack or temporary fix, this is how things should be. (sjb).
	if (!FStringNull(pev->noiseMovement))
		EMIT_SOUND(edict(), CHAN_STATIC, STRINGV(pev->noiseMovement), m_volume, m_flAttenuation);

	pev->frame = 1;// XDM3037
	m_soundPlaying = TRUE;
}

void CBasePlatTrain::StopSound(void)// XDM
{
	if (!FStringNull(pev->noiseMovement))
		STOP_SOUND(edict(), CHAN_STATIC, STRINGV(pev->noiseMovement));

	if (!FStringNull(pev->noiseStopMoving))
		EMIT_SOUND(edict(), CHAN_STATIC, STRINGV(pev->noiseStopMoving), m_volume, m_flAttenuation);

	pev->frame = 0;// XDM3037
	m_soundPlaying = FALSE;
}

//
//====================== PLAT code ====================================================
//
/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed	default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in
the extended position until it is trigger, when it will lower and become a normal plat.

If the "height" key is set, that will determine the amount the plat moves, instead of
being implicitly determined by the model's height.

Set "sounds" to one of the following:
1) base fast
2) chain slow
*/
LINK_ENTITY_TO_CLASS(func_plat, CFuncPlat);

void CFuncPlat::Setup(void)
{
	//pev->noiseMovement = MAKE_STRING("plats/platmove1.wav");
	//pev->noiseStopMoving = MAKE_STRING("plats/platstop1.wav");

	if (m_flTLength == 0.0f)
		m_flTLength = 80.0f;
	if (m_flTWidth == 0.0f)
		m_flTWidth = 10.0f;

	pev->angles.Clear();

	if (FBitSet(pev->spawnflags, SF_PLAT_PASSABLE))// XDM3037a
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(this, pev->origin);		// set size and link into world
	UTIL_SetSize(this, pev->mins, pev->maxs);
	SET_MODEL(edict(), STRING(pev->model));

	// vecPosition1 is the top position, vecPosition2 is the bottom
	m_vecPosition1 = pev->origin;
	m_vecPosition2 = pev->origin;

	if (m_flHeight != 0.0f)
		m_vecPosition2.z = pev->origin.z - m_flHeight;
	else
		m_vecPosition2.z = pev->origin.z - pev->size.z + 8;

	if (pev->speed == 0)
		pev->speed = 150;

	if (m_volume <= 0.0f)
		m_volume = 0.85f;
	else if (m_volume > 1.0f)// XDM
		m_volume = VOL_NORM;

	if (m_flAttenuation <= 0.0f)// XDM3038c: BAD: we cannot allow 0.0f
		m_flAttenuation = ATTN_NORM;
}

void CFuncPlat::Precache(void)
{
	CBasePlatTrain::Precache();
	//PRECACHE_SOUND("plats/platmove1.wav");
	//PRECACHE_SOUND("plats/platstop1.wav");
	if (!IsTogglePlat())
		CreateTriggerForPlat(this);		// the "start moving" trigger
}

void CFuncPlat::Spawn(void)
{
	Precache();// XDM3038c: moved before spawn()
	Setup();

	// If this platform is the target of some button, it starts at the TOP position,
	// and is brought down by that button.  Otherwise, it starts at BOTTOM.
	if ( !FStringNull(pev->targetname) )
	{
		UTIL_SetOrigin(this, m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		SetUse( &CFuncPlat::PlatUse );// XDM3035: 20100828
	}
	else
	{
		UTIL_SetOrigin(this, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
	}
}

//
// Used by SUB_UseTargets, when a platform is the target of a button.
// Start bringing platform down.
//
void CFuncPlat::PlatUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (IsTogglePlat())
	{
		// Top is off, bottom is on
		if (!ShouldToggle(useType, (m_toggle_state == TS_AT_BOTTOM)))
			return;

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
		else if (m_toggle_state == TS_AT_BOTTOM)
			GoUp();
	}
	else
	{
		SetUseNull();
		if (m_toggle_state == TS_AT_TOP)
			GoDown();
	}
}

//
// Platform is at top, now starts moving down.
//
void CFuncPlat::GoDown(void)
{
	ASSERT(m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP);
	StartSound();// XDM
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone(&CFuncPlat::CallHitBottom);// XDM3035: 20100828
	LinearMove(m_vecPosition2, pev->speed);
}

//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlat::HitBottom(void)
{
	ASSERT(m_toggle_state == TS_GOING_DOWN);
	StopSound();// XDM
	m_toggle_state = TS_AT_BOTTOM;
}

//
// Platform is at bottom, now starts moving up
//
void CFuncPlat::GoUp(void)
{
	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);
	StartSound();// XDM
	m_toggle_state = TS_GOING_UP;
	SetMoveDone(&CFuncPlat::CallHitTop);// XDM3035: 20100828
	LinearMove(m_vecPosition1, pev->speed);
}

//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlat::HitTop(void)
{
	ASSERT(m_toggle_state == TS_GOING_UP);
	StopSound();// XDM
	m_toggle_state = TS_AT_TOP;

	if (!IsTogglePlat())
	{
		// After a delay, the platform will automatically start going down again.
		SetThink(&CFuncPlat::CallGoDown);
		pev->nextthink = pev->ltime + 3;
	}
}

void CFuncPlat::Blocked(CBaseEntity *pOther)
{
	conprintf(2, "%s Blocked by %s\n", STRING(pev->classname), STRING(pOther->pev->classname));
	// Hurt the blocker a little
	pOther->TakeDamage(this, m_hActivator?(CBaseEntity *)m_hActivator:this, 1, DMG_CRUSH);

	if (!FStringNull(pev->noiseMovement))
		STOP_SOUND(edict(), CHAN_STATIC, STRINGV(pev->noiseMovement));

	// Send the platform back where it came from
	ASSERT(m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN);
	if (m_toggle_state == TS_GOING_UP)
		GoDown();
	else if (m_toggle_state == TS_GOING_DOWN)
		GoUp ();
}




LINK_ENTITY_TO_CLASS( func_platrot, CFuncPlatRot );

TYPEDESCRIPTION	CFuncPlatRot::m_SaveData[] =
{
	DEFINE_FIELD( CFuncPlatRot, m_end, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncPlatRot, m_start, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFuncPlatRot, CFuncPlat );

void CFuncPlatRot::SetupRotation(void)
{
	if (m_vecFinalAngle.x != 0)		// This plat rotates too!
	{
		AxisDir();// XDM3038c: was CBaseToggle::
		m_start	= pev->angles;
		m_end = pev->angles + pev->movedir * m_vecFinalAngle.x;
	}
	else
	{
		m_start.Clear();
		m_end.Clear();
	}
	if (!FStringNull(pev->targetname))	// Start at top
		pev->angles = m_end;
}

void CFuncPlatRot::Spawn(void)
{
	CFuncPlat::Spawn();
	SetupRotation();
}

void CFuncPlatRot::GoDown(void)
{
	CFuncPlat::GoDown();
	RotMove(m_start, pev->nextthink - pev->ltime);
}

//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlatRot::HitBottom(void)
{
	CFuncPlat::HitBottom();
	pev->avelocity.Clear();
	pev->angles = m_start;
}

//
// Platform is at bottom, now starts moving up
//
void CFuncPlatRot::GoUp(void)
{
	CFuncPlat::GoUp();
	RotMove(m_end, pev->nextthink - pev->ltime);
}

//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlatRot::HitTop(void)
{
	CFuncPlat::HitTop();
	pev->avelocity.Clear();
	pev->angles = m_end;
}

void CFuncPlatRot::RotMove(const Vector &destAngle, float time)
{
	// set destdelta to the vector needed to move
	Vector vecDestDelta = destAngle - pev->angles;

	// Travel time is so short, we're practically there already;  so make it so.
	if (time >= 0.1)
		pev->avelocity = vecDestDelta / time;
	else
	{
		pev->avelocity = vecDestDelta;
		pev->nextthink = pev->ltime + 1;
	}
}



//
//====================== TRAIN code ==================================================
//

// XDM: class definition moved to header

LINK_ENTITY_TO_CLASS( func_train, CFuncTrain );

TYPEDESCRIPTION	CFuncTrain::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTrain, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrain, m_pevCurrentTarget, FIELD_EVARS ),
	DEFINE_FIELD( CFuncTrain, m_activated, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFuncTrain, m_iszLastTarget, FIELD_STRING ),// XDM3038c: conflict: we now use pev->message for BlockedTarget
};

IMPLEMENT_SAVERESTORE( CFuncTrain, CBasePlatTrain );

/* XDM3035c: why?
int CFuncTrain::Save(CSave &save)
{
	if (!CBasePlatTrain::Save(save))
		return 0;

	return save.WriteFields( "CFuncTrain", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

int CFuncTrain::Restore(CRestore &restore)
{
	if (!CBasePlatTrain::Restore(restore))
		return 0;

	return restore.ReadFields( "CFuncTrain", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}*/

void CFuncTrain::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
/*#if defined(MOVEWITH)
	else if (FStrEq(pkvd->szKeyName, "move"))// XDM
	{
		m_iszMoveTargetName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
#endif*/
	else
		CBasePlatTrain::KeyValue(pkvd);
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal
*/
void CFuncTrain::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	Precache();
	if (pev->speed == 0)
		pev->speed = 100;

	if (FStringNull(pev->target))
		conprintf(0, "Design error: %s[%d] \"%s\" with no target!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));

	if (pev->dmg == 0)
		pev->dmg = 2;

	pev->movetype = MOVETYPE_PUSH;

	if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_PASSABLE))
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(this, pev->mins, pev->maxs);
	UTIL_SetOrigin(this, pev->origin);

	m_activated = FALSE;
	m_soundPlaying = FALSE;// XDM: replay sound after restore

	if (m_volume == 0)
		m_volume = 0.85;

	if (m_flAttenuation <= 0)// XDM3038c
		m_flAttenuation = ATTN_NORM;
	/*if (m_iszMoveTargetName != NULL)// XDM
	{
		CBaseEntity *pNewEntity = NULL;
		while ((pNewEntity = UTIL_FindEntityByTargetname(pNewEntity, STRING(m_iszMoveTargetName))) != NULL)
			pNewEntity->m_vecMoveOriginDelta = pNewEntity->pev->origin - pev->origin;
	}*/
}

/*void CFuncTrain::Precache(void)
{
	CBasePlatTrain::Precache();
}*/

void CFuncTrain::Blocked(CBaseEntity *pOther)
{
	if (gpGlobals->time < m_flActivateFinished)
		return;

	m_flActivateFinished = gpGlobals->time + 0.5f;

	// XDM3038: Fire the OnBlocked target if there is one
	if (!FStringNull(pev->message))
	{
		FireTargets(STRING(pev->message), this, this, USE_ON, 1);
		if (FBitSet(pev->spawnflags, SF_TRAIN_BLOCKEDONCE))
			pev->message = iStringNull;
	}

	if (pev->dmg > 0)// XDM3038
		pOther->TakeDamage(this, m_hActivator?(CBaseEntity *)m_hActivator:this, pev->dmg, DMG_CRUSH);// XDM3035: m_hActivator

	// XDM3038: UNDONE: find other plats using this path and stop them
	/*if (pev->enemy)// current target
	{
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, STRING(pev->classname))) != NULL)
		{
			if (pEntity == this)
				continue;// recursion
			if (pEntity->pev->enemy == pev->enemy ||
				FStrEq(STRING(pEntity->pev->enemy->v.targetname), STRING(pev->target)) ||
				FStrEq(STRING(pEntity->pev->target), STRING(pev->enemy->v.targetname)) ||
				pEntity->pev->target == pev->target)// same path
			{
				// don't do this! pEntity->Blocked(pOther);
	//???			pEntity->Use(this, this, USE_OFF, 0.0f);
				// How do we pause other trains!?
			}
		}
	}*/
}

void CFuncTrain::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (IsLockedByMaster(pActivator))// XDM3037: disabled train
		return;

	if (!ShouldToggle(useType, !pev->velocity.IsZero()))// XDM3038: TESTME!
		return;

	if (FBitSet(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER) || useType == USE_ON)
	{
		// Move toward my target
		ClearBits(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER);
		StartSound();// XDM
		Next();
		return;
	}

	if (!FBitSet(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER) || useType == USE_OFF)
	{
		SetBits(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER);
		// Pop back to last target if it's available
		if (pev->enemy)
			pev->target = pev->enemy->v.targetname;

		DontThink();// XDM3038a
		pev->velocity.Clear();
		StopSound();// XDM
		/*if (m_iszMoveTargetName != NULL)// XDM
		{
			CBaseEntity *pNewEntity = NULL;
			while ((pNewEntity = UTIL_FindEntityByTargetname(pNewEntity, STRING(m_iszMoveTargetName))) != NULL)
				pNewEntity->pev->velocity.Clear();
		}*/
		return;
	}
}

// Called upon arrival to another target
void CFuncTrain::Wait(void)
{
	DBG_PRINT_ENT_THINK(Wait);
	if (m_pevCurrentTarget)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(ENT(m_pevCurrentTarget));// XDM3038c: a much better way to Use()
		if (pEntity)
		{
			// NO!! This will toggle entity! pEntity->Use(m_hActivator.Get()?m_hActivator:this, this, USE_TOGGLE, 0);
			if (FClassnameIs(pEntity->pev, "path_corner"))
			{
				((CPathCorner *)pEntity)->FireOnPass(m_hActivator.Get()?(CBaseEntity *)m_hActivator:this, this, USE_TOGGLE, 0);
				if (FBitSet(pEntity->pev->spawnflags, SF_CORNER_WAITFORTRIG))
					SetBits(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER);
			}
			else if (FClassnameIs(pEntity->pev, "path_track"))
			{
				((CPathTrack *)pEntity)->FireOnPass(m_hActivator.Get()?(CBaseEntity *)m_hActivator:this, this, USE_TOGGLE, 0);
				if (FBitSet(pEntity->pev->spawnflags, SF_PATH_DISABLED))
					SetBits(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER);
			}
			// need pointer to LAST target.
			if (FBitSet(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER))// XDM3038c: unhacking valve's pile of garbage
			{
				StopSound();// XDM
				DontThink();// XDM3038a
				return;
			}
		}
	}

	// ALERT ( at_console, "%f\n", m_flWait );
	if (m_flWait != 0)// -1 wait will wait forever!
	{
		StopSound();// XDM
		pev->nextthink = pev->ltime + m_flWait;
		SetThink(&CFuncTrain::Next);
	}
	else
		Next();// do it RIGHT now!
}

// Train next - path corner needs to change to next target
void CFuncTrain::Next(void)
{
	DBG_PRINT_ENT_THINK(Think);
	// now find our next target
	CBaseEntity	*pTarg = GetNextTarget();
    if (!pTarg)
	{
		// Play stop sound
		StopSound();// XDM
		return;
	}

	// Save last target in case we need to find it again
	m_iszLastTarget = pev->target;
	pev->target = pTarg->pev->target;
	m_flWait = pTarg->GetDelay();

	if ( m_pevCurrentTarget && m_pevCurrentTarget->speed != 0 )// don't copy speed from target if it is 0 (uninitialized)
	{
        pev->speed = m_pevCurrentTarget->speed;// save last target's speed
		//SPAM	ALERT( at_aiconsole, "Train %s speed to %4.2f\n", STRING(pev->targetname), pev->speed );
	}
	m_pevCurrentTarget = pTarg->pev;// keep track of this since path corners change our target for us.
    pev->enemy = pTarg->edict();//hack

	if (FClassnameIs(m_pevCurrentTarget, "path_corner") && FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_TELEPORT))// XDM3038c: WARNING: spawn flag only applies to path_corner!!!
	{
		// Path corner has indicated a teleport to the next corner.
		ClearBits(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER);
		SetBits(pev->effects, EF_NOINTERP);
		UTIL_SetOrigin(this, pTarg->pev->origin - (pev->mins + pev->maxs)* 0.5);
		/*if (m_iszMoveTargetName != NULL)// XDM
		{
			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(m_iszMoveTargetName))) != NULL)
			{
				//pEntity->m_vecMoveOriginDelta = pEntity->pev->origin - pev->origin;
				UTIL_SetOrigin(pEntity, pEntity->m_vecMoveOriginDelta + pev->origin);
			}
		}*/
		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		ClearBits(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER);
		// Normal linear move.
		StartSound();// XDM
		ClearBits(pev->effects, EF_NOINTERP);
		SetMoveDone( &CFuncTrain::Wait );// XDM3035: 20100828
		//if (FBitSet(pTarg->pev->spawnflags, SF_CORNER_INTERPOLATE))
		//	InterpolateMove(pTarg->pev->origin - (pev->mins + pev->maxs)*0.5, pev->speed, pTarg->pev->speed);
		//else
			LinearMove(pTarg->pev->origin - (pev->mins + pev->maxs)*0.5, pev->speed);
	}
}

void CFuncTrain::Activate(void)
{
	DBG_PRINT_ENT("Activate");
	// Not yet active, so teleport to first target
	if ( !m_activated )
	{
		m_activated = TRUE;
		entvars_t	*pevTarg = VARS( FIND_ENTITY_BY_TARGETNAME (NULL, STRING(pev->target) ) );
		if (pevTarg)
		{
			pev->target = pevTarg->target;
			m_pevCurrentTarget = pevTarg;// keep track of this since path corners change our target for us.
			UTIL_SetOrigin(this, pevTarg->origin - (pev->mins + pev->maxs) * 0.5f );

			if ( FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_TRAIN_START_ON))// XDM3038c
			{	// not triggered, so start immediately
				pev->nextthink = pev->ltime + 0.1f;
				StartSound();// XDM
				SetThink(&CFuncTrain::Next);
			}
			else
				SetBits(pev->spawnflags, SF_TRAIN_WAIT_RETRIGGER);
		}
	}
}

void CFuncTrain::OverrideReset(void)
{
	DBG_PRINT_ENT("OverrideReset");
	// Are we moving?
	if (!pev->velocity.IsZero() && pev->nextthink != 0)
	{
		pev->target = m_iszLastTarget;
		// now find our next target
		CBaseEntity	*pTarg = GetNextTarget();
		if (!pTarg)
		{
			DontThink();// XDM3038a
			pev->velocity.Clear();
		}
		else// Keep moving for 0.1 secs, then find path_corner again and restart
		{
			SetThink(&CFuncTrain::Next);
			pev->nextthink = pev->ltime + 0.1;
		}
	}
}

// ---------------------------------------------------------------------
//
// Track Train
//
// ---------------------------------------------------------------------

// XDM3035c: the order differs from original HL! (ttrain5 was inserted)
const char *CFuncTrackTrain::MoveSounds[] =
{
	"common/null.wav",
	"plats/ttrain1.wav",
	"plats/ttrain2.wav",
	"plats/ttrain3.wav",
	"plats/ttrain4.wav",
	"plats/ttrain5.wav",
	"plats/ttrain6.wav",
	"plats/ttrain7.wav",
//	"common/null.wav",
};


TYPEDESCRIPTION	CFuncTrackTrain::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTrackTrain, m_ppath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTrackTrain, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_height, FIELD_FLOAT ),
// XDM3038: OBSOLETE	DEFINE_FIELD( CFuncTrackTrain, m_speed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_dir, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_startSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_controlMins, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTrackTrain, m_controlMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTrackTrain, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackTrain, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_flBank, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTrackTrain, m_oldSpeed, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CFuncTrackTrain, CBaseDelay);// XDM3038: fix

LINK_ENTITY_TO_CLASS(func_tracktrain, CFuncTrackTrain);

void CFuncTrackTrain::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wheels"))
	{
		m_length = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_height = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "startspeed"))
	{
		m_startSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	/*else if (FStrEq(pkvd->szKeyName, "maxspeed"))// XDM3038: for testing mostly
	{
		m_speed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_flVolume = (float) (atoi(pkvd->szValue));
		m_flVolume *= 0.1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bank"))
	{
		m_flBank = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal
*/
void CFuncTrackTrain::Spawn(void)
{
	//m_hActivator = NULL;// XDM3035?
	if (pev->speed == 0)
	{
		conprintf(0, "Design error: %s[%d] \"%s\" with no speed!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
		pev->maxspeed = 100;
	}
	else
	{
		pev->maxspeed = pev->speed;
		pev->speed = 0;
	}
	pev->velocity.Clear();
	pev->avelocity.Clear();
	// used for exporting to TrainSpeed	pev->impulse = pev->maxspeed;
	m_dir = 1;

	if (FStringNull(pev->target))
		conprintf(0, "Design error: %s[%d] \"%s\" with no target!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));

	if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_PASSABLE))
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype = MOVETYPE_PUSH;

	Precache();// XDM3038: mv

	SET_MODEL(edict(), STRING(pev->model));

	UTIL_SetSize(this, pev->mins, pev->maxs);
	UTIL_SetOrigin(this, pev->origin);

	// Cache off placed origin for train controls
	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;
	// start trains on the next frame, to make sure their targets have had
	// a chance to spawn/activate
	NextThink(pev->ltime + 0.1f, FALSE);
	SetThink(&CFuncTrackTrain::Find);
}

void CFuncTrackTrain::Precache(void)
{
	if (m_flVolume == 0.0)
		m_flVolume = 1.0;

	if (FStringNull(pev->noise))// XDM3035c: real custom sound has first priority
	{
		if (m_sounds >= 1 && m_sounds <= 7)
			pev->noise = MAKE_STRING(MoveSounds[m_sounds]);
		else
			pev->noise = iStringNull;
	}

	if (!FStringNull(pev->noise))// if user have chosen NO SOUND, disable start/stop sounds too
	{
		PRECACHE_SOUND(STRINGV(pev->noise));

		if (FStringNull(pev->noise1))
			pev->noise1 = MAKE_STRING("plats/ttrain_start1.wav");
		if (FStringNull(pev->noise2))
			pev->noise2 = MAKE_STRING("plats/ttrain_brake1.wav");

		PRECACHE_SOUND(STRINGV(pev->noise1));
		PRECACHE_SOUND(STRINGV(pev->noise2));
	}
	//m_usAdjustPitch = PRECACHE_EVENT(1, "events/train.sc");
}

void CFuncTrackTrain::PrepareForTransfer(void)// XDM3037
{
	CBaseDelay::PrepareForTransfer();
	m_ppath = NULL;
}

void CFuncTrackTrain::NextThink(float thinkTime, BOOL alwaysThink)
{
	if (alwaysThink)
		pev->flags |= FL_ALWAYSTHINK;
	else
		pev->flags &= ~FL_ALWAYSTHINK;

	pev->nextthink = thinkTime;
}

// XDM3038a: stop when game ends
void CFuncTrackTrain::Think(void)
{
	if (g_pGameRules && g_pGameRules->IsMultiplayer() && g_pGameRules->IsGameOver())
	{
		pev->speed = 0;
		pev->velocity.Clear();
		pev->avelocity.Clear();
		StopSound();
		SetThinkNull();
		SetNextThink(10);
	}
	else
		CBaseDelay::Think();
}

void CFuncTrackTrain::Blocked(CBaseEntity *pOther)
{
	// Blocker is on-ground on the train
	if (FBitSet(pOther->pev->flags, FL_ONGROUND) && VARS(pOther->pev->groundentity) == pev)
	{
		float deltaSpeed = fabs(pev->speed);
		if (deltaSpeed > 50)
			deltaSpeed = 50;
		if (pOther->pev->velocity.z == 0.0f)
			pOther->pev->velocity.z += deltaSpeed;

		return;
	}
	else
		pOther->pev->velocity = (pOther->pev->origin - pev->origin).Normalize() * pev->dmg;

	//conprintf(2, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", STRING(pev->targetname), STRING(pOther->pev->classname), pev->dmg );
	if ( pev->dmg <= 0 )
		return;
	// we can't hurt this thing, so we're not concerned with it
	//pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);
	//pOther->TakeDamage(this, pev->euser1==NULL?this:CBaseEntity::Instance(pev->euser1), pev->dmg, DMG_CRUSH);// XDM3035
	pOther->TakeDamage(this, m_hActivator?(CBaseEntity *)m_hActivator:this, pev->dmg, DMG_CRUSH);// XDM3035: new
}

void CFuncTrackTrain::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	bool bForceStop = false;// XDM3038
	if (pActivator && useType == USE_OFF)// XDM3035
	{
		//if (pev->euser1 == pActivator->edict())// XDM: old
		//	pev->euser1 = NULL;
		if (m_hActivator == pActivator)// XDM: invoked by owner
		{
			m_hActivator = NULL;

			conprintf(2, "%s \"%s\": control disabled\n", STRING(pev->classname), STRING(pev->targetname));
			if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_STOPUNCONTROLLED))// XDM3038: force stop if uncontrolled
				bForceStop = true;
			else
				return;// do nothing
		}
		// Invoked by something else, continue with USE_OFF
	}

	if (IsLockedByMaster(pActivator))// XDM3037: disabled train
		return;

	//m_hActivator = pActivator;
	if (useType != USE_SET)
	{
		if (ShouldToggle(useType, (pev->speed != 0)) || bForceStop)
		{
			if (pev->speed == 0 && !bForceStop)
			{
				pev->speed = pev->maxspeed * m_dir;
				PostponeNext();
			}
			else// (pev->speed != 0 || bForceStop)
			{
				pev->speed = 0;
				pev->velocity.Clear();
				pev->avelocity.Clear();
				StopSound();
				SetThinkNull();
			}
		}
	}
	else
	{
		float delta = pev->speed/pev->maxspeed + value/TRAIN_NUMSPEEDMODES;// XDM3035b: WTF was this?!
		//delta = ((int)(pev->speed * 4) / (int)pev->maxspeed)/4 + value/4;
		//delta = ((int)(pev->speed * 4) / (int)pev->maxspeed)*0.25 + 0.25 * value;
		if (delta > 1)
			delta = 1;
		else if (delta < -1)
			delta = -1;

		if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_FORWARDONLY))
		{
			if (delta < 0)
				delta = 0;
		}
		pev->speed = pev->maxspeed * delta;
		PostponeNext();
		//SPAM	ALERT( at_aiconsole, "TRAIN(%s), speed to %.2f\n", STRING(pev->targetname), pev->speed );
	}
}

void CFuncTrackTrain::StopSound(void)
{
	DBG_PRINT_ENT("StopSound");
	// if sound playing, stop it
	if (m_soundPlaying)
	{
	/* XDM3035c: this shit becomes useless
	unsigned short us_encode;
		unsigned short us_sound = ( ( unsigned short )( m_sounds ) & 0x0007 ) << 12;
		us_encode = us_sound;
		PLAYBACK_EVENT_FULL(FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, us_encode, 0, 1, 0);*/

		if (!FStringNull(pev->noise))
			STOP_SOUND(edict(), CHAN_STATIC, STRINGV(pev->noise));

		if (!FStringNull(pev->noise2))
			EMIT_SOUND_DYN(edict(), CHAN_ITEM, STRINGV(pev->noise2), m_flVolume, ATTN_NORM, 0, PITCH_NORM);
	}
	pev->frame = 0;// XDM3037
	m_soundPlaying = 0;
}

// update pitch based on speed, start sound if not playing
// NOTE: when train goes through transition, m_soundPlaying should go to 0,
// which will cause the looped sound to restart.
void CFuncTrackTrain::UpdateSound(void)
{
	DBG_PRINT_ENT("UpdateSound");
	float flpitch = TRAIN_STARTPITCH + (abs(pev->speed) * (TRAIN_MAXPITCH - TRAIN_STARTPITCH) / TRAIN_MAXSPEED);
	if (!m_soundPlaying)
	{
		// play startup sound for train
		if (!FStringNull(pev->noise1))
			EMIT_SOUND_DYN(edict(), CHAN_ITEM, STRINGV(pev->noise1), m_flVolume, ATTN_NORM, 0, PITCH_NORM);

		if (!FStringNull(pev->noise))
			EMIT_SOUND_DYN(edict(), CHAN_STATIC, STRINGV(pev->noise), m_flVolume, ATTN_NORM, 0, (int)flpitch);

		m_soundPlaying = 1;
	}
	else
	{
		// update pitch
		if (!FStringNull(pev->noise))
			EMIT_SOUND_DYN(edict(), CHAN_STATIC, STRINGV(pev->noise), m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, (int)flpitch);

		// volume 0.0 - 1.0 - 6 bits
		// m_sounds 3 bits
		// flpitch = 6 bits
		// 15 bits total
		/*unsigned short us_encode;
		unsigned short us_sound  = ( ( unsigned short )( m_sounds ) & 0x0007 ) << 12;
		unsigned short us_pitch  = ( ( unsigned short )( flpitch / 10.0 ) & 0x003f ) << 6;
		unsigned short us_volume = ( ( unsigned short )( m_flVolume * 40.0 ) & 0x003f );
		us_encode = us_sound | us_pitch | us_volume;
		PLAYBACK_EVENT_FULL(FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, us_encode, 0, 0, 0);*/
	}
	pev->frame = 1;// XDM3037
}

void CFuncTrackTrain::PostponeNext(void)
{
	DBG_PRINT_ENT("PostponeNext");
	// XDM3037: g-cont. well...
#if defined(MOVEWITH)
	if (m_pAssistLink)
		UTIL_DesiredAction(this);
	else
#endif
	DesiredAction();// this simply fix LAARGE BUG with func_traktrain in spirit ;) g-cont
}

void CFuncTrackTrain::DesiredAction(void)// was TrackNext(void)
{
	DBG_PRINT_ENT("DesiredAction");
	if (pev->speed == 0)
	{
		// SPAM	ALERT( at_aiconsole, "TRAIN(%s): Speed is 0\n", STRING(pev->targetname) );
		StopSound();
		return;
	}

	if (m_ppath == NULL)
	{
		//m_ppath = CPathTrack::Instance(FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) ));
		conprintf(2, "TRAIN(%s): Lost path\n", STRING(pev->targetname));
		StopSound();
		if (!FStringNull(pev->target))
		{
			NextThink(pev->ltime + 0.1f, FALSE);// XDM3038
			SetThink(&CFuncTrackTrain::Find);// XDM3038
		}
		return;
	}

	UpdateSound();

	Vector nextPos(pev->origin);
	nextPos.z -= m_height;
	CPathTrack *pNext = m_ppath->LookAhead(&nextPos, pev->speed * 0.1f, 1);
	nextPos.z += m_height;

	pev->velocity = (nextPos - pev->origin) * 10.0f;
	Vector nextFront = pev->origin;

	nextFront.z -= m_height;
	if ( m_length > 0 )
		m_ppath->LookAhead( &nextFront, m_length, 0 );
	else
		m_ppath->LookAhead( &nextFront, 100, 0 );

	nextFront.z += m_height;

	if (!FBitSet(m_ppath->pev->spawnflags, SF_PATH_DONTROTATE))// XDM
	{
		Vector delta = nextFront - pev->origin;
		Vector angles;// = UTIL_VecToAngles( delta );
		VectorAngles(delta, angles);
#if defined (NOSQB)
		angles.x = -angles.x;
#endif
		// The train actually points west
		angles.y += 180;

		// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
		NormalizeAngles360(angles);
		NormalizeAngles360(pev->angles);

		if (!pNext || (delta.x == 0 && delta.y == 0) )
			angles = pev->angles;

		float vy, vx;
		if (!FBitSet(pev->spawnflags, SF_TRACKTRAIN_NOPITCH))
			vx = UTIL_AngleDistance( angles.x, pev->angles.x );
		else
			vx = 0;
		vy = UTIL_AngleDistance( angles.y, pev->angles.y );

		pev->avelocity.y = vy * 10.0f;
		pev->avelocity.x = vx * 10.0f;

		if ( m_flBank != 0 )
		{
			if ( pev->avelocity.y < -5 )
				pev->avelocity.z = UTIL_AngleDistance( UTIL_ApproachAngle( -m_flBank, pev->angles.z, m_flBank*2 ), pev->angles.z);
			else if ( pev->avelocity.y > 5 )
				pev->avelocity.z = UTIL_AngleDistance( UTIL_ApproachAngle( m_flBank, pev->angles.z, m_flBank*2 ), pev->angles.z);
			else
				pev->avelocity.z = UTIL_AngleDistance( UTIL_ApproachAngle( 0, pev->angles.z, m_flBank*4 ), pev->angles.z) * 4;
		}
	}

	float time = 0.5;
	if (pNext)
	{
		if (pNext != m_ppath)
		{
			CPathTrack *pFire;
			if (pev->speed >= 0)
				pFire = pNext;
			else
				pFire = m_ppath;

			m_ppath = pNext;

			// Fire the pass target if there is one
			/* OBSOLETE if (!FStringNull(pFire->pev->message))
			{
				FireTargets( STRING(pFire->pev->message), this, this, USE_TOGGLE, 0 );
				if ( FBitSet( pFire->pev->spawnflags, SF_PATH_FIREONCE ) )
					pFire->pev->message = iStringNull;
			}*/
			pFire->FireOnPass(m_hActivator.Get()?(CBaseEntity *)m_hActivator:this, this, USE_TOGGLE, 0);// XDM3038c: a much better way to Use()

			if (FBitSet(pFire->pev->spawnflags, SF_PATH_DISABLE_TRAIN))
				SetBits(pev->spawnflags, SF_TRACKTRAIN_NOCONTROL);

			// Don't override speed if under user control
			if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_NOCONTROL))
			{
				if (pFire->pev->speed != 0)
				{// don't copy speed from target if it is 0 (uninitialized)
					pev->speed = pFire->pev->speed;
					// SPAM	ALERT( at_aiconsole, "TrackTrain %s speed to %4.2f\n", STRING(pev->targetname), pev->speed );
				}
			}

		}
		SetThink(&CFuncTrackTrain::PostponeNext);
		NextThink(pev->ltime + time, TRUE);
	}
	else	// end of path, stop
	{
		StopSound();
		pev->velocity = (nextPos - pev->origin);
		pev->avelocity.Clear();
		vec_t distance = pev->velocity.Length();
		m_oldSpeed = pev->speed;

		pev->speed = 0;
		// Move to the dead end
		// Are we there yet?
		if (distance > 0)
		{
			// no, how long to get there?
			time = distance / m_oldSpeed;
			pev->velocity *= m_oldSpeed / distance;
			SetThink( &CFuncTrackTrain::DeadEnd );
			NextThink( pev->ltime + time, FALSE );
		}
		else
		{
			DeadEnd();
		}
	}
}

void CFuncTrackTrain::DeadEnd(void)
{
	DBG_PRINT_ENT_THINK(DeadEnd);
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;
	pTrack = m_ppath;

	conprintf(2, "TRAIN(%s): Dead end ", STRING(pev->targetname));
	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if ( pTrack )
	{
		if ( m_oldSpeed < 0 )
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetPrevious(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetNext(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
	}

	pev->velocity.Clear();
	pev->avelocity.Clear();
	if ( pTrack )
	{
		conprintf(2, "at %s\n", STRING(pTrack->pev->targetname));
		if ( pTrack->pev->netname )
			FireTargets( STRING(pTrack->pev->netname), this, this, USE_TOGGLE, 0 );
	}
	else
		conprintf(2, "\n");
}

// Copy bounds of func_traincontrols to me
void CFuncTrackTrain::SetControls(entvars_t *pevControls)
{
	if (pevControls)// XDM3038
	{
		Vector offset = pevControls->origin - pev->oldorigin;
		m_controlMins = pevControls->mins + offset;
		m_controlMaxs = pevControls->maxs + offset;
	}
}

bool CFuncTrackTrain::OnControls(entvars_t *pevTest)
{
	if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_NOCONTROL))
		return false;

	// Transform offset into local coordinates
	UTIL_MakeVectors( pev->angles );// AngleVectors()?
	Vector offset = pevTest->origin - pev->origin;
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = -DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	return PointInBounds(local, m_controlMins, m_controlMaxs);// XDM3038
	/*if (local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z)
		return true;

	return false;*/
}

void CFuncTrackTrain::Find(void)
{
	DBG_PRINT_ENT_THINK(Find);
	m_ppath = CPathTrack::Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target)));
	if (!m_ppath)
		return;

	entvars_t *pevTarget = m_ppath->pev;
	if (!FClassnameIs(pevTarget, "path_track"))
	{
		conprintf(1, "func_track_train must be on a path of path_track\n");// XDM3038c: don't crash
		m_ppath = NULL;
		return;
	}

	Vector nextPos = pevTarget->origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead( &look, m_length, 0 );
	look.z += m_height;

	//pev->angles = UTIL_VecToAngles( look - nextPos );
	VectorAngles(look - nextPos, pev->angles);
	// The train actually points west
	pev->angles.y += 180;// TODO: TESTME: SQB?

	if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_NOPITCH))
		pev->angles.x = 0;
    UTIL_SetOrigin(this, nextPos);
	NextThink( pev->ltime + 0.1f, FALSE );
	SetThink( &CFuncTrackTrain::PostponeNext );
	pev->speed = m_startSpeed;

	UpdateSound();
}

void CFuncTrackTrain::NearestPath(void)
{
	DBG_PRINT_ENT_THINK(NearestPath);
	CBaseEntity *pTrack = NULL;
	CBaseEntity *pNearest = NULL;
	vec_t dist, closest = g_psv_zmax?g_psv_zmax->value:2048;

	while ((pTrack = UTIL_FindEntityByClassname(pTrack, "path_track")) != NULL)// XDM3038
	{
		// filter out non-tracks
		// XDM3038	if ( !(pTrack->pev->flags & (FL_CLIENT|FL_MONSTER)) && FClassnameIs( pTrack->pev, "path_track" ) )
		{
			dist = (pev->origin - pTrack->pev->origin).Length();
			if ( dist < closest )
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if (!pNearest)
	{
		conprintf(1, "%s \"%s\" Can't find a nearby track !!!\n", STRING(pev->classname), STRING(pev->targetname));
		SetThinkNull();
		return;
	}

	//SPAM	ALERT( at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING(pev->targetname), STRING(pNearest->pev->targetname) );
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack *)pNearest)->GetNext();
	if ( pTrack )
	{
		if ( (pev->origin - pTrack->pev->origin).Length() < (pev->origin - pNearest->pev->origin).Length() )
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack *)pNearest;

	if ( pev->speed != 0 )
	{
		NextThink(pev->ltime + 0.1f, FALSE);
		SetThink(&CFuncTrackTrain::PostponeNext);// OK
	}
}

void CFuncTrackTrain::OverrideReset(void)
{
	DBG_PRINT_ENT("OverrideReset");
	NextThink(pev->ltime + 0.1f, FALSE);
	SetThink( &CFuncTrackTrain::NearestPath );// OK
}

CFuncTrackTrain *CFuncTrackTrain::Instance( edict_t *pent )
{
	if ( FClassnameIs( pent, "func_tracktrain" ) )
		return (CFuncTrackTrain *)GET_PRIVATE(pent);
	return NULL;
}








// This class defines the volume of space that the player must stand in to control the train
class CFuncTrainControls : public CBaseEntity
{
public:
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void Spawn(void);
	void EXPORT Find(void);
};

LINK_ENTITY_TO_CLASS( func_traincontrols, CFuncTrainControls );

void CFuncTrainControls::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(this, pev->mins, pev->maxs);
	UTIL_SetOrigin(this, pev->origin);
	SetThink(&CFuncTrainControls::Find);
	SetNextThink(0);
}

void CFuncTrainControls::Find(void)
{
	DBG_PRINT_ENT_THINK(Find);
	edict_t *pTarget = NULL;
	do
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING(pev->target) );
	} while ( !FNullEnt(pTarget) && !FClassnameIs(pTarget, "func_tracktrain") );

	if ( FNullEnt( pTarget ) )
	{
		conprintf(1, "No train %s\n", STRING(pev->target));
		return;
	}

	CFuncTrackTrain *pTrain = CFuncTrackTrain::Instance(pTarget);
	if (pTrain)
		pTrain->SetControls(pev);

	UTIL_Remove( this );
}


// This is the rotating platform that moves trains up/down
LINK_ENTITY_TO_CLASS( func_trackchange, CFuncTrackChange );

TYPEDESCRIPTION	CFuncTrackChange::m_SaveData[] =
{
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackTop, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackBottom, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_train, FIELD_CLASSPTR ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackTopName, FIELD_STRING ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trackBottomName, FIELD_STRING ),
	DEFINE_GLOBAL_FIELD( CFuncTrackChange, m_trainName, FIELD_STRING ),
	DEFINE_FIELD( CFuncTrackChange, m_code, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackChange, m_targetState, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTrackChange, m_use, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncTrackChange, CFuncPlatRot );

void CFuncTrackChange::Spawn(void)
{
	Setup();
	if ( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
		m_vecPosition2.z = pev->origin.z;

	SetupRotation();

	if ( FBitSet( pev->spawnflags, SF_TRACK_STARTBOTTOM ) )
	{
		UTIL_SetOrigin(this, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
		pev->angles = m_start;
		m_targetState = TS_AT_TOP;
	}
	else
	{
		UTIL_SetOrigin(this, m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		pev->angles = m_end;
		m_targetState = TS_AT_BOTTOM;
	}

	EnableUse();
	pev->nextthink = pev->ltime + 2.0f;
	SetThink( &CFuncTrackChange::Find );
	Precache();
}

void CFuncTrackChange::Precache(void)
{
	// Can't trigger sound
	PRECACHE_SOUND( "buttons/button11.wav" );
	CFuncPlatRot::Precache();
}

// UNDONE: Filter touches before re-evaluating the train.
void CFuncTrackChange::Touch(CBaseEntity *pOther)
{
#if 0
	TRAIN_CODE code;
	entvars_t *pevToucher = pOther->pev;
#endif
}

void CFuncTrackChange::KeyValue(KeyValueData *pkvd)
{
	if ( FStrEq(pkvd->szKeyName, "train") )
	{
		m_trainName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "toptrack") )
	{
		m_trackTopName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "bottomtrack") )
	{
		m_trackBottomName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CFuncPlatRot::KeyValue(pkvd);		// Pass up to base class
}

void CFuncTrackChange::OverrideReset(void)
{
	DBG_PRINT_ENT("OverrideReset");
	pev->nextthink = pev->ltime + 1.0f;
	SetThink( &CFuncTrackChange::Find );
}

void CFuncTrackChange::Find(void)
{
	DBG_PRINT_ENT_THINK(Find);
	// Find track entities
	edict_t *target;

	target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(m_trackTopName) );
	if ( !FNullEnt(target) )
	{
		m_trackTop = CPathTrack::Instance( target );
		target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(m_trackBottomName) );
		if ( !FNullEnt(target) )
		{
			m_trackBottom = CPathTrack::Instance( target );
			target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(m_trainName) );
			if ( !FNullEnt(target) )
			{
				m_train = CFuncTrackTrain::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING(m_trainName) ) );
				if ( !m_train )
				{
					ALERT( at_error, "Can't find train for track change! %s\n", STRING(m_trainName) );
					return;
				}
				Vector center(VecBModelOrigin(pev));
				m_trackBottom = m_trackBottom->Nearest( center );
				m_trackTop = m_trackTop->Nearest( center );
				UpdateAutoTargets( m_toggle_state );
				SetThinkNull();
				return;
			}
			else
			{
				ALERT( at_error, "Can't find train for track change! %s\n", STRING(m_trainName) );
				target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(m_trainName) );
			}
		}
		else
			ALERT( at_error, "Can't find bottom track for track change! %s\n", STRING(m_trackBottomName) );
	}
	else
		ALERT( at_error, "Can't find top track for track change! %s\n", STRING(m_trackTopName) );
}

TRAIN_CODE CFuncTrackChange::EvaluateTrain( CPathTrack *pcurrent )
{
	// Go ahead and work, we don't have anything to switch, so just be an elevator
	if ( !pcurrent || !m_train )
		return TRAIN_SAFE;

	if ( m_train->m_ppath == pcurrent || (pcurrent->m_pprevious && m_train->m_ppath == pcurrent->m_pprevious) ||
		 (pcurrent->m_pnext && m_train->m_ppath == pcurrent->m_pnext) )
	{
		if ( m_train->pev->speed != 0 )
			return TRAIN_BLOCKING;

		Vector dist = pev->origin - m_train->pev->origin;
		vec_t length = dist.Length2D();
		if ( length < m_train->m_length )		// Empirically determined close distance
			return TRAIN_FOLLOWING;
		else if ( length > (150 + m_train->m_length) )
			return TRAIN_SAFE;

		return TRAIN_BLOCKING;
	}

	return TRAIN_SAFE;
}

void CFuncTrackChange::UpdateTrain( Vector &dest )
{
	if (!m_train)
		return;

	// XDM3038c: don't control train that is not touching me!
	if (!BoundsIntersect(pev->absmin+pev->mins, pev->absmax+pev->maxs, m_train->pev->absmin/*+m_train->pev->mins*/, m_train->pev->absmax/*+m_train->pev->maxs*/))
		return;

	float time = (pev->nextthink - pev->ltime);
	m_train->pev->velocity = pev->velocity;
	m_train->pev->avelocity = pev->avelocity;
	m_train->NextThink( m_train->pev->ltime + time, FALSE );

	// Attempt at getting the train to rotate properly around the origin of the trackchange
	if ( time <= 0 )
		return;

	Vector offset(m_train->pev->origin); offset -= pev->origin;
	Vector delta(dest); delta -= pev->angles;
	// Transform offset into local coordinates
	UTIL_MakeInvVectors( delta, gpGlobals );
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );
	local -= offset;
	m_train->pev->velocity = pev->velocity + (local * (1.0f/time));
	SetBits(m_train->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL);// XDM3038
}

void CFuncTrackChange::GoDown(void)
{
	DBG_PRINT_ENT_THINK(GoDown);
	if ( m_code == TRAIN_BLOCKING )
		return;

	// HitBottom may get called during CFuncPlat::GoDown(), so set up for that
	// before you call GoDown()

	UpdateAutoTargets( TS_GOING_DOWN );
	// If ROTMOVE, move & rotate
	SetMoveDone(&CFuncPlat::CallHitBottom);// XDM3035: 20100828
	pev->frame = 1;// XDM3038

	if ( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		m_toggle_state = TS_GOING_DOWN;
		AngularMove( m_start, pev->speed );
	}
	else
	{
		CFuncPlat::GoDown();
		RotMove( m_start, pev->nextthink - pev->ltime );
	}
	// Otherwise, rotate first, move second

	// If the train is moving with the platform, update it
	if ( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_start );
		m_train->m_ppath = NULL;
	}
}

//
// Platform is at bottom, now starts moving up
//
void CFuncTrackChange::GoUp(void)
{
	DBG_PRINT_ENT_THINK(GoUp);
	if ( m_code == TRAIN_BLOCKING )
		return;

	// HitTop may get called during CFuncPlat::GoUp(), so set up for that
	// before you call GoUp();
	SetMoveDone(&CFuncPlat::CallHitTop);// XDM3035: 20100828
	pev->frame = 1;// XDM3038

	UpdateAutoTargets( TS_GOING_UP );
	if ( FBitSet( pev->spawnflags, SF_TRACK_DONT_MOVE ) )
	{
		m_toggle_state = TS_GOING_UP;
		AngularMove( m_end, pev->speed );
	}
	else
	{
		// If ROTMOVE, move & rotate
		CFuncPlat::GoUp();
		RotMove( m_end, pev->nextthink - pev->ltime );
	}

	// Otherwise, move first, rotate second

	// If the train is moving with the platform, update it
	if ( m_code == TRAIN_FOLLOWING )
	{
		UpdateTrain( m_end );
		m_train->m_ppath = NULL;
// TODO?		m_train->m_iOnTrackChange = true;
	}
}

// Normal track change
void CFuncTrackChange::UpdateAutoTargets( int toggleState )
{
	if ( !m_trackTop || !m_trackBottom )
		return;

	if ( toggleState == TS_AT_TOP )
		ClearBits( m_trackTop->pev->spawnflags, SF_PATH_DISABLED );
	else
		SetBits( m_trackTop->pev->spawnflags, SF_PATH_DISABLED );

	if ( toggleState == TS_AT_BOTTOM )
		ClearBits( m_trackBottom->pev->spawnflags, SF_PATH_DISABLED );
	else
		SetBits( m_trackBottom->pev->spawnflags, SF_PATH_DISABLED );
}

void CFuncTrackChange::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	DBG_PRINT_ENT_USE(Use);
	if ( m_toggle_state != TS_AT_TOP && m_toggle_state != TS_AT_BOTTOM )
		return;

	// If train is in "safe" area, but not on the elevator, play alarm sound
	if ( m_toggle_state == TS_AT_TOP )
		m_code = EvaluateTrain( m_trackTop );
	else if ( m_toggle_state == TS_AT_BOTTOM )
		m_code = EvaluateTrain( m_trackBottom );
	else
		m_code = TRAIN_BLOCKING;

	if (m_code == TRAIN_BLOCKING)
	{
		// Play alarm and return
		EMIT_SOUND(edict(), CHAN_VOICE, "buttons/button11.wav", VOL_NORM, ATTN_NORM);
		return;
	}

	// Otherwise, it's safe to move
	// If at top, go down
	// at bottom, go up

	DisableUse();
	if (m_toggle_state == TS_AT_TOP)
		GoDown();
	else
		GoUp();
}

//
// Platform has hit bottom. Stops and waits forever.
//
void CFuncTrackChange::HitBottom(void)
{
	CFuncPlatRot::HitBottom();
	if ( m_code == TRAIN_FOLLOWING )
	{
		//UpdateTrain();
		ClearBits(m_train->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL);// XDM3038
		m_train->SetTrack( m_trackBottom );
	}
	SetThinkNull();
	pev->nextthink = -1;
	pev->frame = 0;// XDM3038
	UpdateAutoTargets( m_toggle_state );
	EnableUse();
}

//
// Platform has hit bottom. Stops and waits forever.
//
void CFuncTrackChange::HitTop(void)
{
	CFuncPlatRot::HitTop();
	if ( m_code == TRAIN_FOLLOWING )
	{
		//UpdateTrain();
		ClearBits(m_train->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL);// XDM3038
		m_train->SetTrack( m_trackTop );
	}
	// Don't let the plat go back down
	SetThinkNull();
	pev->nextthink = -1;
	pev->frame = 0;// XDM3038
	UpdateAutoTargets( m_toggle_state );
	EnableUse();
}


class CFuncTrackAuto : public CFuncTrackChange
{
public:
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void UpdateAutoTargets( int toggleState );
};

LINK_ENTITY_TO_CLASS( func_trackautochange, CFuncTrackAuto );

// Auto track change
void CFuncTrackAuto::UpdateAutoTargets( int toggleState )
{
	CPathTrack *pTarget, *pNextTarget;

	if ( !m_trackTop || !m_trackBottom )
		return;

	if ( m_targetState == TS_AT_TOP )
	{
		pTarget = m_trackTop->GetNext();
		pNextTarget = m_trackBottom->GetNext();
	}
	else
	{
		pTarget = m_trackBottom->GetNext();
		pNextTarget = m_trackTop->GetNext();
	}
	if ( pTarget )
	{
		ClearBits( pTarget->pev->spawnflags, SF_PATH_DISABLED );
		if ( m_code == TRAIN_FOLLOWING && m_train && m_train->pev->speed == 0 )
			m_train->Use( this, this, USE_ON, 0 );
	}

	if ( pNextTarget )
		SetBits( pNextTarget->pev->spawnflags, SF_PATH_DISABLED );

}

void CFuncTrackAuto::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !UseEnabled() )
		return;

	CPathTrack *pTarget;
	if ( m_toggle_state == TS_AT_TOP )
		pTarget = m_trackTop;
	else if ( m_toggle_state == TS_AT_BOTTOM )
		pTarget = m_trackBottom;
	else
		pTarget = NULL;

	if ( FClassnameIs(pActivator->pev, "func_tracktrain" ) )
	{
		m_code = EvaluateTrain( pTarget );
		// Safe to fire?
		if ( m_code == TRAIN_FOLLOWING && m_toggle_state != m_targetState )
		{
			DisableUse();
			if (m_toggle_state == TS_AT_TOP)
				GoDown();
			else
				GoUp();
		}
	}
	else
	{
		if ( pTarget )
			pTarget = pTarget->GetNext();
		if ( pTarget && m_train->m_ppath != pTarget && ShouldToggle( useType, m_targetState != STATE_OFF) )
		{
			if ( m_targetState == TS_AT_TOP )
				m_targetState = TS_AT_BOTTOM;
			else
				m_targetState = TS_AT_TOP;
		}

		UpdateAutoTargets( m_targetState );
	}
}

// ----------------------------------------------------------
//
//
// pev->speed is the travel speed
// pev->health is current health
// pev->max_health is the amount to reset to each time it starts
#define FGUNTARGET_START_ON			0x0001

class CGunTarget : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Activate(void);
	virtual void Stop(void);
	virtual int BloodColor(void) { return DONT_BLEED; }
	virtual int Classify(void) { return CLASS_MACHINE; }
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual Vector BodyTarget(const Vector &posSrc) { return pev->origin; }
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	void EXPORT Next(void);
	void EXPORT Start(void);
	void EXPORT Wait(void);

	static	TYPEDESCRIPTION m_SaveData[];

private:
	BOOL m_on;
};


LINK_ENTITY_TO_CLASS( func_guntarget, CGunTarget );

TYPEDESCRIPTION	CGunTarget::m_SaveData[] =
{
	DEFINE_FIELD( CGunTarget, m_on, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CGunTarget, CBaseMonster );

void CGunTarget::Spawn(void)
{
	pev->solid		= SOLID_BSP;
	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(this, pev->origin);
	SET_MODEL(edict(), STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	// Don't take damage until "on"
	pev->takedamage = DAMAGE_NO;
	pev->flags |= FL_MONSTER;

	m_on = FALSE;
	pev->max_health = pev->health;

	if (FBitSet(pev->spawnflags, FGUNTARGET_START_ON))
	{
		SetThink( &CGunTarget::Start );
		pev->nextthink = pev->ltime + 0.3f;
	}
}

void CGunTarget::Activate(void)
{
	// now find our next target
	CBaseEntity	*pTarg = GetNextTarget();
	if (pTarg)
	{
		m_hTargetEnt = pTarg;
		UTIL_SetOrigin(this, pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5f);
	}
}

void CGunTarget::Start(void)
{
	Use( this, this, USE_ON, 0 );
}

void CGunTarget::Next(void)
{
	SetThinkNull();

	m_hTargetEnt = GetNextTarget();
	CBaseEntity *pTarget = m_hTargetEnt;

	if ( !pTarget )
	{
		Stop();
		return;
	}
	SetMoveDone(&CGunTarget::Wait);// XDM3035: 20100828
	LinearMove( pTarget->pev->origin - (pev->mins + pev->maxs) * 0.5, pev->speed );
}

void CGunTarget::Wait(void)
{
	CBaseEntity *pTarget = m_hTargetEnt;
	if ( !pTarget )
	{
		Stop();
		return;
	}

	// Fire the pass target if there is one
	if (!FStringNull(pTarget->pev->message))
	{
		FireTargets(STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0);
		if ( FBitSet( pTarget->pev->spawnflags, SF_CORNER_FIREONCE ) )
			pTarget->pev->message = iStringNull;
	}

	m_flWait = pTarget->GetDelay();

	pev->target = pTarget->pev->target;
	SetThink( &CGunTarget::Next );
	if (m_flWait != 0)
	{// -1 wait will wait forever!
		pev->nextthink = pev->ltime + m_flWait;
	}
	else
	{
		Next();// do it RIGHT now!
	}
}

void CGunTarget::Stop(void)
{
	pev->velocity.Clear();
	DontThink();// XDM3038a
	pev->takedamage = DAMAGE_NO;
}

int	CGunTarget::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pev->health > 0)
	{
		pev->health -= flDamage;
		if (pev->health <= 0)
		{
			pev->health = 0;
			Stop();
			if (!FStringNull(pev->message))
				FireTargets(STRING(pev->message), this, this, USE_TOGGLE, 0);
		}
	}
	return 0;
}

void CGunTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!ShouldToggle(useType, m_on == TRUE))
		return;

	if ( m_on )
	{
		Stop();
	}
	else
	{
		pev->takedamage = DAMAGE_AIM;
		m_hTargetEnt = GetNextTarget();
		if ( m_hTargetEnt.Get() == NULL )
			return;
		pev->health = pev->max_health;
		Next();
	}
}
