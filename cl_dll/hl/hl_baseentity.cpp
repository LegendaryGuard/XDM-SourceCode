// Stubs for server functions without which shared code won't compile on client side.
// A good idea is to implement as much of them as possible.
#include "extdll.h"
#include "util.h"
#include "util_vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons/weapons.h"
#include "skill.h"
#include "util_vector.h"
#include "cl_dll.h"
#include "event_api.h"
#include "r_efx.h"
#include "pm_defs.h"

// Globals used by game logic
//int gmsgWeapPickup = 0;
enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;
struct skilldata_t gSkillData;

#if defined (_DEBUG)
edict_t *DBG_EntOfVars(const entvars_t *pev)
{
	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;

	/*ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine\n");
	edict_t *pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
	if (pent == NULL)
		ALERT(at_console, "WARNING! FindEntityByVars failed!\n");

	((entvars_t *)pev)->pContainingEntity = pent;
	return pent;*/
	return NULL;
}
#endif // _DEBUG

// XDM3038c: STUB
edict_t *CREATE_NAMED_ENTITY(const char *szClassname)
{
	return NULL;
}

// Global Stubs
void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch)
{
	gEngfuncs.pEventAPI->EV_PlaySound(ENTINDEX(entity), (entity != NULL)?entity->v.origin:g_vecZero, channel, sample, volume, attenuation, flags, pitch);
}

void ClearMultiDamage(void) {}
int ApplyMultiDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker) { return 0; }
int AddMultiDamage(CBaseEntity *pInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType) { return 0; }
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage) {}
int DamageDecal(CBaseEntity *pEntity, const int &bitsDamageType) { return 0; }
void DecalGunshot(TraceResult *pTrace, const int &iBulletType) {}
void EjectBrass(const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype) {}
//int AddAmmoNameToAmmoRegistry(const char *szAmmoname) { return 0; }
//int GetAmmoIndex(const char *psz) { return -1; }

// UTIL_* Stubs
void UTIL_PrecacheOther(const char *szClassname, const string_t iszCustomModel) {}// XDM3037a
void UTIL_BloodDrips(const Vector &origin, const Vector &direction, int color, int amount) {}
void UTIL_DecalTrace(TraceResult *pTrace, int decalNumber) {}
void UTIL_GunshotDecalTrace(TraceResult *pTrace, int decalNumber) {}

/*void UTIL_SetOrigin(entvars_t *pev, const Vector &vecOrigin)
{
	SET_ORIGIN(ENT(pev), vecOrigin);
	pev->origin = vecOrigin;
}*/

void UTIL_LogPrintf(char *fmt, ...)
{
//?	COM_Log(NULL, fmt, argptr);
}

void ClientPrint(entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4) {}

/*void StreakSplash(const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange)
{
	gEngfuncs.Con_DPrintf("CL: ERROR! Obsolete StreakSplash() called!\n");
	gEngfuncs.pEfxAPI->R_StreakSplash(origin, direction, color, count, speed, -velocityRange, velocityRange);
//	FX_StreakSplash(origin, direction, gTracerColors[color], count, speed, -velocityRange, velocityRange);
}*/

void UTIL_MakeAimVectors(const Vector &vecAngles)
{
#if defined (NOSQB)
	AngleVectors(vecAngles, gpGlobals->v_forward, gpGlobals->v_right, gpGlobals->v_up);
#else
	float rgflVec[3];
	vecAngles.CopyToArray(rgflVec);
	rgflVec[0] = -rgflVec[0];
	AngleVectors(rgflVec, gpGlobals->v_forward, gpGlobals->v_right, gpGlobals->v_up);
#endif
}

void UTIL_Remove(CBaseEntity *pEntity)
{
	if (pEntity == NULL)
		return;

	if (pEntity->ShouldRespawn())// XDM3035
	{
		//pEntity->SetThink(&CBaseEntity::SUB_Respawn);
		//pEntity->pev->nextthink = gpGlobals->time + mp_monsrespawntime.value;// XDM: TODO
	}
	else
	{
		pEntity->UpdateOnRemove();
		pEntity->pev->flags |= FL_KILLME;
		pEntity->pev->targetname = 0;
		//	ALERT(at_console, "UTIL_Remove(%s)\n", STRING(pEntity->pev->classname));
		//crash		REMOVE_ENTITY(ENT(pEntity->pev));// XDM3035
	}
}

//-----------------------------------------------------------------------------
// Purpose: Edict version
// Input  : *pent - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsValidEntity(edict_t *pent)
{
	if (pent == NULL || pent->free || (pent->v.flags & FL_KILLME))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Validate entity by all means! MUST BE BULLETPROOF!
// Input  : *pEntity - test subject
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsValidEntity(CBaseEntity *pEntity)
{
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (pEntity)
		{
			if (pEntity->pev == NULL)
				return false;
			if (pEntity->pev->flags & FL_KILLME)
				return false;
			if (pEntity->edict() == NULL)
				return false;
			if (pEntity->edict()->free)
				return false;

			return true;
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		gEngfuncs.Con_Printf("ERROR: CL: UTIL_IsValidEntity() exception!\n");
		DBG_FORCEBREAK
	}
#endif
	return false;
}

void UTIL_SetSize(entvars_t *pev, const Vector &vecMin, const Vector &vecMax)
{
	pev->mins = vecMin;
	pev->maxs = vecMax;
}

void UTIL_SetSize(CBaseEntity *pEntity, const Vector &vecMin, const Vector &vecMax)// XDM
{
	pEntity->pev->mins = vecMin;
	pEntity->pev->maxs = vecMax;
}

CBaseEntity *UTIL_FindEntityInSphere(CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius){ return 0;}
/*Vector UTIL_VecToAngles(const Vector &vec)
{
	Vector ang;
	VectorAngles(vec, ang);
	return ang;
}*/

float UTIL_GetWeaponWorldScale(void)
{
	if (gpGlobals->deathmatch > 0.0f)// stub
		return 1.25f;//, WEAPON_WORLD_SCALE);
	else
		return 1.0f;
}

int g_groupmask = 0;
int g_groupop = 0;

void UTIL_SetGroupTrace(const int &groupmask, const int &op)
{
	g_groupmask		= groupmask;
	g_groupop		= op;
	//ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

void UTIL_UnsetGroupTrace(void)
{
	g_groupmask		= 0;
	g_groupop		= 0;
	//ENGINE_SETGROUPMASK(0, 0);
}


// XDM3038c
string_t MAKE_STRING(const char *str)
{
	//if (sv_strings_alloc.value > 0)
	//	return ALLOC_STRING(str);
	//else
		return (uint64)str - (uint64)STRING(0);
}

// XDM3037a
int PRECACHE_MODEL(char *szModel)
{
	if (szModel && *szModel)
		return (*g_engfuncs.pfnPrecacheModel)(szModel);

	return 0;
}

// XDM3037a
void SET_MODEL(edict_t *pEntity, const char *szModel)
{
	if (pEntity && szModel && *szModel)
	{
		pEntity->v.modelindex = gEngfuncs.pEventAPI->EV_FindModelIndex(szModel);
		pEntity->v.model = MAKE_STRING(szModel);
	}
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




//-----------------------------------------------------------------------------
// CBaseEntity Stubs
//-----------------------------------------------------------------------------
void *CBaseEntity::operator new(size_t stAllocateBlock, edict_t *pEdict)
{
	void *pData = malloc(stAllocateBlock);// CL_DLL ONLY!
	pEdict->pvPrivateData = pData;
	return pData;//(void *)ALLOC_PRIVATE(pEdict, stAllocateBlock);// IEngineStudio.Mem_Calloc( ?
}

#if defined(_MSC_VER) && _MSC_VER >= 1200 // only build this code if MSVC++ 6.0 or higher
void CBaseEntity::operator delete(void *pMem, edict_t *pEdict)
{
	if (pEdict)
	{
		pEdict->v.flags |= FL_KILLME;
		pEdict->pvPrivateData = NULL;// CL_DLL ONLY!
	}
	free(pMem);// CL_DLL ONLY!
}
#endif

CBaseEntity::CBaseEntity()
{
	pev = NULL;
	m_pGoalEnt = NULL;
	m_pLink = NULL;
}

CBaseEntity::~CBaseEntity()
{
	pev = NULL;
	m_pGoalEnt = NULL;
	m_pLink = NULL;
}

void CBaseEntity::KeyValue(struct KeyValueData_s *) {}

void CBaseEntity::Spawn(void)
{
	Precache();
}
void CBaseEntity::Spawn(byte restore)
{
	Spawn();
}
void CBaseEntity::Precache(void) {}
void CBaseEntity::Materialize(void)
{
	UTIL_SetOrigin(this, pev->origin);// XDM3038c
}
void CBaseEntity::Destroy(void) {}// XDM3038c
int CBaseEntity::Save(CSave &save) { return 1; }
int CBaseEntity::Restore(CRestore &restore) { return 1; }
void CBaseEntity::OnFreePrivateData(void) {}// XDM3035
CBaseEntity *CBaseEntity::StartRespawn(void) { return NULL; }// XDM3038c
void CBaseEntity::OnRespawn(void) {}// XDM3038c
void CBaseEntity::PrepareForTransfer(void)// XDM3037
{
#if defined(MOVEWITH)
	//m_pMoveWith ?
	m_pChildMoveWith = NULL;
	m_pSiblingMoveWith = NULL;
	m_pAssistLink = NULL;
#endif
}

int CBaseEntity::ObjectCaps(void) { return 0; }
void CBaseEntity::ScheduleRespawn(const float delay) {}
int CBaseEntity::ShouldCollide(CBaseEntity *pOther) { return 1; }// XDM3035
void CBaseEntity::SetObjectCollisionBox(void) {}
int CBaseEntity::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase) { return 0; }// XDM3037
float CBaseEntity::TakeHealth(const float &flHealth, const int &bitsDamageType) { return 1; }// XDM3037a: why 1?
int CBaseEntity::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType) { return 1; }
CBaseEntity *CBaseEntity::GetNextTarget(void) { return NULL; }

int	CBaseEntity::Intersects(CBaseEntity *pOther)
{
	return BoundsIntersect(pOther->pev->absmin, pOther->pev->absmax, pev->absmin, pev->absmax);// XDM3037
}

void CBaseEntity::MakeDormant(void)
{
	SetBits(pev->flags, FL_DORMANT);
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetBits(pev->effects, EF_NODRAW);
	pev->nextthink = 0;
	UTIL_SetOrigin(this, pev->origin);
}

bool CBaseEntity::IsDormant(void) const
{
	return FBitSet(pev->flags, FL_DORMANT);
}

bool CBaseEntity::IsBSPModel(void) const
{
	if ((pev->solid == SOLID_BSP) || (pev->movetype == MOVETYPE_PUSHSTEP))
		return true;

	/*if (!FStringNull(pev->model))
	{
		if (*STRING(pev->model) == '*')// XDM3037a; reliable
			return true;
	}*/
	return false;
}

bool CBaseEntity::IsInWorld(void) const { return true; }
bool CBaseEntity::IsPlayer(void) const
{
	if (pev && (pev->flags & FL_CLIENT))
		return true;

	return false;
}

bool CBaseEntity::HasTarget(string_t targetname)
{
	return FStrEq(STRING(targetname), STRING(pev->targetname));
}

int CBaseEntity::ShouldToggle(USE_TYPE useType, bool currentState)
{
	if (useType != USE_TOGGLE && useType != USE_SET)
	{
		if ((currentState && useType == USE_ON) || (!currentState && useType == USE_OFF))
			return 0;
	}
	return 1;
}

int	CBaseEntity::DamageDecal(const int &bitsDamageType) { return -1; }
CBaseEntity *CBaseEntity::Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner) { return NULL; }// XDM3038
CBaseEntity *CBaseEntity::Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecVeloity, edict_t *pentOwner, int spawnflags) { return NULL; }// XDM3038
//CBaseEntity *CBaseEntity::Create(string_t iName, const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecVeloity, edict_t *pentOwner, int spawnflags) { return NULL; }// XDM3035: we really NEED this!
//CBaseEntity *CBaseEntity::Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, const Vector &vecVelocity, edict_t *pentOwner) { return NULL; }// XDM
//CBaseEntity *CBaseEntity::Create(const char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner) { return NULL; }
CBaseEntity *CBaseEntity::Instance(edict_t *pent) { return (CBaseEntity *)GET_PRIVATE(pent); }

void CBaseEntity::Disintegrate(void) {}// XDM3035
void CBaseEntity::SUB_Disintegrate(void) {}// XDM3035
void CBaseEntity::SUB_FadeOut(void) {}
void CBaseEntity::SUB_Remove(void) { pev->health = 0; }
void CBaseEntity::SUB_Respawn(void) {}
//-----------------------------------------------------------------------------
// Purpose: Stop all thinking
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
	//ALERT(at_console, "DontThink for %s\n", STRING(pev->targetname));
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
	return (pev->flags & FL_KILLME) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: THE ONLY WAY TO SET ENTITY CLASS NAME!
// Warning: never use any other mechanisms!!!
//-----------------------------------------------------------------------------
void CBaseEntity::SetClassName(const char *szClassName)
{
	if (szClassName)
	{
		strncpy(m_szClassName, szClassName, MAX_ENTITY_STRING_LENGTH);// XDM3038: good place to store entity's classname
		pev->classname = MAKE_STRING(m_szClassName);// A MUST!
	}
}

void CBaseEntity::UpdateOnRemove(void) {}// XDM3034
void CBaseEntity::SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value) {}// XDM3034
void CBaseEntity::Think(void)
{
	if (m_pfnThink)
		(this->*m_pfnThink)();
}

void CBaseEntity::Touch(CBaseEntity *pOther)
{
	if (m_pfnTouch)
		(this->*m_pfnTouch)(pOther);
}

void CBaseEntity::Blocked(CBaseEntity *pOther)
{
	if (m_pfnBlocked)
		(this->*m_pfnBlocked)(pOther);
}

void CBaseEntity::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_pfnUse)
		(this->*m_pfnUse)(pActivator, pCaller, useType, value);
}

float CBaseEntity::DamageForce(const float &damage) { return 0.0f; }// XDM3035
short CBaseEntity::DamageInteraction(const int &bitsDamageType, const Vector &vecSrc) { return DMGINT_NONE; }// XDM3037
CBaseEntity *CBaseEntity::GetDamageAttacker(void)
{
	if (m_hOwner)
		return m_hOwner;

	return this;
}
bool CBaseEntity::FBecomeProne(void) { return false; }
bool CBaseEntity::FVisible(CBaseEntity *pEntity) { return false; }
bool CBaseEntity::FVisible(const Vector &vecOrigin) { return false; }
bool CBaseEntity::FBoxVisible(CBaseEntity *pTarget, Vector &vecTargetOrigin, float flSize) { return false; }
int CBaseEntity::IRelationship(CBaseEntity *pTarget) { return 0; }// XDM3038a
void CBaseEntity::SetModelCollisionBox(void) {}// XDM3035b
void CBaseEntity::AlignToFloor(void) {}// XDM3035b
void CBaseEntity::CheckEnvironment(void) {}// XDM3035b
void CBaseEntity::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType) {}
void CBaseEntity::TraceBleed(const float &flDamage, const Vector &vecDir, TraceResult *ptr, const int &bitsDamageType) {}

// If weapons code "kills" an entity, just set its effects to EF_NODRAW
void CBaseEntity::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	pev->effects |= EF_NODRAW;
}

void CBaseEntity::ReportState(int printlevel)
{
	DBG_PRINT_ENT_THINK(ReportState);
	conprintf(printlevel, "CL: %d: \"%s\", %s, %s, exstate: %u\n targetname: \"%s\", globalname: \"%s\", target: \"%s\", netname: \"%s\"\n model: \"%s\", rmode: %d, rfx: %d, color: (%g %g %g), amt: %g\n health: %g, origin: (%g %g %g), angles: (%g %g %g),\n",
		entindex(), STRING(pev->classname), FBitSet(pev->spawnflags, SF_NOREFERENCE)?"virtual":"real", IsDormant()?"dormant":"nondormant", GetExistenceState(),
		STRING(pev->targetname), STRING(pev->globalname), STRING(pev->target), STRING(pev->netname),
		STRING(pev->model), pev->rendermode, pev->renderfx, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt,
		pev->health, pev->origin.x, pev->origin.y, pev->origin.z, pev->angles.x, pev->angles.y, pev->angles.z);
}




//-----------------------------------------------------------------------------
// Purpose: XDM3038b: tunable
//-----------------------------------------------------------------------------
const float GetDefaultBulletDistance(void)
{
	return BULLET_DEFAULT_DIST;// BAD! UNDONE: TODO: transmit server value!
}

//-----------------------------------------------------------------------------
// Purpose: Only produces random numbers to match the server ones.
// Input  : cShots - number of bullets
//			&vecSrc - trace source
//			&vecDirShooting - 
//			&vecSpread - 
//			*endpos - output: hit position
//			flDistance - maximum distance
//			iBulletType - Bullet enum
//			fDamage - can override standard skill-based bullet damage
//			*pInflictor - attacker's gun
//			*pAttacker - the attacker
//			shared_rand - for players
// Output : Vector 
//-----------------------------------------------------------------------------
Vector FireBullets(ULONG cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, Vector *endpos, float flDistance, int iBulletType, float fDamage, CBaseEntity *pInflictor, CBaseEntity *pAttacker, int shared_rand)
{
	if (pInflictor == NULL)
		return vecSpread;

	if (pAttacker == NULL)
		pAttacker = pInflictor;// the default attacker is ourselves

	float x = 0, y = 0, z = 0;
	for (ULONG iShot = 1; iShot <= cShots; ++iShot)
	{
		if (pAttacker)// Use player's random seed
		{
			x = UTIL_SharedRandomFloat(shared_rand + /*0+*/iShot, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (1 + iShot), -0.5, 0.5);
			y = UTIL_SharedRandomFloat(shared_rand + (2 + iShot), -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (3 + iShot), -0.5, 0.5);
			z = x*x + y*y;
		}
		else// get circular gaussian spread
		{
			do
			{
				x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
				y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
				z = x*x+y*y;
			} while (z > 1);
		}
	}
	return Vector(x*vecSpread.x, y*vecSpread.y, 0.0f);
}

// CBaseDelay Stubs
CBaseDelay::CBaseDelay() : CBaseEntity()
{
	m_iState = STATE_ON;// well, in HL entities are mostly ON unless _START_OFF flag is specified for them
}

void CBaseDelay::KeyValue(struct KeyValueData_s *) {}
void CBaseDelay::Spawn(void) {}// XDM3035c
int CBaseDelay::Save(class CSave &save) { return 1; }
int CBaseDelay::Restore(class CRestore &restore) { return 1; }
void CBaseDelay::SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value) {}// XDM3034
void CBaseDelay::UseTargets(string_t iszTarget, string_t iszKillTarget, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {}// XDM3038c
void CBaseDelay::SetState(STATE newstate)// XDM3035c
{
	if (m_iState != newstate)//&& CanChangeState(newstate))
	{
		STATE oldstate = m_iState;
		m_iState = newstate;
		this->OnStateChange(oldstate);
	}
}
void CBaseDelay::OnStateChange(STATE oldstate) {}
bool CBaseDelay::IsLockedByMaster(const CBaseEntity *pActivator) { return false; }// XDM3038a
bool CBaseDelay::IsTriggered(const CBaseEntity *pActivator) { return true; }// XDM3038
CBaseEntity *CBaseDelay::GetDamageAttacker(void)// XDM3037
{
	CBaseEntity *pAttacher = CBaseEntity::GetDamageAttacker();
	if (pAttacher == this && m_hActivator.Get())// CBaseEntity returned no real attacker
		pAttacher = m_hActivator;

	return pAttacher;
}
void CBaseDelay::DelayThink(void) {}// XDM3034
void CBaseDelay::ReportState(int printlevel)
{
	CBaseEntity::ReportState(printlevel);
	conprintf(printlevel, "State: %d, Delay: %g, KillTarget: %s, Master: %s, Activator: %d\n", m_iState, m_flDelay, STRING(m_iszKillTarget), STRING(m_iszMaster), m_hActivator.Get()?m_hActivator->entindex():0);
}


// CBaseAnimating Stubs
void CBaseAnimating::Spawn(void) {}// XDM
void CBaseAnimating::Precache(void) {}// XDM3038a
int CBaseAnimating::Restore(class CRestore &) { return 1; }
int CBaseAnimating::Save(class CSave &) { return 1; }
void CBaseAnimating::HandleAnimEvent(MonsterEvent_t *pEvent) {}// XDM3035b
int CBaseAnimating::LookupActivity(int activity) { return 0; }
int CBaseAnimating::LookupActivityHeaviest(int activity) { return 0; }
int CBaseAnimating::LookupSequence(const char *label) { return 0; }
void CBaseAnimating::ResetSequenceInfo(void) {}
BOOL CBaseAnimating::GetSequenceFlags(void) { return FALSE; }
void CBaseAnimating::DispatchAnimEvents(float flInterval) {}
float CBaseAnimating::SetBoneController(int iController, float flValue) { return 0.0; }
void CBaseAnimating::InitBoneControllers(void) {}
float CBaseAnimating::SetBlending(byte iBlender, float flValue) { return 0; }
void CBaseAnimating::GetBonePosition(int iBone, Vector &origin, Vector &angles) {}
void CBaseAnimating::GetAttachment(int iAttachment, Vector &origin, Vector &angles) {}
int CBaseAnimating::FindTransition(int iEndingSequence, int iGoalSequence, int *piDir) { return -1; }
//void CBaseAnimating::GetAutomovement(Vector &origin, Vector &angles, float flInterval) {}
void CBaseAnimating::SetBodygroup(int iGroup, int iValue) {}
int CBaseAnimating::GetBodygroup(int iGroup) { return 0; }
float CBaseAnimating::StudioFrameAdvance(float flInterval) { return 0.0; }
void CBaseAnimating::ReportState(int printlevel)
{
	CBaseDelay::ReportState(printlevel);
	conprintf(printlevel, "SequenceFinished: %d, SequenceLoops: %d, nFrames: %d, FrameRate: %g\n", m_fSequenceFinished, m_fSequenceLoops, m_nFrames, m_flFrameRate);
}


// CBaseToggle Stubs
int CBaseToggle::Restore(class CRestore &restore) { return 1; }
int CBaseToggle::Save(class CSave &save) { return 1; }
void CBaseToggle::KeyValue(struct KeyValueData_s *) {}
void CBaseToggle::LinearMove(const Vector &vecDest, const float &flSpeed) {}
void CBaseToggle::AngularMove(const Vector &vecDestAngles, const float &flSpeed) {}
void CBaseToggle::ReportState(int printlevel)
{
	CBaseAnimating::ReportState(printlevel);
	conprintf(printlevel, "toggle_state: %d\n", m_toggle_state);
}


// CBaseMonster Stubs
void CBaseMonster::Precache(void) { CBaseToggle::Precache(); }
void CBaseMonster::Spawn(void) {}// XDM3038c
CBaseEntity *CBaseMonster::StartRespawn(void) { return this; }// XDM3038c
Vector CBaseMonster::EyePosition(void) { return (pev->origin + pev->view_ofs); }// XDM3038c: don't bother
void CBaseMonster::SetEyePosition(void) {}
bool CBaseMonster::ShouldGibMonster(int iGib) const { return false; }
void CBaseMonster::CallGibMonster(void) {}
CBaseEntity *CBaseMonster::CheckTraceHullAttack(const float &flDist, const float &fDamage, const int &iDmgType) { return NULL; }
void CBaseMonster::Eat(float flFullDuration) {}
bool CBaseMonster::FShouldEat(void) const { return true; }
void CBaseMonster::BarnacleVictimBitten(CBaseEntity *pBarnacle) {}
void CBaseMonster::BarnacleVictimReleased(void) {}
void CBaseMonster::Listen(void) {}
float CBaseMonster::FLSoundVolume(CSound *pSound) { return 0.0; }
bool CBaseMonster::FValidateHintType(short sHint) { return false; }
void CBaseMonster::Look(int iDistance) {}
int CBaseMonster::ISoundMask(void) { return 0; }
CSound *CBaseMonster::PBestSound(void) { return NULL; }
CSound *CBaseMonster::PBestScent(void) { return NULL; } 
float CBaseMonster::FallDamage(const float &flFallVelocity) { return 1.0f; }// XDM3035c
void CBaseMonster::MonsterThink(void) {}
void CBaseMonster::MonsterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {}
int CBaseMonster::IgnoreConditions(void) { return 0; }
void CBaseMonster::RouteClear(void) {}
void CBaseMonster::RouteNew(void) {}
bool CBaseMonster::FRouteClear(void) { return false; }
bool CBaseMonster::FRefreshRoute(void) { return false; }
BOOL CBaseMonster::MoveToEnemy(Activity movementAct, float waitTime) { return FALSE; }
BOOL CBaseMonster::MoveToLocation(Activity movementAct, float waitTime, const Vector &goal) { return FALSE; }
BOOL CBaseMonster::MoveToTarget(Activity movementAct, float waitTime) { return FALSE; }
BOOL CBaseMonster::MoveToNode(Activity movementAct, float waitTime, const Vector &goal) { return FALSE; }
void CBaseMonster::Stop(void) {}// XDM3038c
//int ShouldSimplify(int routeType) { return TRUE; }
void CBaseMonster::RouteSimplify(CBaseEntity *pTargetEnt) {}
bool CBaseMonster::FBecomeProne(void) { return true; }
BOOL CBaseMonster::CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
BOOL CBaseMonster::CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
BOOL CBaseMonster::CheckMeleeAttack1(float flDot, float flDist) { return FALSE; }
BOOL CBaseMonster::CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
void CBaseMonster::CheckAttacks(CBaseEntity *pTarget, const float &flDist) {}
bool CBaseMonster::FCanCheckAttacks(void) { return false; }
int CBaseMonster::CheckEnemy(CBaseEntity *pEnemy) { return 0; }
void CBaseMonster::PushEnemy(CBaseEntity *pEnemy, Vector &vecLastKnownPos) {}
BOOL CBaseMonster::PopEnemy(void) { return FALSE; }
void CBaseMonster::SetActivity(Activity NewActivity) {}
void CBaseMonster::SetSequenceByName(char *szSequence) {}
int CBaseMonster::CheckLocalMove(const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist) { return 0; }
float CBaseMonster::OpenDoorAndWait(CBaseEntity *pDoor) { return 0.0; }
void CBaseMonster::AdvanceRoute(float distance) {}
int CBaseMonster::RouteClassify(int iMoveFlag) { return 0; }
bool CBaseMonster::BuildRoute(const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget) { return false; }
bool CBaseMonster::FGetNodeRoute(const Vector &vecDest) { return true; }
void CBaseMonster::InsertWaypoint(const Vector &vecLocation, const int &afMoveFlags) {}
BOOL CBaseMonster::FTriangulate(const Vector &vecStart , const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApex) { return FALSE; }
void CBaseMonster::Move(float flInterval) {}
BOOL CBaseMonster::ShouldAdvanceRoute(float flWaypointDist) { return FALSE; }
void CBaseMonster::MoveExecute(CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval) {}
void CBaseMonster::MonsterInit(void) {}
void CBaseMonster::MonsterInitThink(void) {}
void CBaseMonster::StartMonster(void) {}
void CBaseMonster::MovementComplete(void) {}
int CBaseMonster::TaskIsRunning(void) { return 0; }
int CBaseMonster::IRelationship(CBaseEntity *pTarget) { return 0; }
bool CBaseMonster::FindCover(const Vector &vecThreat, const Vector &vecViewOffset, float flMinDist, float flMaxDist) { return FALSE; }
bool CBaseMonster::BuildNearestRoute(const Vector &vecThreat, const Vector &vecViewOffset, float flMinDist, float flMaxDist) { return false; }
CBaseEntity *CBaseMonster::BestVisibleEnemy(void) { return NULL; }
bool CBaseMonster::FInViewCone(CBaseEntity *pEntity) { return FALSE; }
bool CBaseMonster::FInViewCone(const Vector &origin) { return FALSE; }
void CBaseMonster::MakeIdealYaw(const Vector &vecTarget) {}
float CBaseMonster::FlYawDiff(void) { return 0.0; }
float CBaseMonster::ChangeYaw(float yawSpeed) { return 0; }
float CBaseMonster::VecToYaw(const Vector &vecDir) { return 0.0f; }
void CBaseMonster::HandleAnimEvent(MonsterEvent_t *pEvent) {}
Vector CBaseMonster::ShootAtEnemy(const Vector &shootOrigin) { return g_vecZero; }
Vector CBaseMonster::BodyTarget(const Vector &posSrc) { return Center(); }
Vector CBaseMonster::GetGunPosition(void) { return pev->origin; }
int CBaseMonster::FindHintNode(void) { return -1; }
//void CBaseMonster::ReportAIState(void) {}
void CBaseMonster::KeyValue(KeyValueData *pkvd) {}
bool CBaseMonster::FCheckAITrigger(void) { return FALSE; }
int CBaseMonster::CanPlaySequence(BOOL fDisregardMonsterState, int interruptLevel) { return FALSE; }
BOOL CBaseMonster::FindLateralCover(const Vector &vecThreat, const Vector &vecViewOffset) { return FALSE; }
bool CBaseMonster::FacingIdeal(void) { return false; }
bool CBaseMonster::FCanActiveIdle(void) { return false; }
void CBaseMonster::PlaySentence(const char *pszSentence, float duration, float volume, float attenuation) {}
void CBaseMonster::PlayScriptedSentence(const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener) {}
void CBaseMonster::SentenceStop(void) {}
void CBaseMonster::CorpseFallThink(void) {}
void CBaseMonster::MonsterInitDead(void) {}
BOOL CBaseMonster::BBoxFlat(void) { return TRUE; }
BOOL CBaseMonster::GetEnemy(void) { return FALSE; }
void CBaseMonster::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType) {}
CBaseEntity *CBaseMonster::DropItem(const char *pszItemName, const Vector &vecPos, const Vector &vecAng) { return NULL; }
bool CBaseMonster::ShouldFadeOnDeath(void) const { return false; }
//void CBaseMonster::RadiusDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int iClassIgnore, int bitsDamageType) {}
//void CBaseMonster::RadiusDamage(const Vector &vecSrc, CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int iClassIgnore, int bitsDamageType) {}
void CBaseMonster::FadeMonster(void) {}
void CBaseMonster::DeathSound(void) {}
bool CBaseMonster::GibMonster(void) { return false; }
bool CBaseMonster::HasHumanGibs(void) { return false; }
bool CBaseMonster::HasAlienGibs(void) { return false; }
Activity CBaseMonster::GetDeathActivity(void) { return ACT_DIE_HEADSHOT; }
MONSTERSTATE CBaseMonster::GetIdealState(void) { return MONSTERSTATE_ALERT; }
Schedule_t *CBaseMonster::GetScheduleOfType(int Type) { return NULL; }
Schedule_t *CBaseMonster::GetSchedule(void) { return NULL; }
void CBaseMonster::RunTask(Task_t *pTask) {}
void CBaseMonster::StartTask(Task_t *pTask) {}
Schedule_t *CBaseMonster::ScheduleFromName(const char *pName) { return NULL;}
void CBaseMonster::BecomeDead(void) {}
void CBaseMonster::RunAI(void) {}
void CBaseMonster::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib) {}
bool CBaseMonster::IsHeavyDamage(float flDamage, int bitsDamageType) const// XDM: same as in server.DLL
{
	if (bitsDamageType & DMG_DROWNRECOVER)
		return false;

	if (flDamage <= 0)
		return false;

	if (pev->max_health / flDamage <= 2.8)// ~36% and more!
		return true;
	else
		return false;
}
bool CBaseMonster::IsHuman(void) { return false; }// XDM3035b
bool CBaseMonster::ShouldRespawn(void) const { return false; }// XDM3035
void CBaseMonster::RunPostDeath(void) {}// XDM3037
float CBaseMonster::TakeHealth(const float &flHealth, const int &bitsDamageType) { return 0; }// XDM3037a
int CBaseMonster::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType) { return 0; }
int CBaseMonster::Restore(class CRestore &) { return 1; }
int CBaseMonster::Save(class CSave &) { return 1; }
void CBaseMonster::FrozenThink(void) {}// XDM
void CBaseMonster::FrozenEnd(void) {}
void CBaseMonster::FrozenStart(float freezetime) {}
bool CBaseMonster::CanAttack(void)
{
	if (m_fFrozen)
		return false;

	if (m_flNextAttack > gpGlobals->time)
		return false;

	return true;
}
// XDM3035c: special
bool CBaseMonster::HasTarget(string_t targetname)
{
	if (FStrEq(STRING(targetname), STRING(m_iszTriggerTarget)))
		return true;

	return CBaseToggle::HasTarget(targetname);
}
void CBaseMonster::ReportState(int printlevel)
{
	CBaseToggle::ReportState(printlevel);
}


// CBasePlayer Stubs
int  CBasePlayer::Classify(void) { return 0; }
void CBasePlayer::Precache(void) {}
int CBasePlayer::ObjectCaps(void) { return 0; }// XDM3037
void CBasePlayer::Touch(CBaseEntity *pOther) {}
int CBasePlayer::Save(CSave &save) { return 0; }
int CBasePlayer::Restore(CRestore &restore) { return 0; }
void CBasePlayer::Jump(void) {}
void CBasePlayer::Duck(void) {}
void CBasePlayer::Land(void) {}// XDM3037
void CBasePlayer::PreThink(void) {}
void CBasePlayer::PostThink(void) {}
void CBasePlayer::DeathSound(void) {}
void CBasePlayer::TraceAttack(CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType) {}
int CBasePlayer::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType) { return 0; }
int CBasePlayer::IRelationship(CBaseEntity *pTarget) { return 0; }// XDM3038c
uint32 CBasePlayer::PackImportantItems(void) { return 0; }// XDM3038c
void CBasePlayer::PackDeadPlayerItems(void) {}
void CBasePlayer::RemoveAllItems(bool bRemoveSuit, bool bRemoveImportant) {}

Vector CBasePlayer::BodyTarget(const Vector &posSrc)
{
	//static Vector vecTarget(Center());
	//vecTarget += pev->view_ofs * RANDOM_FLOAT(0.5f, 1.1f);
	return Center();//vecTarget;
}

void CBasePlayer::SetAnimation(const int &playerAnim) {}
void CBasePlayer::SetWeaponAnimType(const char *szExtention)
{
	if (szExtention)
		strcpy(m_szAnimExtention, szExtention);
}
void CBasePlayer::WaterMove(void) {}
void CBasePlayer::PlayerDeathThink(void) {}
//void CBasePlayer::StartDeathCam(void) {}
//void CBasePlayer::DeathCamObserver(Vector vecPosition, Vector vecViewAngle) {}

bool CBasePlayer::ObserverStart(const Vector &vecPosition, const Vector &vecViewAngle, int mode, CBaseEntity *pTarget) { return false; }// XDM
bool CBasePlayer::ObserverStop(void) { return false; }
void CBasePlayer::ObserverFindNextPlayer(bool bReverse) {}
void CBasePlayer::ObserverHandleButtons(void) {}
void CBasePlayer::ObserverSetMode(int iMode) {}
bool CBasePlayer::ObserverValidateTarget(CBaseEntity *pTarget) { return false; }// XDM3038a
bool CBasePlayer::ObserverSetTarget(CBaseEntity *pTarget) { return false; }// XDM3038a
void CBasePlayer::ObserverCheckTarget(void) {}// HL20130901
void CBasePlayer::ObserverCheckProperties(void) {}
void CBasePlayer::FrozenThink(void) {}
void CBasePlayer::FrozenEnd(void) {}
void CBasePlayer::FrozenStart(float freezetime) {}
void CBasePlayer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {}
void CBasePlayer::PlayerUse(void) {}
void CBasePlayer::CheckTimeBasedDamage(void)  {}
void CBasePlayer::UpdateGeigerCounter(void) {}
void CBasePlayer::CheckSuitUpdate(void) {}
void CBasePlayer::SetSuitUpdate(const char *name, int fgroup, int iNoRepeatTime) {}
void CBasePlayer::UpdatePlayerSound(void) {}
CBaseEntity *FindEntityForward(CBaseEntity *pMe) { return NULL; }
bool CBasePlayer::FlashlightIsOn(void)
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}
void CBasePlayer::FlashlightTurnOn(void) {}
void CBasePlayer::FlashlightTurnOff(void) {}
void CBasePlayer::ForceClientDllUpdate(void) {}
void CBasePlayer::ImpulseCommands(void) {}
void CBasePlayer::CheatImpulseCommands(const int &iImpulse) {}
int CBasePlayer::AddPlayerItem(CBasePlayerItem *pItem) { return 0; }
bool CBasePlayer::RemovePlayerItem(CBasePlayerItem *pItem) { return true; }// true???
void CBasePlayer::ItemPreFrame(void) {}
void CBasePlayer::ItemPostFrame(void) {}
int CBasePlayer::AmmoInventory(const int &iAmmoIndex)
{
	if (iAmmoIndex < 0)
		return -1;

	return m_rgAmmo[iAmmoIndex];
}
void CBasePlayer::TabulateAmmo(void) {}
void CBasePlayer::SendAmmoUpdate(void) {}
void CBasePlayer::SendWeaponsUpdate(void) {}// XDM
void CBasePlayer::UpdateClientData(void) {}
bool CBasePlayer::FBecomeProne(void) { return true; }
void CBasePlayer::BarnacleVictimBitten(CBaseEntity *pBarnacle) {}
void CBasePlayer::BarnacleVictimReleased(void) {}
int CBasePlayer::Illumination(void) { return 0; }
int CBasePlayer::ShouldCollide(CBaseEntity *pOther)
{
	if (pOther->IsPlayerItem())
	{
		if (pOther->pev->owner == edict())
			return 0;
	}
	return CBaseMonster::ShouldCollide(pOther);
}
bool CBasePlayer::ShouldBeSentTo(CBasePlayer *pClient)
{
	//if (m_iSpawnState < SPAWNSTATE_SPAWNED)// XDM3038c: prevent connecting players from showing at (0,0,0)
		return false;

	//return CBaseMonster::ShouldBeSentTo(pClient);
}

//float CBasePlayer::GetShootSpreadFactor(void) { return 1.0f; }// XDM3037
Vector CBasePlayer::GetAutoaimVector(const float &flDelta) { return g_vecZero; }
Vector CBasePlayer::AutoaimDeflection(const Vector &vecSrc, const float &flDist, const float &flDelta) { return g_vecZero; }
void CBasePlayer::ResetAutoaim(void) {}
void CBasePlayer::SetCustomDecalFrames(int nFrames) {}
int CBasePlayer::GetCustomDecalFrames(void) { return -1; }
bool CBasePlayer::DropPlayerItem(CBasePlayerItem *pItem) { return false; }
bool CBasePlayer::DropPlayerItem(const char *pszItemName) { return false; }
Vector CBasePlayer::GetGunPosition(void) { return pev->origin + pev->view_ofs; }
#if defined (OLD_WEAPON_AMMO_INFO)
int CBasePlayer::GiveAmmo(const int &iAmount, const int &iIndex, const int &iMax) { return 0; }
#else
int CBasePlayer::GiveAmmo(const int &iAmount, const int &iIndex) { return 0; }
#endif
//int CBasePlayer::GiveAmmo(const int &iAmount, char *szName, const int &iMax) { return 0; }
float CBasePlayer::FallDamage(const float &flFallVelocity) { return 10.0f; }// XDM3035c
//bool CBasePlayer::IsAdministrator(void) { return false; }// OBSOLETE
bool CBasePlayer::CanAttack(void)// XDM3035
{
	if (m_fFrozen)
		return false;

	//if (IsOnLadder() && (mp_laddershooting.value <= 0.0f))// XDM3035: TESTME
	//	return false;

	if (m_flNextAttack > UTIL_WeaponTimeBase())// this prevents player from shooting while changing weapons
		return false;

	return true;
}

void CBasePlayer::UpdateOnRemove(void)
{
	//DBG_PRINT_ENT("UpdateOnRemove()");
	//ClientDisconnect(edict());
}
void CBasePlayer::ReportState(int printlevel)
{
	CBaseMonster::ReportState(printlevel);
	conprintf(printlevel, "SpawnState: %hd, GameFlags: %d, AnimExtention: %s\n", m_iSpawnState, m_iGameFlags, m_szAnimExtention);
}




void CBasePlayerAmmo::KeyValue(KeyValueData *pkvd) {}// XDM3038c
void CBasePlayerAmmo::Spawn(void) {}
void CBasePlayerAmmo::Precache(void) {}// XDM
void CBasePlayerAmmo::Materialize(void) {}
void CBasePlayerAmmo::OnRespawn(void) {}
CBaseEntity *CBasePlayerAmmo::StartRespawn(void) { return this; }
void CBasePlayerAmmo::DefaultTouch(CBaseEntity *pOther) {}
int CBasePlayerAmmo::AddAmmo(CBaseEntity *pOther) { return 1; }// XDM
int CBasePlayerAmmo::Restore(class CRestore &) { return 1; }// XDM
int CBasePlayerAmmo::Save(class CSave &) { return 1; }// XDM
bool CBasePlayerAmmo::ShouldRespawn(void) const { return false; }// XDM3038c
bool CBasePlayerAmmo::IsPushable(void) { return false; }// XDM3038a




int GetSequenceCount(void *pmodel)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return 0;

	return pstudiohdr->numseq;
}






// Don't actually trace, but act like the trace didn't hit anything.
void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr)
{
	memset(ptr, 0, sizeof(*ptr));
	ptr->flFraction = 1.0f;
}

void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr)
{
	memset(ptr, 0, sizeof(*ptr));
	ptr->flFraction = 1.0f;
}

// For debugging, draw a box around a player made out of particles
/*void UTIL_ParticleBox(CBasePlayer *player, float *mins, float *maxs, float life, unsigned char r, unsigned char g, unsigned char b)
{
	int i;
	vec3_t mmin, mmax;
	for (i = 0; i < 3; ++i)
	{
		mmin[i] = player->pev->origin[i] + mins[i];
		mmax[i] = player->pev->origin[i] + maxs[i];
	}
	gEngfuncs.pEfxAPI->R_ParticleBox((float *)&mmin, (float *)&mmax, r,g,b, life);
}*/

// For debugging, draw boxes for other collidable players
/*void UTIL_ParticleBoxes(void)
{
	int idx;
	physent_t *pe;
	vec3_t mins, maxs;
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(player->index-1);
	for (idx = 1; idx < MAX_PHYSENTS; ++idx)
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent(idx);
		if (!pe)
			break;

		if (pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients())
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;
			gEngfuncs.pEfxAPI->R_ParticleBox((float *)&mins, (float *)&maxs, 0, 0, 255, 2.0);
		}
	}
	gEngfuncs.pEventAPI->EV_PopPMStates();
}*/
