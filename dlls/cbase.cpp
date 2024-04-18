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
//
// XDM: This system is obsolete and ineffective.
// There should really be 'reference' and 'instance' for each entity on a map...
// Unfortunately, it depends on the HL engine and design tools.
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "decals.h"
#include "game.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "maprules.h"
#include "skill.h"
#include "weapons.h"
#include "globals.h"
#include "animation.h"
#include "pm_shared.h"
#include "nodes.h"
#include <new>
#ifdef USE_EXCEPTIONS
#include <exception>
#endif
//#include <float.h>

// All exports are now in h_export.cpp

//-----------------------------------------------------------------------------
// This array defines what [0] thinks of [1] (or what is [1] to [0])
// Read from left to right: subject is commented column, earch row contains reactions of this subject to encountered objects.
// The subject-to-object sequence is critical! Non-mutual relations do exist!
// Example: g_iRelationshipTable[pSubject->Classify()][pWhatHeThinksOfThisObject->Classify()]
//-----------------------------------------------------------------------------
/*static */int g_iRelationshipTable[NUM_RELATIONSHIP_ClASSES][NUM_RELATIONSHIP_ClASSES] =
{//	sbj[0]:   obj[1]->NONE	MACH	PLYR	HPASS	HMIL	AMIL	APASS	AMONST	APREY	APRED	INSECT	PLRALY	PBWPN	ABWPN	GREN	GIB		BARNCL	HMONST	
/*CLASS_NONE*/		{R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO},
/*MACHINE*/			{R_NO,	R_NO,	R_DL,	R_DL,	R_NO,	R_NM,	R_NO,	R_HT,	R_DL,	R_HT,	R_NO,	R_DL,	R_NO,	R_NO,	R_FR,	R_NO,	R_NO,	R_DL},
/*PLAYER*/			{R_NO,	R_DL,	R_NO,	R_AL,	R_HT,	R_HT,	R_DL,	R_HT,	R_DL,	R_HT,	R_NO,	R_AL,	R_FR,	R_FR,	R_FR,	R_NO,	R_NO,	R_HT},
/*HUMANPASSIVE*/	{R_NO,	R_NO,	R_AL,	R_AL,	R_FR,	R_FR,	R_NO,	R_HT,	R_DL,	R_FR,	R_NO,	R_AL,	R_NO,	R_FR,	R_FR,	R_NO,	R_NO,	R_FR},
/*HUMANMILITAR*/	{R_NO,	R_NO,	R_HT,	R_DL,	R_AL,	R_NM,	R_NO,	R_DL,	R_DL,	R_DL,	R_NO,	R_HT,	R_DL,	R_NM,	R_FR,	R_NO,	R_NO,	R_HT},
/*ALIENMILITAR*/	{R_NO,	R_DL,	R_HT,	R_DL,	R_NM,	R_AL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_DL,	R_NM,	R_NO,	R_FR,	R_NO,	R_NO,	R_DL},
/*ALIENPASSIVE*/	{R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_AL,	R_AL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_DL},
/*ALIENMONSTER*/	{R_NO,	R_DL,	R_DL,	R_DL,	R_DL,	R_AL,	R_NO,	R_AL,	R_NO,	R_NO,	R_NO,	R_DL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_DL},
/*ALIENPREY*/		{R_NO,	R_FR,	R_DL,	R_DL,	R_DL,	R_NO,	R_NO,	R_NO,	R_AL,	R_FR,	R_HT,	R_DL,	R_NO,	R_NO,	R_DL,	R_DL,	R_NO,	R_FR},
/*ALIENPREDATOR*/	{R_NO,	R_NO,	R_DL,	R_DL,	R_DL,	R_NO,	R_NO,	R_NO,	R_HT,	R_DL,	R_NO,	R_DL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_DL},
/*INSECT*/			{R_NO,	R_NO,	R_FR,	R_FR,	R_FR,	R_FR,	R_NO,	R_FR,	R_FR,	R_FR,	R_AL,	R_FR,	R_NO,	R_NO,	R_NO,	R_HT,	R_NO,	R_FR},
/*PLAYERALLY*/		{R_NO,	R_DL,	R_AL,	R_AL,	R_DL,	R_HT,	R_NO,	R_DL,	R_DL,	R_DL,	R_NO,	R_AL,	R_NO,	R_NO,	R_FR,	R_NO,	R_NO,	R_DL},
/*PBIOWEAPON*/		{R_NO,	R_NO,	R_DL,	R_NO,	R_DL,	R_NM,	R_DL,	R_HT,	R_DL,	R_HT,	R_NO,	R_NO,	R_NO,	R_DL,	R_NO,	R_NO,	R_DL,	R_DL},
/*ABIOWEAPON*/		{R_NO,	R_NO,	R_DL,	R_DL,	R_NM,	R_AL,	R_NO,	R_DL,	R_DL,	R_NO,	R_NO,	R_DL,	R_DL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NM},
/*GRENADE*/			{R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_DL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO},
/*GIB*/				{R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO},
/*BARNACLE*/		{R_NO,	R_DL,	R_DL,	R_DL,	R_DL,	R_NO,	R_NO,	R_NO,	R_HT,	R_DL,	R_DL,	R_DL,	R_DL,	R_NO,	R_DL,	R_DL,	R_NO,	R_NO},
/*HUMANMONSTER*/	{R_NO,	R_DL,	R_DL,	R_DL,	R_DL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_DL,	R_NO,	R_NO,	R_NO,	R_NO,	R_NO,	R_AL},
};


//-----------------------------------------------------------------------------
// Purpose: Engine creates an entity.
// Note   : Not for players
// Input  : *pent - allocated entity
// Output : int -1 deny, 0 allow?
//-----------------------------------------------------------------------------
int DispatchSpawn(edict_t *pent)
{
	if (pent == NULL)
	{
		conprintf(1, "Error! DispatchSpawn() pent is NULL!\n");
		DBG_FORCEBREAK
		return -1;
	}
	DBG_SV_PRINT("DispatchSpawn(%d)\n", ENTINDEX(pent));
	CBaseEntity *pEntity = CBaseEntity::Instance(pent);
	//ASSERT(pEntity != NULL);
	if (pEntity)
	{
//#if defined (_DEBUG)
//	if (!FStringNull(pEntity->pev->target))// XDM3038c
//		conprintf(1, "%s[%d] \"%s\" has target \"%s\"\n", STRING(pEntity->pev->classname), pEntity->entindex(), STRING(pEntity->pev->targetname), STRING(pEntity->pev->target));
//#endif

		// HACKFIX: engine does not initialize entvars with adequate values!
		if (pEntity->pev->gravity == 0.0f)
			pEntity->pev->gravity = 1.0f;// XDM3037: engine is fine with 0 but we are not!
		if (pEntity->pev->framerate <= 0.0f)
			pEntity->pev->framerate = 1.0f;// XDM3038c: engine is fine with 0 but we are not!
		if (pEntity->pev->scale <= 0.0f)
			pEntity->pev->scale = 1.0f;// XDM3035: engine is fine with 0 but we are not!

		// Initialize these or entities who don't link to the world won't have anything in here
		pEntity->pev->absmin = pEntity->pev->origin - Vector(1,1,1);
		pEntity->pev->absmax = pEntity->pev->origin + Vector(1,1,1);
		pEntity->m_vecSpawnSpot = pEntity->pev->origin;// XDM3035
		pEntity->Spawn(FALSE);

		// Try to get the pointer again, in case the spawn function deleted the entity. (CLight without names, CNodeEnt and others may call REMOVE_ENTITY)
		// UNDONE: Spawn() should really return a code to ask that the entity be deleted, but
		// that would touch too much code for me to do that right now.
		pEntity = CBaseEntity::Instance(pent);

		if (pEntity)
		{
			//WTF is this?	SAVE_SPAWN_PARMS(pent);
			if (pEntity->IsRemoving())// FL_KILLME
				return -1;
			if (!FStringNull(pEntity->m_iszGameRulesPolicy) && VerifyGameRulesPolicy(STRING(pEntity->m_iszGameRulesPolicy)) == false)
			{
				conprintf(1, "Entity %s \"%s\" \"%s\" rejected by its game rules policy: %s.\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), STRING(pEntity->pev->globalname), STRING(pEntity->m_iszGameRulesPolicy));
				return -1;
			}
			if (g_pGameRules && g_pGameRules->IsAllowedToSpawn(pEntity) == false)
			{
				conprintf(1, "Entity %s \"%s\" \"%s\" rejected by game rules: %s.\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), STRING(pEntity->pev->globalname), g_pGameRules->GetGameDescription());
				return -1;
			}
			if (gSkillData.iSkillLevel == SKILL_EASY && FBitSet(pEntity->m_iSkill, SKF_NOTEASY) ||
				gSkillData.iSkillLevel == SKILL_MEDIUM && FBitSet(pEntity->m_iSkill, SKF_NOTMEDIUM) ||
				gSkillData.iSkillLevel == SKILL_HARD && FBitSet(pEntity->m_iSkill, SKF_NOTHARD))
			{
				conprintf(1, "Entity %s \"%s\" \"%s\" rejected by its skill flags: %d.\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), STRING(pEntity->pev->globalname), pEntity->m_iSkill);
				return -1;
			}
			if (!FStringNull(pEntity->pev->globalname))// Handle global stuff here
			{
				const globalentity_t *pGlobal = gGlobalState.EntityFromTable(pEntity->pev->globalname);
				if (pGlobal)
				{
					// Already dead? delete
					if (pGlobal->state == GLOBAL_DEAD)
						return -1;
					else if (!FStrEq(STRING(gpGlobals->mapname), pGlobal->levelName))
						pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
					// In this level and not dead, continue on as normal
				}
				else
				{
					// Spawned entities default to 'On'
					gGlobalState.EntityAdd(pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON);
					//conprintf(1, "Added global entity %s (%s)\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->globalname) );
				}
			}
			UTIL_FixRenderColor(pEntity->pev->rendermode, pEntity->pev->rendercolor);// XDM3035b: do it here and not in KeyValue, when rendermode is known
			if (g_pWorld && g_pWorld != pEntity)// && g_pWorld->pev->gravity != 0.0f)// XDM3037: apply gravity to all entities
				pEntity->pev->gravity *= g_pWorld->pev->gravity;// XDM3037: HACK! But we have no other choice!

			ASSERT(pEntity->GetExistenceState() != ENTITY_EXSTATE_VOID);// XDM3038c
			// XDM3037: TODO	pEntity->pev->max_health = pEntity->pev->health;
		}
		return 0;// pent->serialnumber ? entindex() ?
	}
	conprintf(1, "DispatchSpawn() error: no instance for %s!\n", STRING(pent->v.classname));
	//ALERT(at_error, "ERROR IN DispatchSpawn()!!! Invalid pointer!\n");
	//DBG_FORCEBREAK
	return -1;
}

//static char g_szKeyValueMacroExpanded[256];
//-----------------------------------------------------------------------------
// Purpose: Called for each key/value pair parsed from the map entity table
// TODO: any value may contain some %macro%
// For example: %cvar sv_gravity% or %random_long 0 10%
// Input  : *pentKeyvalue - entity
//			*pkvd - key/value pair from the map
//-----------------------------------------------------------------------------
void DispatchKeyValue(edict_t *pentKeyvalue, KeyValueData *pkvd)
{
//	DBG_SV_PRINT("DispatchKeyValue(%d, %s %s)\n", pentKeyvalue->serialnumber, pkvd->szKeyName, pkvd->szValue);
	if (!pkvd || !pentKeyvalue)
	{
		conprintf(1, "Error! DispatchKeyValue() something is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	// allow cvar names to be overridden EntvarsKeyvalue(VARS(pentKeyvalue), pkvd);

	// If the key was an entity variable, or there's no class set yet, don't look for the object, it may not exist yet.
	if (pkvd->fHandled || pkvd->szClassName == NULL)
	{
		// This should be valid for classname only!!
		//ASSERT(strcmp(pkvd->szKeyName, "classname") == 0);
		EntvarsKeyvalue(VARS(pentKeyvalue), pkvd);// XDM3035c: now EntvarsKeyvalue() is the last thing to get called by CBaseEntity
		return;
	}

	// Get the actualy entity object
	CBaseEntity *pEntity = CBaseEntity::Instance(pentKeyvalue);
	if (pEntity)
	{
		/*TODO	static char *pRealValue;
		if (*pkvd->szValue && *pkvd->szValue == '%')
		{
			pRealValue = pkvd->szValue;
			pkvd->szValue = ExpandMacro(pkvd->szValue);
		}*/
		pEntity->KeyValue(pkvd);
		//pkvd->szValue = pRealValue;// always restore original memory pointer!
		if (pkvd->fHandled == TRUE)
		{
			if (g_ServerActive && pEntity->GetExistenceState() == ENTITY_EXSTATE_WORLD && sv_reliability.value > 0)// XDM3038c: TEMP HACK TODO: allow entities to send their state to clients
				pEntity->SendClientData(NULL, MSG_ALL, SCD_SELFUPDATE);
		}
		else if (pkvd->fHandled == FALSE)
			conprintf(1, "Entity \"%s\" has unknown key \"%s\" (\"%s\")!\n", STRING(pEntity->pev->classname), pkvd->szKeyName, pkvd->szValue);
		else if (pkvd->fHandled == 2)// XDM3037
			conprintf(1, "Entity \"%s\" has invalid value for key \"%s\": \"%s\"!\n", STRING(pEntity->pev->classname), pkvd->szKeyName, pkvd->szValue);
	}
}


// HACKHACK -- this is a hack to keep the node graph entity from "touching" things (like triggers) while it builds the graph
bool gTouchDisabled = false;

//-----------------------------------------------------------------------------
// Purpose: Engine callback, used very often
// Input  : *pentTouched
//			*pentOther
//-----------------------------------------------------------------------------
void DispatchTouch(edict_t *pentTouched, edict_t *pentOther)
{
	if (pentTouched == NULL)
	{
		conprintf(1, "Error! DispatchTouch() pentTouched is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	if (pentOther == NULL)
	{
		conprintf(1, "Error! DispatchTouch() pentOther is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	DBG_SV_PRINT("DispatchTouch(%d, %d)\n", ENTINDEX(pentTouched), ENTINDEX(pentOther));
	if (gTouchDisabled)
		return;

	if (FBitSet(pentTouched->v.flags, FL_KILLME))// about to be deleted
	{
		conprintf(1, "Error! DispatchTouch() called for removed pentTouched %s!\n", STRING(pentTouched->v.classname));
		return;
	}
	if (FBitSet(pentOther->v.flags, FL_KILLME))
	{
		conprintf(1, "Error! DispatchTouch() called for removed pentOther %s!\n", STRING(pentTouched->v.classname));
		return;
	}
	//try
	//{
	CBaseEntity *pEntity = CBaseEntity::Instance(pentTouched);
	CBaseEntity *pOther = CBaseEntity::Instance(pentOther);
	if (pEntity && pOther)//UTIL_IsValidEntity(pOther))
	{
		/*if (pOther == g_pWorld)
		{
			if (FBitSet(pEntity->pev->flags, FL_ONGROUND) && Length(pEntity->pev->velocity) > 0.0f)
				pEntity->Bounce(pOther);
		}*/
		//conprintf(2, "DispatchTouch(): %s (%s) (%s): %s\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), STRING(pEntity->pev->globalname), NAME_FOR_FUNCTION((unsigned long)((void *)*((int *)((char *)pEntity + (offsetof(CBaseEntity,m_pfnTouch)))))));
		pEntity->Touch(pOther);
	}
	/*}
	catch (...)
	{
		DBG_FORCEBREAK
		printf("*** DispatchTouch(%s, %s) exception!\n", STRING(pEntity->pev->classname), STRING(pOther->pev->classname));
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Engine may want to use some entity?
// Input  : *pentUsed
//			*pentOther
//-----------------------------------------------------------------------------
void DispatchUse(edict_t *pentUsed, edict_t *pentOther)
{
	if (pentUsed == NULL)
	{
		conprintf(1, "Error! DispatchUse() pentUsed is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	if (pentOther == NULL)
	{
		conprintf(1, "Error! DispatchUse() pentOther is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	DBG_SV_PRINT("DispatchUse(%d, %d)\n", ENTINDEX(pentUsed), ENTINDEX(pentOther));
	//?	UTIL_IsValidEntity(pentUsed);
	CBaseEntity *pEntity = CBaseEntity::Instance(pentUsed);
	if (pEntity && !pEntity->IsRemoving())//FBitSet(pEntity->pev->flags, FL_KILLME))
	{
		//conprintf(2, "DispatchUse(): %s (%s) (%s): %s\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname), STRING(pEntity->pev->globalname), NAME_FOR_FUNCTION((unsigned long)((void *)*((int *)((char *)pEntity + (offsetof(CBaseEntity,m_pfnUse)))))));
		CBaseEntity *pOther = CBaseEntity::Instance(pentOther);
		pEntity->Use(pOther, pOther, USE_TOGGLE, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Callback for all entities except players, called when pev->nextthink is now
// Warning: won't be called for entities with bad nextthink
// Input  : *pent
//-----------------------------------------------------------------------------
void DispatchThink(edict_t *pent)
{
#if defined (_DEBUG)
	if (pent == NULL)
	{
		conprintf(1, "Error! DispatchThink() pent is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
#endif
	DBG_SV_PRINT("DispatchThink(%d)\n", ENTINDEX(pent));
	CBaseEntity *pEntity = CBaseEntity::Instance(pent);
	if (pEntity)
	{
		if (FBitSet(pEntity->pev->flags, FL_DORMANT))
		{
			conprintf(1, "WARNING! Dormant entity %s is thinking!!\n", STRING(pEntity->pev->classname));// XDM: not an ERROR!
			return;
		}
		pEntity->CheckEnvironment();// XDM: affects everyone!
		//try
		//{
			pEntity->Think();
		/*}
		catch (...)
		{
			printf("*** DispatchThink(%s) exception Think()!\n", STRING(pEntity->pev->classname));
			DBG_FORCEBREAK
		}*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when an entity is unable to move because of another entity
// Input  : *pentBlocked
//			*pentOther
//-----------------------------------------------------------------------------
void DispatchBlocked(edict_t *pentBlocked, edict_t *pentOther)
{
	if (pentBlocked == NULL)
	{
		conprintf(1, "Error! DispatchBlocked() pentBlocked is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	if (pentOther == NULL)
	{
		conprintf(1, "Error! DispatchBlocked() pentOther is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	DBG_SV_PRINT("DispatchBlocked(%d, %d)\n", ENTINDEX(pentBlocked), ENTINDEX(pentOther));
	if (pentBlocked->free || pentOther->free)
	{
		conprintf(1, "Error! DispatchBlocked() called for freed entity!\n");
		return;
	}
	if (FBitSet(pentBlocked->v.flags | pentOther->v.flags, FL_KILLME))
	{
		conprintf(1, "Error! DispatchBlocked() called for removed entity!\n");
		return;
	}
	CBaseEntity *pEntity = CBaseEntity::Instance(pentBlocked);
	if (pEntity)
	{
		CBaseEntity *pOther = CBaseEntity::Instance(pentOther);
		pEntity->Blocked(pOther);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called to fill save data structure
// Input  : *pent
//			*pSaveData
//-----------------------------------------------------------------------------
void DispatchSave(edict_t *pent, SAVERESTOREDATA *pSaveData)
{
	DBG_SV_PRINT("DispatchSave(%d, %d)\n", ENTINDEX(pent), pSaveData->currentIndex);
	CBaseEntity *pEntity = CBaseEntity::Instance(pent);
	if (pEntity && pSaveData)
	{
		ENTITYTABLE *pTable = &pSaveData->pTable[pSaveData->currentIndex];
		if (pTable->pent != pent)
			ALERT(at_error, "DispatchSave: ENTITY TABLE OR INDEX IS WRONG!!!!\n");

		if (FBitSet(pEntity->ObjectCaps(), FCAP_DONT_SAVE))
			return;

		// These don't use ltime and nextthink as times really, but we'll fudge around it.
		if (pEntity->pev->movetype == MOVETYPE_PUSH)
		{
			float delta = pEntity->pev->nextthink - pEntity->pev->ltime;
			pEntity->pev->ltime = gpGlobals->time;
			pEntity->pev->nextthink = pEntity->pev->ltime + delta;
		}

		pTable->location = pSaveData->size;		// Remember entity position for file I/O
		pTable->classname = pEntity->pev->classname;	// Remember entity class for respawn

		CSave saveHelper(pSaveData);
		if (pEntity->Save(saveHelper) == 0)// XDM3038
			conprintf(1, "ERROR: entity %d (%s) Save() failed!\n", pSaveData->currentIndex, STRING(pEntity->pev->classname));

		pTable->size = pSaveData->size - pTable->location;	// Size of entity block is data size written to block
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called to unpack save data structure
// Input  : *pent
//			*pSaveData
//			globalEntity - 1/0
// Output : int -1 deny, 0 allow?
//-----------------------------------------------------------------------------
int DispatchRestore(edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity)
{
	DBG_SV_PRINT("DispatchRestore(%d, %d, %d)\n", ENTINDEX(pent), pSaveData->currentIndex, globalEntity);
	CBaseEntity *pEntity = CBaseEntity::Instance(pent);
	if (pEntity && pSaveData)
	{
		entvars_t tmpVars;
		Vector oldOffset;
		CRestore restoreHelper(pSaveData);
		if (globalEntity)
		{
			CRestore tmpRestore(pSaveData);
			tmpRestore.PrecacheMode(0);
			tmpRestore.ReadEntVars(&tmpVars);

			// HACKHACK - reset the save pointers, we're going to restore for real this time
			pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
			pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;

			const globalentity_t *pGlobal = gGlobalState.EntityFromTable(tmpVars.globalname);
			// Don't overlay any instance of the global that isn't the latest
			// pSaveData->szCurrentMapName is the level this entity is coming from
			// pGlobla->levelName is the last level the global entity was active in.
			// If they aren't the same, then this global update is out of date.
			if (!FStrEq(pSaveData->szCurrentMapName, pGlobal->levelName))
				return 0;

			// Compute the new global offset
			oldOffset = pSaveData->vecLandmarkOffset;
			CBaseEntity *pNewEntity = UTIL_FindGlobalEntity(tmpVars.classname, tmpVars.globalname);
			if (pNewEntity)
			{
				//conprintf(1, "Overlay %s with %s\n", STRING(pNewEntity->pev->classname), STRING(tmpVars.classname));
				// Tell the restore code we're overlaying a global entity from another level
				restoreHelper.SetGlobalMode(1);// Don't overwrite global fields
				pSaveData->vecLandmarkOffset = (pSaveData->vecLandmarkOffset - pNewEntity->pev->mins) + tmpVars.mins;
				pEntity = pNewEntity;// we're going to restore this data OVER the old entity
				pent = ENT(pEntity->pev);
				// Update the global table to say that the global definition of this entity should come from this level
				gGlobalState.EntityUpdate(pEntity->pev->globalname, gpGlobals->mapname);
			}
			else
			{
				// This entity will be freed automatically by the engine. If we don't do a restore on a matching entity (below)
				// or call EntityUpdate() to move it to this level, we haven't changed global state at all.
				return 0;
			}
		}

		if (pEntity->Restore(restoreHelper) == 0)// XDM3038
			conprintf(1, "ERROR: entity %d (%s) Restore() failed!\n", pSaveData->currentIndex, STRING(pEntity->pev->classname));

		if (FBitSet(pEntity->ObjectCaps(), FCAP_MUST_SPAWN))
		{
			//pEntity->Restore(restoreHelper);
			pEntity->Spawn(TRUE);
		}
		else
		{
			//pEntity->Restore(restoreHelper);
			pEntity->Precache();
		}

		// Again, could be deleted, get the pointer again.
		pEntity = CBaseEntity::Instance(pent);
#if 0
		if (pEntity && pEntity->pev->globalname && globalEntity) 
			conprintf(1, "Global %s is %s\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->model) );
#endif
		// Is this an overriding global entity (coming over the transition), or one restoring in a level
		if (globalEntity)
		{
			//conprintf(1, "After: %f %f %f %s\n", pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->origin.z, STRING(pEntity->pev->model) );
			pSaveData->vecLandmarkOffset = oldOffset;
			if (pEntity)
			{
				UTIL_SetOrigin(pEntity, pEntity->pev->origin);
				pEntity->OverrideReset();
			}
		}
		else if (pEntity && !FStringNull(pEntity->pev->globalname)) 
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable(pEntity->pev->globalname);
			if (pGlobal)
			{
				// Already dead? delete
				if (pGlobal->state == GLOBAL_DEAD)
					return -1;
				else if (!FStrEq(STRING(gpGlobals->mapname), pGlobal->levelName))
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				// In this level and not dead, continue on as normal
			}
			else
			{
				ALERT(at_error, "Global Entity %s (%s) not in table!!!\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->classname));
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd(pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON);
			}
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called by engine's SV_LinkEdict before linking entity into the world
// Note   : Used by engine's SetOrigin, SetSize, DropToFloor
// Input  : *pent
//-----------------------------------------------------------------------------
void DispatchAbsBox(edict_t *pent)
{
	if (pent->free)
	{
		conprintf(1, "Error! DispatchAbsBox() called for freed entity!\n");
		return;
	}
	if (FBitSet(pent->v.flags, FL_KILLME))
	{
		conprintf(1, "Error! DispatchAbsBox() called for removed entity!\n");
		return;
	}
	// too much spam DBG_SV_PRINT("DispatchAbsBox(%d)\n", ENTINDEX(pent));
	// XDM3037a: hopefully this will prevent more crashes
	if (fabs(pent->v.origin.x) >= MAX_ABS_ORIGIN || fabs(pent->v.origin.y) >= MAX_ABS_ORIGIN || fabs(pent->v.origin.z) >= MAX_ABS_ORIGIN)
	{
		conprintf(2, "Warning: entity %s[%d] %s \"%s\" outside world (%g %g %g)!\n", STRING(pent->v.classname), ENTINDEX(pent), STRING(pent->v.globalname), STRING(pent->v.targetname), pent->v.origin.x, pent->v.origin.y, pent->v.origin.z);
		if (sv_deleteoutsiteworld.value > 0 && IsSafeToRemove(pent))
		{
			conprintf(2, " Removing.\n");
			pent->v.solid = SOLID_NOT;// little hack to make engine ignore it right now (1/2)
			pent->v.skin = CONTENTS_EMPTY;// (2/2)
			pent->v.flags |= FL_KILLME;
			//pEntity->SetThinkNull();
			//pEntity->SetTouchNull();
			// return;
		}
		else
		{
			conprintf(2, "\n");
			pent->v.velocity.Clear();
			if (pent->v.origin.z < -MAX_ABS_ORIGIN)// hacky hack
				pent->v.origin.z = 0;
			// CRASH pent->v.origin.Set(0,0,1024);// ??
			// CRASH SET_ORIGIN(pent, pent->v.origin);
		}
	}
	else
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(pent);
		if (pEntity)
			pEntity->SetObjectCollisionBox();
		else
			SetObjectCollisionBox(&pent->v);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine
//-----------------------------------------------------------------------------
void SaveWriteFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount)
{
	CSave saveHelper(pSaveData);
	saveHelper.WriteFields(pname, pBaseData, pFields, fieldCount);
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine
//-----------------------------------------------------------------------------
void SaveReadFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount)
{
	CRestore restoreHelper(pSaveData);
	restoreHelper.ReadFields(pname, pBaseData, pFields, fieldCount);
}




//-----------------------------------------------------------------------------
// Purpose: A safe way to store entity pointers. Invalidates itself.
// Warning: Validation is based on edict's serialnumber.
//-----------------------------------------------------------------------------
EHANDLE::EHANDLE(void)// XDM3035
{
	m_pent = NULL;
	m_serialnumber = 0;
#if defined(_DEBUG)
	m_debuglevel = 0;
#endif
}

EHANDLE::EHANDLE(CBaseEntity *pEntity)// XDM3038a
{
	//m_pent = NULL;
	//m_serialnumber = 0;
#if defined(_DEBUG)
	m_debuglevel = 0;
#endif
	*this = pEntity;// is this a right thing to do? calls operator =
}

EHANDLE::EHANDLE(edict_t *pent)// XDM3038a
{
#if defined(_DEBUG)
	m_debuglevel = 0;
#endif
	Set(pent);
}

edict_t *EHANDLE::Get(void)
{
	if (m_pent)
	{
		if (IsValid())//if (m_pent->serialnumber == m_serialnumber)
			return m_pent;
		else
		{
#if defined(_DEBUG)
			if (m_debuglevel > 0)
			{
				DBG_PRINTF("EHANDLE for %s is INVALID (%d != %d)!\n", STRING(m_pent->v.classname), m_pent->serialnumber, m_serialnumber);
				if (m_debuglevel > 1){
					DBG_FORCEBREAK
				}
			}
#endif
			m_pent = NULL;// XDM3038a
			m_serialnumber = 0;
		}
	}
	return NULL; 
}

edict_t *EHANDLE::Set(edict_t *pent)
{ 
	m_pent = pent;
	if (m_pent)
		m_serialnumber = m_pent->serialnumber;
	else
		m_serialnumber = 0;

	return m_pent;
}

// XDM3038c: made this read-only check (that does not fix self) for constant functions
// Warning: Do not use this if there is absolutely no other choice!
const bool EHANDLE::IsValid(void) const
{
	if (m_pent)
	{
		if (m_pent->serialnumber == m_serialnumber)
			return true;
	}
	return false;
}

//CBaseEntity *EHANDLE::GetEntity(void)
//{ 
//	return CBaseEntity::Instance(Get());
//}

EHANDLE::operator CBaseEntity *()
{
	return CBaseEntity::Instance(Get());
}

EHANDLE::operator const CBaseEntity *()
{
	return CBaseEntity::Instance(Get());
}

EHANDLE::operator const int()// const
{
	return (Get() == NULL)?0:1;
}

CBaseEntity *EHANDLE::operator = (CBaseEntity *pEntity)
{
	if (pEntity)
	{
		m_pent = pEntity->edict();
		if (m_pent)
			m_serialnumber = m_pent->serialnumber;
	}
	else
	{
#if defined(_DEBUG)
		if (m_debuglevel > 0)
		{
			if (m_pent)
				DBG_PRINTF("EHANDLE %s[%d] (sn %d): set to NULL\n", STRING(m_pent->v.classname), ENTINDEX(m_pent), m_serialnumber);
			else
				DBG_PRINTF("EHANDLE null (sn %d): reset to NULL\n", m_serialnumber);
		}
#endif
		m_pent = NULL;
		m_serialnumber = 0;
	}
	return pEntity;
}

CBaseEntity *EHANDLE::operator ->()
{
	return CBaseEntity::Instance(Get());
}



// Global Savedata for Delay
TYPEDESCRIPTION	CBaseEntity::m_SaveData[] = 
{
	DEFINE_FIELD(CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR),
	//DEFINE_FIELD(CBaseEntity, m_pLink, FIELD_CLASSPTR),// Don't save

	DEFINE_FIELD(CBaseEntity, m_iSkill, FIELD_INTEGER),// XDM
	DEFINE_FIELD(CBaseEntity, m_iszGameRulesPolicy, FIELD_STRING),
	//DEFINE_FIELD(CBaseEntity, m_iszIcon, FIELD_STRING),
	DEFINE_FIELD(CBaseEntity, m_flBurnTime, FIELD_TIME),
	DEFINE_FIELD(CBaseEntity, m_flRemoveTime, FIELD_TIME),// XDM3038a
	DEFINE_FIELD(CBaseEntity, m_vecSpawnSpot, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseEntity, m_hOwner, FIELD_EHANDLE),// XDM3037
	DEFINE_FIELD(CBaseEntity, m_hTBDAttacker, FIELD_EHANDLE),// XDM3037
	DEFINE_ARRAY(CBaseEntity, m_szClassName, FIELD_CHARACTER, MAX_ENTITY_STRING_LENGTH),// XDM3038
	DEFINE_FIELD(CBaseEntity, m_iExistenceState, FIELD_UINT32),// XDM3038

	DEFINE_FIELD(CBaseEntity, m_pfnThink, FIELD_FUNCTION),// UNDONE: Build table of these!!!
	DEFINE_FIELD(CBaseEntity, m_pfnTouch, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnUse, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnBlocked, FIELD_FUNCTION),
#if defined(MOVEWITH)
	DEFINE_FIELD(CBaseEntity, m_iszMoveWith, FIELD_STRING),
#endif
	//DEFINE_FIELD(CBaseEntity, m_iszMoveTargetName, FIELD_STRING),
	//DEFINE_FIELD(CBaseEntity, m_vecMoveOriginDelta, FIELD_VECTOR),
};

/* original
void *operator new(size_t stAllocateBlock, entvars_t *pev)
{
	return (void *)ALLOC_PRIVATE(edict(), stAllocateBlock);
}*/

//-----------------------------------------------------------------------------
// Purpose: Allow engine to allocate instance data
// Warning: pData is the space for future CBaseEntity-derived class, but it is
//			ALLOCATED and FREED by the ENGINE!
// Warning: Do not alter pData!!! It will be overwritten by constructor!
// Note   : After ALLOC_PRIVATE, pEdict->pvPrivateData == pData, which is filled with 0s
// Input  : stAllocateBlock - std
//			*pEdict - src
// Output : void *pData
//-----------------------------------------------------------------------------
void *CBaseEntity::operator new(size_t stAllocateBlock, edict_t *pEdict)
{
/*#if defined(_DEBUG)
	if (ASSERT(stAllocateBlock > 0))
	{
		conprintf(1, "CBaseEntity::new(%s)\n", pEdict?STRING(pEdict->v.classname):"NULL");
		char *pData = (char *)ALLOC_PRIVATE(pEdict, stAllocateBlock);
		if (pData)
		{
			for (size_t i=0; i<stAllocateBlock; ++i)
				putchar(pData[i]);

			pEdict->pvPrivateData = pData;
		}
		return pData;
	}
	return NULL;
#else*/
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		void *pNewEntPrivateData = ALLOC_PRIVATE(pEdict, stAllocateBlock);
		ASSERT(pNewEntPrivateData != NULL);
//#endif
		return pNewEntPrivateData;
#if defined (USE_EXCEPTIONS)
	}
	catch (std::bad_alloc &ba)// XDM3038b
	{
		printf("XDM: PvAllocEntPrivateData() exception!\n");
		DBG_PRINTF(ba.what());
		conprintf(0, ba.what());
		DBG_FORCEBREAK
		return NULL;
	}
#endif
}

// Don't use this!
#if _MSC_VER >= 1200// only build this code if MSVC++ 6.0 or higher
void CBaseEntity::operator delete(void *pMem, edict_t *pEdict)
{
	DBG_FORCEBREAK
	conprintf(2, "CBaseEntity::delete(%s)!\n", pEdict?STRING(pEdict->v.classname):"NULL");
	if (pEdict)
		pEdict->v.flags |= FL_KILLME;
}
/*void CBaseEntity::operator delete(void *pMem)
{
	::delete(pMem);
}*/
#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
// XDM3035c: the only way to pre-set variables before KeyValue() takes place
//-----------------------------------------------------------------------------
CBaseEntity::CBaseEntity()
{
	pev = NULL;
	m_pGoalEnt = NULL;
	m_pLink = NULL;
#if defined(MOVEWITH)
	m_pMoveWith = NULL;
	m_pChildMoveWith = NULL;
	m_pAssistLink = NULL;
	m_iszMoveWith = iStringNull;
#endif
	// m_hOwner is 0
	m_flRemoveTime = 0;// XDM3038a
	m_iExistenceState = ENTITY_EXSTATE_WORLD;// XDM3038
}

#if !defined(_MSC_VER) || _MSC_VER > 1200
//-----------------------------------------------------------------------------
// Purpose: Base destructor
// XDM3037a: hopefully this will help after reallocation
//-----------------------------------------------------------------------------
CBaseEntity::~CBaseEntity()
{
	pev = NULL;
	m_pGoalEnt = NULL;
	m_pLink = NULL;
	m_pfnThink = NULL;
	m_pfnTouch = NULL;
	m_pfnUse = NULL;
	m_pfnBlocked = NULL;
#if defined(MOVEWITH)
	m_pMoveWith = NULL;
	m_pChildMoveWith = NULL;
	m_pAssistLink = NULL;
	m_iszMoveWith = iStringNull;
#endif
}
#endif // #if !defined(_MSC_VER) || _MSC_VER > 1200

//-----------------------------------------------------------------------------
// Purpose: Called before spawn() for each KV pair set for ent in world editor
// NOTE   : Set pkvd->fHandled to TRUE if the key was accepted
// Input  : *pkvd - (and also output) key/value data structure
//-----------------------------------------------------------------------------
void CBaseEntity::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "rendercolor"))
	{
		if (isalpha(*pkvd->szValue))
		{
			if (strbegin(pkvd->szValue, "teamcolor") && isdigit(pkvd->szValue[9]))// set team color
			{
				byte r,g,b;
				GetTeamColor(atoi(pkvd->szValue + 9), r,g,b);
				pev->rendercolor.x = r;
				pev->rendercolor.y = g;
				pev->rendercolor.z = b;
				pkvd->fHandled = TRUE;
			}
			/*else if (strbegin(pkvd->szValue, "source") && isspace(pkvd->szValue[6]))// XDM3037: copy from entity
			{
				// useless, because target may not exist yet
			}*/
		}
		else if (StringToVec(pkvd->szValue, pev->rendercolor))
		{
			pkvd->fHandled = TRUE;
		}

		if (pkvd->fHandled == FALSE)// XDM3036: only fix rendercolor if something went wrong
		{
			conprintf(1, "Warning: fixing bad %s in entity %s: \"%s\"\n", pkvd->szKeyName, pkvd->szClassName, pkvd->szValue);
			UTIL_FixRenderColor(pev->rendermode, pev->rendercolor);
			pkvd->fHandled = TRUE;// mark as known
		}
	}
	else if (strbegin(pkvd->szKeyName, "_minlight") > 0)// Hammer/Zoner's Compile Tools
		pkvd->fHandled = TRUE;
	else if (strbegin(pkvd->szKeyName, "_light") > 0)
		pkvd->fHandled = TRUE;
	else if (strbegin(pkvd->szKeyName, "_fade") > 0)
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "style"))
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "delay"))
		pkvd->fHandled = TRUE;
#if defined(MOVEWITH)
	else if (FStrEq(pkvd->szKeyName, "movewith"))
	{
		//m_pMoveWith = UTIL_FindEntityByTargetname(this, pkvd->szValue);// NO! All ents must be spawned first!
		m_iszMoveWith = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
#endif // MOVEWITH
#if defined(SHL_LIGHTS)
	else if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
#endif // SHL_LIGHTS
	else if (FStrEq(pkvd->szKeyName, "skill"))
	{
		m_iSkill = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "gamerules"))
	{
		m_iszGameRulesPolicy = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "origin"))// Overrides entvars set in DispatchKeyValue()
	{
		if (StringToVec(pkvd->szValue, pev->origin))
		{
			UTIL_SetOrigin(this, pev->origin);// if someone changes origin during the game
			pkvd->fHandled = TRUE;
		}
		else
			pkvd->fHandled = FALSE;
	}
	else if (FStrEq(pkvd->szKeyName, "angles"))
	{
		if (StringToVec(pkvd->szValue, pev->angles))
			pkvd->fHandled = TRUE;
		else
			pkvd->fHandled = FALSE;
	}
	else if (FStrEq(pkvd->szKeyName, "topcolor"))// XDM3038c: same as entering colormap, but much more convenient
	{
		int c = atoi(pkvd->szValue);
		pev->colormap = (pev->colormap & 0xFFFFFF00) | (c & 0x000000FF);
	}
	else if (FStrEq(pkvd->szKeyName, "bottomcolor"))// XDM3038c
	{
		int c = atoi(pkvd->szValue);
		pev->colormap = (pev->colormap & 0xFFFF00FF) | ((c & 0x000000FF) << CHAR_BIT);
	}
	else if (FStrEq(pkvd->szKeyName, "drawalways"))
	{
		if (atoi(pkvd->szValue) > 0)
		{
			pev->flags |= FL_DRAW_ALWAYS;
			pkvd->fHandled = TRUE;
		}
		else
			pkvd->fHandled = FALSE;
	}
	else if (FStrEq(pkvd->szKeyName, "icon"))// XDM3035c
	{
		//	m_iszIcon = ALLOC_STRING(pkvd->szValue);
		//	if (g_ServerActive)//IsRunTime())
		//hang		pev->iuser4 = MODEL_INDEX(STRINGV(m_iszIcon));

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "comment"))
	{
		//if (g_pCvarDeveloper && g_pCvarDeveloper->value > 1)
		//conprintf(2, "Entity %s (%s) comment: %s\n", STRING(pev->classname), pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	//else if (FStrEq(pkvd->szKeyName, "{"))// XDM3036 TEST
	//{
	//}
	else
	{
		EntvarsKeyvalue(pev, pkvd);
		// no alerts here
		//pkvd->fHandled = FALSE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Entity is being created (simple version)
//-----------------------------------------------------------------------------
void CBaseEntity::Spawn(void)
{
	Precache();
	Materialize();// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: Entity is being created (restore-aware version)
// Input  : restore - 1 if spawning after Restore (loading saved game) with FCAP_MUST_SPAWN
//-----------------------------------------------------------------------------
void CBaseEntity::Spawn(byte restore)
{
	Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache resources. Called either from Spawn() or directly.
//-----------------------------------------------------------------------------
void CBaseEntity::Precache(void)
{
	//? pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));

	// BAD: we must not precache this, but transmit its name
	//	if (!FStringNull(m_iszIcon))
	//hang		pev->iuser4 = PRECACHE_MODEL(STRINGV(m_iszIcon));// XDM3035c
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: virtual overridable, called during [re]spawn
// Note   : As usual, call parent's function.
//-----------------------------------------------------------------------------
void CBaseEntity::Materialize(void)
{
	//if (GetExistenceState() != ENTITY_EXSTATE_CARRIED){ ???
	UTIL_SetOrigin(this, pev->origin);// XDM3038c
	m_vecSpawnSpot = pev->origin;// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: virtual overridable, destroy this entity. Soft-shutdown
// Warning: Removes entity NOW, not on next frame.
// WARNING: NEVER SET/CALL SUB_Remove FROM HERE!!!
// Note   : As usual, call parent's function.
//-----------------------------------------------------------------------------
void CBaseEntity::Destroy(void)
{
	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Overridable. Initiate respawn by calling ScheduleRespawn() from your subclass.
// Output : CBaseEntity * - the entity that will spawn. Can be a copy. NULL means fail and remove.
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::StartRespawn(void)
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Non-overridable, make this entity SUB_Respawn in N seconds
// Warning: May get called by partially picked items that do not change state (touchable) while waiting to respawn!
// Call chain: -> SUB_Respawn -> Spawn(false) -> Materialize
// Input  : delay - seconds
//-----------------------------------------------------------------------------
void CBaseEntity::ScheduleRespawn(const float delay)
{
	SetThink(&CBaseEntity::SUB_Respawn);
	SetNextThink(delay);
}

//-----------------------------------------------------------------------------
// Purpose: Respawn. Overridable. For effects, anything.
// Warning: Called after Spawn(FALSE);
//-----------------------------------------------------------------------------
void CBaseEntity::OnRespawn(void)
{
	//DBG_PRINTF("%s[%d]::OnRespawn()\n", STRING(pev->classname), entindex());
	EMIT_SOUND_DYN(edict(), CHAN_BODY, DEFAULT_RESPAWN_SOUND, VOL_NORM, ATTN_NORM, 0, PITCH_LOW);// XDM3035
	SetBits(pev->effects, EF_MUZZLEFLASH);
	if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())// do this AFTER restoring entity origin
	{
		// TODO: compbine in a single message/event
		ParticleBurst(pev->origin, 32, 128, 10);
		BeamEffect(TE_BEAMCYLINDER, pev->origin, pev->origin + Vector(0,0,56), g_iModelIndexShockWave, 0,25, 10, 64,32, Vector(95,95,255), 255, 2);// small shockwave
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &save - buffer
// Output : int - 1 success, 0 failure
//-----------------------------------------------------------------------------
int CBaseEntity::Save(CSave &save)
{
	if (save.WriteEntVars(pev))
		return save.WriteFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Restore class data after loading, called before Spawn(TRUE)
// Input  : &restore - buffer
// Output : int - 1 success, 0 failure
//-----------------------------------------------------------------------------
int CBaseEntity::Restore(CRestore &restore)
{
	//conprintf(2, "Restore(%s[%d])\n"STRING(pev->classname), entindex());

	int status = restore.ReadEntVars(pev);
	if (status)
		status = restore.ReadFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

	if (pev->modelindex != 0 && !FStringNull(pev->model))
	{
		Vector mins(pev->mins);// Set model is about to destroy these
		Vector maxs(pev->maxs);
		pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
		SET_MODEL(edict(), STRING(pev->model));
		UTIL_SetSize(this, mins, maxs);// Restore them
	}
	return status;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: called before deleting[] this entity... probably
// Note   : Called by the engine via OnFreePrivateData(), NEW_DLL_FUNCTIONS
// Warning: Only available through NEW_DLL_FUNCTIONS!
// Note   : MAY be more reliable than UpdateOnRemove()?
//-----------------------------------------------------------------------------
void CBaseEntity::OnFreePrivateData(void)
{
	edict_t *pClientEd = NULL;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pClientEd = UTIL_ClientEdictByIndex(i);
		if (pClientEd && pClientEd->pvPrivateData && pClientEd->free == 0 && !FStringNull(pClientEd->v.netname))// engine may be shutting down or the client might got disconnected
		{
			if (pClientEd->v.euser2 == edict())// XDM3035c: if a player was watching through this entity
			{
				UTIL_SetView(pClientEd, pClientEd);
				pClientEd->v.euser2 = NULL;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: entity is about to be transfered to another map
// Clear links and other sensitive data pointers here
//-----------------------------------------------------------------------------
void CBaseEntity::PrepareForTransfer(void)
{
#if defined(MOVEWITH)
	m_pMoveWith = NULL;// XDM-only: TESTME!
	m_pChildMoveWith = NULL;
	m_pSiblingMoveWith = NULL;
	m_pAssistLink = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Entity capabilities flags. XDM: global safety
// Output : int
//-----------------------------------------------------------------------------
int CBaseEntity::ObjectCaps(void)
{
	if (IsBSPModel())
		return 0;

	return FCAP_ACROSS_TRANSITION;
}

//-----------------------------------------------------------------------------
// Purpose: Decide if this entity should collide with other entity logically
// Warning: Due to error in the engine, this function is NOT used from outside! (see ::ShouldCollide comments)
// Note   : Called by the engine via ShouldCollide(), unpredictably
// Warning: Only available through NEW_DLL_FUNCTIONS!
// Input  : *pOther - 
//-----------------------------------------------------------------------------
int CBaseEntity::ShouldCollide(CBaseEntity *pOther)
{
	//if (pOther == m_hOwner)
	//	return 0;

	// Don't: the engine checks that.	return Intersects(pOther);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize absmin and absmax to the appropriate box (non-OOP version)
// Note   : Called by the engine via DispatchAbsBox()
// Input  : *pev - the object
//-----------------------------------------------------------------------------
void SetObjectCollisionBox(entvars_t *pev)
{
	if ((pev->solid == SOLID_BSP) && !pev->angles.IsZero())//(pev->angles.x || pev->angles.y || pev->angles.z))
	{	// expand for rotation
		float max, v;
		unsigned int i;
		max = 0;
		for (i=0 ; i<3 ; ++i)
		{
			v = fabs(pev->mins[i]);
			if (v > max)
				max = v;
			v = fabs(pev->maxs[i]);
			if (v > max)
				max = v;
		}
		for (i=0 ; i<3 ; ++i)
		{
			pev->absmin[i] = pev->origin[i] - max;
			pev->absmax[i] = pev->origin[i] + max;
		}
	}
	else
	{
		//pev->absmin = pev->origin + pev->mins;
		pev->absmin = pev->origin;
		pev->absmin += pev->mins;
		//pev->absmax = pev->origin + pev->maxs;
		pev->absmax = pev->origin;
		pev->absmax += pev->maxs;
	}
	pev->absmin.x -= 1;
	pev->absmin.y -= 1;
	pev->absmin.z -= 1;
	pev->absmax.x += 1;
	pev->absmax.y += 1;
	pev->absmax.z += 1;
}

//-----------------------------------------------------------------------------
// Purpose: Overridable
//-----------------------------------------------------------------------------
void CBaseEntity::SetObjectCollisionBox(void)
{
	::SetObjectCollisionBox(pev);
}

//-----------------------------------------------------------------------------
// Purpose: Send necessary updates to one or many clients as requested
// Note   : Implements "on-off" principle, eliminating the need of constant updates
// Note   : Check msgtype and sendcase INDEPENDANTLY of each other
// Input  : *pClient - destination (NULL for MSG_ALL)
//			msgtype - MSG_ONE/MSG_ALL (all other messages are NOT recommended for external use)
//			sendcase - SCD_GLOBALUPDATE, SCD_CLIENTUPDATEREQUEST, SCD_ENTREMOVE
// Output : int - 0 means no data were sent, 1+ - number of messages formed by this call (to prevent overflows)
//-----------------------------------------------------------------------------
int CBaseEntity::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	// example:	if (IsRemoving())// helps to detect if this entity is about to send "removed" signal
	// example: if (msgtype == MSG_ALL)// means this data is requested by the world or the entity itself for everyone to recieve
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Simplified version
// Input  : *szName - must be a pointer to constant memory, e.g. "monster_class" because the entity will keep a pointer to it after this call.
// Output : CBaseEntity *
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner)
{
	return Create(szName, vecOrigin, vecAngles, g_vecZero, pentOwner);
}

//-----------------------------------------------------------------------------
// Purpose: The one and only way to create an entity! Spawn() will be called.
// Input  : *szName - class name XDM3038: now it's kept internally so temporary strings are ok.
//			&vecOrigin - all these values are set BEFORE Spawn()ing
//			&vecAngles - 
//			&vecVelocity - 
//			*pentOwner - will be saved into m_hOwner
//			spawnflags - 
// Output : CBaseEntity *
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecVelocity, edict_t *pentOwner, int spawnflags)
{
	if (szName == NULL || *szName == '\0')
		return NULL;

	edict_t	*pent = NULL;
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		pent = CREATE_NAMED_ENTITY(szName);//pent = CREATE_NAMED_ENTITY(MAKE_STRING(szName));// WARNING: /!\ this string is temporary, but is written to entvars now!
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		fprintf(stderr, " *** CREATE_NAMED_ENTITY exception! ***\n");
		return NULL;
	}
#endif
	if (FNullEnt(pent))
	{
		conprintf(0, "CBaseEntity::Create(%s): Unable to create entity!\n", szName);
		return NULL;
	}

	CBaseEntity *pEntity = Instance(pent);
	if (pEntity)
	{
		pEntity->SetClassName(szName);// XDM3038: make this string permanent now!!!
		//pEntity->pev->owner = pentOwner;
		pEntity->m_hOwner.Set(pentOwner);// XDM3037: OWNER
		pEntity->pev->owner = NULL;
		pEntity->pev->origin = vecOrigin;
		pEntity->pev->angles = vecAngles;
		pEntity->pev->velocity = vecVelocity;
		pEntity->pev->spawnflags = spawnflags;
		DispatchSpawn(pent);
#if defined(_DEBUG)
		if (pEntity->entindex() >= gpGlobals->maxEntities)//2048)// MAX_EDICTS is purely theoretical :(
		{
			SERVER_PRINT("WARNING! ENTITY INDEX >= gpGlobals->maxEntities!\n");
			conprintf(0, "WARNING: CBaseEntity::Create(%s): index >= %d!\n", szName, gpGlobals->maxEntities);
		}
#endif
	}
	else
		conprintf(0, "CBaseEntity::Create(%s): NULL CBaseEntity!\n", szName);

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: A more or less safe way to duplicate an entity (entvars only)! Use this!
// Warning: Right now it only copies entvars! The only workaround can be the use of CSave/CRestore.
// Undone : TODO: Write a copy constructor for CBaseEntity and utilize this functionality inside.
// Input  : iName - new classname of a new entity (allocated string_t)
//			*pSource - entity to copy
//			spawnflags - will be set prior to spawning
//			spawn - Spawn() will be called if true
// Output : CBaseEntity *
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::CreateCopy(const char *szNewClassName, CBaseEntity *pSource, int spawnflags, bool spawn)
{
	// future if (!PARM_CHK_NULL(szNewClassName) || !PARM_CHK_NULL(pSourceVars))
	if (/*FStringNull(iName)*/ szNewClassName == NULL || pSource == NULL)
		return NULL;

	edict_t	*pent = NULL;
	entvars_t *pSourceVars = pSource->pev;
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		pent = CREATE_NAMED_ENTITY(szNewClassName);//MAKE_STRING(szNewClassName));// WARNING: /!\ this string is temporary, but is written to pev
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		fprintf(stderr, " *** CREATE_NAMED_ENTITY exception! ***\n");
		return NULL;
	}
#endif
	if (FNullEnt(pent))
	{
		conprintf(1, "CBaseEntity::CreateCopy: Unable to create entity '%s'!\n", szNewClassName);//STRING(iName));
		return NULL;
	}

	CBaseEntity *pEntity = Instance(pent);
	if (pEntity)// && pSourceVars)
	{
		/*if (strcmp(STRING(pSource->pev->classname), STRING(pEntity->pev->classname)) == 0)
		{
			// concept if (sizeof(pSource) == sizeof(pEntity))
			// UNDONE // TODO
			memcpy(pEntity, pSource, sizeof(entvars_t));
		}*/
		//!string_t classname = pEntity->pev->classname;
		string_t globalname = pEntity->pev->globalname;
		edict_t *pNewContainingEntity = pEntity->pev->pContainingEntity;
		memcpy(pEntity->pev, pSourceVars, sizeof(entvars_t));
		//!pEntity->pev->classname = classname;
		pEntity->SetClassName(szNewClassName);
		pEntity->pev->globalname = globalname;
		pEntity->pev->pContainingEntity = pNewContainingEntity;
		pEntity->pev->spawnflags = spawnflags;// allow override
		if (spawn)
			DispatchSpawn(pEntity->edict());
		//else
		//	pEntity->m_iExistenceState = ENTITY_EXSTATE_VOID;// TESTME: also makes it invisible
	}
	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Add some sanity to this [c|t]rash code. Get CBaseEntity of edict_t
// Warning: This function should allow entities with FL_KILLME!
// Input  : *pent - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::Instance(edict_t *pent)
{
	//ASSERTSZ(pent != NULL, "CBaseEntity::Instance(NULL)!\n");
	//if (pent == NULL)// XDM3035c: TESTME! DANGER!
	//	pent = ENT(0);// WTF?!! This is the world!
	if (pent == NULL)
		return NULL;
	if (pent->free)
		return NULL;
	if (pent->pvPrivateData == NULL)
	{
		conprintf(2, "CBaseEntity::Instance() failed: no private data!\n");
		return false;
	}
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);
	if (pEntity)
	{
		if (pEntity->pev == NULL)
		{
			ALERT(at_warning, "ERROR! Fixing entity without entvars in CBaseEntity::Instance()!\n");
			DBG_FORCEBREAK
			pEntity->pev = &pent->v;
		}
		else if (ASSERTSZ(&pent->v == pEntity->pev, "CBaseEntity::Instance() ERROR: memory corruption detected!!!\n") == false)// XDM3037a: memory corruption detected
		{
			pent->v.flags = FL_KILLME;
			pEntity = NULL;
		}
#if defined(_DEBUG)
		size_t *vptr = *(size_t **)pEntity;
		if (vptr == NULL)
		{
			ALERT(at_warning, "FATAL ERROR! Entity without VIRTUAL FUNCTION POINTERS in CBaseEntity::Instance()!!!\n");
			pent->v.flags = FL_KILLME;
			DBG_FORCEBREAK
			return NULL;
		}
		// this fails FL_KILLME	ASSERT(UTIL_IsValidEntity(pEntity) == true);
#endif // defined(_DEBUG)
	}
	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine from DispatchThink()
//-----------------------------------------------------------------------------
void CBaseEntity::Think(void)
{
	if (m_pfnThink)
		(this->*m_pfnThink)();
/* this is normal and occurs too often #if defined (_DEBUG)
	else
		DBG_PRINTF("%s[%d] \"%s\": WARNING: Think() is called while m_pfnThink is NULL!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
#endif*/
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine from DispatchTouch()
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBaseEntity::Touch(CBaseEntity *pOther)
{
	if (m_pfnTouch)
		(this->*m_pfnTouch)(pOther);
/*#if defined (_DEBUG)
	else
		DBG_PRINTF("%s[%d] \"%s\": WARNING: Touch() is called while m_pfnTouch is NULL!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
#endif*/
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine from DispatchBlocked()
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBaseEntity::Blocked(CBaseEntity *pOther)
{
	if (m_pfnBlocked)
		(this->*m_pfnBlocked)(pOther);
/*#if defined (_DEBUG)
	else
		DBG_PRINTF("%s[%d] \"%s\": WARNING: Blocked() is called while m_pfnBlocked is NULL!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
#endif*/
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine from DispatchUse()
// Input  : *pActivator - person who started the chain (for CBaseEntity::Use())
//			*pCaller - exact entity that fires the target
//			useType - USE_TYPE
//			value - some user data
//-----------------------------------------------------------------------------
void CBaseEntity::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_pfnUse)
		(this->*m_pfnUse)(pActivator, pCaller, useType, value);
#if defined (_DEBUG)
	else
		DBG_PRINTF("%s[%d] \"%s\": WARNING: Use() is called while m_pfnUse is NULL!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Stop all thinking, but don't nullify the think pointer!
//-----------------------------------------------------------------------------
void CBaseEntity::DontThink(void)
{
#if defined(MOVEWITH)
	m_fNextThink = 0;
	if (m_pMoveWith == NULL && m_pChildMoveWith == NULL)
	{
		pev->nextthink = 0;
		m_fPevNextThink = 0;
	}
#else
	pev->nextthink = 0;
#endif
	//conprintf(1, "DontThink for %s\n", STRING(pev->targetname));
}

//-----------------------------------------------------------------------------
// Purpose: Should be used instead of setting pev->nextthink manually
// Input  : &delay - 
//-----------------------------------------------------------------------------
void CBaseEntity::SetNextThink(const float &delay)
{
	pev->nextthink = gpGlobals->time + delay;
}

//-----------------------------------------------------------------------------
// Purpose: Is this entity is about to be removed
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsRemoving(void)// XDM3037
{
	return FBitSet(pev->flags, FL_KILLME);
}

//-----------------------------------------------------------------------------
// Purpose: THE ONLY WAY TO SET ENTITY CLASS NAME!
// Warning: never use any other mechanisms!!!
// Input  : *szClassName - a simple 0-terminated string
//-----------------------------------------------------------------------------
void CBaseEntity::SetClassName(const char *szClassName)
{
	if (szClassName)
	{
		strncpy(m_szClassName, szClassName, MAX_ENTITY_STRING_LENGTH);// XDM3038: good place to store entity's classname
		pev->classname = MAKE_STRING(m_szClassName);// A MUST!
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set size from bbmin/bbmax of a model's sequence
// Warning: requires pev->sequence to be set! (0 is ok)
//-----------------------------------------------------------------------------
void CBaseEntity::SetModelCollisionBox(void)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)GET_MODEL_PTR(edict());
	if (pstudiohdr == NULL)
	{
		conprintf(1, "SetModelCollisionBox() error: unable to get model pointer for %s[%d]: \"%s\"!\n", STRING(pev->classname), entindex(), STRING(pev->model));
	}
	else
	{
		//mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);
		//UTIL_SetSize(this, pseqdesc[pev->sequence].bbmin, pseqdesc[pev->sequence].bbmax);
		ExtractBbox(pstudiohdr, pev->sequence, pev->mins, pev->maxs);
	}
	UTIL_SetSize(this, pev->mins, pev->maxs);
}

//-----------------------------------------------------------------------------
// Purpose: Absolute min/max size box intersection of two entities
// Input  : *pOther - 
// Output : int - 1/0
//-----------------------------------------------------------------------------
int	CBaseEntity::Intersects(CBaseEntity *pOther)
{
	return BoundsIntersect(pOther->pev->absmin, pOther->pev->absmax, pev->absmin, pev->absmax)?1:0;// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: Has something to do with global entities
//-----------------------------------------------------------------------------
void CBaseEntity::MakeDormant(void)
{
	DBG_PRINT_ENT_THINK(MakeDormant);
	// XDM3038a: TESTME!!!
	//if (g_pGameRules && g_pGameRules->IsCoOp() && g_pGameRules->GetGameMode() == COOP_MODE_LEVEL && POINT_CONTENTS(Center()) != CONTENTS_SOLID)
	if (g_pGameRules != NULL && /*old origin! POINT_CONTENTS(Center()) != CONTENTS_SOLID && */!g_pGameRules->FShouldMakeDormant(this))
	{
		conprintf(1, "Unblocking dormant entity %s[%d] \"%s\"\n", STRING(pev->classname), entindex(), STRING(pev->globalname));
		ClearBits(pev->flags, FL_DORMANT);
	}
	else
	{
		SetBits(pev->flags, FL_DORMANT);
		SetBits(pev->effects, EF_NODRAW);
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
		ASSERT(m_iExistenceState != ENTITY_EXSTATE_CARRIED);// XDM3038c
		//m_iExistenceState = ENTITY_EXSTATE_WORLD;// XDM3038c
		DontThink();// XDM3038a
	}
	UTIL_SetOrigin(this, pev->origin);
}

//-----------------------------------------------------------------------------
// Purpose: Dormant means this entity went outside the map during level change and was made purely virtual
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsDormant(void) const
{
	return FBitSet(pev->flags, FL_DORMANT);
}

//-----------------------------------------------------------------------------
// Purpose: Should reliably tell if an entity has BSP model (map brush)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsBSPModel(void) const
{
	if ((pev->solid == SOLID_BSP) || (pev->movetype == MOVETYPE_PUSHSTEP))
		return true;

	if (!FStringNull(pev->model))
	{
		if (*STRING(pev->model) == '*')// XDM3037a; reliable
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Basic check, DO NOT overestimate it! Kind of obsolete...
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsInWorld(void) const
{
	//if (POINT_CONTENTS(Center()) == CONTENTS_SOLID)
	//	return false;

	// position 
	if (pev->origin.x > MAX_ABS_ORIGIN) return false;// XDM
	if (pev->origin.y > MAX_ABS_ORIGIN) return false;
	if (pev->origin.z > MAX_ABS_ORIGIN) return false;
	if (pev->origin.x < -MAX_ABS_ORIGIN) return false;
	if (pev->origin.y < -MAX_ABS_ORIGIN) return false;
	if (pev->origin.z < -MAX_ABS_ORIGIN) return false;

	// speed
	/*if (pev->velocity.x > MAX_ABS_VELOCITY) return FALSE;
	if (pev->velocity.y > MAX_ABS_VELOCITY) return FALSE;
	if (pev->velocity.z > MAX_ABS_VELOCITY) return FALSE;
	if (pev->velocity.x < -MAX_ABS_VELOCITY) return FALSE;
	if (pev->velocity.y < -MAX_ABS_VELOCITY) return FALSE;
	if (pev->velocity.z < -MAX_ABS_VELOCITY) return FALSE;*/
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Should never get here when checking a real player, but still...
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::IsPlayer(void) const
{
#if defined(_DEBUG)
	if (pev && FBitSet(pev->flags, FL_CLIENT))// should never get here when checking players!
	{
		ASSERTSZ(0, "CBaseEntity::IsPlayer() called for a client!\n");
		return true;
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Should this entity toggle according to given signal and current state
// Input  : useType - 
//			currentState - true is "active", "on", "working"
// Output : int - 1/0
//-----------------------------------------------------------------------------
int CBaseEntity::ShouldToggle(USE_TYPE useType, bool currentState)
{
	if ((currentState && useType == USE_ON) || (!currentState && useType == USE_OFF))
		return 0;

	// probably differs from original USE_SET concept if ((useType == USE_SET) && (currentState == value > 0.0f))
	//	return 0;

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Which decal should be drawn on this entity for specified damage type
// Input  : &bitsDamageType - 
// Output : Server decal index
//-----------------------------------------------------------------------------
int	CBaseEntity::DamageDecal(const int &bitsDamageType)
{
	if (pev->rendermode == kRenderTransAlpha)
		return -1;

	if (pev->rendermode != kRenderNormal)
		return DECAL_BPROOF1;

	return DECAL_GUNSHOT1 + RANDOM_LONG(0,4);
}

//-----------------------------------------------------------------------------
// Purpose: Align entity to underlying surface (sets pev->angles)
// BUGBUG: UNDONE: TODO: REWRITE THIS!
//-----------------------------------------------------------------------------
void CBaseEntity::AlignToFloor(void)
{
	DBG_PRINT_ENT_THINK(AlignToFloor);
	TraceResult tr;
	//UTIL_TraceLine(pev->origin, pev->origin - Vector(0,0,pev->size.z), ignore_monsters, edict(), &tr);
	Vector vec2;
	if (FBitSet(pev->flags, FL_ONGROUND) || pev->velocity.IsZero())
	{
		vec2 = pev->origin;
		vec2.z -= pev->size.z;// /2
	}
	else
		vec2 = pev->origin + pev->velocity*pev->size.z;//(pev->size.Length());

	UTIL_TraceLine(pev->origin+Vector(0,0,1), vec2, ignore_monsters, edict(), &tr);
	if (tr.flFraction < 1.0f)
	{
		//conprintf(1, "AlignToFloor: tr.flFraction < 1.0\n");
		/*Vector end = pev->origin + tr.vecPlaneNormal*16;
		UTIL_ShowLine(pev->origin, end, 5.0, 0,0,255);
		AngleVectors(pev->angles, NULL, NULL, end);
		//AngleVectors(pev->angles, end, NULL, NULL);
		UTIL_ShowLine(pev->origin, pev->origin+end*10, 5.0, 0,0,255);*/
		// normal should become the UP for the model
		// but we can only set FORWARD
		VectorAngles(tr.vecPlaneNormal, pev->angles);
//#if !defined (SV_NO_PITCH_CORRECTION)
		pev->angles.x += 90.0f;
//#endif
		/*pev->angles.x += test1.value;
		pev->angles.y += test2.value;
		pev->angles.z += test3.value;*/
		/*AngleVectors(pev->angles, NULL, NULL, end);
		UTIL_ShowLine(pev->origin, pev->origin + end*12, 5.0, 255,0,0);*/
		/*Vector n;
		VectorAngles(tr.vecPlaneNormal, n);
#if defined (NOSQB)
		pev->angles.x = n.x + test1.value;
		pev->angles.y = n.y + test2.value;
		pev->angles.z = n.z + test3.value;
		pev->angles.x = n.x + 90;
		pev->angles.y = n.y + 180;
		pev->angles.z = n.z + 0;
#else
		pev->angles.x = n.x + 270.0f;// XDM: ignore rotation .y (against vertical axis)
#endif
		pev->angles.z = n.z;*/
		//UTIL_SetOrigin(this, pev->origin);// should not be changed
	}
}

#define BURNS_PER_SECOND	2.0f
//-----------------------------------------------------------------------------
// Purpose: Check if in lava/slime/other stuff and take desired actions
// Warning: Engine may use pev->dmg and pev->dmgtime for similar purpose
//-----------------------------------------------------------------------------
void CBaseEntity::CheckEnvironment(void)
{
	//DBG_PRINT_ENT_THINK(CheckEnvironment);
	if (pev->takedamage != DAMAGE_NO && pev->radsuit_finished <= gpGlobals->time)// HACK: radsuit_finished is not used anyway
	{
		if (pev->waterlevel > WATERLEVEL_NONE)
		{
			if (/*!FBitSet(pev->flags, FL_IMMUNE_LAVA) && */pev->watertype == CONTENTS_LAVA)	// do damage
			{
				TakeDamage(g_pWorld, g_pWorld, 10.0f * pev->waterlevel, DMG_BURN | DMG_SLOWBURN);
			}
			else// not lava
			{
				if (!FBitSet(pev->flags, FL_IMMUNE_SLIME) && pev->watertype == CONTENTS_SLIME)// do damage
					TakeDamage(g_pWorld, g_pWorld, 4.0f * pev->waterlevel, DMG_ACID);

				if (m_flBurnTime >= gpGlobals->time)
				{
					// TODO: Add some PSSSHHHH sound here
					if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
					{
						Vector c(Center());
						MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, pev->origin);
							WRITE_BYTE(TE_SMOKE);
							WRITE_COORD(c.x);
							WRITE_COORD(c.y);
							WRITE_COORD(c.z);
							WRITE_SHORT(g_iModelIndexSmoke);
							WRITE_BYTE(30);// scale * 10
							WRITE_BYTE(20);// framerate
						MESSAGE_END();
					}
					m_flBurnTime = 0.0f;// stop burning
					m_hTBDAttacker = NULL;// XDM3037
				}
			}
			//conprintf(1, "CBaseEntity::CheckEnvironment() 2\n");
		}
		else// waterlevel
		{
			if (m_flBurnTime != 0.0f)
			{
				// Currently damage amount is determined by the burning time
				if (m_flBurnTime > gpGlobals->time)// UNDONE: damage attacker!!
				{
					// WARNING! Do not use DMG_IGNITE here! Recursion!! Burnrate is specified in seconds, we do half damage twice a second.
					// TODO: find out which is worse NULL or g_pWorld
					TakeDamage(m_hTBDAttacker.Get()?NULL:g_pWorld, m_hTBDAttacker, TD_SLOWBURN_DAMAGE/BURNS_PER_SECOND, DMG_BURN | DMG_SLOWBURN | DMG_NEVERGIB);// Don't DMG_IGNITE!!!
				}
				else
				{
					m_flBurnTime = 0.0f;// finished burning
					m_hTBDAttacker = NULL;// XDM3037
				}
			}
		}
		pev->radsuit_finished = gpGlobals->time + 1.0f/BURNS_PER_SECOND;// don't check too fast
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if a line can be traced from the caller's eyes to the target
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::FVisible(CBaseEntity *pEntity)
{
	if (FBitSet(pEntity->pev->flags, FL_NOTARGET))
		return false;

	// don't look through water
	if ((pev->waterlevel < WATERLEVEL_HEAD && pEntity->pev->waterlevel >= WATERLEVEL_HEAD) || (pev->waterlevel >= WATERLEVEL_HEAD && pEntity->pev->waterlevel == WATERLEVEL_NONE))// we can see enemy feet while in water
		return false;

	// TODO: UNDONE: make a 2D projection of entity's bounding box on a plane perpendicular to the line of sight and check its corners (or some other points for better precision) -- probably bad because that rectangle may slip through surrounding thin walls
	TraceResult tr;
	UTIL_TraceLine(EyePosition(), pEntity->EyePosition(), ignore_monsters, ignore_glass, edict()/*pentIgnore*/, &tr);
	if (tr.flFraction != 1.0f)
		return false;// Line of sight is not established

	return true;// line of sight is valid.
}

//-----------------------------------------------------------------------------
// Purpose: Checks if a line can be traced from the caller's eyes to the target vector
// Input  : vecOrigin - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::FVisible(const Vector &vecOrigin)
{
	if (pev->waterlevel < WATERLEVEL_HEAD && (POINT_CONTENTS(vecOrigin) <= CONTENTS_WATER))// XDM3035c: don't look through water
		return false;

	TraceResult tr;
	UTIL_TraceLine(EyePosition(), vecOrigin, ignore_monsters, ignore_glass, edict()/*pentIgnore*/, &tr);
	if (tr.flFraction != 1.0f)
		return false;// Line of sight is not established

	return true;// line of sight is valid.
}

//-----------------------------------------------------------------------------
// Purpose: WTF
// Input  : *pTarget - 
// vecTargetOrigin is output
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::FBoxVisible(CBaseEntity *pTarget, Vector &vecTargetOrigin, float flSize)
{
	if ((pev->waterlevel < WATERLEVEL_HEAD && pTarget->pev->waterlevel >= WATERLEVEL_HEAD) || (pev->waterlevel >= WATERLEVEL_HEAD && pTarget->pev->waterlevel == WATERLEVEL_NONE))
		return false;

	TraceResult tr;
	Vector vecTarget, vecEye(EyePosition());
	for (short i = 0; i < 5; ++i)
	{
		vecTarget = pTarget->pev->origin;
		vecTarget.x += RANDOM_FLOAT(pTarget->pev->mins.x + flSize, pTarget->pev->maxs.x - flSize);
		vecTarget.y += RANDOM_FLOAT(pTarget->pev->mins.y + flSize, pTarget->pev->maxs.y - flSize);
		vecTarget.z += RANDOM_FLOAT(pTarget->pev->mins.z + flSize, pTarget->pev->maxs.z - flSize);
		UTIL_TraceLine(vecEye, vecTarget, ignore_monsters, ignore_glass, edict(), &tr);
		if (tr.flFraction == 1.0f)
		{
			vecTargetOrigin = vecTarget;
			return true;// line of sight is valid.
		}
	}
	return false;// Line of sight is not established
}

//-----------------------------------------------------------------------------
// Purpose: Returns an integer that describes the relationship between this entity and target.
// Warning: May be called before Spawn()!
// Input  : *pTarget - 
// Output : int R_NO
//-----------------------------------------------------------------------------
int CBaseEntity::IRelationship(CBaseEntity *pTarget)
{
	if (pTarget->IsProjectile())// XDM3038a: special treatment
	{
		if (pTarget->pev->solid == SOLID_NOT)// cannot bite anyway
			return R_NO;

		if (pTarget->pev->movetype == MOVETYPE_NONE)
			return R_NO;
	}
	return g_iRelationshipTable[Classify()][pTarget->Classify()];
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: what force should be applied to this entity upon taking damage
// Input  : &damage - amount (considering it is DMGM_PUSH)
// Output : float - force (scalar velocity)
//-----------------------------------------------------------------------------
float CBaseEntity::DamageForce(const float &damage)
{
	if (!IsPushable())
		return 0.0f;

	if (pev->movetype == MOVETYPE_NONE)
		return 0.0f;

	// Reference volume (player volume) / entity volume
	float force = 2.0f * damage * (HULL_RADIUS*HULL_RADIUS*(HULL_MAX-HULL_MIN) / (pev->size.Volume()));// XDM3035c: tuned

	if (IsMultiplayer() && sv_overdrive.value > 0.0)// XDM3037: lol
		force *= (sv_overdrive.value + 1.0f);

	if (force > 2048.0f)
		force = 2048.0f;

	return force;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: how should shock wave interact with this entity
// Input  : &bitsDamageType - DMG_GENERIC
//			&vecSrc - source point of explosion
// Output : short - DMG_INTERACTION
//-----------------------------------------------------------------------------
short CBaseEntity::DamageInteraction(const int &bitsDamageType, const Vector &vecSrc)
{
	if (FBitSet(bitsDamageType, DMG_RADIATION|DMG_WALLPIERCING))// material??
		return DMGINT_THRU;

	if (IsMonster() || IsPlayer())
	{
		if (FBitSet(bitsDamageType, DMG_BLAST|DMG_SONIC|DMG_NERVEGAS|DMG_RADIATION))
			return DMGINT_THRU;
	}
	/*else if (IsBSPModel())
	{
		if (FBitSet(bitsDamageType, DMG_WALLPIERCING))
			return DMGINT_THRU;
	}*/
	return DMGINT_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: doesn't sound very good, but does its job. Recommended for everyday use.
// Warning: This function is for Attacker only! Not inflictor or owner.
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::GetDamageAttacker(void)
{
	if (m_hOwner.Get())
		return m_hOwner;

	return this;
}

//-----------------------------------------------------------------------------
// Purpose: Sprays blood on nearby walls
// Input  : &flDamage - amount of damage
//			&vecDir - direction at which the attacker was aiming
//			*ptr - trace that hit this entity
//			bitsDamageType - indicates the type of damage sustained, ie: DMG_SHOCK
//-----------------------------------------------------------------------------
void CBaseEntity::TraceBleed(const float &flDamage, const Vector &vecDir, TraceResult *ptr, const int &bitsDamageType)
{
	if (BloodColor() == DONT_BLEED)
		return;

	if (!FBitSet(bitsDamageType, DMGM_BLEED))
		return;

	if (flDamage <= 0.0f)
		return;

	// Already checked in TraceAttack()	if (bitsDamageType & DMG_DONT_BLEED)
	//	return;

	TraceResult Bloodtr;
	Vector vecTraceDir;
	float flNoise = clamp(flDamage, 1, 1000)*0.01f;
	/*int cCount;
	if (flDamage < 10.0f)
	{
		flNoise = 0.1f;
		cCount = 1;
	}
	else if (flDamage < 25.0f)
	{
		flNoise = 0.2f;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}*/
	size_t i, c;
	c = clamp((int)fabs(flDamage*0.1f), 1, 10);
	for (i = 0 ; i < c ; ++i)
	{
		vecTraceDir = -vecDir + RandomVector()*flNoise;// trace in the opposite direction the shot came from (the direction the shot is going)
		UTIL_TraceLine(ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172.0f, ignore_monsters, edict(), &Bloodtr);
		if (Bloodtr.flFraction != 1.0f)
			UTIL_BloodDecalTrace(&Bloodtr, BloodColor());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display damage effects here (but don't decrease health!)
// Input  : *pAttacker - Can be NULL!
//			flDamage - damage amount
//			vecDir - direciton (vecEnd - vecAttackerOrigin), must be normalized!
//			ptr - trace which did the hit
//			bitsDamageType - indicates type of damage inflicted, ie: DMG_CRUSH
//-----------------------------------------------------------------------------
void CBaseEntity::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (pev->takedamage != DAMAGE_NO)
	{
		AddMultiDamage(pAttacker, this, flDamage, bitsDamageType);
		int blood = BloodColor();
		if (blood != DONT_BLEED && !FBitSet(bitsDamageType, DMG_DONT_BLEED|DMG_NEVERGIB))// XDM3038a: faster
		{
			Vector vecOrigin(ptr->vecEndPos);
			vecOrigin -= vecDir * 4.0f;
			UTIL_BloodDrips(vecOrigin, vecDir, blood, flDamage);// a little surface blood.
			TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		}
		g_vecAttackDir = vecDir;// g_vecAttackDir.NormalizeSelf();// XDM3038c
	}
	if (IsPushable() && FBitSet(bitsDamageType, DMGM_PUSH))// XDM3038c
		pev->velocity += vecDir/*.Normalize()*/ * DamageForce(flDamage);
}

//-----------------------------------------------------------------------------
// Purpose: Give some health to this entity
// Input  : &flHealth - 
//			&bitsDamageType - 
// Output : float - actually added value
//-----------------------------------------------------------------------------
float CBaseEntity::TakeHealth(const float &flHealth, const int &bitsDamageType)
{
	if (pev->takedamage == DAMAGE_NO)// ???
		return 0;
	if (!IsAlive())// XDM3038a: don't half-resurrect things
		return 0;
	if (pev->health >= pev->max_health)
		return 0;

	pev->health += flHealth;

	float added;// the health points actually added

	if (pev->health > pev->max_health)
	{
		added = flHealth - (pev->health - pev->max_health);
		pev->health = pev->max_health;
	}
	else
		added = flHealth;

	return added;
}

//-----------------------------------------------------------------------------
// Purpose: Inflict damage on this entity
// UNDONE : return float!
// Input  : *pInflictor - Can be NULL!
//			*pAttacker - Can be NULL!
//			flDamage - damage amount
//			bitsDamageType - indicates type of damage inflicted, ie: DMG_CRUSH
// Output : int - right now it's like BOOL
//-----------------------------------------------------------------------------
int CBaseEntity::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	DBG_PRINT_ENT_TAKEDAMAGE;
	if (pev->takedamage == DAMAGE_NO)
		return 0;

	// UNDONE: some entity types may be immune or resistant to some bitsDamageType
	/*int bitsDamage = FBitSet(bitsDamageType, ~m_bitsDamageImmune);// only those bits must remain which are not in m_bitsDamageImmune
	if (bitsDamage == 0)// no suitable damage type
		return 0;*/

	if (g_vecAttackDir.IsZero())// XDM3038a: HACK. This is like (pInflictor->Center() - this->Center()).Normalize();
	{
		// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
		// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
		if (pInflictor)// an actual missile was involved.
		{
			g_vecAttackDir = Center();/*VecBModelOrigin(pev);*/ g_vecAttackDir -= pInflictor->Center(); g_vecAttackDir.NormalizeSelf();// XDM3038c: reversed
		}
		else if (pAttacker)
		{
			g_vecAttackDir = Center();/*VecBModelOrigin(pev);*/ g_vecAttackDir -= pAttacker->Center(); g_vecAttackDir.NormalizeSelf();// XDM3038c: reversed
		}
	}

	// save damage based on the target's armor level

	// figure momentum add (don't let hurt brushes or other triggers move player)
	// WARNING! RadiusDamage() already added some velocity!!!
	if (FBitSet(bitsDamageType, DMGM_PUSH) && IsPushable() && !g_vecAttackDir.IsZero() && pInflictor && (pInflictor->pev->solid != SOLID_TRIGGER))
	{
		//Vector vecDir((pev->origin - pInflictor->Center()).Normalize());
		//float v = (VEC_HULL_MAX - VEC_HULL_MIN).Volume();// (32 * 32 * 72.0f)
		//float flForce = flDamage * ((32 * 32 * 72.0f) / (pev->size.x * pev->size.y * pev->size.z)) * 5.0f;
		//if (flForce > 1000.0f) 
		//	flForce = 1000.0f;
		if (pev->movetype != MOVETYPE_NONE && pev->movetype != MOVETYPE_NOCLIP && pev->movetype != MOVETYPE_FOLLOW)
		{
			float flForce = DamageForce(flDamage);
			pev->velocity += g_vecAttackDir * flForce;// XDM3038c: reversed
			if (pev->solid != SOLID_TRIGGER && pev->solid != SOLID_BSP && pev->movetype != MOVETYPE_PUSHSTEP)
			{
				pev->origin.z += 1.0f;// hack
				ClearBits(pev->flags, FL_ONGROUND);// hack
			}
		}
	}

	float fTaken = flDamage;
	// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0.0f)
	{
		int iGib = GIB_NORMAL;// XDM
		if (FBitSet(bitsDamageType, DMG_NEVERGIB))
			iGib = GIB_NEVER;
		else if (FBitSet(bitsDamageType, DMG_ALWAYSGIB))
			iGib = GIB_ALWAYS;

		Killed(pInflictor, pAttacker, iGib);
		return 0;// TODO: FIXME: WHY!?
	}
	return ceilf(fTaken);
}

//-----------------------------------------------------------------------------
// Purpose: Entity was killed as a result of TakeDamage() (most likely)
// Input  : *pInflictor - weapon/projectile
//			*pAttacker - weapon owner
//			iGib - GIBMODE
//-----------------------------------------------------------------------------
void CBaseEntity::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)// new
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: Overload
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/* example bool CBaseEntity::ShouldRespawn(void) const
{
	if (FBitSet(pev->spawnflags, SF_NORESPAWN))
		return false;

	return true;
}*/

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: send an entity (monster) into PRONE state. Used by barnacles.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::FBecomeProne(void)
{
	DBG_MON_PRINTF("%s[%d]::FBecomeProne()\n", STRING(pev->classname), entindex());
	if (m_flBurnTime >= gpGlobals->time)
		return false;

	if (FBitSet(pev->flags, FL_ONGROUND))
		ClearBits(pev->flags, FL_ONGROUND);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Has this entity targetname as its target?
// Input  : targetname - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::HasTarget(string_t targetname)
{
	return FStrEq(STRING(targetname), STRING(pev->target));// XDM3035c: target! Not targetname!!!
}

//-----------------------------------------------------------------------------
// Purpose: Just UTIL_FindEntityByTargetname. Not really "next".
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::GetNextTarget(void)
{
	if (FStringNull(pev->target))
		return NULL;

	return UTIL_FindEntityByTargetname(NULL, STRING(pev->target));// XDM3035c
}

//-----------------------------------------------------------------------------
// Purpose: This updates global tables that need to know about entities being removed
// Warning: Do not call this for respawning entities!!
//-----------------------------------------------------------------------------
void CBaseEntity::UpdateOnRemove(void)
{
	DBG_PRINT_ENT_THINK(UpdateOnRemove);
	SetBits(pev->flags, FL_KILLME);// XDM3037: give entities a chance to update client side
	pev->ltime = 0;// XDM3038a
	pev->deadflag = DEAD_DEAD;// XDM3038c
	SendClientData(NULL, MSG_ALL, SCD_ENTREMOVE);
	DontThink();// XDM3038a
	SetThinkNull();// XDM3037
	SetTouchNull();
	SetUseNull();
	SetBlockedNull();

	if (g_pGameRules)
		g_pGameRules->OnEntityRemoved(this);// XDM3038a

	// this entity was a LinkEnt in the world node graph, so we must remove it from
	// the graph since we are removing it from the world.
	if (FBitSet(pev->flags, FL_GRAPHED))
	{
		for (int i = 0 ; i < WorldGraph.m_cLinks ; ++i)
		{
			if (WorldGraph.m_pLinkPool[i].m_pLinkEnt == pev)
			{
				// if this link has a link ent which is the same ent that is removing itself, remove it!
				WorldGraph.m_pLinkPool[i].m_pLinkEnt = NULL;
			}
		}
	}
	if (!FStringNull(pev->globalname))
		gGlobalState.EntitySetState(pev->globalname, GLOBAL_DEAD);

#if !defined(NEW_DLL_FUNCTIONS)
	//if (m_pfnThink != &CBaseEntity::SUB_Respawn)// XDM3038: a lot of memory cleanup is done there!
		OnFreePrivateData();// XDM3035c: since nobody will call this externally in older engine versions
#endif
//#if defined (_DEBUG)
	pev->targetname = MAKE_STRING("???REMOVED???");// XDM3038a: AWESOME debugging idea!!
//#else
//	pev->targetname = iStringNull;// XDM3037
//#endif
	pev->modelindex = 0;// XDM3038c
	pev->model = iStringNull;// XDM3038c
	pev->movetype = MOVETYPE_NONE;// XDM3038c
	pev->solid = SOLID_NOT;// XDM3038b
	pev->target = iStringNull;// XDM3038a
	pev->absmin.Clear();// XDM3038b
	pev->absmax.Clear();// XDM3038b
	m_iExistenceState = ENTITY_EXSTATE_VOID;// XDM3038
}

//-----------------------------------------------------------------------------
// Purpose: THINK Convenient way to delay removing oneself. Calls Destroy()
// Warning: It is ONLY safe to call this by SetThink()! NEVER CALL IT DIRECTLY!
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_Remove(void)
{
	DBG_PRINT_ENT_THINK(SUB_Remove);
	Destroy();// XDM3038c
}

//-----------------------------------------------------------------------------
// Purpose: THINK Convenient way to explicitly do nothing (passed to functions that require a method)
// Note:    May be used in cases when entity is required to keep thinking
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_DoNothing(void)
{
	DBG_PRINT_ENT_THINK(SUB_DoNothing);
	SetNextThink(2.0f);
}

//-----------------------------------------------------------------------------
// Purpose: THINK Slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER!
// SET A FUTURE THINK AND A RENDERMODE!!
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_StartFadeOut(void)
{
	DBG_PRINT_ENT_THINK(SUB_StartFadeOut);
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}
	SetThinkNull();// XDM3038a
	SetTouchNull();
	SetUseNull();
	SetBlockedNull();
	//no!	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	//?	pev->avelocity.Clear();
	SetNextThink(0.1f);
	SetThink(&CBaseEntity::SUB_FadeOut);
}

//-----------------------------------------------------------------------------
// Purpose: THINK Fade process
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_FadeOut(void)
{
	DBG_PRINT_ENT_THINK(SUB_FadeOut);
	if (pev->renderamt > 8.0f)
	{
		pev->renderamt -= 8.0f;
		SetNextThink(0.1f);
	}
	else
	{
		pev->renderamt = 0.0f;
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.2f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tried to make HL2-style disintegration effect
// Warning: REQUIRES that class that use it DOES NOT have Think() override!
// Warning: Do NOT call this from Destroy()!
// UNDONE: TODO: Move to client (remove real entity and create client-side copy) and draw some fancy effects!
// Note   : uses mins/maxs
//-----------------------------------------------------------------------------
void CBaseEntity::Disintegrate(void)
{
	DBG_PRINT_ENT_THINK(Disintegrate);
	pev->origin.z += 0.5f;// HACK
	pev->velocity.Set(0.0f,0.0f,4.0f);
	pev->angles.x += 2.0f;
	pev->angles.y += RANDOM_FLOAT(-2,2);
	pev->avelocity = RandomVector(6.0f);
	pev->punchangle.Set(RANDOM_INT2(-90,90), RANDOM_INT2(-90,90), RANDOM_INT2(-90,90));// XDM3038a
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->gravity = 0.0f;
	pev->friction = 0.0f;
	pev->framerate *= 0.25f;// slow down movements
	pev->health = 0;
	SetBits(pev->flags, FL_FLY);
	SetThinkNull();// XDM3038a
	SetTouchNull();
	SetUseNull();
	SetBlockedNull();
	//m_iExistenceState = ENTITY_EXSTATE_VOID;// NO! Must still be sent to clients!

	//if (g_pGameRules->FAllowEffects())// Disintegration effect allowed?
	//{
		SetBits(pev->effects, EF_MUZZLEFLASH);
		pev->rendermode = kRenderTransTexture;
		pev->renderfx = kRenderFxDisintegrate;
		pev->rendercolor.Set(127,127,127);
		pev->renderamt = 160;
		SetThink(&CBaseEntity::SUB_Disintegrate);
		ParticleBurst(Center(), __min(80, __max(4, fabs((pev->maxs - pev->mins).Length()*0.5f))), 5, 10);
	/*}
	else
	{
		//pev->health = 0;
		Destroy();
		return;
	}*/
	SetNextThink(0);
}

#define DISINTEGRATION_RATE		1// how many updates per 100 msec, mostly for testing because too much overloads the net

//-----------------------------------------------------------------------------
// Purpose: THINK function, calls SUB_Remove in the end.
// Warning: Don't use this directly! Call Disintegrate()!
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_Disintegrate(void)
{
	DBG_PRINT_ENT_THINK(SUB_Disintegrate);
	if (pev->renderamt >= 8/DISINTEGRATION_RATE)
	{
		pev->origin.z += 0.2f/DISINTEGRATION_RATE;// 0.1f if renderamt -= 4
		pev->renderamt -= 8/DISINTEGRATION_RATE;
		SetNextThink(0.1f/DISINTEGRATION_RATE);
		if (g_pGameRules == NULL || g_pGameRules->FAllowEffects())
		{
			pev->scale += 0.02f/DISINTEGRATION_RATE;
			if (RANDOM_LONG(0,3) == 0)
			{
				SetBits(pev->effects, EF_MUZZLEFLASH);
				UTIL_Sparks(pev->origin);
			}
		}
	}
	else// time is up, reset&remove
	{
		pev->angles.x = 0.0f;// don't really need this?
		pev->angles.y = 0.0f;
		pev->movetype = MOVETYPE_NONE;
		SetBits(pev->effects, EF_NODRAW);
		ClearBits(pev->flags, FL_FLY);
		pev->scale = 1.0f;
		pev->renderamt = 0;
		//pev->rendermode = kRenderNormal;
		pev->rendercolor.Clear();
		//pev->avelocity.Clear();
		pev->framerate *= 4.0f;// restore
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.1f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Delay call to Use() using pev->nextthink
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_CallUseToggle(void)
{
	DBG_PRINT_ENT_THINK(SUB_CallUseToggle);
	Use(this, this, USE_TOGGLE, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Respawn. Used by monsters in multiplayer.
// Note   : Disables thinking, nodraw, calls Spawn(FALSE), and, if succeeds (no FL_KILLME), OnRespawn()
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_Respawn(void)
{
	DBG_PRINT_ENT_THINK(SUB_Respawn);
	SetBits(pev->effects, EF_NOINTERP);//EF_NODRAW|
	SetThinkNull();
	DontThink();// XDM3038a
	if (FBitSet(pev->flags, FL_KILLME))
	{
		conprintf(1, "%s[%d]::SUB_Respawn() error: marked for removal!\n", STRING(pev->classname), entindex());
		return;
	}
	pev->avelocity.Clear();
	//pev->angles.x = 0.0f;// prevent mosters from respawning upside down
	//pev->angles.y = 0.0f;
	pev->angles.Clear();
	pev->punchangle.Clear();// XDM3035b
	pev->idealpitch = 0.0f;
	pev->ideal_yaw = 0.0f;
	//pev->yaw_speed = 0.0f;
	if (pev->health < 0.0f)
		pev->health = 0.0f;// XDM3038a: class spawn code must reset this to default!

	if (IsMultiplayer())// HACK TODO: what if this entity had some effect??
	{
		pev->rendermode = kRenderNormal;
		pev->renderamt = 255;
		pev->renderfx = kRenderFxNone;
		if (IsMonster() || IsBSPModel())// Don't do checks for items, etc.
		{
			if (!SpawnPointCheckObstacles(this, m_vecSpawnSpot, true, IsMultiplayer()&&(mp_spawnkill.value > 0.0f)))// XDM3038b: monsters have only one "respawn" spot
				conprintf(2, "%s[%d]::SUB_Respawn() warning: SpawnPointCheckObstacles() failed!\n", STRING(pev->classname), entindex());
		}
	}

	ClearBits(pev->flags, FL_ONGROUND|FL_PARTIALGROUND);// XDM3038b: prevent SV_MoveToOrigin crash
	//pev->startpos = pev->origin;
	pev->origin = m_vecSpawnSpot;
	UTIL_SetOrigin(this, pev->origin);

	ClearBits(pev->effects, EF_NODRAW);
	//GET_SPAWN_PARMS(edict()); obsolete, not in engine

	m_iExistenceState = ENTITY_EXSTATE_WORLD;
	Spawn(FALSE);// Spawn(), Precache(), Materialize()

	if (!FStringNull(pev->globalname))// XDM3038: reverse UTIL_Remove
		gGlobalState.EntitySetState(pev->globalname, GLOBAL_ON);

	if (!FBitSet(pev->flags, FL_KILLME))
	{
		//m_iRespawnTimes++;
		OnRespawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Simple, non-delayed version
// Input  : *pActivator - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT("SUB_UseTargets");
	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), pActivator, this, useType, value);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Subclasses must call parent's function first!
// Warning: Each subclass should first call MyParent::ReportState()
// Warning: Only works and prints data on server!
// Input  : printlevel - minimum "developer" value at which the message is shown
//-----------------------------------------------------------------------------
void CBaseEntity::ReportState(int printlevel)
{
	DBG_PRINT_ENT_THINK(ReportState);
	conprintf(printlevel, "---------------- reporting state ----------------\n%d: \"%s\", %s, %s, exstate: %u\n targetname: \"%s\", globalname: \"%s\", target: \"%s\", netname: \"%s\"\n model: \"%s\", rmode: %d, rfx: %d, color: (%g %g %g), amt: %g\n health: %g/%g, origin: (%g %g %g), angles: (%g %g %g),\n",
		entindex(), STRING(pev->classname), FBitSet(pev->spawnflags, SF_NOREFERENCE)?"noref":"ref", IsDormant()?"dormant":"nondormant", GetExistenceState(),
		STRING(pev->targetname), STRING(pev->globalname), STRING(pev->target), STRING(pev->netname),
		STRING(pev->model), pev->rendermode, pev->renderfx, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt,
		pev->health, pev->max_health, pev->origin.x, pev->origin.y, pev->origin.z, pev->angles.x, pev->angles.y, pev->angles.z);
}
