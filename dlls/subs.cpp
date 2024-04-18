/*
===== subs.cpp ========================================================
  hacks nobody cares about
*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "doors.h"
#include "game.h"
#include "shared_resources.h"


//-----------------------------------------------------------------------------
// Purpose: Returns constant string for USE_TYPE (for debugging purposes)
// Input  : useType -
// Output : const char *
//-----------------------------------------------------------------------------
const char *GetStringForUseType(USE_TYPE useType)
{
	switch (useType)
	{
	case USE_OFF: return "USE_OFF";
	case USE_ON: return "USE_ON";
	case USE_SET: return "USE_SET";
	case USE_TOGGLE: return "USE_TOGGLE";
	case USE_KILL: return "USE_KILL";
	case USE_SAME: return "USE_SAME";
	case USE_NOT: return "USE_NOT";
	default: return "USE_UNKNOWN";
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns constant string for USE_TYPE (for debugging purposes)
// Input  : state - 
// Output : const char *
//-----------------------------------------------------------------------------
const char *GetStringForState(STATE state)
{
	switch (state)
	{
	case STATE_ON: return "ON";
	case STATE_OFF: return "OFF";
	case STATE_TURN_ON: return "TURN ON";
	case STATE_TURN_OFF: return "TURN OFF";
	case STATE_IN_USE: return "IN USE";
	//case STATE_DEAD: return "DEAD";
	default: return "UNKNOWN";
	}
}

//-----------------------------------------------------------------------------
// Purpose: One of shitty SHL things
// Input  : *string - 
// Output : STATE
//-----------------------------------------------------------------------------
STATE GetStateForString(const char *string)
{
	if (!_stricmp(string, "ON"))
		return STATE_ON;
	else if (!_stricmp(string, "OFF"))
		return STATE_OFF;
	else if (!_stricmp(string, "TURN ON"))
		return STATE_TURN_ON;
	else if (!_stricmp(string, "TURN OFF"))
		return STATE_TURN_OFF;
	else if (!_stricmp(string, "IN USE"))
		return STATE_IN_USE;
	//else if (!_stricmp(string, "DEAD"))
	//	return STATE_DEAD;
	else if(isdigit(string[0]))
		return (STATE)atoi(string);

	// assume error
#if defined(_DEBUG)
	conprintf(0, "GetStateForString(%s) ERROR: unknown state!\n", string);
#endif
	return (STATE)-1;
}

//-----------------------------------------------------------------------------
// Purpose: This is one of the fundamental things in HL. This method is used
// UNDONE : "name1;name2" - use many targets at once
// by all entities that can activate ("fire") something by targetname.
// Input  : *pTargetName - other pev->targetname to search by OR a special command
//			*pActivator - person who started the chain (for CBaseEntity::Use())
//			*pCaller - exact entity that fires the target
//			useType - USE_TYPE
//			value - float
//-----------------------------------------------------------------------------
unsigned int FireTargets(const char *pTargetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value, fnEntityUseCallback Callback)
{
	if (pTargetName == NULL)
		return 0;

	size_t argc = 0;
	const char *args[32];// pointers to strings
	const char *pProcessedTargetName = pTargetName;// target name will be modified
	bool bCommand = false;
	// SHL: force some use types
	if (pTargetName[0] == '+')
	{
		pProcessedTargetName++;
		useType = USE_ON;
	}
	else if (pTargetName[0] == '-')
	{
		pProcessedTargetName++;
		useType = USE_OFF;
	}
 	else if (pTargetName[0] == '<')
	{
		pProcessedTargetName++;
		useType = USE_SET;
	}
	/*else if (pTargetName[0] == '>')
	{
		pProcessedTargetName++;
		useType = USE_RESET;
	}
	else if (pTargetName[0] == '!')
	{
		pProcessedTargetName++;
		useType = USE_REMOVE;USE_TOGGLE?
	}*/
	else if (pTargetName[0] == TARGET_OPERATOR_CHAR)// XDM3038c: special command "#mytarget set rendercolor \"0 255 255\""
	{
		pProcessedTargetName++;
		char cmdbuffer[512];// WARNING: this will not work as a string after this! strlen() won't work!
		strncpy(cmdbuffer, pProcessedTargetName, 512);
		cmdbuffer[511] = 0;
		size_t i = 0;
		bool bGotSpace = true;// pretend string started after space
		bool bInQuotes = false;
		while (cmdbuffer[i] != 0)
		{
			if (cmdbuffer[i] == '\"' || cmdbuffer[i] == '\'')
			{
				if (bInQuotes)// we were inside quoted argument
				{
					cmdbuffer[i] = 0;// terminate
					bInQuotes = false;
				}
				else
				{
					cmdbuffer[i] = 0;
					++i;
					args[argc] = &cmdbuffer[i];// remember this as the start of an argument
					++argc;
					bInQuotes = true;
				}
			}
			else if (!bInQuotes)
			{
				/*else */if (isspace(cmdbuffer[i]))
				{
					//if (!bInQuotes)
					{
						bGotSpace = true;
						cmdbuffer[i] = 0;
					}
				}
				else
				{
					//if (!bInQuotes)
					{
						if (bGotSpace)// previously was a space character
						{
							args[argc] = &cmdbuffer[i];// remember this as the start of an argument
							++argc;
						}
						bGotSpace = false;
					}
				}
			}
			++i;
		}
		args[argc] = NULL;// terminate
		pProcessedTargetName = args[0];// point to the entity name, not the entire command string
		// strictly! if (argc > 1)// if there were actually some arguments specified
			bCommand = true;
	}

//#if defined (_DEBUG)
	conprintf(2, "FireTargets(tgt \"%s\", acr %s[%d] \"%s\", clr %s[%d] \"%s\", ut %d (%s), val %g)\n", pTargetName,
		pActivator?STRING(pActivator->pev->classname):NULL, pActivator?pActivator->entindex():0, pActivator?STRING(pActivator->pev->targetname):NULL,
		pCaller?STRING(pCaller->pev->classname):NULL, pCaller?pCaller->entindex():0, pCaller?STRING(pCaller->pev->targetname):NULL,
		useType, GetStringForUseType(useType), value);
//#endif

	size_t n = 0;
	CBaseEntity *pTarget = NULL;
	bool bUse;
	while ((pTarget = UTIL_FindEntityByTargetname(pTarget, pProcessedTargetName)) != NULL)
	{
		if (pTarget == pCaller)
		{
			conprintf(2, " FireTargets(\"%s\"): WARNING! found self in activation list!\n", pProcessedTargetName);
				//continue;// XDM3037: WARNING! NOTE: this may cause some things to not to work
		}
		if (Callback)
			bUse = Callback(pTarget, pActivator, pCaller);
		else
			bUse = true;

		if (bUse)
		{
			if (bCommand)
			{
				conprintf(2, " Executing command: %s[%d]\n", STRING(pTarget->pev->classname), pTarget->entindex());
				Cmd_EntityAction(1, argc, args, pTarget, pActivator, false);
			}
			else
			{
				conprintf(2, " Firing: %s[%d]\n", STRING(pTarget->pev->classname), pTarget->entindex());
				pTarget->Use(pActivator, pCaller, useType, value);
			}
		}
		else
			conprintf(2, " Ignoring: %s[%d] (denied by callback)\n", STRING(pTarget->pev->classname), pTarget->entindex());

		++n;
	}
	if (n == 0)
		conprintf(2, "FireTargets(\"%s\"): nothing found.\n", pProcessedTargetName);
	else
		DBG_PRINTF("FireTargets(\"%s\"): %u targets fired\n", pProcessedTargetName, n);

	return n;
}

//-----------------------------------------------------------------------------
// Purpose: Create a temporary DelayedUse to fire at a later time
// WARNING! Lots of hacks here!
// Input  : iszTarget - 
//			iszKillTarget -
//			delay - 
//			*pActivator - person who started the chain (for CBaseEntity::Use())
//			*pCaller - exact entity that fires the target
//			useType - USE_TYPE
//			value -
//-----------------------------------------------------------------------------
void FireTargetsDelayed(string_t iszTarget, string_t iszKillTarget, float delay, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(iszTarget) && FStringNull(iszKillTarget))
		return;
//#if defined (_DEBUG)
	conprintf(2, "FireTargetsDelayed(tgt \"%s\", kt \"%s\", dly %g, acr %s[%d] \"%s\", clr %s[%d] \"%s\", ut %d (%s), val %g)\n", STRING(iszTarget), STRING(iszKillTarget), delay,
		pActivator?STRING(pActivator->pev->classname):NULL, pActivator?pActivator->entindex():0, pActivator?STRING(pActivator->pev->targetname):NULL,
		pCaller?STRING(pCaller->pev->classname):NULL, pCaller?pCaller->entindex():0, pCaller?STRING(pCaller->pev->targetname):NULL,
		useType, GetStringForUseType(useType), value);
//#endif

	CBaseDelay *pTemp = GetClassPtr((CBaseDelay *)NULL, "DelayedUse");// XDM
	if (pTemp == NULL)
	{
		conprintf(0, "FireTargetsDelayed() ERROR: failed to allocate DelayedUse!\n");
		return;
	}
	/*if (pCaller == NULL)
	{
		conprintf(1, "FireTargetsDelayed() ERROR: pCaller can not be NULL!\n");
		return;
	}*/
/*#if defined (_DEBUG)
	char tn[64];
	_snprintf(tn, "@%s\0", STRING(pev->targetname));
	pTemp->pev->targetname = ALLOC_STRING(tn);// XDM3037: to track history and NOT be usable
#endif*/
	if (pCaller)
		pTemp->pev->targetname = pCaller->pev->targetname;// XDM3037: this doesn't have any Use() method anyway

	pTemp->SetNextThink(delay);
	pTemp->SetThink(&CBaseDelay::DelayThink);
	// Save the useType
	pTemp->pev->button = (int)useType;
	pTemp->pev->armorvalue = value;// XDM3035c
	pTemp->m_iszKillTarget = iszKillTarget;
	pTemp->m_flDelay = 0; // prevent "recursion"
	pTemp->pev->target = iszTarget;
	pTemp->m_hOwner = pCaller;// XDM3038
	pTemp->m_hTBDAttacker = pCaller;// XDM3038
	if (pActivator)// XDM
	{
		pTemp->m_hActivator = pActivator;
		pTemp->pev->owner = pActivator->edict();//old
	}
	else
	{
		pTemp->m_hActivator = NULL;
		pTemp->pev->owner = NULL;//old
	}
	//pTemp->m_pGoalEnt = this;// XDM3035c: 20121121 HACK to remember the REAL caller (this). WARNING! This may actually be removed long before m_flDelay in which case m_pGoalEnt will become invalid!
}




//-----------------------------------------------------------------------------
// Purpose: Common Spawn function for Landmark class
//-----------------------------------------------------------------------------
void CPointEntity::Spawn(void)
{
	CBaseEntity::Spawn();//Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->frame = 0;
	// better not pev->model = iStringNull;
	if (sv_showpointentities.value > 0)
	{
		if (pev->modelindex == 0)// XDM3038c
			pev->modelindex = g_iModelIndexTestSphere;

		pev->effects = EF_NOINTERP;
	}
	else
	{
		// NO! Some entities use it!	pev->modelindex = 0;	
		pev->effects = EF_NODRAW;
	}
	UTIL_SetSize(this, g_vecZero, g_vecZero);
}






// Should never be palced by mapper!!
LINK_ENTITY_TO_CLASS(DelayedUse, CBaseDelay);

// Global Savedata for Delay
TYPEDESCRIPTION	CBaseDelay::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDelay, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseDelay, m_iszKillTarget, FIELD_STRING ),
	DEFINE_FIELD( CBaseDelay, m_iszMaster, FIELD_STRING ),
	DEFINE_FIELD( CBaseDelay, m_hActivator, FIELD_EHANDLE ),// XDM3035
	DEFINE_FIELD( CBaseDelay, m_iState, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CBaseDelay, CBaseEntity);

//-----------------------------------------------------------------------------
// Purpose: XDM3035c: the only way to pre-set variables before KeyValue() takes place
//-----------------------------------------------------------------------------
CBaseDelay::CBaseDelay() : CBaseEntity()
{
	m_iState = STATE_ON;// COMPATIBILITY: well, in HL entities are mostly ON unless _START_OFF flag is specified for them
}

//-----------------------------------------------------------------------------
// Purpose: delay, state, master, killtarget set here
//-----------------------------------------------------------------------------
void CBaseDelay::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "delay"))
	{
		m_flDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "killtarget"))
	{
		m_iszKillTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_iszMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "state"))// XDM3035c
	{
		m_iState = (STATE)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: Derived classes must call it!
//-----------------------------------------------------------------------------
void CBaseDelay::Spawn(void)
{
	CBaseEntity::Spawn();// <- Precache <- Materialize
	// just thoughts	if (m_iState != STATE_OFF) Activate();
	//	DO NOT SetUse()! This entity should NOT be usable!
	// TEST	pev->button = USE_TOGGLE;
}

//-----------------------------------------------------------------------------
// Purpose: Use my pev->target
//
// Search for (string)targetname in all entities that match (string)self.target
// and call their Use function.
//
// Remove all entities with a (string)targetname that match (string)iszKillTarget,
// so some events can remove other triggers.
//
// Warning: If m_flDelay is set, a DelayedUse entity will be created that will actually
// do the SUB_UseTargets after that many seconds have passed.
//
// Input  : *pActivator - person who started the chain (for CBaseEntity::Use())
//			useType - USE_TYPE
//			value -
//-----------------------------------------------------------------------------
void CBaseDelay::SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value)
{
	UseTargets(pev->target, m_iszKillTarget, pActivator, this, useType, value);// XDM3038c: customization layer
}

//-----------------------------------------------------------------------------
// Purpose: Customization layer, includes custom target and killtarget
// UNDONE : Multiple targets
// Input  : iszTarget - 
//			iszKillTarget -
//			*pActivator - person who started the chain (for CBaseEntity::Use())
//			*pCaller - exact entity that fires the target
//			useType - USE_TYPE
//			value -
//-----------------------------------------------------------------------------
void CBaseDelay::UseTargets(string_t iszTarget, string_t iszKillTarget, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(iszTarget) && FStringNull(iszKillTarget))// entities tend to call this without checking
		return;
//#if defined (_DEBUG)
	conprintf(2, "%s[%d]\"%s\": UseTargets(tgt \"%s\", kt \"%s\", dly %g, acr %s[%d] \"%s\", clr %s[%d] \"%s\", ut %d (%s), val %g)\n", ENTFMT_P, STRING(iszTarget), STRING(iszKillTarget), m_flDelay,
		pActivator?STRING(pActivator->pev->classname):NULL, pActivator?pActivator->entindex():0, pActivator?STRING(pActivator->pev->targetname):NULL,
		pCaller?STRING(pCaller->pev->classname):NULL, pCaller?pCaller->entindex():0, pCaller?STRING(pCaller->pev->targetname):NULL,
		useType, GetStringForUseType(useType), value);
//#endif

	if (m_flDelay > 0.0f)
	{
		FireTargetsDelayed(iszTarget, iszKillTarget, m_flDelay, pActivator, pCaller, useType, value);// XDM3038a
	}
	else
	{
		// fire targets. WARNING! Some mappers set KillTarget to self!!! (test map c4a1 3 buttons)
		// NO! CBaseEntity::SUB_UseTargets(pActivator, useType, value);// XDM3037: OOP design !!! CBaseEntity:: IMPORTANT!!!
		FireTargets(STRING(iszTarget), pActivator, pCaller, useType, value);
		// kill the killtargets
		if (!FStringNull(iszKillTarget))// UNDONE: TODO: multiple killtargets "targ1;targ2;targN" BUT! We need to sort them and if there is 'this', it must be last!
		{
			conprintf(2, "%s[%d] %s: KillTarget: %s\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(iszKillTarget));
			size_t n = 0;
			CBaseEntity *pKillTarget = NULL;
			/* WARNING: Cannot use because CBaseEntity pointers are invalidated!
			while ((pKillTarget = UTIL_FindEntityByTargetname(pKillTarget, STRING(iszKillTarget))) != NULL)
			{
				if (pKillTarget == this)
				{
					conprintf(2, " KillTarget(%s): WARNING! found self in deletion list!\n", STRING(iszKillTarget));
					//continue;// XDM3038a: WARNING! NOTE: this may cause some things to not to work
				}
				conprintf(2, " Deleting: %d %s %s\n", pKillTarget->entindex(), STRING(pKillTarget->pev->classname), STRING(pKillTarget->pev->targetname));
				UTIL_Remove(pKillTarget);
				++n;
			}*/
			edict_t *pentKillTarget = NULL;
			pentKillTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(iszKillTarget));
			bool bSelfDestruct = false;
			while (!FNullEnt(pentKillTarget))
			{
				pKillTarget = CBaseEntity::Instance(pentKillTarget);
				if (pKillTarget == this)// XDM3038c: new algorithm
				{
					if (!bSelfDestruct)
					{
						conprintf(2, " KillTarget(%s): WARNING! found self in deletion list!\n", STRING(iszKillTarget));
						bSelfDestruct = true;
					}
				}
				else if (pKillTarget->IsRemoving())
				{
					DBG_PRINTF(" Already removed: %d %s %s\n", pKillTarget->entindex(), STRING(pKillTarget->pev->classname), STRING(pKillTarget->pev->targetname));
				}
				else
				{
					conprintf(2, " Deleting: %d %s %s\n", pKillTarget->entindex(), STRING(pKillTarget->pev->classname), STRING(pKillTarget->pev->targetname));
					UTIL_Remove(pKillTarget);
					++n;
				}
				pentKillTarget = FIND_ENTITY_BY_TARGETNAME(pentKillTarget, STRING(iszKillTarget));
			}
			if (bSelfDestruct)
			{
				pKillTarget = this;
				conprintf(2, " Deleting self: %d %s %s\n", pKillTarget->entindex(), STRING(pKillTarget->pev->classname), STRING(pKillTarget->pev->targetname));
				UTIL_Remove(pKillTarget);
				++n;
			}
			if (n == 0)
				conprintf(2, "KillTarget(%s) warning: nothing found!\n", STRING(iszKillTarget));
#if defined (_DEBUG)
			else
				DBG_PRINTF(" - %u targets deleted\n", n);
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: USE THIS to change state! (m_iState made private on purpose)
//-----------------------------------------------------------------------------
void CBaseDelay::SetState(STATE newstate)
{
	if (m_iState != newstate)//&& CanChangeState(newstate))
	{
		STATE oldstate = m_iState;
		m_iState = newstate;// must be set before calling OnStateChange(), so user may use it for reference
		OnStateChange(oldstate);// must be overloaded
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overloadable. Called when state changes.
//-----------------------------------------------------------------------------
void CBaseDelay::OnStateChange(STATE oldstate)
{
#if defined (_DEBUG)
	if (pev)// may get called from the constructor!!!
		conprintf(2, "%s[%d] %s changed state from %s to %s\n", STRING(pev->classname), entindex(), STRING(pev->targetname), GetStringForState(oldstate), GetStringForState(m_iState));
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Checks if our master is triggered
// Input  : *pActivator - XDM3038a: now mandatory (something may not activate without it), m_hActivator will be used if NULL.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseDelay::IsLockedByMaster(const CBaseEntity *pActivator)
{
	return (!FStringNull(m_iszMaster) && !UTIL_IsMasterTriggered(STRING(m_iszMaster), pActivator?pActivator:(CBaseEntity *)m_hActivator));
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseDelay::IsTriggered(const CBaseEntity *pActivator)
{
	if (m_iState == STATE_ON)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseDelay::GetDamageAttacker(void)
{
	CBaseEntity *pAttacher = CBaseEntity::GetDamageAttacker();
	if (pAttacher == this && m_hActivator.Get())// CBaseEntity returned no real attacker
		pAttacher = m_hActivator;

	return pAttacher;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CBaseDelay::ReportState(int printlevel)
{
	CBaseEntity::ReportState(printlevel);
	conprintf(printlevel, "State: %d (%s), Delay: %g, KillTarget: %s, Master: %s, Activator: %d\n", GetState(), GetStringForState(GetState()), m_flDelay, STRING(m_iszKillTarget), STRING(m_iszMaster), m_hActivator.Get()?m_hActivator->entindex():0);
}

//-----------------------------------------------------------------------------
// Purpose: Time to fire my targets
//-----------------------------------------------------------------------------
void CBaseDelay::DelayThink(void)
{
	SUB_UseTargets(m_hActivator, (USE_TYPE)pev->button, pev->armorvalue);// XDM3035c: don't FireTargets() because we may have m_iszKillTarget and stuff
	REMOVE_ENTITY(edict());
}





// Global Savedata for Toggle
TYPEDESCRIPTION	CBaseToggle::m_SaveData[] =
{
	DEFINE_FIELD( CBaseToggle, m_toggle_state, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseToggle, m_flActivateFinished, FIELD_TIME ),
	DEFINE_FIELD( CBaseToggle, m_flMoveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flLip, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flTWidth, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flTLength, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_vecPosition1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecPosition2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecAngle1, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( CBaseToggle, m_vecAngle2, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( CBaseToggle, m_cTriggersLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseToggle, m_flHeight, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_pfnCallWhenMoveDone, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseToggle, m_vecFinalDest, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecFinalAngle, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_bitsDamageInflict, FIELD_INTEGER ),	// damage type inflicted
};

IMPLEMENT_SAVERESTORE(CBaseToggle, CBaseAnimating);

//-----------------------------------------------------------------------------
// Purpose: values for togglable and moving things
//-----------------------------------------------------------------------------
void CBaseToggle::KeyValue(KeyValueData *pkvd)
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
	else if (FStrEq(pkvd->szKeyName, "distance"))
	{
		m_flMoveDistance = atof(pkvd->szValue);
		// XDM3038c: allow destination points to be updated during run-time // TESTME: SF_DOOR_START_OPEN
		m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x-2)) + fabs(pev->movedir.y * (pev->size.y-2)) + fabs(pev->movedir.z * (pev->size.z-2)) - m_flLip));
		m_vecAngle2	= pev->angles + pev->movedir * m_flMoveDistance;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);// XDM3038c: FIXED
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CBaseToggle::ReportState(int printlevel)
{
	CBaseAnimating::ReportState(printlevel);
	conprintf(printlevel, "toggle_state: %d\n", GetToggleState());
}

//-----------------------------------------------------------------------------
// Purpose: Calculate pev->velocity and pev->nextthink to reach vecDest from pev->origin traveling at flSpeed
// Input  : vecDest - 
//			flSpeed -
//-----------------------------------------------------------------------------
void CBaseToggle::LinearMove(const Vector &vecDest, const float &flSpeed)
{
	//conprintf(1, "LinearMove(%f)\n", flSpeed);
	ASSERTSZ(flSpeed != 0, "LinearMove: no speed is defined!");
	//ASSERTSZ(m_pfnCallWhenMoveDone != NULL, "LinearMove: no post-move function defined");

	// Already there?
	if (vecDest == pev->origin)
	{
		m_vecFinalDest = vecDest;
		LinearMoveDone();
		return;
	}
	if (flSpeed == 0.0f)// XDM3038c
		return;

	m_vecFinalDest = vecDest;
	// set destdelta to the vector needed to move
	Vector vecDestDelta(vecDest);
	vecDestDelta -= pev->origin;

	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink(&CBaseToggle::LinearMoveDone);

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
	//conprintf(1, "LinearMove(%f, %f, %f)\n", pev->velocity.x, pev->velocity.y, pev->velocity.z);
}

//-----------------------------------------------------------------------------
// Purpose: Another hack
// After moving, set origin to exact final destination, call "move done" function
//-----------------------------------------------------------------------------
void CBaseToggle::LinearMoveDone(void)
{
	//conprintf(1, "LinearMoveDone()\n");
	Vector delta(m_vecFinalDest);
	delta -= pev->origin;// HL20130901
	vec_t error = delta.Length();
	if (error > 0.03125)
	{
		LinearMove(m_vecFinalDest, 100);
		return;
	}

	UTIL_SetOrigin(this, m_vecFinalDest);
	pev->velocity.Clear();
	pev->nextthink = -1;
	SetThinkNull();// XDM3035a: this prevents double call of LinearMoveDone() 

	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}

//-----------------------------------------------------------------------------
// Purpose: Calculate pev->avelocity and pev->nextthink to reach vecDest from pev->origin traveling at flSpeed (like LinearMove)
// Input  : vecDestAngles - 
//			flSpeed -
//-----------------------------------------------------------------------------
void CBaseToggle::AngularMove(const Vector &vecDestAngles, const float &flSpeed)
{
	//conprintf(1, "AngularMove(%f)\n", flSpeed);
	ASSERTSZ(flSpeed != 0, "AngularMove: no speed is defined!");
	//ASSERTSZ(m_pfnCallWhenMoveDone != NULL, "AngularMove: no post-move function defined");

	// Already there?
	if (vecDestAngles == pev->angles)
	{
		m_vecFinalAngle = vecDestAngles;
		AngularMoveDone();
		return;
	}

	if (flSpeed == 0.0f)// XDM3038c
		return;

	m_vecFinalAngle = vecDestAngles;
	// set destdelta to the vector needed to move
	Vector vecDestDelta(vecDestAngles);
	vecDestDelta -= pev->angles;

	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to AngularMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink(&CBaseToggle::AngularMoveDone);

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->avelocity = vecDestDelta / flTravelTime;
}

//-----------------------------------------------------------------------------
// Purpose: another hack
// After rotating, set angle to exact final angle, call "move done" function
//-----------------------------------------------------------------------------
void CBaseToggle::AngularMoveDone(void)
{
	//conprintf(1, "AngularMoveDone()\n");
	pev->angles = m_vecFinalAngle;
	pev->avelocity.Clear();
	pev->nextthink = -1;
	SetThinkNull();// XDM3035a

	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}

//-----------------------------------------------------------------------------
// Purpose: another hack
//-----------------------------------------------------------------------------
float CBaseToggle::AxisValue(int flags, const Vector &angles)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angles.z;
	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angles.x;

	return angles.y;
}

//-----------------------------------------------------------------------------
// Purpose: another hack
//-----------------------------------------------------------------------------
void CBaseToggle::AxisDir(void)
{
	if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_Z))
		pev->movedir.Set(0, 0, 1);	// around z-axis
	else if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_X))
		pev->movedir.Set(1, 0, 0);	// around x-axis
	else// SF_DOOR_ROTATE_Y is 0 and is default
		pev->movedir.Set(0, 1, 0);	// around y-axis
}

//-----------------------------------------------------------------------------
// Purpose: another hack
//-----------------------------------------------------------------------------
float CBaseToggle::AxisDelta(int flags, const Vector &angle1, const Vector &angle2)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angle1.z - angle2.z;

	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angle1.x - angle2.x;

	// SF_DOOR_ROTATE_Y is 0 and is default
	return angle1.y - angle2.y;
}

/*STATE CBaseToggle::GetState(void)// XDM
{
	switch (m_toggle_state)
	{
		case TS_AT_TOP:		return STATE_ON;
		case TS_AT_BOTTOM:	return STATE_OFF;
		case TS_GOING_UP:	return STATE_TURN_ON;
		case TS_GOING_DOWN:	return STATE_TURN_OFF;
		default:			return STATE_OFF; // This should never happen.
	}
};*/
