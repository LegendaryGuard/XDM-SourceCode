// Functions and interfaces exported by the library
/*#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include "windef.h"
#endif*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "client.h"
#include "pm_shared.h"
#include "basemonster.h"
#include "game.h"
#include "gamerules.h"
#include "pm_defs.h"
//#include "pm_materials.h"
#include "pm_shared.h"
//#include <io.h>
//#include <fcntl.h>
//#include <sys/stat.h>

// Used for security and crash prevention
const char *g_szNonEntityExports[] =
{
	"GiveFnptrsToDll",
	"GetEntityAPI",
	"GetEntityAPI2",
	"GetNewDLLFunctions",
	"Server_GetBlendingInterface",
	"Classify",
	"EntityIs",
	"EntityRelationship",
	"EntityObjectCaps",
	"CanHaveItem",
	"GetActiveGameRules",
	NULL// IMPORTANT: terminator
};


// Holds engine functionality callbacks
enginefuncs_t g_engfuncs;// enginecallback.h
globalvars_t *gpGlobals;// util.h
DLL_GLOBAL int g_iProtocolVersion = 0;

#if defined(_WIN32)
/*
#define far
typedef unsigned long DWORD;
typedef void *PVOID;
typedef void far *LPVOID;

#ifdef STRICT
typedef void *HANDLE;
#define DECLARE_HANDLE(n) typedef struct n##__{int i;}*n
#else
typedef PVOID HANDLE;
#define DECLARE_HANDLE(n) typedef HANDLE n
#endif
typedef HANDLE *PHANDLE,*LPHANDLE;

DECLARE_HANDLE(HINSTANCE);

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
*/
// Required DLL entry point
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	DBG_PRINTF("XDM: DllMain(%d)\n", fdwReason);
	return TRUE;
}
#endif// _WIN32


// XDM3035c+: stubs to eliminate holes in older engine API versions

sequenceEntry_s *pfnSequenceGet(const char *fileName, const char *entryName)
{
	return NULL;//(*g_engfuncs.pfnSequenceGet)(fileName, entryName);
}
sentenceEntry_s *pfnSequencePickSentence(const char *groupName, int pickMethod, int *picked)
{
	return NULL;//(*g_engfuncs.pfnSequencePickSentence)(groupName, pickMethod, picked);
}
int pfnGetFileSize(char *filename)
{
	if (pmove)
	{
		return pmove->COM_FileSize(filename);
	}
	else// slower
	{
		//return (*g_engfuncs.pfnGetFileSize)(filename);
		int l = 0;
		byte *pFile = (*g_engfuncs.pfnLoadFileForMe)(filename, &l);
		if (pFile)
			(*g_engfuncs.pfnFreeFile)(pFile);
		//int fh = 0;
		//if ((fh = _open(filename, _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE)) != -1)
		//	l = _filelength(fh);
		return l;
	}
}
unsigned int pfnGetApproxWavePlayLen(const char *filepath)
{
	return 10.0f;//(*g_engfuncs.pfnGetApproxWavePlayLen)(filepath);
}
int pfnIsCareerMatch(void)
{
	return 0;// (*g_engfuncs.pfnIsCareerMatch)();
}
int pfnGetLocalizedStringLength(const char *label)
{
	return strlen(label)*4;//(*g_engfuncs.pfnGetLocalizedStringLength)(label);
}
void pfnRegisterTutorMessageShown(int mid)
{
	//(*g_engfuncs.pfnRegisterTutorMessageShown)(mid);
}
int pfnGetTimesTutorMessageShown(int mid)
{
	return 0;//(*g_engfuncs.pfnGetTimesTutorMessageShown)(mid);
}
void pfnProcessTutorMessageDecayBuffer(int *buffer, int bufferLength)
{
	//(*g_engfuncs.pfnProcessTutorMessageDecayBuffer)(buffer, bufferLength);
}
void pfnConstructTutorMessageDecayBuffer(int *buffer, int bufferLength)
{
	//(*g_engfuncs.pfnConstructTutorMessageDecayBuffer)(buffer, bufferLength);
}
void pfnResetTutorMessageDecayData(void)
{
	//(*g_engfuncs.pfnResetTutorMessageDecayData)();
}
void pfnQueryClientCvarValue(const edict_t *player, const char *cvarName)
{
	//(*g_engfuncs.pfnQueryClientCvarValue)(player, cvarName);
}
void pfnQueryClientCvarValue2(const edict_t *player, const char *cvarName, int requestID)
{
	//(*g_engfuncs.pfnQueryClientCvarValue2)(player, cvarName, requestID);
}
int pfnCheckParm(const char *pchCmdLineToken, char **ppnext)
{
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Gives game DLL a chance to copy base server API
// Warning: MUST be exported as number 1!
// Input  : *pengfuncsFromEngine - pointer to a (temporary?) structure that contains pointers to engine functions
//			*pGlobals - pointer to a server globals structure
//-----------------------------------------------------------------------------
extern "C" void DLLEXPORT STDCALL GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals)
{
	gpGlobals = pGlobals;

	// detect game protocol version
	g_iProtocolVersion = 46;// HL1110-, safest, default
	int exe_build = 0;
	const char *version_string = (*pengfuncsFromEngine->pfnCVarGetString)("sv_version");// "sv_version" is "0123456789abcdefghijklmnopqrstu,48,4554"
	if (version_string)
	{
		//char PatchVersion[32];// from XDM/steam.inf, max 32 chars
		//fail	sscanf(version_string, "%s,%d,%d", PatchVersion, &protocol_ver, &exe_build);// == 3?
		const char *protocol_ver_str = strchr(version_string, ',');// safe: HL is smart enough to filter all ',' from these values
		if (protocol_ver_str)
			if (sscanf(protocol_ver_str, ",%d,%d", &g_iProtocolVersion, &exe_build) != 2)
				printf("XDM: error parsing engine version string\n");
	}

	memset(&g_engfuncs, 0, sizeof(enginefuncs_t));
	// now use proper preset
	size_t engineFunctionsSize;
#if defined(SVDLL_NEWFUNCTIONS)// now it's safe to define it for all HL versions
	if (g_iProtocolVersion > 48)// some ultra-new HL
	{
		printf("XDM: engine protocol version (%d) is newer than expected\n", g_iProtocolVersion);
		engineFunctionsSize = sizeof(enginefuncs_t);// we don't know what's new
	}
	else if (g_iProtocolVersion == 48)// HL1121
	{
		engineFunctionsSize = sizeof(enginefuncs_t);
		//engineFunctionsSize = offsetof(enginefuncs_t, pfnCheckParm);
		//g_engfuncs.pfnCheckParm = pfnCheckParm;// does exist
	}
	else if (g_iProtocolVersion == 47)// HL1120
	{
		engineFunctionsSize = offsetof(enginefuncs_t, pfnQueryClientCvarValue);
		// stubs
		g_engfuncs.pfnQueryClientCvarValue = pfnQueryClientCvarValue;
		g_engfuncs.pfnQueryClientCvarValue2 = pfnQueryClientCvarValue2;
		g_engfuncs.pfnCheckParm = pfnCheckParm;
	}
	else// HL1110 and older
	{
		engineFunctionsSize = offsetof(enginefuncs_t, pfnSequenceGet);
		// stubs
		//g_engfuncs.pfnKeyNameForBinding = pfnKeyNameForBinding was never implemented
		g_engfuncs.pfnSequenceGet = pfnSequenceGet;
		g_engfuncs.pfnSequencePickSentence = pfnSequencePickSentence;
		g_engfuncs.pfnGetFileSize = pfnGetFileSize;
		g_engfuncs.pfnGetApproxWavePlayLen = pfnGetApproxWavePlayLen;
		g_engfuncs.pfnIsCareerMatch = pfnIsCareerMatch;
		g_engfuncs.pfnGetLocalizedStringLength = pfnGetLocalizedStringLength;
		g_engfuncs.pfnRegisterTutorMessageShown = pfnRegisterTutorMessageShown;
		g_engfuncs.pfnGetTimesTutorMessageShown = pfnGetTimesTutorMessageShown;
		g_engfuncs.ProcessTutorMessageDecayBuffer = pfnProcessTutorMessageDecayBuffer;// HL20130901: I'm not even surprised to see such %s :-/
		g_engfuncs.ConstructTutorMessageDecayBuffer = pfnConstructTutorMessageDecayBuffer;
		g_engfuncs.ResetTutorMessageDecayData = pfnResetTutorMessageDecayData;
		g_engfuncs.pfnQueryClientCvarValue = pfnQueryClientCvarValue;
		g_engfuncs.pfnQueryClientCvarValue2 = pfnQueryClientCvarValue2;
		g_engfuncs.pfnCheckParm = pfnCheckParm;
	}
#else
	engineFunctionsSize = sizeof(enginefuncs_t);// without SVDLL_NEWFUNCTIONS it has smallest list of functions
#endif

	char *str = UTIL_VarArgs("XDM: GiveFnptrsToDll(): using enginefuncs_t size %d (assuming protocol %d)\n", engineFunctionsSize, g_iProtocolVersion);
	printf(str);
	(*pengfuncsFromEngine->pfnServerPrint)(str);
	memcpy(&g_engfuncs, pengfuncsFromEngine, engineFunctionsSize);
}

// Converts char* to const char* (client needs const)
char PM_FindTextureType2(char *name)
{
	return PM_FindTextureType(name);
}

/*static*/ DLL_FUNCTIONS gEntityInterface = 
{
	GameDLLInit,				//pfnGameInit
	DispatchSpawn,				//pfnSpawn
	DispatchThink,				//pfnThink
	DispatchUse,				//pfnUse
	DispatchTouch,				//pfnTouch
	DispatchBlocked,			//pfnBlocked
	DispatchKeyValue,			//pfnKeyValue
	DispatchSave,				//pfnSave
	DispatchRestore,			//pfnRestore
	DispatchAbsBox,				//pfnAbsBox

	SaveWriteFields,			//pfnSaveWriteFields
	SaveReadFields,				//pfnSaveReadFields

	SaveGlobalState,			//pfnSaveGlobalState
	RestoreGlobalState,			//pfnRestoreGlobalState
	ResetGlobalState,			//pfnResetGlobalState

	ClientConnect,				//pfnClientConnect
	ClientDisconnect,			//pfnClientDisconnect
	ClientKill,					//pfnClientKill
	ClientPutInServer,			//pfnClientPutInServer
	ClientCommand,				//pfnClientCommand
	ClientUserInfoChanged,		//pfnClientUserInfoChanged
	ServerActivate,				//pfnServerActivate
	ServerDeactivate,			//pfnServerDeactivate

	PlayerPreThink,				//pfnPlayerPreThink
	PlayerPostThink,			//pfnPlayerPostThink

	StartFrame,					//pfnStartFrame
	ParmsNewLevel,				//pfnParmsNewLevel
	ParmsChangeLevel,			//pfnParmsChangeLevel

	GetGameDescription,         //pfnGetGameDescription    Returns string describing current .dll game.
	PlayerCustomization,        //pfnPlayerCustomization   Notifies .dll of new customization for player.

	SpectatorConnect,			//pfnSpectatorConnect      Called when spectator joins server
	SpectatorDisconnect,        //pfnSpectatorDisconnect   Called when spectator leaves the server
	SpectatorThink,				//pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

	Sys_Error,					//pfnSys_Error				Called when engine has encountered an error

	PM_Move,					//pfnPM_Move
	PM_Init,					//pfnPM_Init				Server version of player movement initialization
	PM_FindTextureType2,		//pfnPM_FindTextureType

	SetupVisibility,			//pfnSetupVisibility        Set up PVS and PAS for networking for this client
	UpdateClientData,			//pfnUpdateClientData       Set up data sent only to specific client
	AddToFullPack,				//pfnAddToFullPack
	CreateBaseline,				//pfnCreateBaseline			Tweak entity baseline for network encoding, allows setup of player baselines, too.
	RegisterEncoders,			//pfnRegisterEncoders		Callbacks for network encoding
	GetWeaponData,				//pfnGetWeaponData
	CmdStart,					//pfnCmdStart
	CmdEnd,						//pfnCmdEnd
	ConnectionlessPacket,		//pfnConnectionlessPacket
	GetHullBounds,				//pfnGetHullBounds
	CreateInstancedBaselines,   //pfnCreateInstancedBaselines
	InconsistentFile,			//pfnInconsistentFile
	AllowLagCompensation,		//pfnAllowLagCompensation
};

//-----------------------------------------------------------------------------
// Purpose: Called by the engine to get pointers to server DLL fundamental functions.
// Warning: Not called if GetEntityAPI2 is available.
// Input  : *pFunctionTable - pointer to table to fill
//			interfaceVersion - engine provides version for comparison
// Output : int 0 fail, 1 success
//-----------------------------------------------------------------------------
extern "C" EXPORT int GetEntityAPI(DLL_FUNCTIONS *pFunctionTable, int interfaceVersion)
{
	// check if engine's pointer is valid and version is correct...
	if (pFunctionTable == NULL)
	{
		printf("XDM: GetEntityAPI(): bad arguments!\n");
		return 0;
	}
	if (interfaceVersion != INTERFACE_VERSION)
	{
		printf("XDM: GetEntityAPI(): incompatible interface version %d! (local %d)\n", interfaceVersion, INTERFACE_VERSION);
		return 0;
	}

	memcpy(pFunctionTable, &gEntityInterface, sizeof(gEntityInterface));
	printf("XDM: GetEntityAPI(): size = %d, version = %d\n", sizeof(gEntityInterface), interfaceVersion);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine to get pointers to server DLL fundamental functions.
// Warning: Called first, GetEntityAPI is ignored.
// Input  : *pFunctionTable - pointer to table to fill
//			*interfaceVersion - provides engine version, write DLL version into it
// Output : int 0 fail, 1 success
//-----------------------------------------------------------------------------
extern "C" EXPORT int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
	if (pFunctionTable == NULL || interfaceVersion == NULL)
	{
		printf("XDM: GetEntityAPI2(): bad arguments!\n");
		return 0;
	}
	if (*interfaceVersion != INTERFACE_VERSION)
	{
		printf("XDM: GetEntityAPI2(): incompatible interface version %d! (local %d)\n", *interfaceVersion, INTERFACE_VERSION);
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return 0;// should we?
	}

	memcpy(pFunctionTable, &gEntityInterface, sizeof(gEntityInterface));
	printf("XDM: GetEntityAPI2(): size = %d, version = %d\n", sizeof(gEntityInterface), *interfaceVersion);
	return 1;
}






//-----------------------------------------------------------------------------
// Purpose: FREE_PRIVATE() calls this
// Input  : *pEnt - 
//-----------------------------------------------------------------------------
void OnFreeEntPrivateData(edict_t *pEnt)
{
	if (pEnt == NULL)
	{
		conprintf(1, "Error! OnFreeEntPrivateData() pEnt is NULL!\n");
		DBG_FORCEBREAK
		return;
	}
	CBaseEntity *pEntity = CBaseEntity::Instance(pEnt);
	if (pEntity)
		pEntity->OnFreePrivateData();
		// TODO: pEntity->~CBaseEntity(); ? will this work with overloads?
}

#if defined (_DEBUG_INFILOOPS)
edict_t *g_pentOther = NULL;
edict_t *g_pentTouched = NULL;
size_t g_CollideCounterOther = 0;
size_t g_CollideCounterOtherAlert = 0;// alert every ~4000 calls in case it is really endless
size_t g_CollideCounterTouched = 0;
size_t g_CollideCounterTouchedAlert = 0;
#endif
//-----------------------------------------------------------------------------
// Purpose: ShouldCollide
// WARNING: This function is buggy inside the engine! If one collision is denied, entity stops colliding with ALL other objects!
// Note   : Called by the engine, for collisions and traces.
// Input  : *pentTouched - to check collision with
//			*pentOther - subject
// Output : int return 1 to restore default behavior
//-----------------------------------------------------------------------------
#if defined (USE_SHOULDCOLLIDE)// READ THE WARNING!!!!
int ShouldCollide(edict_t *pentTouched, edict_t *pentOther)
{
	if (pentTouched == NULL)
	{
		conprintf(1, "Error! ShouldCollide() pentTouched is NULL!\n");
		DBG_FORCEBREAK
		return 0;
	}
	if (pentOther == NULL)// XDM3038c: it really happened! On c4a3 MakeFriend(): UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 32), node.m_vecOrigin + Vector(0, 0, 32), dont_ignore_monsters, large_hull, NULL, &tr);
	{
		conprintf(1, "Error! ShouldCollide() pentOther is NULL!\n");
		DBG_FORCEBREAK
		return 0;
	}
	//if (pentOther->v.solid == SOLID_TRIGGER)
	//	return 0;// TEST
	if (FBitSet(pentTouched->v.flags, FL_KILLME) || FBitSet(pentOther->v.flags, FL_KILLME))
	{
		conprintf(1, "Error! ShouldCollide() called for removed entity!\n");
		return 0;
	}

	CBaseEntity *pEntity = CBaseEntity::Instance(pentTouched);
	if (pEntity)
	{
		CBaseEntity *pOther = CBaseEntity::Instance(pentOther);
		if (pOther)
		{
#if defined (_DEBUG_INFILOOPS)
			if (sv_dbg_maxcollisionloops.value > 0)// DEBUG HACK UNHACK WTF: We count calls and delete problematic entities... That's the valve fight.
			{
				if (g_pentOther != pentOther)
				{
					if (g_CollideCounterOther > sv_dbg_maxcollisionloops.value)
						conprintf(2, "WARNING!!! Possible infinite loop was detected in ShouldCollide() with pentOther: %u calls total!\n", g_CollideCounterOther);

					g_pentOther = pentOther;
					g_CollideCounterOther = 0;
					g_CollideCounterOtherAlert = 0;
				}
				else
				{
					if (g_CollideCounterOther > sv_dbg_maxcollisionloops.value)
					{
						conprintf(2, "WARNING!!! Possible infinite loop detected in ShouldCollide(%d %s, %d %s) with pentOther (%u calls)!\n", pEntity->entindex(), STRING(pEntity->pev->classname), pOther->entindex(), STRING(pOther->pev->classname), g_CollideCounterOther);
						//DBG_FORCEBREAK
						if (/*pOther->IsProjectile() && */IsSafeToRemove(pentOther))
						{
							UTIL_Remove(pOther);
							g_pentOther = NULL;
							g_CollideCounterOther = 0;
							//g_CollideCounterOtherAlert = 0;
						}
						g_CollideCounterOtherAlert = 0;
						return 0;
					}
					else
					{
						++g_CollideCounterOther;
						++g_CollideCounterOtherAlert;
					}
				}
				if (g_pentTouched != pentTouched)
				{
					if (g_CollideCounterTouched > sv_dbg_maxcollisionloops.value)
						conprintf(2, "WARNING!!! Possible infinite loop was detected in ShouldCollide() with pentTouched: %u calls total!\n", g_CollideCounterTouched);

					g_pentTouched = pentTouched;
					g_CollideCounterTouched = 0;
					g_CollideCounterTouchedAlert = 0;
				}
				else
				{
					if (g_CollideCounterTouched > sv_dbg_maxcollisionloops.value)
					{
						conprintf(2, "WARNING!!! Possible infinite loop detected in ShouldCollide(%d %s, %d %s) with pentTouched (%u calls)!\n", pEntity->entindex(), STRING(pEntity->pev->classname), pOther->entindex(), STRING(pOther->pev->classname), g_CollideCounterTouched);
						//DBG_FORCEBREAK
						if (/*pEntity->IsProjectile() && */IsSafeToRemove(pentTouched))
						{
							UTIL_Remove(pEntity);
							g_pentTouched = NULL;
							g_CollideCounterTouched = 0;
							//g_CollideCounterTouchedAlert = 0;
						}
						g_CollideCounterTouchedAlert = 0;
						return 0;
					}
					else
					{
						++g_CollideCounterTouched;
						++g_CollideCounterTouchedAlert;
					}
				}
			}
#endif
			//if (pEntity->pev->solid == SOLID_TRIGGER && pOther->pev->solid == SOLID_TRIGGER)
			//	return 0;// XDM3038a
#if defined(DEBUG_COLLISIONS)
			int iResult = pOther->ShouldCollide(pEntity);// XDM3038c: order
			if (iResult == 0)
				conprintf(2, "ShouldCollide(): colliison of %s[%d] with %s[%d] DENIED\n", STRING(pOther->pev->classname), pOther->entindex(), STRING(pEntity->pev->classname), pEntity->entindex());
#else
			return pOther->ShouldCollide(pEntity);// XDM3038c: order
#endif
		}
	}
	return 0;// XDM3038: nothing to collide!
}
#endif // USE_SHOULDCOLLIDE

#if defined(HL1120)
void CvarValue(const edict_t *pEnt, const char *value)
{
}

void CvarValue2(const edict_t *pEnt, int requestID, const char *cvarName, const char *value)
{
}
#endif // HL1120

/*static*/ NEW_DLL_FUNCTIONS gNewDLLFunctions = 
{
	OnFreeEntPrivateData,	//pfnOnFreeEntPrivateData
	GameDLLShutdown,		//pfnGameShutdown
#if defined(USE_SHOULDCOLLIDE)// XDM3038c: this code is buggy inside engine!
	ShouldCollide,			//pfnShouldCollide
#if defined(HL1120)// XDM3035: these causes API glitches in earlier HL engine such as ClientCommand will be never called
	CvarValue,				//pfnCvarValue
	CvarValue2,				//pfnCvarValue2
#endif // HL1120
#endif // USE_SHOULDCOLLIDE
};


//-----------------------------------------------------------------------------
// Purpose: Called by the engine to get pointers to server DLL additional functions.
// Warning: Called by HL1110+ (or little earlier) versions only! Optional!
// Warning: Make sure this function is added to .def file as well!
// Input  : *pFunctionTable - pointer to table to fill
//			*interfaceVersion - provides engine version, write DLL version into it
// Output : int 0 fail, 1 success
//-----------------------------------------------------------------------------
extern "C" EXPORT int GetNewDLLFunctions(NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)// XDM: I need this
{
	if (pFunctionTable == NULL || interfaceVersion == NULL)
	{
		printf("XDM: GetNewDLLFunctions(): bad arguments!\n");
		return 0;
	}
	if (*interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		printf("XDM: GetNewDLLFunctions(): incompatible interface version %d! (local %d)\n", *interfaceVersion, NEW_DLL_FUNCTIONS_VERSION);
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return 0;
	}

#if defined(HL1120)// TODO: get rid of this, use stubs if necessary
	size_t newFunctionsSize;

	if (g_iProtocolVersion >= 47)
		newFunctionsSize = sizeof(gNewFunctionTable);
	else
	{
		newFunctionsSize = offsetof(NEW_DLL_FUNCTIONS, pfnCvarValue);
		gNewDLLFunctions.pfnCvarValue = CvarValue;
		gNewDLLFunctions.pfnCvarValue2 = CvarValue2;
	}
	memcpy(pFunctionTable, &gNewDLLFunctions, newFunctionsSize);
	printf("XDM: GetNewDLLFunctions(): size = %d, version = %d\n", newFunctionsSize, *interfaceVersion);
#else
	memcpy(pFunctionTable, &gNewDLLFunctions, sizeof(gNewDLLFunctions));
	printf("XDM: GetNewDLLFunctions(): size = %d, version = %d\n", sizeof(gNewDLLFunctions), *interfaceVersion);
#endif
	return 1;
}

/* TODO: optional interface
extern "C" EXPORT int Server_GetBlendingInterface(int version, struct sv_blending_interface_s **ppinterface, struct engine_studio_api_s *pstudio, float (*rotationmatrix)[3][4], float (*bonetransform)[MAXSTUDIOBONES][3][4])
{
	return 0;
}*/

// XDM3035b: useful for external DLLs like XBM

//-----------------------------------------------------------------------------
// Purpose: Classify entity
// Input  : *entity - 
// Output : extern "C" int EXPORT
//-----------------------------------------------------------------------------
extern "C" int EXPORT Classify(edict_t *entity)
{
	CBaseEntity *pEntity = CBaseEntity::Instance(entity);
	if (pEntity)
		return pEntity->Classify();

	return CLASS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Reliably determine entity class in XDM
// Input  : entity
// Output : Returns 1 for player, 2-3 for monster
// and 0 for other entities.
//-----------------------------------------------------------------------------
extern "C" int EXPORT EntityIs(edict_t *entity)
{
	if (!UTIL_IsValidEntity(entity))
		return ENTIS_INVALID;

	CBaseEntity *pEntity = CBaseEntity::Instance(entity);
	if (!UTIL_IsValidEntity(pEntity))
		return ENTIS_INVALID;

	if (pEntity->IsPlayer())// check first!
		return ENTIS_PLAYER;
	else if (pEntity->IsMonster())
	{
		if (pEntity->IsHuman())
			return ENTIS_HUMAN;
		else
			return ENTIS_MONSTER;
	}
	else if (pEntity->IsGameGoal())
		return ENTIS_GAMEGOAL;
	else if (pEntity->IsProjectile())
		return ENTIS_PROJECTILE;
	else if (pEntity->IsPlayerWeapon())
		return ENTIS_PLAYERWEAPON;
	else if (pEntity->IsPlayerItem())
		return ENTIS_PLAYERITEM;
	else if (pEntity->IsPickup())
		return ENTIS_OTHERPICKUP;
	else if (pEntity->IsPushable())
		return ENTIS_PUSHABLE;
	else if (pEntity->IsBreakable())
		return ENTIS_BREAKABLE;
	else if (pEntity->IsTrigger())
		return ENTIS_TRIGGER;

	return ENTIS_OTHER;
}

//-----------------------------------------------------------------------------
// Purpose: Reliably determine relationship between two entities in XDM
// Input  : entity1, entity2
// Output : Returns PlayerRelationship(ent2) e.g. GR_NOTTEAMMATE for player,
//			IRelationship(ent2) for monster and 0 for other entities.
//-----------------------------------------------------------------------------
extern "C" int EXPORT EntityRelationship(edict_t *entity1, edict_t *entity2)
{
	CBaseEntity *pEntity1 = CBaseEntity::Instance(entity1);
	CBaseEntity *pEntity2 = CBaseEntity::Instance(entity2);
	if (pEntity1 == NULL)
		return 0;
	if (pEntity2 == NULL)
		return 0;

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (pEntity1->IsPlayer())
		{
			if (g_pGameRules)
				return g_pGameRules->PlayerRelationship(pEntity1, pEntity2);
		}
		else if (pEntity1->MyMonsterPointer())//IsMonster())
			return pEntity1->MyMonsterPointer()->IRelationship(pEntity2);
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		conprintf(0, "EntityRelationship() exception!\n");
	}
#endif
	return 0;//R_NO;
}

//-----------------------------------------------------------------------------
// Purpose: Access to entity's ObjectCaps()
// Input  : *entity - 
// Output : int - flags
//-----------------------------------------------------------------------------
extern "C" int EXPORT EntityObjectCaps(edict_t *entity)
{
	CBaseEntity *pEntity = CBaseEntity::Instance(entity);
	if (pEntity == NULL)
		return 0;

	int caps = 0;
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (UTIL_IsValidEntity(pEntity))
			caps = pEntity->ObjectCaps();
		else
			conprintf(1, "EntityObjectCaps() invalid entity!\n");
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		conprintf(0, "EntityObjectCaps() exception!\n");
	}
#endif
	return caps;
}

//-----------------------------------------------------------------------------
// Purpose: CanHavePlayerItem
// Input  : *entity - 
//			*item - CBasePlayerItem only!
// Output : int 1/0
//-----------------------------------------------------------------------------
extern "C" int EXPORT CanHaveItem(edict_t *entity, edict_t *item)
{
	CBaseEntity *pEntity1 = CBaseEntity::Instance(entity);
	CBaseEntity *pEntity2 = CBaseEntity::Instance(item);
	if (pEntity1 == NULL)
		return 0;
	if (pEntity2 == NULL)
		return 0;

	if (g_pGameRules && pEntity1->IsPlayer() && pEntity2->IsPlayerItem())// luckily these allow us to cast directly to desired types
		return g_pGameRules->CanHavePlayerItem((CBasePlayer *)pEntity1, (CBasePlayerItem *)pEntity2);

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Reliably get game rules
// Input  : *type - pointer to be filled
//			*mode - 
// Output : byte game rules state: 0 - not installed, 1 - normal, 2 - game over
//-----------------------------------------------------------------------------
extern "C" int EXPORT GetActiveGameRules(short *type, short *mode)
{
	if (g_pGameRules)
	{
		if (type)
			*type = g_pGameRules->GetGameType();

		if (mode)
			*mode = g_pGameRules->GetGameMode();

		if (g_pGameRules->IsGameOver())
			return AGR_GAMEOVER;

		return AGR_ACTIVE;
	}
	return AGR_NOTINSTALLED;
}
