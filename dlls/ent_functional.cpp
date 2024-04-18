#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "ent_functional.h"
#include "doors.h"
#include "sound.h"
#include "weapons.h"
#include "globals.h"
#include "effects.h"
#include "game.h"
#include "skill.h"
#if defined(MOVEWITH)
#include "movewith.h"
#endif

extern bool gTouchDisabled;

LINK_ENTITY_TO_CLASS(info_teleport_destination, CPointEntity);
// Lightning target, just alias landmark
LINK_ENTITY_TO_CLASS(info_target, CPointEntity);


LINK_ENTITY_TO_CLASS(multisource, CMultiSource);

TYPEDESCRIPTION CMultiSource::m_SaveData[] =
{
	//!!!BUGBUG FIX
	DEFINE_ARRAY(CMultiSource, m_rgEntities, FIELD_EHANDLE, MS_MAX_TARGETS),
	DEFINE_ARRAY(CMultiSource, m_rgTriggered, FIELD_BOOLEAN, MS_MAX_TARGETS),
	DEFINE_FIELD(CMultiSource, m_iTotal, FIELD_UINT32),// XDM3038c
	DEFINE_FIELD(CMultiSource, m_globalstate, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CMultiSource, CBaseDelay);// XDM3038c CPointEntity);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pkvd - 
//-----------------------------------------------------------------------------
void CMultiSource::KeyValue(KeyValueData *pkvd)
{
	CBaseDelay::KeyValue(pkvd);//CPointEntity::KeyValue(pkvd);
	if (pkvd->fHandled)
		return;

	if (FStrEq(pkvd->szKeyName, "style") ||
		FStrEq(pkvd->szKeyName, "height") ||
		FStrEq(pkvd->szKeyName, "killtarget") ||
		FStrEq(pkvd->szKeyName, "value1") ||
		FStrEq(pkvd->szKeyName, "value2") ||
		FStrEq(pkvd->szKeyName, "value3"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	//else checked first
	//	CPointEntity::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiSource::Spawn(void)
{
	CBaseDelay::Spawn();//CPointEntity::Spawn();
	// set up think for later registration
	//pev->solid = SOLID_NOT;
	//pev->movetype = MOVETYPE_NONE;
	//old SetBits(pev->spawnflags, SF_MULTISOURCE_INIT);	// Until it's initialized
	SetThink(&CMultiSource::Register);
	if (gTouchDisabled)// XDM3037: node graph is building
		SetNextThink(2.0);// XDM3038a HACK
	else
		SetNextThink(0.25);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: 
// WARNING! pCaller may be CBaseDelay!!
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CMultiSource::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	// Still initializing?
	//old	if (FBitSet(pev->spawnflags, SF_MULTISOURCE_INIT))
	if (pev->impulse == 0)
	{
		conprintf(1, "Warning: %s[%d] %s used by %s %s while initializing!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pCaller->pev->classname), STRING(pCaller->pev->targetname));
		//return;
	}
	// Find the entity in our list
	if (FClassnameIs(pCaller->pev, "DelayedUse"))// XDM3035c: 20121121 HACK to accept "delayed use" from buttons, doors, etc.
	{
		CBaseEntity *pRealCaller = (CBaseEntity *)pCaller->m_hOwner;// XDM3038: lesser hack
		if (pRealCaller && pRealCaller == pCaller->m_hTBDAttacker)// old UTIL_IsValidEntity(pCaller->m_pGoalEnt))
			pCaller = pRealCaller;// m_pGoalEnt;// DELAY_ACTIVATOR_HACK
		else
			conprintf(1, "Warning: %s[%d] %s used by delayed invalid caller!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
	}
	size_t i = 0;
	while (i < m_iTotal)
	{
		if ((CBaseEntity *)m_rgEntities[i] == pCaller)
			break;
		++i;
	}

	// if we didn't find it, report error and leave
	if (i >= m_iTotal)
	{
		conprintf(1, "Warning: %s[%d] %s used by non member %s %s!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pCaller->pev->classname), STRING(pCaller->pev->targetname));
		return;
	}

	// CONSIDER: a Use input to the multisource always toggles.  Could check useType for ON/OFF/TOGGLE
	//if (ShouldToggle(useType, m_rgTriggered[i-1]))
	//m_rgTriggered[i-1] ^= 1;
	m_rgTriggered[i] ^= 1;

	if (IsTriggered(pActivator))
	{
		conprintf(2, "%s[%d] %s enabled (%u inputs)\n", STRING(pev->classname), entindex(), STRING(pev->targetname), m_iTotal);
		USE_TYPE useType2 = USE_TOGGLE;
		if (m_globalstate)
			useType2 = USE_ON;

		SUB_UseTargets(/* NULL*/pActivator, useType2, 0);// XDM3035a: TESTME!
	}
}

//-----------------------------------------------------------------------------
// Purpose: Did all my registered entities activate me?
// Input  : *pActivator - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiSource::IsTriggered(const CBaseEntity *pActivator)
{
	// Still initializing?
	//if (FBitSet(pev->spawnflags, SF_MULTISOURCE_INIT))
	if (pev->impulse == 0)
		return false;

	// Is everything triggered?
	uint32 i = 0;
	while (i < m_iTotal)
	{
		if (m_rgEntities[i].Get() && m_rgTriggered[i] == 0)// at least one untriggered entity is enough to stop // XDM3038c: added entity existence check. Consider that entity was destroyed/removed.
			break;
		++i;
	}

	if (i == m_iTotal)
	{
		if (!m_globalstate || gGlobalState.EntityGetState(m_globalstate) == GLOBAL_ON)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Find and register entities that have me as their target
// Note: this code is MUCH cleaner, faster and more reliable than SoHL or anything else 8)
//-----------------------------------------------------------------------------
void CMultiSource::Register(void)
{
	m_iTotal = 0;
	if (FStringNull(pev->targetname))
	{
		conprintf(1, "%s[%d] with no targetname! Removing!\n", STRING(pev->classname), entindex());
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}
	memset(m_rgEntities, 0, MS_MAX_TARGETS * sizeof(EHANDLE));
	SetThink(&CBaseEntity::SUB_DoNothing);
	DontThink();// XDM3038a
	//SetThinkNull();
	conprintf(2, "%s[%d] %s: registering sources:\n", STRING(pev->classname), entindex(), STRING(pev->targetname));

	// TODO: replace this all with global iteration with HasTarget() check
	int i = gpGlobals->maxClients+1;// NO!!! MAX_PLAYERS+1;
	CBaseEntity	*pEntity = NULL;
	CBaseEntity	*pTarget = NULL;
	//edict_t		*pEdict = ;//INDEXENT(i);// skip players as they can't be multisource sources
	for (; i < gpGlobals->maxEntities; ++i)//, ++pEdict)
	{
		/*pEdict = INDEXENT(i);

		if (!UTIL_IsValidEntity(pEdict))
			continue;
		pEntity = CBaseEntity::Instance(pEdict);*/

		pEntity = UTIL_EntityByIndex(i);

		if (pEntity == NULL)
			continue;
		if (pEntity == this)
			continue;

		pTarget = pEntity;
		/* this is useless now because even DelayedUse's "caller" gets iterated in the process
		if (FClassnameIs(pEdict, "DelayedUse"))
		{
			if (!UTIL_IsValidEntity(pTarget->m_pGoalEnt))// DELAY_ACTIVATOR_HACK WTF? VITAL! This can NULL or a deleted trigger_auto! TODO: use special EHANDLE
				continue;

			conprintf(2, " %u: redirecting %s %s\n", m_iTotal, STRING(pTarget->pev->classname), STRING(pTarget->pev->targetname));
			pTarget = pTarget->m_pGoalEnt;// DELAY_ACTIVATOR_HACK: look for the real caller
		}*/

		if (pTarget->HasTarget(pev->targetname))
		{
			if (FBitSet(pev->spawnflags, SF_MULTISOURCE_ON))
				m_rgTriggered[m_iTotal] = 1;
			else
				m_rgTriggered[m_iTotal] = 0;

			conprintf(2, " %u: %s[%d] %s (triggered: %d)\n", m_iTotal, STRING(pTarget->pev->classname), pTarget->entindex(), STRING(pTarget->pev->targetname), m_rgTriggered[m_iTotal]);
			m_rgEntities[m_iTotal] = pTarget;
			++m_iTotal;
		}
	}
	conprintf(2, " registered total: %u\n", m_iTotal);
	//ClearBits(pev->spawnflags, SF_MULTISOURCE_INIT);
	pev->impulse = 1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CMultiSource::ReportState(int printlevel)
{
	CBaseDelay::ReportState(printlevel);//CPointEntity::ReportState(printlevel);
	conprintf(1, " globalstate: %s, targets: %u:\n", STRING(m_globalstate), m_iTotal);
	for (uint32 cIndex = 0; cIndex < m_iTotal; cIndex++)
		conprintf(1, " %u: %s[%d] triggered: %d\n", cIndex, m_rgEntities[cIndex].Get()?STRING(m_rgEntities[cIndex]->pev->classname):"", m_rgEntities[cIndex].Get()?m_rgEntities[cIndex]->entindex():0, m_rgTriggered[cIndex]);
}






LINK_ENTITY_TO_CLASS( multi_manager, CMultiManager );

// Global Savedata for multi_manager
TYPEDESCRIPTION	CMultiManager::m_SaveData[] =
{
	DEFINE_FIELD( CMultiManager, m_cTargets, FIELD_UINT32 ),// XDM3038c
	DEFINE_FIELD( CMultiManager, m_index, FIELD_UINT32 ),// XDM3038c
	DEFINE_FIELD( CMultiManager, m_startTime, FIELD_TIME ),
	DEFINE_ARRAY( CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS ),
	DEFINE_FIELD( CMultiManager, m_iszThreadName, FIELD_STRING ),// SHL
	DEFINE_FIELD( CMultiManager, m_iszLocusThread, FIELD_STRING ),// SHL
};

IMPLEMENT_SAVERESTORE(CMultiManager, CBaseToggle);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pkvd - 
//-----------------------------------------------------------------------------
void CMultiManager::KeyValue(KeyValueData *pkvd)
{
	CBaseToggle::KeyValue(pkvd);// XDM3036
	if (pkvd->fHandled)
		return;

	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	/*else if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	else if (FStrEq(pkvd->szKeyName, "m_iszThreadName"))// SHL
	{
		m_iszThreadName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszLocusThread"))// SHL
	{
		m_iszLocusThread = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else // add this field to the target list
	{
		// All additional fields are targetnames and their values are delay values.
		if (m_cTargets < MAX_MULTI_TARGETS)
		{
			if (pkvd->szKeyName[0] == TARGET_OPERATOR_CHAR)// XDM3038c
				m_iTargetName[m_cTargets] = ALLOC_STRING(pkvd->szKeyName);
			else
			{
				char tmp[128];
				UTIL_StripToken(pkvd->szKeyName, tmp);
				m_iTargetName[m_cTargets] = ALLOC_STRING(tmp);
			}
			m_flTargetDelay[m_cTargets] = atof(pkvd->szValue);
			++m_cTargets;
			pkvd->fHandled = TRUE;
		}
		else
			conprintf(2, "Error: %s[%d] %s has too many targets: \"%s\" was not registered!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pkvd->szKeyName);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// TESTME on maps c1a0a, c2a1
//-----------------------------------------------------------------------------
void CMultiManager::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->frame = 0;
	pev->model = iStringNull;
	if (sv_showpointentities.value > 0)
	{
		pev->modelindex = g_iModelIndexTestSphere;
		pev->renderamt = 255;
		pev->effects = EF_NOINTERP;
	}
	else
	{
		pev->modelindex = 0;	
		pev->effects = EF_NODRAW;
	}
	UTIL_SetSize(this, g_vecZero, g_vecZero);

	if (m_cTargets > MAX_MULTI_TARGETS)
	{
		conprintf(2, "WARNING: %s[%d] %s has too many targets: %u (max %d)\n", STRING(pev->classname), entindex(), STRING(pev->targetname), m_cTargets, MAX_MULTI_TARGETS);
		m_cTargets = MAX_MULTI_TARGETS;
	}

	SetState(STATE_OFF);

	// Sort targets
	// Quick and dirty bubble sort
	bool swapped = 1;
	while (swapped)
	{
		swapped = 0;
		for (uint32 i = 1; i < m_cTargets; ++i)
		{
			if (m_flTargetDelay[i] < m_flTargetDelay[i-1])
			{
				// Swap out of order elements
				string_t name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i-1];
				m_flTargetDelay[i] = m_flTargetDelay[i-1];
				m_iTargetName[i-1] = name;
				m_flTargetDelay[i-1] = delay;
				swapped = 1;
			}
		}
	}

	SetUse(&CMultiManager::ManagerUse);
	SetThink(&CMultiManager::ManagerThink);

#if defined(_DEBUG)
	ReportState(2);// XDM3035a
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiManager::Activate(void)
{
	DBG_PRINT_ENT("Activate()");
	if (FBitSet(pev->spawnflags, SF_MULTIMAN_SPAWNFIRE))
	{
		//SetThink(&CMultiManager::UseThink);
		//SetUseNull();
		Use(g_pWorld, this, USE_ON, 1.0f);
#if defined(MOVEWITH)
		UTIL_DesiredThink(this);
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Special case: CMultiManager doesn't have "target" field and has many targets
// Input  : targetname - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiManager::HasTarget(string_t targetname)
{
	for (uint32 i = 0; i < m_cTargets; ++i)
		if (FStrEq(STRING(targetname), STRING(m_iTargetName[i])))
			return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Think function
// Designers were using this to fire targets that may or may not exist --
// so I changed it to use the standard target fire code, made it a little simpler.
//-----------------------------------------------------------------------------
void CMultiManager::ManagerThink(void)
{
	if (!IsActive())
	{
		SetNextThink(0.2);
		return;
	}

	float dtime = gpGlobals->time - m_startTime;
	while (m_index < m_cTargets && m_flTargetDelay[m_index] <= dtime)
	{
		if (FStrEq(STRING(m_iTargetName[m_index]), STRING(pev->targetname)))// Found self in list!!
		{
			ALERT(at_debug, "%s[%d] %s: ManagerThink(): WARNING: activating self!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
			/*m_iTargetName[m_index] = 0;// don't hit that twice
			m_index = 0;// pretend it's been restarted
			m_startTime = gpGlobals->time;
			SetNextThink(0);
			return;//break;*/
		}

		const char *strTarget = STRING(m_iTargetName[m_index]);
		/* OBSOLETE if (isspace(strTarget[3]) && _strnicmp(strTarget, "SET", 3) == 0)// XDM3037: NEW: manager now accepts 'SET entity key value'-style commands! NOTE: editor loves to mess with the case, so use insensitive compare
		{
			UTIL_SetTargetValue(strTarget, true);// , (m_iActivationCount & 1) == 0); // UNDONE: use OFF signal every second time
		}
		else
		{*/
			if (FireTargets(strTarget, m_hActivator, this, USE_TOGGLE, 0) == 0)
				conprintf(2, "%s[%d] %s: warning: nothing found for: %s\n", STRING(pev->classname), entindex(), STRING(pev->targetname), strTarget);
		//}
//finishloop:
		m_index++;
	}

	if (m_index >= m_cTargets)// have we fired all targets?
	{
		if (IsLooping())
		{
			conprintf(2, "%s[%d] %s: ManagerThink(): loop\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
			m_index = 0;
			m_startTime = gpGlobals->time;
			SetNextThink(0);
		}
		else
		{
			SetState(STATE_OFF);// finished
			conprintf(2, "%s[%d] %s: ManagerThink(): finished\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
			if (IsClone() || FBitSet(pev->spawnflags, SF_MULTIMAN_ONLYONCE))
			{
				SetThink(&CMultiManager::SUB_Remove);
				SetNextThink(0.001);
			}
			else
			{
				SetThinkNull();
				DontThink();// XDM3038c: may fix some problems on They Hunger maps
				SetUse(&CMultiManager::ManagerUse);// allow manager re-use
			}
		}
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[m_index];// don't think until next target needs to be fired
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CMultiManager
//-----------------------------------------------------------------------------
CMultiManager *CMultiManager::Clone(void)
{
	DBG_PRINT_ENT("Clone()");
	CMultiManager *pMulti = (CMultiManager *)CreateCopy(STRING(pev->classname), this, pev->spawnflags|SF_MULTIMAN_CLONE, false);// XDM3035c: TESTED
	if (pMulti == NULL)
		return NULL;

	pMulti->m_cTargets = m_cTargets;
	memcpy(pMulti->m_iTargetName, m_iTargetName, sizeof(m_iTargetName));
	memcpy(pMulti->m_flTargetDelay, m_flTargetDelay, sizeof(m_flTargetDelay));
	// SHL: TODO?
	/*if (m_iszThreadName) pMulti->pev->targetname = m_iszThreadName;
	pMulti->m_triggerType = m_triggerType;
	pMulti->m_iMode = m_iMode;
	pMulti->m_flWait = m_flWait;
	pMulti->m_flMaxWait = m_flMaxWait;*/
	return pMulti;
}

//-----------------------------------------------------------------------------
// Purpose: 
// The USE function builds the time table and starts the entity thinking.
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CMultiManager::ManagerUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	//DBG_PRINTF("%s[%d] %s: ManagerUse(%s, %s, %d, %g)\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pActivator?STRING(pActivator->pev->classname):"", pCaller?STRING(pCaller->pev->classname):"", useType, value);
	DBG_PRINT_ENT_USE(ManagerUse);
	//ShouldToggle()?
	if (IsLooping())// SHL compatibility
	{
		if (IsActive())// if we're on, or turning on...
		{
			if (useType != USE_ON)// ...then turn it off if we're asked to.
			{
				SetState(STATE_OFF);
				if (IsClone() || FBitSet(pev->spawnflags, SF_MULTIMAN_ONLYONCE))
				{
					SetUseNull();
					SetThink(&CMultiManager::SUB_Remove);
					SetNextThink(0.1);
					ALERT(at_debug, "%s[%d] %s: Use(): loop halted (removing)\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
				}
				else
				{
					SetThinkNull();
					ALERT(at_debug, "%s[%d] %s: Use(): loop halted\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
				}
			}
			else// else we're already on and being told to turn on, so do nothing.
				ALERT(at_debug, "%s[%d] %s: Use(): loop already active\n", STRING(pev->classname), entindex(), STRING(pev->targetname));

			return;
		}
		else if (useType == USE_OFF)// it's already off
		{
			ALERT(at_debug, "%s[%d] %s: Use(): loop already inactive\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
			return;
		}
		// otherwise, start firing targets as normal.
	}
	/*else // XDM3038a: BUG20150618
	{
		if (IsActive())
			ALERT(at_debug, "%s[%d] %s: Use(): (non-looping) used while active, restarting\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
	}*/

	if (IsLockedByMaster(pActivator))
	{
		ALERT(at_debug, "%s[%d] %s: Use(): locked by master \"%s\"\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(m_iszMaster));
		return;
	}

	// In multiplayer games, clone the MM and execute in the clone (like a thread) to allow multiple players to trigger the same multimanager
	if (ShouldClone())
	{
		CMultiManager *pClone = Clone();
		if (pClone)
		{
			pClone->ManagerUse(pActivator, pCaller, useType, value);
			if (m_iszLocusThread)
				FireTargets(STRING(m_iszLocusThread), pClone, this, USE_TOGGLE, 0);
		}
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;
	SetState(STATE_ON);

	//if (FBitSet(pev->spawnflags, SF_MULTIMAN_SAMETRIG))// SHL compatibility
	//	m_triggerType = useType;

	if (IsLooping())// XDM3035c: toggle
		SetUse(&CMultiManager::ManagerUse);
	else
		SetUseNull();// disable use until all targets have fired

	SetThink(&CMultiManager::ManagerThink);
	SetNextThink(0);
	// XDM3038a: BUG20150618	ManagerThink(); TESTMAP: c4a1e, mm1 has mm2 and mm3 only has mm1 (ugly valve hack to create loops) Don't call think directly because it will form a stack and the mm2 will call mm1 back!
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : oldstate - 
//-----------------------------------------------------------------------------
void CMultiManager::OnStateChange(STATE oldstate)
{
	if (GetState() == STATE_ON)
		m_startTime = gpGlobals->time;
	else if (GetState() == STATE_OFF)
		m_startTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CMultiManager::ReportState(int printlevel)
{
	CBaseToggle::ReportState(printlevel);
	conprintf(1, " targets: %u:\n", m_cTargets);
	for (uint32 cIndex = 0; cIndex < m_cTargets; ++cIndex)
		conprintf(1, " %u: %s %g\n", cIndex, STRING(m_iTargetName[cIndex]), m_flTargetDelay[cIndex]);
}





TYPEDESCRIPTION CEnvGlobal::m_SaveData[] =
{
	DEFINE_FIELD( CEnvGlobal, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CEnvGlobal, m_triggermode, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvGlobal, m_initialstate, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvGlobal, CPointEntity );

LINK_ENTITY_TO_CLASS( env_global, CEnvGlobal );

void CEnvGlobal::KeyValue(KeyValueData *pkvd)
{
	// XDM3035c?	pkvd->fHandled = TRUE;

	if (FStrEq(pkvd->szKeyName, "globalstate"))// State name
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggermode"))
	{
		m_triggermode = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "initialstate"))
	{
		m_initialstate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CEnvGlobal::Spawn(void)
{
	if (FStringNull(m_globalstate))
	{
		REMOVE_ENTITY(edict());
		return;
	}
	//CPointEntity::Spawn();
	if (FBitSet(pev->spawnflags, SF_GLOBAL_SET))
	{
		if (!gGlobalState.EntityInTable(m_globalstate))
			gGlobalState.EntityAdd(m_globalstate, gpGlobals->mapname, (GLOBALESTATE)m_initialstate);
	}
}

void CEnvGlobal::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	DBG_PRINT_ENT_USE(Use);
	GLOBALESTATE oldState = gGlobalState.EntityGetState(m_globalstate);
	GLOBALESTATE newState;

	switch (m_triggermode)
	{
	case 0: newState = GLOBAL_OFF; break;
	case 1: newState = GLOBAL_ON; break;
	case 2: newState = GLOBAL_DEAD; break;
	case 3:
	default:
		if (oldState == GLOBAL_ON)
			newState = GLOBAL_OFF;
		else if (oldState == GLOBAL_OFF)
			newState = GLOBAL_ON;
		else
			newState = oldState;
	}

	if (gGlobalState.EntityInTable(m_globalstate))
		gGlobalState.EntitySetState(m_globalstate, newState);
	else
		gGlobalState.EntityAdd(m_globalstate, gpGlobals->mapname, newState);
}




TYPEDESCRIPTION CEnvState::m_SaveData[] =
{
	DEFINE_FIELD( CEnvState, m_fTurnOnTime, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvState, m_fTurnOffTime, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CEnvState, CBaseToggle);

LINK_ENTITY_TO_CLASS( env_state, CEnvState );

void CEnvState::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "turnontime"))
	{
		m_fTurnOnTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "turnofftime"))
	{
		m_fTurnOffTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

void CEnvState::Spawn(void)
{
	if (sv_showpointentities.value > 0)
	{
		pev->modelindex = g_iModelIndexTestSphere;
		pev->effects = EF_NOINTERP;
	}
	else
	{
		pev->modelindex = 0;	
		pev->effects = EF_NODRAW;
	}

	if (FBitSet(pev->spawnflags, SF_ENVSTATE_START_ON))
		SetState(STATE_ON);
	else
		SetState(STATE_OFF);
}

void CEnvState::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (!ShouldToggle(useType, GetState() != STATE_OFF) || IsLockedByMaster(pActivator))// XDM3037: STATE cast -- TESTME
	{
		if (FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
		{
			conprintf(2,"DEBUG: env_state \"%s\" ", STRING(pev->targetname));
			if (IsLockedByMaster(pActivator))
				conprintf(2,"ignored trigger %s; locked by master \"%s\".\n", GetStringForUseType(useType), STRING(m_iszMaster));
			else if (useType == USE_ON)
				conprintf(2,"ignored trigger USE_ON; already on\n");
			else if (useType == USE_OFF)
				conprintf(2,"ignored trigger USE_OFF; already off\n");
			else
				conprintf(2,"ignored trigger %s.\n", GetStringForUseType(useType));
		}
		return;
	}

	switch (GetState())
	{
	case STATE_ON:
	case STATE_TURN_ON:
		if (m_fTurnOffTime)
		{
			SetState(STATE_TURN_OFF);
			if (FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
				conprintf(2,"DEBUG: env_state \"%s\" triggered; will turn off in %f seconds.\n", STRING(pev->targetname), m_fTurnOffTime);

			SetNextThink(m_fTurnOffTime);
		}
		else
		{
			SetState(STATE_OFF);
			if (FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
			{
				conprintf(2,"DEBUG: env_state \"%s\" triggered, turned off", STRING(pev->targetname));
				if (pev->target)
				{
					conprintf(2,": firing \"%s\"",STRING(pev->target));
					if (!FStringNull(pev->noise2))
						conprintf(2," and \"%s\"",STRING(pev->noise2));
				}
				else if (!FStringNull(pev->noise2))
					conprintf(2,": firing \"%s\"",STRING(pev->noise2));

				conprintf(2,".\n");
			}
			FireTargets(STRING(pev->target),pActivator,this,USE_OFF,0);
			FireTargets(STRING(pev->noise2),pActivator,this,USE_TOGGLE,0);
			SetNextThink(0);
		}
		break;
	case STATE_OFF:
	case STATE_TURN_OFF:
		if (m_fTurnOnTime)
		{
			SetState(STATE_TURN_ON);
			if (FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
				conprintf(2,"DEBUG: env_state \"%s\" triggered; will turn on in %f seconds.\n", STRING(pev->targetname), m_fTurnOnTime);

			SetNextThink(m_fTurnOnTime);
		}
		else
		{
			SetState(STATE_ON);
			if (FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
			{
				conprintf(2,"DEBUG: env_state \"%s\" triggered, turned on", STRING(pev->targetname));
				if (pev->target)
				{
					conprintf(2,": firing \"%s\"",STRING(pev->target));
					if (!FStringNull(pev->noise1))
						conprintf(2," and \"%s\"",STRING(pev->noise1));
				}
				else if (!FStringNull(pev->noise1))
					conprintf(2,": firing \"%s\"", STRING(pev->noise1));

				conprintf(2,".\n");
			}
			FireTargets(STRING(pev->target), pActivator, this, USE_ON, 0);
			FireTargets(STRING(pev->noise1), pActivator, this, USE_TOGGLE, 0);
			SetNextThink(0);
		}
		break;
	}
}

void CEnvState::Think(void)
{
	STATE iState = GetState();
	if (iState == STATE_TURN_ON)
	{
		SetState(STATE_ON);
		if (FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
		{
			conprintf(2,"DEBUG: env_state \"%s\" turned itself on",STRING(pev->targetname));
			if (pev->target)
			{
				conprintf(2,": firing %s",STRING(pev->target));
				if (!FStringNull(pev->noise1))
					conprintf(2," and %s",STRING(pev->noise1));
			}
			else if (!FStringNull(pev->noise1))
				conprintf(2,": firing %s",STRING(pev->noise1));

			conprintf(2,".\n");
		}
		FireTargets(STRING(pev->target),this,this,USE_ON,0);
		FireTargets(STRING(pev->noise1),this,this,USE_TOGGLE,0);
	}
	else if (iState == STATE_TURN_OFF)
	{
		SetState(STATE_OFF);
		if (FBitSet(pev->spawnflags, SF_ENVSTATE_DEBUG))
		{
			conprintf(2,"DEBUG: env_state \"%s\" turned itself off",STRING(pev->targetname));
			if (pev->target)
				conprintf(2,": firing %s",STRING(pev->target));
				if (!FStringNull(pev->noise2))
					conprintf(2," and %s",STRING(pev->noise2));
			else if (!FStringNull(pev->noise2))
				conprintf(2,": firing %s",STRING(pev->noise2));

			conprintf(2,".\n");
		}
		FireTargets(STRING(pev->target), this, this, USE_OFF, 0);
		FireTargets(STRING(pev->noise2), this, this, USE_TOGGLE, 0);
	}
	CBaseEntity::Think();// XDM3037: allow SetThink()
}




LINK_ENTITY_TO_CLASS(env_beverage, CEnvBeverage);

void CEnvBeverage::Precache(void)
{
	UTIL_PrecacheOther("item_sodacan");// XDM
}

void CEnvBeverage::Spawn(void)
{
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	CPointEntity::Spawn();
	pev->frags = 0;

	if (pev->health <= 0)
		pev->health = 10;

	if (pev->dmg <= 0)// XDM3038a
		pev->dmg = gSkillData.foodHeal;
}

void CEnvBeverage::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (pev->frags > 0 || pev->health == 0)// XDM3035c: CAN be -1 (infinite)
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	CBaseEntity *pCan = Create("item_sodacan", pev->origin, pev->angles, edict());
	if (pCan)
	{
		//if (pev->skin == 0)// XDM3035a: was 6. But I want some fun!
			pCan->pev->skin = RANDOM_LONG(0, pCan->pev->impulse-1);// random
		//else
		//	pCan->pev->skin = pev->skin;

		pev->frags++;
		pev->health--;
	}
	//if (pev->health <= 0){
	//SetThink(&CBaseEntity::SUB_Remove);
	//SetNextThink(0);
}

void CEnvBeverage::DeathNotice(CBaseEntity *pChild)
{
	if (pChild)
	{
		if (pChild->m_hOwner == this)
			pev->frags--;
	}
}




LINK_ENTITY_TO_CLASS(env_ammodispenser, CFuncAmmoDispenser);

/*void CFuncAmmoDispenser::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	pev->modelindex = 0;
}

void CFuncAmmoDispenser::Precache(void)
{
	//UTIL_PrecacheOther(STRING(pev->message));// XDM
}*/

void CFuncAmmoDispenser::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (pActivator == NULL)
		return;

	//if (pActivator->IsPlayer())
	{
		int ammo_id = GetAmmoIndexFromRegistry(STRING(pev->message));
		if (ammo_id >= 0)
		{
#if defined(OLD_WEAPON_AMMO_INFO)
			if (pActivator->GiveAmmo((int)pev->frags, ammo_id, MaxAmmoCarry(ammo_id)) <= 0)
#else
			if (pActivator->GiveAmmo((int)pev->frags, ammo_id) <= 0)
#endif
				conprintf(1, "%s[%d] %s: unable to give ammo\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
		}
		else
			conprintf(1, "%s[%d] %s: invalid ammo type: %s!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), STRING(pev->message));
	}
}




LINK_ENTITY_TO_CLASS(env_cache, CEnvCache);

void CEnvCache::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "targetname"))
	{
		pev->targetname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "graphic"))// avoid conflicts with entvars_t
	{
		if (m_iNumModels < ENVCACHE_MAX_MODELS)
		{
			strcpy(m_MdlNames[m_iNumModels], pkvd->szValue);
			m_iNumModels++;
			pkvd->fHandled = TRUE;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sound"))
	{
		if (m_iNumSounds < ENVCACHE_MAX_SOUNDS)
		{
			strcpy(m_SndNames[m_iNumSounds], pkvd->szValue);
			m_iNumSounds++;
			pkvd->fHandled = TRUE;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "entity"))
	{
		if (m_iNumEnts < ENVCACHE_MAX_ENTITIES)
		{
			strcpy(m_EntNames[m_iNumEnts], pkvd->szValue);
			m_iNumEnts++;
			pkvd->fHandled = TRUE;
		}
	}
	else
		CPointEntity::KeyValue(pkvd);// really useless
}

/*void CEnvCache::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	pev->modelindex = 0;
}*/

void CEnvCache::Precache(void)
{
	uint32 m,s,e;// size_t
	for (m = 0; m < m_iNumModels; ++m)
		PRECACHE_MODEL(m_MdlNames[m]);

	for (s = 0; s < m_iNumSounds; ++s)
		PRECACHE_SOUND(m_SndNames[s]);

	for (e = 0; e < m_iNumEnts; ++e)
		UTIL_PrecacheOther(m_EntNames[e]);

	conprintf(2, "%s[%d] %s: precached %u models %u sounds %u entities\n", STRING(pev->classname), entindex(), STRING(pev->targetname), m,s,e);
}



//-----------------------------------------------------------------------------
// CMultiMaster - multi_watcher
//-----------------------------------------------------------------------------
enum multi_watcher_logic_e
{
	LOGIC_AND = 0,	// fire if all objects active
	LOGIC_OR,		// fire if any object active
	LOGIC_NAND,		// fire if not all objects active
	LOGIC_NOR,		// fire if all objects disable
	LOGIC_XOR,		// fire if only one (any) object active
	LOGIC_XNOR		// fire if active any number objects, but < then all
};


LINK_ENTITY_TO_CLASS(multi_watcher, CMultiMaster);

TYPEDESCRIPTION CMultiMaster::m_SaveData[] =
{
	DEFINE_FIELD( CMultiMaster, m_cTargets, FIELD_UINT32 ),
	DEFINE_FIELD( CMultiMaster, m_iLogicMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiMaster, m_iSharedState, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiMaster, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiMaster, m_iTargetState, FIELD_INTEGER, MAX_MULTI_TARGETS ),
};
IMPLEMENT_SAVERESTORE(CMultiMaster, CBaseDelay);

void CMultiMaster::KeyValue(KeyValueData *pkvd)
{
	CBaseDelay::KeyValue(pkvd);
	if (pkvd->fHandled)
		return;

	if (FStrEq(pkvd->szKeyName, "logic"))
	{
		m_iLogicMode = GetLogicModeForString(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "state"))
	{
		m_iSharedState = GetStateForString(pkvd->szValue);
		m_bGlobalState = TRUE;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "offtarget"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else // add this field to the target list
	{
		// All additional fields are targetnames and their values are delay values.
		if (m_cTargets < MAX_MULTI_TARGETS)
		{
			if (pkvd->szKeyName[0] == TARGET_OPERATOR_CHAR)// XDM3038c
			{
				m_iTargetName[m_cTargets] = ALLOC_STRING(pkvd->szKeyName);
			}
			else
			{
				char tmp[128];
				UTIL_StripToken(pkvd->szKeyName, tmp);
				m_iTargetName[m_cTargets] = ALLOC_STRING(tmp);
			}
			m_iTargetState[m_cTargets] = GetStateForString(pkvd->szValue);
			++m_cTargets;
			pkvd->fHandled = TRUE;
		}
		else
			conprintf(2, "Error: %s[%d] %s has too many targets: \"%s\" was not registered!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pkvd->szKeyName);
	}
}

int CMultiMaster::GetLogicModeForString(const char *string)
{
	if (string == NULL)
		return -1;

	if (!_stricmp(string, "AND"))
		return LOGIC_AND;
	else if (!_stricmp(string, "OR"))
		return LOGIC_OR;
	else if (!_stricmp(string, "NAND") || !_stricmp(string, "!AND"))
		return LOGIC_NAND;
	else if (!_stricmp(string, "NOR") || !_stricmp(string, "!OR"))
		return LOGIC_NOR;
	else if (!_stricmp(string, "XOR") || !_stricmp(string, "^OR"))
		return LOGIC_XOR;
	else if (!_stricmp(string, "XNOR") || !_stricmp(string, "^!OR"))
		return LOGIC_XNOR;
	else if (isdigit(string[0]))// WTF?!
		return atoi(string);

	ALERT(at_error, "%s[%d] %s: unknown logic mode '%s' specified\n", STRING(pev->classname), entindex(), STRING(pev->targetname), string);
	return -1;
}

void CMultiMaster::Spawn(void)
{
	// use local states instead
	if (!m_bGlobalState)
		m_iSharedState = (STATE)-1;

	SetNextThink(0.1);
}

bool CMultiMaster::CheckState(STATE state, uint32 targetnum)
{
	// global state for all targets
	if (m_iSharedState == -1)
	{
		if ((STATE)m_iTargetState[targetnum] == state)
			return true;
	}
	else
	{
		if (m_iSharedState == state)
			return true;
	}
	return false;
}

void CMultiMaster::Think(void)
{
	if (EvalLogic(NULL)) 
	{
		if (GetState() == STATE_OFF)
		{
			SetState(STATE_ON);
			/*UTIL_*/FireTargets(STRING(pev->target), this, this, USE_ON, 1.0f);
		}
	}
	else 
	{
		if (GetState() == STATE_ON)
		{
			SetState(STATE_OFF);
			/*UTIL_*/FireTargets(STRING(pev->netname), this, this, USE_OFF, 0.0f);
		}
	}
 	SetNextThink(0.01);
}

bool CMultiMaster::EvalLogic(CBaseEntity *pActivator)
{
	bool xorgot = false;
	CBaseEntity *pEntity;
	for (uint32 i = 0; i < m_cTargets; ++i)
	{
		pEntity = UTIL_FindEntityByTargetname(NULL, STRING(m_iTargetName[i]), pActivator);
		if (!pEntity) continue;

		// handle the states for this logic mode
		if (CheckState(pEntity->GetState(), i))
		{
			switch (m_iLogicMode)
			{
			case LOGIC_OR:
				return true;
				break;
			case LOGIC_NOR:
				return false;
				break;
			case LOGIC_XOR: 
				if (xorgot)
					return false;
				xorgot = true;
				break;
			case LOGIC_XNOR:
				if (xorgot)
					return true;
				xorgot = true;
				break;
			}
		}
		else // state is false
		{
			switch (m_iLogicMode)
			{
	        case LOGIC_AND:
	         	return false;
				break;
			case LOGIC_NAND:
				return true;
				break;
			}
		}
	}

	// handle the default cases for each logic mode
	switch (m_iLogicMode)
	{
	case LOGIC_AND:
	case LOGIC_NOR:
		return true;
		break;
	case LOGIC_XOR:
		return xorgot;
		break;
	case LOGIC_XNOR:
		return !xorgot;
		break;
	default:
		return false;
		break;
	}
}



//-----------------------------------------------------------------------------
// CSwitcher - multi_switcher
//-----------------------------------------------------------------------------
enum switcher_modes_e
{
	SWITCHER_MODE_INCREMENT = 0,
	SWITCHER_MODE_DECREMENT,
	SWITCHER_MODE_RANDOM_VALUE
};

LINK_ENTITY_TO_CLASS( multi_switcher, CSwitcher );

// Global Savedata for switcher
TYPEDESCRIPTION CSwitcher::m_SaveData[] = 
{
	DEFINE_FIELD( CSwitcher, m_index, FIELD_UINT32 ),// XDM3038c
	DEFINE_FIELD( CSwitcher, m_cTargets, FIELD_UINT32 ),// XDM3038c
	DEFINE_ARRAY( CSwitcher, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
};
IMPLEMENT_SAVERESTORE( CSwitcher, CBaseDelay );

void CSwitcher::KeyValue(KeyValueData *pkvd)
{
	CBaseDelay::KeyValue(pkvd);
	if (pkvd->fHandled)
		return;

	if (FStrEq(pkvd->szKeyName, "mode"))
	{
		pev->button = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (m_cTargets < MAX_MULTI_TARGETS)
	{
		// add this field to the target list
		// this assumes that additional fields are targetnames and their values are delay values.
		char tmp[128];
		UTIL_StripToken(pkvd->szKeyName, tmp);
		m_iTargetName[m_cTargets] = ALLOC_STRING(tmp);
		m_cTargets++;
		pkvd->fHandled = TRUE;
	}
}

void CSwitcher::Spawn(void)
{
	uint32 r_index = 0;// UNDONE!!! TESTME!!! WARNING!! switched from int to uint!
	uint32 w_index = m_cTargets - 1;
	while (r_index < w_index)
	{
		// we store target with right index in tempname
		string_t name = m_iTargetName[r_index];
		// target with right name is free, record new value from wrong name
		m_iTargetName[r_index] = m_iTargetName[w_index];
		// ok, we can swap targets
		m_iTargetName[w_index] = name;
		++r_index;
		--w_index;
	}
	m_index = 0;
	if (FBitSet(pev->spawnflags, SF_SWITCHER_START_ON))
	{
 		SetNextThink(m_flDelay);
		SetState(STATE_ON);
	}
	else
		SetState(STATE_OFF);
}

void CSwitcher::Next(void)
{
	if (pev->button == SWITCHER_MODE_INCREMENT)
	{
		if (m_index >= m_cTargets)
			m_index = 0;
		else
			++m_index;
	}
	else if (pev->button == SWITCHER_MODE_DECREMENT)
	{
		if (m_index == 0)
			m_index = m_cTargets - 1;
		else
			--m_index;
	}
	else if (pev->button == SWITCHER_MODE_RANDOM_VALUE)
		m_index = RANDOM_LONG(0, m_cTargets - 1);
}

void CSwitcher::Think(void)
{
	Next();
	SetNextThink(m_flDelay);
}

void CSwitcher::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (IsLockedByMaster(pActivator))// XDM3038a: pActivator won't be remembered
		return;

	m_hActivator = pActivator;// XDM3035c: moved AFTER master check

	if (useType == USE_SET)
	{
		// set new target for activate (direct choose or increment/decrement)
		if (FBitSet(pev->spawnflags, SF_SWITCHER_START_ON))
		{
			SetState(STATE_ON);
			SetNextThink(m_flDelay);
			//return;
		}
		else if (value > 0)// useType is the index (starts at 1)
		{
			m_index = (value - 1);
			if (m_index >= m_cTargets)// || m_index < -1)
				m_index = MAX_MULTI_TARGETS;// XDM3038c: invalid value

			//return;
		}
		else
			Next();
	}
	/*else if (useType == USE_RESET)
	{
		// reset switcher
		m_iState = STATE_OFF;
		SetThinkNull();//DontThink();
		m_index = 0;
		return;
	}*/
	else if (m_index < m_cTargets)//MAX_MULTI_TARGETS) // fire any other USE_TYPE and right index
	{
		/*UTIL_*/FireTargets(STRING(m_iTargetName[m_index]), m_hActivator, this, useType, value);
	}
}
