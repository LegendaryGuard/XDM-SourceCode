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

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "triggers.h"
#include "trains.h"// trigger_camera has train functionality
#include "gamerules.h"
#include "maprules.h"
#include "game.h"// XDM: cvar.value
#include "globals.h"
#include "shake.h"
#include "shared_resources.h"
#include "soundent.h"

// XDM3038c: this code is a nightmare, but we need SHL compatibility :(
// UNDONE/TODO
TYPEDESCRIPTION	CInOutRegister::m_SaveData[] =
{
	DEFINE_FIELD(CInOutRegister, m_pField, FIELD_CLASSPTR),
	DEFINE_FIELD(CInOutRegister, m_pNext, FIELD_CLASSPTR),
	DEFINE_FIELD(CInOutRegister, m_hValue, FIELD_EHANDLE),
};
IMPLEMENT_SAVERESTORE(CInOutRegister,CPointEntity);

LINK_ENTITY_TO_CLASS(zone_register, CInOutRegister);

bool CInOutRegister::IsRegistered(CBaseEntity *pValue)
{
	if (m_hValue == pValue)
		return true;
	else if (m_pNext)
		return m_pNext->IsRegistered(pValue);

	return false;
}

CInOutRegister *CInOutRegister::Add(CBaseEntity *pValue)
{
	if (m_hValue == pValue)
	{
		return this;// it's already in the list, don't need to do anything
	}
	else if (m_pNext)// keep looking
	{
		m_pNext = m_pNext->Add(pValue);
		return this;
	}
	else
	{
		// reached the end of the list; add the new entry, and trigger
		CInOutRegister *pResult = (CInOutRegister *)CBaseEntity::Create(STRING(pev->classname), pev->origin, pev->angles, edict());// XDM3038c //GetClassPtr((CInOutRegister *)NULL);
		if (pResult)
		{
			pResult->m_hValue = pValue;
			pResult->m_pNext = this;
			pResult->m_pField = m_pField;
			//pResult->pev->classname = pev->classname;// don't need to reallocate // kinda bad
			// OK pResult->SetClassName("zone_register");
			// BAD! pResult->pev->classname = MAKE_STRING("zone_register");
			m_pField->FireOnEntry(pValue);
		}
		return pResult;
	}
}

CInOutRegister *CInOutRegister::Prune(void)
{
	if (m_hValue.Get())
	{
		ASSERTSZ(m_pNext != NULL, "CInOutRegister::Prune(): invalid InOut registry terminator!\n");
		if (m_pField->Intersects(m_hValue))
		{
			// this entity is still inside the field, do nothing
			m_pNext = m_pNext->Prune();
			return this;
		}
		else
		{
			// this entity has just left the field, trigger
			m_pField->FireOnLeave(m_hValue);
			SetThink(&CInOutRegister::SUB_Remove);
			SetNextThink(0.1);
			return m_pNext->Prune();
		}
	}
	else// this register has a missing or null value
	{
		if (m_pNext)
		{
			// this is an invalid list entry, remove it
			SetThink(&CInOutRegister::SUB_Remove);
			SetNextThink(0.1);
			return m_pNext->Prune();
		}
		else
			return this;// this is the list terminator, leave it.
	}
}


//-----------------------------------------------------------------------------
// BASE TRIGGER
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger, CBaseTrigger);

TYPEDESCRIPTION	CBaseTrigger::m_SaveData[] =
{
//	DEFINE_FIELD(CBaseTrigger, m_iszAltTarget, FIELD_STRING),
//	DEFINE_FIELD(CBaseTrigger, m_iszBothTarget, FIELD_STRING),
	DEFINE_FIELD(CBaseTrigger, m_pIORegister, FIELD_CLASSPTR),
	DEFINE_ARRAY(CBaseTrigger, m_hzTouchedPlayers, FIELD_EHANDLE, MAX_CLIENTS),
//	DEFINE_DYNARRAY(CBaseTrigger, m_TouchedEntities, FIELD_EHANDLE),
};
IMPLEMENT_SAVERESTORE(CBaseTrigger, CBaseToggle);

// Cache user-entity-field values until spawn is called.
void CBaseTrigger::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = (int) atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		if (pkvd->szValue && strcmp(pkvd->szValue, "0"))// XDM: this prevents 'sound not precached' bug
			pev->noise = ALLOC_STRING(pkvd->szValue);// By the way, where does this sound gets precached?!

		pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "height"))// XDM
	{
		m_flHeight = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

bool CBaseTrigger::CanTouch(CBaseEntity *pToucher)
{
	if (pToucher == NULL)// XDM3038c: !
		return false;

#if 0
		// if the trigger has an angles field, check player's facing direction
		if (!pev->movedir.IsZero())
		{
			Vector forward;
			AngleVectors(pToucher->pev->angles, forward, NULL, NULL);
			if (DotProduct(forward, pev->movedir) < 0)
				return FALSE;// not facing the right way
		}
#endif
	if (FStringNull(pev->netname))
	{
		if (FBitSet(pev->spawnflags, SF_TRIGGER_EVERYTHING))// XDM3037
			return true;
		// Only touch clients, monsters, or pushables (depending on flags)
		if (pToucher->IsPlayer())
		{
			if (((CBasePlayer *)pToucher)->IsObserver())// XDM3038a
				return false;

			return !FBitSet(pev->spawnflags, SF_TRIGGER_NOCLIENTS);
		}
		else if (pToucher->IsMonster())
			return FBitSet(pev->spawnflags, SF_TRIGGER_ALLOWMONSTERS);
		else if (pToucher->IsPushable())//(FClassnameIs(pToucher,"func_pushable"))
			return FBitSet(pev->spawnflags, SF_TRIGGER_PUSHABLES);
		else
			return false;//FBitSet(pev->spawnflags, SF_TRIGGER_EVERYTHING);
	}
	else// XDM3037: LAME!
	{
		// If netname is set, it's an entity-specific trigger; we ignore the spawnflags.
		if (!FClassnameIs(pToucher->pev, STRING(pev->netname)) &&
			(FStringNull(pToucher->pev->targetname) || !FStrEq(STRING(pToucher->pev->targetname), STRING(pev->netname))))
			return false;
	}
	return true;
}

void CBaseTrigger::InitTrigger(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	Precache();
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (!pev->angles.IsZero())
		SetMovedir(pev);

	if (FBitSet(pev->spawnflags, SF_TRIGGER_START_OFF))// XDM3035c: TESTME!
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_TRIGGER;

	pev->movetype = MOVETYPE_NONE;
	if (!FStringNull(pev->model))
		SET_MODEL(edict(), STRING(pev->model));// set size and link into world

	if (showtriggers.value <= 0.0f)
		SetBits(pev->effects, EF_NODRAW);
}

bool CBaseTrigger::RegisterToucher(CBaseEntity *pOther)// XDM3038c
{
	if (pOther)
	{
		ASSERT(WasActivePlayer(pOther));
		for (int i=0; i<MAX_CLIENTS; ++i)
		{
			if (m_hzTouchedPlayers[i].Get() == NULL)
			{
				m_hzTouchedPlayers[i] = pOther;
				return true;
			}
			else if (m_hzTouchedPlayers[i] == pOther)
				return true;
		}
	}
	return false;
}

int CBaseTrigger::FindToucher(CBaseEntity *pOther)// XDM3038c
{
	for (int i=0; i<MAX_CLIENTS; ++i)
	{
		if (m_hzTouchedPlayers[i] == pOther)
			return i;
	}
	return MAX_CLIENTS;
}

// XDM3038c: count if all players touched this trigger (useful for CoOp
// NOTE: when player disconnects, his EHANDLE is invalidated in XHL
bool CBaseTrigger::CountTouchers(void)
{
	int i;
	int c = 0;
	for (i=0; i<MAX_CLIENTS; ++i)
	{
		if (m_hzTouchedPlayers[i].Get())
			++c;
	}
	int a = 0;
	for (i=0; i<MAX_CLIENTS; ++i)// don't count players who never joined the game (only spectating)
	{
		if (WasActivePlayer(UTIL_ClientByIndex(i)))
			++a;
	}
	if (c == a)
	{
		DBG_PRINT_ENT("CountTouchers(): all players touched.");
		return true;
	}
	return false;
}

// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void CBaseTrigger::ActivateMultiTrigger(CBaseEntity *pActivator)
{
	if (pev->nextthink > gpGlobals->time)
		return;// still waiting for reset time

	if (IsLockedByMaster(pActivator))
		return;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_REQALLPLAYERS))
	{
		if (RegisterToucher(pActivator))
		{
			if (!CountTouchers())
				return;
		}
	}

	if (!FStringNull(pev->noise))
		EMIT_SOUND(edict(), CHAN_VOICE, STRINGV(pev->noise), VOL_NORM, ATTN_NORM);

	// don't trigger again until reset
	// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0.0f);

	if (!FStringNull(pev->message) && pActivator && pActivator->IsPlayer())
		ClientPrint(pActivator->pev, HUD_PRINTHUD, STRING(pev->message));

	if (m_flWait > 0)
	{
		SetThink(&CBaseTrigger::MultiWaitOver);
		SetNextThink(m_flWait);
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouchNull();
		SetNextThink(0.1f);
		SetThink(&CBaseEntity::SUB_Remove);
	}
}

// the wait time has passed, so set back up for another activation
void CBaseTrigger::MultiWaitOver(void)
{
//	if (pev->max_health)
//		{
//		pev->health		= pev->max_health;
//		pev->takedamage	= DAMAGE_YES;
//		pev->solid		= SOLID_BBOX;
//		}
	SetThinkNull();
}

void CBaseTrigger::CounterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(CounterUse);
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if (m_cTriggersLeft < 0)
		return;
/*
	BOOL fTellActivator = (m_hActivator != 0) && pActivator->IsPlayer() && !FBitSet(pev->spawnflags, SPAWNFLAG_NOMESSAGE);

	if (m_cTriggersLeft != 0)
	{
		if (fTellActivator)
			conprintf(1, "Only %d more to go...\n", m_cTriggersLeft);

		return;
	}

	// !!!UNDONE: I don't think we want these Quakesque messages
	if (fTellActivator)
		conprintf(1, "Sequence completed!");
*/
	ActivateMultiTrigger(m_hActivator);
}

// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
void CBaseTrigger::ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(ToggleUse);
	if (IsLockedByMaster(pActivator))// XDM3035c
		return;

	if (pev->solid == SOLID_NOT)
	{
		m_hActivator = pActivator;// XDM3035
		// if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;
		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{// turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin(this, pev->origin);
}

// XDM3038c: as it was touched
void CBaseTrigger::TouchUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(TouchUse);
	// should be checked inside Touch() if (IsLockedByMaster(pActivator))
	//	return;
	if (pActivator)// XDM3038c: !
		Touch(pActivator);
}

void CBaseTrigger::FireOnEntry(CBaseEntity *pOther)// SHL
{
	DBG_PRINT_ENT("FireOnEntry");
}

void CBaseTrigger::FireOnLeave(CBaseEntity *pOther)// SHL
{
	DBG_PRINT_ENT("FireOnLeave");
}

void CBaseTrigger::FireOnTouch(CBaseEntity *pOther)// SHL
{
	DBG_PRINT_ENT("FireOnTouch");
}




//-----------------------------------------------------------------------------
// Fires when a map is loaded
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_auto, CTriggerAuto);

TYPEDESCRIPTION	CTriggerAuto::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerAuto, m_globalstate, FIELD_STRING),
	DEFINE_FIELD(CTriggerAuto, triggerType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerAuto,CBaseDelay);

void CTriggerAuto::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
	/* XDM3038: mail me if someone ever wants to turn things OFF with trigger_auto		if (type == 0)
			triggerType = USE_OFF;// unfortunately, mappers often leave this 0
		else */if (type == 2)
			triggerType = USE_TOGGLE;
		else
			triggerType = USE_ON;

		pev->impulse = 1;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerAuto::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;// XDM3038
	pev->effects = EF_NODRAW;// XDM3038
	SetNextThink(0.2f);

	if (pev->impulse == 0)// XDM3038: not set by mapper
	{
		triggerType = USE_ON;
		pev->impulse = 2;
	}

	if (FStringNull(pev->target))
	{
		conprintf(0, "Design error: %s[%d] %s at (%g %g %g) has no target! Removing.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		pev->flags = FL_KILLME;//Destroy();
		return;
	}
	/*else if (sv_hack_triggerauto.value > 0.0f)// XDM3038: HACK! Even valve's mappers were doing this crap!
	{
		if (sv_hack_triggerauto.value > 1.0f)
			triggerType = USE_TOGGLE;
		else
			triggerType = USE_ON;
	}*/
}

void CTriggerAuto::Think(void)
{
	if (!m_globalstate || gGlobalState.EntityGetState(m_globalstate) == GLOBAL_ON)
	{
		conprintf(2, "%s[%d] %s at (%g %g %g) firing: %s\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z, STRING(pev->target));
		SUB_UseTargets(this, triggerType, 0);
		if (FBitSet(pev->spawnflags, SF_AUTO_FIREONCE))
			Destroy();
	}
	//else
	//	SetNextThink(5.0f);
}


//-----------------------------------------------------------------------------
// Fires a target after level transition and then dies
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(fireanddie, CFireAndDie);

void CFireAndDie::Spawn(void)
{
	// WTF? SetClassName("fireanddie");//	pev->classname = MAKE_STRING("fireanddie");
	// Don't call Precache() - it should be called on restore
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

void CFireAndDie::Precache(void)
{
	// This gets called on restore
	SetNextThink(m_flDelay);
}

void CFireAndDie::Think(void)
{
	SUB_UseTargets(this, USE_TOGGLE, 0);
	CBaseEntity::Think();// XDM3037: allow SetThink()
	Destroy();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_relay, CTriggerRelay);

TYPEDESCRIPTION	CTriggerRelay::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerRelay, triggerType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerRelay,CBaseDelay);

void CTriggerRelay::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
		if (type == 0)
			triggerType = USE_OFF;
		else if (type == 2)
			triggerType = USE_TOGGLE;
		else
			triggerType = USE_ON;

		pev->impulse = 1;// XDM3038c: set
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerRelay::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if (pev->impulse == 0)// XDM3038c: don't allow USE_OFF by default!!
		triggerType = USE_TOGGLE;
}

void CTriggerRelay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	SUB_UseTargets(/* this*/pActivator, triggerType, 0);// XDM3035c: pActivator
	if (FBitSet(pev->spawnflags, SF_RELAY_FIREONCE))
		Destroy();
}




//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(func_friction, CFrictionModifier);

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CFrictionModifier::m_SaveData[] =
{
	DEFINE_FIELD(CFrictionModifier, m_frictionFraction, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CFrictionModifier,CBaseEntity);

// Modify an entity's friction
void CFrictionModifier::Spawn(void)
{
	pev->effects = EF_NODRAW;// XDM3038
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(edict(), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch(&CFrictionModifier::ChangeFriction);
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::ChangeFriction(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(ChangeFriction);
	if (pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE)
		pOther->pev->friction = m_frictionFraction;
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100.0f;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}


//-----------------------------------------------------------------------------
// trigger_monsterjump
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_monsterjump, CTriggerMonsterJump);

void CTriggerMonsterJump::Spawn(void)
{
	SetMovedir(pev);
	InitTrigger();

	DontThink();// XDM3038a
	pev->speed = 200.0f;
	if (m_flHeight == 0.0f)// XDM
		m_flHeight = 150.0f;

	if (!FStringNull(pev->targetname))// if targetted, spawn turned off
	{
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin(this, pev->origin); // Unlink from trigger list
		SetUse(&CBaseTrigger::ToggleUse);
	}
}

void CTriggerMonsterJump::Think(void)
{
	pev->solid = SOLID_NOT;// kill the trigger for now !!!UNDONE
	UTIL_SetOrigin(this, pev->origin); // Unlink from trigger list
	SetThinkNull();
}

void CTriggerMonsterJump::Touch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	//if (!FBitSet(pevOther->flags , FL_MONSTER))
	if (!pOther->IsMonster())// XDM3038b
	{// touched by a non-monster.
		return;
	}

	entvars_t *pevOther = pOther->pev;
	pevOther->origin.z += 1;

	if (FBitSet(pevOther->flags, FL_ONGROUND))// clear the onground so physics don't bitch
		ClearBits(pevOther->flags, FL_ONGROUND);

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	SetNextThink(0.0f);
}




//-----------------------------------------------------------------------------
// trigger_cdaudio - starts/stops cd audio tracks
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_cdaudio, CTriggerCDAudio);

void CTriggerCDAudio::Spawn(void)
{
	InitTrigger();
	pev->impulse = 0;// prevent exploits
	pev->air_finished = 0.0f;
	SetThinkNull();
}

// UNDONE: pause, etc.
void CTriggerCDAudio::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (pev->impulse > 0 && !FBitSet(pev->spawnflags, SF_TRIGGERCDAUDIO_REPEATABLE))// activated
		return;

	// UNDONE	if (!ShouldToggle(useType, pev->impulse > 0))
	//		return;

	if (g_pGameRules == NULL || g_pGameRules->FAllowMapMusic())
	{
		CBasePlayer *pPlayer = NULL;
		if (!FBitSet(pev->spawnflags, SF_TRIGGERCDAUDIO_EVERYONE))
		{
			if (pActivator->IsPlayer())
				pPlayer = (CBasePlayer *)pActivator;// XDM3037: NULL means everyone
			else
			{
				conprintf(2, "%s[%d] error: pActivator[%d] is not a player!\n", STRING(pev->classname), entindex(), pActivator->entindex());
				return;
			}
		}
		int cmd = MPSVCMD_STOP;
		char buffer[32];
		const char *pTrackName = NULL;
		if (FStringNull(pev->message))// filename has higher priority than track number
		{
			if (pev->health == 0)// "track #"
			{
				//pTrackName = "";
				cmd = MPSVCMD_STOP;
			}
			else
			{
				_snprintf(buffer, 32, "%d", (int)pev->health);//pTrackName = _itoa((int)pev->health, buffer, 10);
				pTrackName = buffer;
				cmd = FBitSet(pev->spawnflags, SF_TRIGGERCDAUDIO_LOOP)?MPSVCMD_PLAYTRACKLOOP:MPSVCMD_PLAYTRACK;
			}
			if (g_pWorld && FBitSet(pev->spawnflags, SF_TRIGGERCDAUDIO_EVERYONE))// XDM3038c: save in world so connecting players will get an update
				g_pWorld->m_iAudioTrack = (int)pev->health;
		}
		else// we have a filename
		{
			pTrackName = STRING(pev->message);
			cmd = FBitSet(pev->spawnflags, SF_TRIGGERCDAUDIO_LOOP)?MPSVCMD_PLAYFILELOOP:MPSVCMD_PLAYFILE;

			if (g_pWorld && FBitSet(pev->spawnflags, SF_TRIGGERCDAUDIO_EVERYONE))// XDM3038c: save in world so connecting players will get an update
				g_pWorld->pev->noise = pev->message;
		}
		PlayAudioTrack(pPlayer, pTrackName, cmd, (int)pev->air_finished);
	}
	pev->impulse = 1;// activated
	m_hActivator = pActivator;
	m_hTBDAttacker = pCaller;
}

// Changes tracks or stops CD when player touches
// !!!HACK - overloaded HEALTH to avoid adding new field
void CTriggerCDAudio::Touch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	if (!pOther->IsPlayer())// only clients may trigger these events
		return;

	Use(pOther, this, USE_ON, pev->health);
}


//-----------------------------------------------------------------------------
// This plays a CD track when fired or when the player enters it's radius
// Same spawnflags as trigger_cdaudio
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(target_cdaudio, CTargetCDAudio);

void CTargetCDAudio::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->scale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CTriggerCDAudio::KeyValue(pkvd);
}

void CTargetCDAudio::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->impulse = 0;
	if (pev->scale > 0.0f)
	{
		SetThink(&CTargetCDAudio::SearchThink);// XDM3038: start searching
		SetNextThink(1.0f);
	}
	else
		SetThinkNull();// Use-only otherwise
}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::SearchThink(void)
{
	SetNextThink(0.5f);
	edict_t *pentPlayer = NULL;
	edict_t *pFirst = NULL;
	while ((pentPlayer = FIND_CLIENT_IN_PVS(edict())) != NULL)// XDM3035c: tricky!
	{
		if (pFirst == NULL)
			pFirst = pentPlayer;
		else if (FNullEnt(pentPlayer) || pFirst == pentPlayer)// reached first again
			break;

		if ((pentPlayer->v.origin - pev->origin).Length() <= pev->scale)
		{
			DontThink();// XDM3038a
			Use(CBaseEntity::Instance(pentPlayer), this, USE_ON, pev->health);
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_hurt, CTriggerHurt);

void CTriggerHurt::Spawn(void)
{
	InitTrigger();
	//SetTouch(HurtTouch);

	if (FStringNull(pev->targetname))
		SetUseNull();
	else
		SetUse(&CBaseTrigger::ToggleUse);

	if (FBitSet(m_bitsDamageInflict, DMG_RADIATION))
	{
		SetThink(&CTriggerHurt::RadiationThink);
		SetNextThink(RANDOM_FLOAT(0.0, 0.5));
	}

	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_START_OFF))// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin(this, pev->origin);// Link into the list
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger
void CTriggerHurt::RadiationThink(void)
{
	// check to see if a player is in pvs if not, continue
	// set origin to center of trigger so that this check works
	Vector origin(pev->origin);
	Vector view_ofs(pev->view_ofs);
	pev->origin = VecBModelOrigin(pev);//(pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs.Clear();
	edict_t *pentPlayer = FIND_CLIENT_IN_PVS(edict());
	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin
	if (!FNullEnt(pentPlayer))
	{
		CBaseEntity *pOther = CBasePlayer::Instance(pentPlayer);
		if (pOther && pOther->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pOther;
			Vector vecRange(VecBModelOrigin(pev)); vecRange -= pPlayer->Center();
			vec_t flRange = vecRange.Length();
			// if player's current geiger counter range is larger
			// than range to this trigger hurt, reset player's
			// geiger counter range
			if (pPlayer->m_flgeigerRange >= flRange)
				pPlayer->m_flgeigerRange = flRange;
		}
	}
	SetNextThink(0.25f);
}

// FIXME!!!
// When touched, a hurt trigger does DMG points of damage each half-second
void CTriggerHurt::Touch(CBaseEntity *pOther)// XDM
{
	DBG_PRINT_ENT_TOUCH(Touch);
	if (pOther->pev->takedamage == DAMAGE_NO)
		return;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_CLIENTONLYTOUCH) && !pOther->IsPlayer())
		return;// this trigger is only allowed to touch clients, and this ain't a client.

	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_NO_CLIENTS) && pOther->IsPlayer())
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if (IsMultiplayer())
	{
		if (pev->dmgtime > gpGlobals->time)
		{
			if (gpGlobals->time != pev->pain_finished)
			{// too early to hurt again, and not same frame with a different entity
				if (pOther->IsPlayer())
				{
					int playerMask = 1 << (pOther->entindex() - 1);
					// If I've already touched this player (this time), then bail out
					if (pev->impulse & playerMask)
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;// WRONG WRONG WRONG!
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if (pOther->IsPlayer())
			{
				int playerMask = 1 << (pOther->entindex() - 1);
				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
		// XDM3034 HACK? Prevent overgibbage and HL crashes.
		if (gpGlobals->maxClients > 8 && (sv_clientgibs.value <= 0.0f))// more accurate but SLOW!	if (g_pGameRules->CountPlayers() > 8)
			SetBits(m_bitsDamageInflict, DMG_NEVERGIB);
	}
	else// Original code -- single player
	{
		if (pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished)
			return;// too early to hurt again, and not same frame with a different entity
	}
	// If this is time_based damage (poison, radiation), override the pev->dmg with a
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger
	float fldmg = pev->dmg * 0.5f;	// 0.5 seconds worth of damage, pev->dmg is damage/second
	// JAY: Cut this because it wasn't fully realized.  Damage is simpler now.
#if 0
	switch (m_bitsDamageInflict)
	{
	default: break;
	case DMG_POISON:		fldmg = POISON_DAMAGE/4; break;
	case DMG_NERVEGAS:		fldmg = NERVEGAS_DAMAGE/4; break;
	case DMG_RADIATION:		fldmg = RADIATION_DAMAGE/4; break;
	case DMG_PARALYZE:		fldmg = PARALYZE_DAMAGE/4; break; // UNDONE: cut this? should slow movement to 50%
	case DMG_ACID:			fldmg = ACID_DAMAGE/4; break;
	case DMG_SLOWBURN:		fldmg = SLOWBURN_DAMAGE/4; break;
	case DMG_SLOWFREEZE:	fldmg = SLOWFREEZE_DAMAGE/4; break;
	}
#endif
	if (!FStringNull(pev->target))// XDM3038a: moved BEFORE applying damage to prevent using removed pOther
	{
		// trigger has a target it wants to fire.
		if (pOther->IsPlayer() || !FBitSet(pev->spawnflags, SF_TRIGGER_HURT_CLIENTONLYFIRE))
		{
			SUB_UseTargets(pOther, USE_TOGGLE, fldmg);// XDM3038a: fldmg
			if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_TARGETONCE))
				pev->target = iStringNull;
		}
	}

	if (fldmg < 0)
		pOther->TakeHealth(-fldmg, m_bitsDamageInflict);
	else
		pOther->TakeDamage(this, m_hActivator?(CBaseEntity *)m_hActivator:this, fldmg, m_bitsDamageInflict);// XDM3035: m_hActivator

	//pOther = NULL; MAY HAVE BEEN REMOVED!!!!!!!!!!

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;
	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;// half second delay until this trigger can hurt toucher again
}


//-----------------------------------------------------------------------------
/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
*/
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_multiple, CTriggerMultiple);

void CTriggerMultiple::Spawn(void)
{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger();

// XDM wtf?	ASSERTSZ(pev->health == 0, "trigger_multiple with health");
//	UTIL_SetOrigin(this, pev->origin);
//	SET_MODEL(edict(), STRING(pev->model));
//	if (pev->health > 0)
//	{
//		if (FBitSet(pev->spawnflags, SPAWNFLAG_NOTOUCH))
//			ALERT(at_error, "trigger_multiple spawn: health and notouch don't make sense");
//		pev->max_health = pev->health;
//UNDONE: where to get pfnDie from?
//		pev->pfnDie = multi_killed;
//		pev->takedamage = DAMAGE_YES;
//		pev->solid = SOLID_BBOX;
//		UTIL_SetOrigin(this, pev->origin);  // make sure it links into the world
//		SetTouchNull();// XDM
//	}
//	else
/*	{
XDM			SetTouch(MultiTouch);
	}*/
}

void CTriggerMultiple::Precache(void)
{
	CBaseTrigger::Precache();
	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(STRINGV(pev->noise));
}

void CTriggerMultiple::Touch(CBaseEntity *pOther)// XDM
{
	DBG_PRINT_ENT_TOUCH(Touch);
	if (IsLockedByMaster(pOther))// XDM3035c
		return;

	bool bSoundActivate = false;
	if (FBitSet(pev->spawnflags, SF_TRIGGER_SOUNDACTIVATE))
	{
		CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(pOther->edict()));
		if (pSound)
		{
			if (pSound->m_iVolume > TRIGGER_ACTIVATE_VOLUME)
				bSoundActivate = true;
		}
	}
	// Only touch clients, monsters, or pushables (depending on flags)
	if (bSoundActivate || CanTouch(pOther))// XDM3035c
		ActivateMultiTrigger(pOther);
}

void CTriggerMultiple::FireOnEntry(CBaseEntity *pOther)// SHL
{
	DBG_PRINT_ENT("FireOnEntry");
}

void CTriggerMultiple::FireOnLeave(CBaseEntity *pOther)// SHL
{
	DBG_PRINT_ENT("FireOnLeave");
}

//-----------------------------------------------------------------------------
// trigger_once
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_once, CTriggerOnce);

void CTriggerOnce::Spawn(void)
{
	m_flWait = -1;
	CTriggerMultiple::Spawn();
}


/*
QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
*/
//-----------------------------------------------------------------------------
// trigger_counter
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_counter, CTriggerCounter);

void CTriggerCounter::Spawn(void)
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;

	SetUse(&CBaseTrigger::CounterUse);
}


// ====================== TRIGGER_CHANGELEVEL ================================


//-----------------------------------------------------------------------------
// trigger_transition
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_transition, CTriggerVolume);

// Define space that allows entities to travel across a level transition
void CTriggerVolume::Spawn(void)
{
	InitTrigger();// XDM3035c: use common mechanism for triggers
	pev->solid = SOLID_NOT;
}


//-----------------------------------------------------------------------------
// trigger_changelevel
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(trigger_changelevel, CChangeLevel);

// Global Savedata for changelevel trigger
TYPEDESCRIPTION	CChangeLevel::m_SaveData[] =
{
	DEFINE_ARRAY(CChangeLevel, m_szMapName, FIELD_CHARACTER, MAX_MAPNAME),
	DEFINE_ARRAY(CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, MAX_MAPNAME),
	DEFINE_FIELD(CChangeLevel, m_changeTarget, FIELD_STRING),
	DEFINE_FIELD(CChangeLevel, m_changeTargetDelay, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CChangeLevel,CBaseTrigger);

//-----------------------------------------------------------------------------
// Purpose: Cache user-entity-field values until spawn is called.
// Input  : *pkvd - 
//-----------------------------------------------------------------------------
void CChangeLevel::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= MAX_MAPNAME)
			conprintf(0, "ERROR: Map name \"%s\" too long (%d chars max)\n", pkvd->szValue, MAX_MAPNAME);

		strncpy(m_szMapName, pkvd->szValue, MAX_MAPNAME);// XDM3038
		m_szMapName[MAX_MAPNAME-1] = '\0';
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= MAX_MAPNAME)
			conprintf(0, "ERROR: Landmark name \"%s\" too long (%d chars max)\n", pkvd->szValue, MAX_MAPNAME);

		strncpy(m_szLandmarkName, pkvd->szValue, MAX_MAPNAME);// XDM3038
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChangeLevel::Precache(void)
{
	if (g_pGameRules && g_pGameRules->IsCoOp())
		PRECACHE_SOUND("game/dom_touch.wav");
}

//-----------------------------------------------------------------------------
// Purpose: For game rules and bots
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChangeLevel::IsGameGoal(void) const
{
	if (g_pGameRules && g_pGameRules->IsCoOp() && g_pGameRules->GetGameMode() == COOP_MODE_LEVEL)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
// When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
//-----------------------------------------------------------------------------
void CChangeLevel::Spawn(void)
{
	if (FStrEq(m_szMapName, ""))
	{
		conprintf(1, "Design error: %s[%d] \"%s\" doesn't have a map!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
		return;// stay disabled
	}

	if (FStrEq(m_szLandmarkName, ""))
	{
		conprintf(1, "Design error: %s[%d] \"%s\" to %d doesn't have a landmark!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), m_szMapName);
		//return;// stay disabled
	}

	InitTrigger();

	if (!FStringNull(pev->targetname))
		SetUse(&CChangeLevel::UseChangeLevel);

	DontThink();// XDM3038a

	if (g_pGameRules && IsMultiplayer())
	{
		pev->teleport_time = g_pGameRules->GetStartTime() + 60.0f;// XDM3038: hack to prevent players from rushing/falling/jumping into wrong trigger
		if (g_pGameRules->IsCoOp())// XDM3035a: don't allow players to cross this trigger, but allow touching
		{
			ClearBits(pev->effects, EF_NODRAW);
			pev->rendermode = kRenderTransColor;
			pev->renderamt = 127;
			//if (pev->rendercolor.IsZero())
			//	pev->rendercolor.y = 255;// green by default
			pev->renderfx = kRenderFxPulseFast;
		}
	}

	if (!FBitSet(pev->spawnflags, SF_CHANGELEVEL_USEONLY))
		SetTouch(&CChangeLevel::TouchChangeLevel);
}

//-----------------------------------------------------------------------------
// Purpose: Use - allows level transitions to be triggered by buttons, etc.
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CChangeLevel::UseChangeLevel(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(UseChangeLevel);
	// UNDONE: coop: trigger level change now! Don't wait for others!
	ChangeLevelNow(pActivator);
}

//-----------------------------------------------------------------------------
// Purpose: Activated by touching
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CChangeLevel::TouchChangeLevel(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(TouchChangeLevel);
	if (pOther->IsPlayer())
		ChangeLevelNow(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//-----------------------------------------------------------------------------
void CChangeLevel::ChangeLevelNow(CBaseEntity *pActivator)
{
	if (IsMultiplayer())
	{
		if (pev->teleport_time >= gpGlobals->time)// trigger is not active yet!
			return;
	}

	// Some people are firing these multiple times in a frame, disable
	if (pev->dmgtime == gpGlobals->time)
	{
		if (pActivator == m_hActivator)
		{
			DBG_PRINTF("ChangeLevelNow() called more than one in a frame for the same activator!\n");
			return;
		}
	}
	ASSERT(!FStrEq(m_szMapName, ""));

	pev->dmgtime = gpGlobals->time;

#if 1
	if (ChangeLevel(m_szMapName, m_szLandmarkName, pActivator, m_changeTarget, m_changeTargetDelay))
	{
		// WARNING: TODO: TESTME: g_pWorld!!
		m_hActivator = pActivator;
		SUB_UseTargets(pActivator, USE_TOGGLE, 0.0f);
	}
#else
	CBasePlayer *pPlayer = NULL;
	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else
	{
		if (IsMultiplayer())
			SERVER_PRINTF("ChangeLevelNow(%s %s) ERROR: called by non-player %s[%d]!\n", m_szMapName, m_szLandmarkName, STRING(pActivator->pev->classname), pActivator->entindex());
		else
			pPlayer = UTIL_ClientByIndex(1);// XDM3037
	}

	if (pPlayer == NULL)
		return;

	if (!InTransitionVolume(pPlayer, m_szLandmarkName))
	{
		conprintf(2, "ChangeLevelNow(): Player %s isn't in the transition volume %s, aborting\n", STRING(pPlayer->pev->netname), m_szLandmarkName);
		return;
	}

	edict_t	*pentLandmark = FindLandmark(m_szLandmarkName);

	if (g_pGameRules && !g_pGameRules->FAllowLevelChange(pPlayer, m_szMapName, pentLandmark))// XDM3035
		return;

	// Create an entity to fire the changetarget
	if (!FStringNull(m_changeTarget))
	{
		CFireAndDie *pFireAndDie = GetClassPtr((CFireAndDie *)NULL);
		if (pFireAndDie)
		{
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			DispatchSpawn(pFireAndDie->edict());
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy(st_szNextMap, m_szMapName);
	st_szNextMap[MAX_MAPNAME-1] = '\0';
	// TEST strlwr(st_szNextMap);// XDM3038c: WTF: HACK: FIX: Half-Life requires all map names in lowercase here!

	m_hActivator = pActivator;
	SUB_UseTargets(pActivator, USE_TOGGLE, 0.0f);
	st_szNextSpot[0] = 0;// Init landmark to NULL

	// TODO: UNDONE: this should be done through g_pGameRules && g_pGameRules->ChangeLevel(st_szNextMap, st_szNextSpot);!!

	// look for a landmark entity
	if (!FNullEnt(pentLandmark))
	{
		strcpy(st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = pentLandmark->v.origin;
	}

	//conprintf(1, "Level touches %d levels\n", ChangeList(levels, 16));
	SERVER_PRINTF("CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot);

	g_pWorld = NULL;// XDM LAST ZOMFD!!!!!!!!!! crash prevention?
	ClearBits(gpGlobals->serverflags, FSERVER_RESTORE);
	CHANGE_LEVEL(st_szNextMap, st_szNextSpot);
#endif
}


//-----------------------------------------------------------------------------
// func_ladder - makes an area vertically negotiable
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(func_ladder, CLadder);

void CLadder::KeyValue(KeyValueData *pkvd)
{
	CBaseTrigger::KeyValue(pkvd);
}

void CLadder::Precache(void)
{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	if (FBitSet(pev->spawnflags, SF_LADDER_START_OFF))// if flagged to Start Turned Off, make trigger nonsolid.
	{
		pev->skin = CONTENTS_EMPTY;
		//pev->solid = SOLID_NOT;
	}
	else
	{
		pev->skin = CONTENTS_LADDER;
		//pev->solid = SOLID_TRIGGER;
	}
	if (showtriggers.value <= 0.0f)
	{
		pev->effects = EF_NODRAW;// XDM3038
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}
	else
		ClearBits(pev->effects, EF_NODRAW);
}

void CLadder::Spawn(void)
{
	//InitTrigger();?
	Precache();
	SET_MODEL(edict(), STRING(pev->model));// set size and link into world
	UTIL_SetOrigin(this, pev->origin);
	pev->movetype = MOVETYPE_PUSH;
}

void CLadder::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (IsLockedByMaster(pActivator))// XDM3035c
		return;

	bool bActive = (pev->skin == CONTENTS_LADDER);
	if (ShouldToggle(useType, bActive))
	{
		bActive = !bActive;
		if (bActive)
		{
			pev->skin = CONTENTS_LADDER;
			//SET_MODEL(edict(), STRING(pev->model));
			UTIL_SetOrigin(this, pev->origin);
		}
		else
			pev->skin = CONTENTS_EMPTY;
	}
}


//-----------------------------------------------------------------------------
// A TRIGGER THAT PUSHES YOU
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_push, CTriggerPush);

/*QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
Pushes the player
*/
void CTriggerPush::Spawn(void)
{
	if (pev->angles.IsZero())
		pev->angles.y = 360;

	InitTrigger();

	if (FBitSet(pev->spawnflags, SF_TRIGGER_PUSH_START_OFF))// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	if (pev->speed == 0)
	{
		pev->speed = 100;// default
		conprintf(1, "%s[%d] \"%s\" without speed, resetting to default of %g\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->speed);
	}
	SetUse(&CBaseTrigger::ToggleUse);
	UTIL_SetOrigin(this, pev->origin);		// Link into the list
}

void CTriggerPush::Touch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	//if (!CanTouch(pOther))// XDM3037: TODO?
	//	return;
	if (pOther->pev->solid == SOLID_NOT || pOther->pev->solid == SOLID_TRIGGER)// XDM3038c: added SOLID_TRIGGER, removed SOLID_BSP
		return;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch (pOther->pev->movetype)// pushables have MOVETYPE_PUSHSTEP
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if (!pOther->IsPushable())// XDM
		return;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_PUSH_ONLY_NOTONGROUND) && FBitSet(pOther->pev->flags, FL_ONGROUND))// XDM3038b: FIX
		return;

	Vector vecPush(pev->movedir);
	vecPush *= pev->speed;

	// Instant trigger, just transfer velocity and remove
	if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
	{
		pOther->pev->velocity += vecPush;
		if (pOther->pev->velocity.z > 0.0f)
			ClearBits(pOther->pev->flags, FL_ONGROUND);

		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.001f);
		return;
	}
	else// Push field, transfer to base velocity
	{
		if (FBitSet(pOther->pev->flags, FL_BASEVELOCITY))
			vecPush += pOther->pev->basevelocity;// keep old basevelocity

		pOther->pev->basevelocity = vecPush;
		SetBits(pOther->pev->flags, FL_BASEVELOCITY);
		//conprintf(1, "Vel %f, base %f\n", pevToucher->velocity.z, pevToucher->basevelocity.z);
	}
}


//-----------------------------------------------------------------------------
// teleport trigger
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_teleport, CTriggerTeleport);

TYPEDESCRIPTION	CTriggerTeleport::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerTeleport, m_hLastTargetSpot, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CTriggerTeleport, CBaseTrigger);

void CTriggerTeleport::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "flagpolicy"))
	{
		pev->weaponanim = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}

void CTriggerTeleport::Spawn(void)
{
	InitTrigger();
	SetUse(&CBaseTrigger::TouchUse);// XDM3038c
	m_hLastTargetSpot = NULL;
	//SetTouch(TeleportTouch);
}

void CTriggerTeleport::Touch(CBaseEntity *pOther)// XDM
{
	DBG_PRINT_ENT_TOUCH(Touch);
	// Only teleport monsters or clients
	if (!CanTouch(pOther))// XDM3037
		return;
	if (IsLockedByMaster(pOther))
		return;
	if (pOther->pev->solid == SOLID_TRIGGER)// XDM3037: kimited functionality, but at least this prevents hangs in SV_TouchLinks
		return;

	if (pOther->IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		if (g_pGameRules && g_pGameRules->GetGameType() == GT_CTF && pPlayer->m_pCarryingObject)
		{
			if (pev->weaponanim == TRIGGER_TELEPORT_P_DENY)// flagpolicy
				return;
			else if (pev->weaponanim == TRIGGER_TELEPORT_P_DROP)
				pPlayer->m_pCarryingObject->Use(pPlayer, pPlayer, USE_TOGGLE, 0.0f);
			else if (pev->weaponanim == TRIGGER_TELEPORT_P_KILL)
			{
				pPlayer->Killed(this, pPlayer, GIB_DISINTEGRATE);
				return;
			}
		}
	}

	// Find the target spot. WARNING: somebody may have changed the target! Test case: dm_blue_paradise
	if (m_hLastTargetSpot.Get() && !FStrEq(pev->target, m_hLastTargetSpot->pev->targetname))
	{
		DBG_PRINTF("%s[%d] \"%s\": m_hLastTargetSpot targetname \"%s\" does not match current target \"%s\"!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(m_hLastTargetSpot->pev->targetname), STRING(pev->target));
		m_hLastTargetSpot = NULL;// prevent endless loops
	}
	CBaseEntity *pStartSpot = m_hLastTargetSpot;// can be NULL or some smartass may have changed pev->target and it won't be found!
	CBaseEntity *pSpot = pStartSpot;// can be NULL
	CBaseEntity *pFirstFoundSpot = NULL;// we need to know if anything was actually found at all
	Vector vecSrc(pOther->pev->origin);// Center();?
	Vector vecDest;
	bool bPassedObstacleCheck = false;
	// pick some sequential/non-obstructed spots
	while ((pSpot = UTIL_FindEntityByTargetname(pSpot, STRING(pev->target))) != pStartSpot)// XDM3038c: support for multiple targets
	{
		if (pSpot)
		{
			if (pFirstFoundSpot == NULL)
				pFirstFoundSpot = pSpot;
			else if (pFirstFoundSpot == pSpot)// loop
				break;

			vecDest = pSpot->pev->origin;
			if (pOther->IsPlayer())
				vecDest.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)

			vecDest.z += 1.0f;
			if (SpawnPointCheckObstacles(pOther, vecDest, false, false))// non-intrusive check
			{
				bPassedObstacleCheck = true;
				break;
			}
		}
	}

	if (pSpot == NULL)
	{
		if (pFirstFoundSpot)
		{
			pSpot = pFirstFoundSpot;
#if defined (_DEBUG)
			conprintf(1, "Error: %s[%d] \"%s\" all targets \"%s\" are obstructed!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->target));
#endif
		}
		else
		{
			conprintf(1, "Error: %s[%d] \"%s\" cannot find target \"%s\"!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->target));
			return;
		}
	}
	//if (pSpot == pStartSpot)?

	m_hActivator = pOther;// XDM3037

	// Clear the spot
	CBaseEntity *pAttacker;
	if (m_hActivator)
		pAttacker = (CBaseEntity *)m_hActivator;
	else
		pAttacker = this;

	vecDest = pSpot->pev->origin;
	if (pOther->IsPlayer())
		vecDest.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)

	vecDest.z += 1.0f;

	if (!bPassedObstacleCheck)
		SpawnPointCheckObstacles(pOther, vecDest, true, (IsMultiplayer() && mp_telegib.value > 0.0f));// clear the way

	DBG_PRINTF("%s[%d] \"%s\": using target \"%s\" [%d]!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->target), pSpot->entindex());
	m_hLastTargetSpot = pSpot;
	// Teleport the entity
	ClearBits(pOther->pev->flags, FL_ONGROUND);

	/*if (FBitSet(pev->spawnflags, SF_TRIGGER_KEEPANGLES))// XDM: undone?
	{
		pOther->pev->angles = pOther->pev->angles + pSpot->pev->angles;
		pOther->pev->fixangle = 0;
	}
	else
	{
		pOther->pev->angles = pSpot->pev->angles;
		//pOther->pev->fixangle = 1;
	}*/
	pOther->pev->angles = pSpot->pev->angles;
	pOther->pev->fixangle = 1;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_CLEARVELOCITY))// XDM
	{
		pOther->pev->velocity.Clear();
		pOther->pev->basevelocity.Clear();
	}

	UTIL_SetOrigin(pOther, vecDest/* + vecDelta*/);
	//if (pOther->IsPlayer())
	//	pOther->pev->v_angle = pentTarget->v.angles;

	// Now do the effects
	if (g_pGameRules && IsMultiplayer())
	{
		if (g_pGameRules->FAllowEffects())// XDM3037: we need delay so player pvs may change beforehand
		{
			int ei = pOther->entindex();
			MESSAGE_BEGIN(MSG_PVS, gmsgTeleport, vecSrc);
				WRITE_BYTE(MSG_TELEPORT_FL_SOUND);// src
				WRITE_SHORT(ei);
				WRITE_COORD(vecSrc.x);// coord coord coord (pos)
				WRITE_COORD(vecSrc.y);
				WRITE_COORD(vecSrc.z);
			MESSAGE_END();
			MESSAGE_BEGIN(MSG_PVS, gmsgTeleport, vecDest);
				WRITE_BYTE(MSG_TELEPORT_FL_DEST|MSG_TELEPORT_FL_SOUND|(IsMultiplayer()?MSG_TELEPORT_FL_FADE:0));// dst
				WRITE_SHORT(ei);
				WRITE_COORD(vecDest.x);// coord coord coord (pos)
				WRITE_COORD(vecDest.y);
				WRITE_COORD(vecDest.z);
			MESSAGE_END();
			// daznt wok PLAYBACK_EVENT_FULL(FEV_RELIABLE, pOther->edict(), g_usTeleport, 0.1, vecDest, vecSrc, 0.0, 0.0, pOther->pev->team, 0, 1, 0);
		}
	}

	// HACK
	//if (pOther->IsProjectile())
	//	pOther->pev->owner = edict();// DON'T RETOUCH!

	if (!FBitSet(pev->spawnflags, SF_TRIGGER_CLEARVELOCITY))// XDM: keep scalar velocity, but reorient the vector
	{
		vec_t k = pOther->pev->velocity.Length();
		AngleVectors(pSpot->pev->angles, pOther->pev->velocity, NULL, NULL);
		pOther->pev->velocity *= k;
	}
}



//-----------------------------------------------------------------------------
// CTriggerSave - autosave in SP, checkpoint in MP
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_autosave, CTriggerSave);

void CTriggerSave::Spawn(void)
{
	InitTrigger();
	pev->impulse = 0;
	if (g_pGameRules && g_pGameRules->IsCoOp())// XDM3038: make these visible
	{
		ClearBits(pev->effects, EF_NODRAW);
		pev->rendermode = kRenderTransColor;
		pev->renderamt = 31;
		pev->rendercolor.Set(95,191,255);
		pev->renderfx = kRenderFxPulseFast;
		//pev->flags = FL_WORLDBRUSH;33554432
	}
	//SetTouch(&CTriggerSave::SaveTouch);
}

/*void CTriggerSave::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "checkpoint"))// XDM3035c: works like master
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}*/

//-----------------------------------------------------------------------------
// Purpose: If player touches this trigger, remember it somehow
// Input  : *pOther - should be a player
//-----------------------------------------------------------------------------
void CTriggerSave::Touch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	// Only save on clients
	if (!pOther->IsPlayer())// MUST be player!
		return;
	if (IsLockedByMaster(pOther))
		return;

	if (g_pGameRules && g_pGameRules->IsCoOp())// XDM3035c: in multiplayer (CoOp) these triggers fit very nice as checkpoints!
	{
		((CBasePlayer *)pOther)->OnCheckPoint(this);
		if (pev->impulse == 0)
			pev->impulse = 1;
	}
	else
	{
		SetTouchNull();
		pev->flags = FL_KILLME;
		Destroy();// so this trigger won't be saved
		SERVER_COMMAND("autosave\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if this player already touched this trigger
// Input  : *pEntity - player
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTriggerSave::IsTriggered(const CBaseEntity *pEntity)
{
	if (IsMultiplayer())
	{
		if (pEntity->IsPlayer())
		{
			if (((CBasePlayer *)pEntity)->PassedCheckPoint(this) == false)
				return false;
		}
	}
	return CBaseTrigger::IsTriggered(pEntity);
}

//-----------------------------------------------------------------------------
// CTriggerEndSection
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_endsection, CTriggerEndSection);

void CTriggerEndSection::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}

void CTriggerEndSection::Spawn(void)
{
	InitTrigger();

	SetUse(&CTriggerEndSection::EndSectionUse);
	// If it is a "use only" trigger, then don't set the touch function.
	if (!FBitSet(pev->spawnflags, SF_ENDSECTION_USEONLY))
		SetTouch(&CTriggerEndSection::EndSectionTouch);
}

void CTriggerEndSection::EndSectionUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(EndSectionUse);
	// Only save on clients
	// XDM3038c: why!? if (pActivator && !pActivator->IsNetClient())
	//	return;
	SetUseNull();
	if (g_pGameRules && IsMultiplayer())// XDM3038c
	{
		SERVER_PRINTF("GAME: ended by %s[%d] \"%s\"\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
		g_pGameRules->EndMultiplayerGame();
	}
	else
	{
		if (!FStringNull(pev->message))
			END_SECTION(STRING(pev->message));
	}
	Destroy();
}

void CTriggerEndSection::EndSectionTouch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(EndSectionTouch);
	// Only save on clients
	if (!pOther->IsNetClient())
		return;

	SetTouchNull();
	EndSectionUse(pOther, pOther, USE_ON, 0.0f);
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_gravity, CTriggerGravity);

void CTriggerGravity::Spawn(void)
{
	InitTrigger();
	SetTouch(&CTriggerGravity::GravityTouch);
	SetUse(&CBaseTrigger::ToggleUse);// XDM3035c TESTME
}

void CTriggerGravity::GravityTouch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	if (pev->solid == SOLID_NOT)// redundant?
		return;
	if (!CanTouch(pOther))// XDM3035a: how's this? >:)
		return;
	//if (IsLockedByMaster(pOther))// XDM3035c
	//	return;

	pOther->pev->gravity = pev->gravity;
}


//-----------------------------------------------------------------------------
// XDM3035c: somebody may want this
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_playerfreeze, CTriggerPlayerFreeze);

void CTriggerPlayerFreeze::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (!pActivator || !pActivator->IsPlayer())
		pActivator = CBaseEntity::Instance(INDEXENT(1));

	if (pActivator->pev->flags & FL_FROZEN)
		((CBasePlayer *)pActivator)->EnableControl(TRUE);
	else
		((CBasePlayer *)pActivator)->EnableControl(FALSE);
}


//-----------------------------------------------------------------------------
// this is a really bad idea.
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_changetarget, CTriggerChangeTarget);

TYPEDESCRIPTION	CTriggerChangeTarget::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeTarget,CBaseDelay);

void CTriggerChangeTarget::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewTarget"))
	{
		m_iszNewTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerChangeTarget::Spawn(void)
{
	pev->effects = EF_NODRAW;
}

void CTriggerChangeTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
	if (pTarget)
	{
		pTarget->pev->target = m_iszNewTarget;// pTarget->KeyValue("target", STRING(m_iszNewTarget));??
		CBaseMonster *pMonster = pTarget->MyMonsterPointer();
		if (pMonster)
			pMonster->m_pGoalEnt = NULL;
		else if (FClassnameIs(pTarget->pev, "trigger_camera"))// XDM3035c: else (faster)
			pTarget->Use(this, this, USE_SET, 1.0);
	}
}


//-----------------------------------------------------------------------------
// CTriggerCamera
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_camera, CTriggerCamera);

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CTriggerCamera::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerCamera, m_hPlayer, FIELD_EHANDLE),
	DEFINE_FIELD(CTriggerCamera, m_hTarget, FIELD_EHANDLE),
	DEFINE_FIELD(CTriggerCamera, m_pentPath, FIELD_CLASSPTR),
	DEFINE_FIELD(CTriggerCamera, m_sPath, FIELD_STRING),
	DEFINE_FIELD(CTriggerCamera, m_flWait, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_flReturnTime, FIELD_TIME),
	DEFINE_FIELD(CTriggerCamera, m_flStopTime, FIELD_TIME),
	DEFINE_FIELD(CTriggerCamera, m_moveDistance, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_targetSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_initialSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_acceleration, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_deceleration, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_state, FIELD_INTEGER),
	DEFINE_FIELD(CTriggerCamera, m_iszViewEntity, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerCamera,CBaseDelay);

void CTriggerCamera::Spawn(void)
{
	//TEST	SET_MODEL(edict(), "models/w_flare.mdl");
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->effects = EF_NODRAW;// only in disabled state!
	pev->impulse = 0;// XDM3038: set to 1 when need to restore player HUD

/*#if defined (_DEBUG)
	pev->modelindex = g_iModelIndexTestSphere;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->effects = EF_BRIGHTFIELD;
#else
	pev->effects = EF_NODRAW;
#endif*/

	m_initialSpeed = pev->speed;
	if (m_acceleration == 0)
		m_acceleration = 500;
	if (m_deceleration == 0)
		m_deceleration = 500;
}

void CTriggerCamera::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "moveto"))
	{
		m_sPath = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
	{
		m_acceleration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "deceleration"))
	{
		m_deceleration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "viewentity"))
	{
		m_iszViewEntity = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerCamera::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (!ShouldToggle(useType, m_state > 0))
		return;

	//if (IsLockedByMaster(pActivator))// XDM3035c: should this ever be locked?
	//	return;

	// Toggle state
	m_state = !m_state;
	//ALERT(at_debug, "CTriggerCamera(%s)::Use(): state set to %d\n", STRING(pev->targetname), m_state);
	if (m_state == 0)
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}

	if (!pActivator || !pActivator->IsPlayer())
	{
		pActivator = CBaseEntity::Instance(INDEXENT(1));
		ALERT(at_debug, "CTriggerCamera(%s)::Use(): has no activator, picked default player\n", STRING(pev->targetname));
	}

	m_hPlayer = pActivator;

	if (m_flWait > 0)// XDM
		m_flReturnTime = gpGlobals->time + m_flWait;
	else
		m_flReturnTime = 0;

	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TARGET))
		m_hTarget = m_hPlayer;
	else
		m_hTarget = GetNextTarget();

	// Nothing to look at!
	if (m_hTarget.Get() == NULL)
	{
		conprintf(1, "CTriggerCamera(%s)::Use(): error: no target!\n", STRING(pev->targetname));
		return;
	}

	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL) && pActivator->IsPlayer())
	{
		CBasePlayer *pPlayerActivator = (CBasePlayer *)pActivator;
		pPlayerActivator->EnableControl(FALSE);
		if (!FBitSet(pPlayerActivator->m_iHideHUD, HIDEHUD_ALL))// XDM3038: hide all HUD indicators, mark to restore
		{
			SetBits(pPlayerActivator->m_iHideHUD, HIDEHUD_ALL);
			pev->impulse = 1;
		}
	}

	if (m_sPath)
		m_pentPath = UTIL_FindEntityByTargetname(NULL, STRING(m_sPath));
	else
		m_pentPath = NULL;

	m_flStopTime = gpGlobals->time;
	if (m_pentPath)
	{
		if (m_pentPath->pev->speed != 0.0f)
			m_targetSpeed = m_pentPath->pev->speed;

		m_flStopTime += m_pentPath->GetDelay();
	}

	SetBits(pev->flags, FL_DRAW_ALWAYS);// XDM3035c: ignore PVS restrictions. We use ShouldBeSentTo() to prevent sending to unneeded players and flooding the network.
	ClearBits(pev->effects, EF_NODRAW);
	pev->modelindex = g_iModelIndexTestSphere;
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransAdd;				// But the entity will be sent to clients!
	pev->framerate = 0.0f;
	//m_hPlayer->pev->iuser1 = OBS_IN_EYE;
	//m_hPlayer->pev->iuser2 = entindex();
	//m_hPlayer->m_hObserverTarget = this;

	// copy over player information
	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_POSITION))
	{
		UTIL_SetOrigin(this, pActivator->pev->origin + pActivator->pev->view_ofs);
#if defined (NOSQB)
		pev->angles.x = pActivator->pev->angles.x;
#else
		pev->angles.x = -pActivator->pev->angles.x;
#endif
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
	{
		UTIL_SetOrigin(this, pev->origin);
		pev->velocity.Clear();
	}

	if (m_iszViewEntity)//LRC
	{
		CBaseEntity *pEntity = UTIL_FindEntityByTargetname(NULL, STRING(m_iszViewEntity));
		if (pEntity)
			UTIL_SetView(pActivator->edict(), pEntity->edict());
		else
			conprintf(1, "CTriggerCamera: unable to find view entity: '%s'!\n", STRING(m_iszViewEntity));
	}
	else
		UTIL_SetView(pActivator->edict(), edict());

	// follow the player down
	SetThink(&CTriggerCamera::FollowTarget);
	SetNextThink(0.0f);
	m_moveDistance = 0;
	Move();
}

// XDM3035c: works perfectly without flooding network channels!
bool CTriggerCamera::ShouldBeSentTo(CBasePlayer *pClient)
{
	if (m_hPlayer.Get() && m_hPlayer == pClient)
		return true;

	return false;// must return false for all clients to override FL_DRAW_ALWAYS
}

void CTriggerCamera::FollowTarget(void)
{
	if (m_hPlayer.Get() == NULL)
		return;

	if (m_hTarget.Get() == NULL || (m_flReturnTime > 0 && m_flReturnTime < gpGlobals->time))// XDM
	{
		Deactivate();
		return;
	}

	Vector vecGoal;
	VectorAngles(m_hTarget->EyePosition() - pev->origin, vecGoal);// XDM: EyePosition
#if !defined (NOSQB)
	vecGoal.x = -vecGoal.x;
#endif
	NormalizeAngle360(&pev->angles.y);// XDM3038

	vec_t dx = vecGoal.x - pev->angles.x;
	vec_t dy = vecGoal.y - pev->angles.y;

	NormalizeAngle180(&dx);// XDM3037a
	NormalizeAngle180(&dy);

	pev->avelocity.x = dx * 40.0f * gpGlobals->frametime;
	pev->avelocity.y = dy * 40.0f * gpGlobals->frametime;

	if (!(FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL)))
	{
		pev->velocity *= 0.8f;
		if (pev->velocity.Length() < 10.0f)
			pev->velocity.Clear();
	}
	SetNextThink(0.0f);
	Move();
}

void CTriggerCamera::Move(void)
{
	// Not moving on a path, return
	if (!m_pentPath)
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if (m_moveDistance <= 0)
	{
		// Fire the passtarget if there is one
		if (!FStringNull(m_pentPath->pev->message))
		{
			FireTargets(STRING(m_pentPath->pev->message), this, this, USE_TOGGLE, 0);
			if (FBitSet(m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE))
				m_pentPath->pev->message = iStringNull;
		}
		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget();

		// Set up next corner
		if (!m_pentPath)
		{
			pev->velocity.Clear();
		}
		else
		{
			if (m_pentPath->pev->speed != 0)
				m_targetSpeed = m_pentPath->pev->speed;

			//Vector delta = m_pentPath->pev->origin - pev->origin;
			//m_moveDistance = delta.Length();
			//pev->movedir = delta.Normalize();
			pev->movedir = m_pentPath->pev->origin - pev->origin;// XDM3038b: faster
			m_moveDistance = pev->movedir.NormalizeSelf();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if (m_flStopTime > gpGlobals->time)
		pev->speed = UTIL_Approach(0, pev->speed, m_deceleration * gpGlobals->frametime);
	else
		pev->speed = UTIL_Approach(m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime);

	float fraction = 2.0f * gpGlobals->frametime;
	pev->velocity = ((pev->movedir * pev->speed) * fraction) + (pev->velocity * (1-fraction));
}

void CTriggerCamera::Deactivate(void)
{
#if defined (_DEBUG)
	conprintf(2, "CTriggerCamera(%s)::Deactivate()\n", STRING(pev->targetname));
#endif
	ASSERT(m_hPlayer.Get() != NULL);
	UTIL_SetView(m_hPlayer->edict(), m_hPlayer->edict());// XDM3035b: even if dead
	if (m_hPlayer->IsAlive())
	{
		CBasePlayer *pPlayerActivator = (CBasePlayer *)(CBaseEntity *)m_hPlayer;
		pPlayerActivator->EnableControl(true);
		if (pev->impulse)
		{
			ClearBits(pPlayerActivator->m_iHideHUD, HIDEHUD_ALL);
			pev->impulse = 0;
		}
	}
	SUB_UseTargets(this, USE_TOGGLE, 0);
	pev->avelocity.Clear();
	ClearBits(pev->flags, FL_DRAW_ALWAYS);
	pev->effects |= EF_NODRAW;
	pev->modelindex = 0;
	pev->renderamt = 0;
	//pev->rendermode = kRenderTransAdd;
	m_state = 0;
}


//-----------------------------------------------------------------------------
// CTriggerBounce
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_bounce, CTriggerBounce);

void CTriggerBounce::Spawn(void)
{
	SetMovedir(pev);
	InitTrigger();
}

void CTriggerBounce::Touch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(Touch);
	if (!CanTouch(pOther))
		return;
	if (IsLockedByMaster(pOther))
		return;

	float dot = DotProduct(pev->movedir, pOther->pev->velocity);
	if (dot < -pev->armorvalue)
	{
		if (FBitSet(pev->spawnflags, SF_BOUNCE_CUTOFF))
			pOther->pev->velocity -= (dot + pev->frags*(dot+pev->armorvalue))*pev->movedir;
		else
			pOther->pev->velocity -= (dot + pev->frags*dot)*pev->movedir;

		SUB_UseTargets(pOther, USE_TOGGLE, 0);
	}
}
