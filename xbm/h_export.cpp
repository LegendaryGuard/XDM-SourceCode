#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "waypoint.h"
#include "engine.h"
//#include <io.h>

#ifndef __linux__

HINSTANCE h_Library = NULL;
HGLOBAL h_global_argv = NULL;
void FreeNameFuncGlobals(void);
bool LoadSymbols(const char *filename);

#else

void *h_Library = NULL;
char h_global_argv[GLOBAL_ARGV_SIZE];

#endif

enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;
char *g_argv;// HACK: TODO: UNDONE: REWRITE!
/* why going into so much trouble allocating that linear array?!
char g_szFakeArg1[FAKE_CMD_ARG_LEN];
char g_szFakeArg2[FAKE_CMD_ARG_LEN];
char g_szFakeArg3[FAKE_CMD_ARG_LEN];
char g_szFakeArg4[FAKE_CMD_ARG_LEN];*/
DLL_GLOBAL int g_iProtocolVersion = 0;

extern DLL_FUNCTIONS gFunctionTable;
extern NEW_DLL_FUNCTIONS gNewFunctionTable;

GIVEFNPTRSTODLL other_GiveFnptrsToDll = NULL;
APIFUNCTION other_GetEntityAPI = NULL;
APIFUNCTION2 other_GetEntityAPI2 = NULL;// XBM: ???
NEW_DLL_FUNCTIONS_FN other_GetNewDLLFunctions = NULL;
SERVER_GETBLENDINGINTERFACE other_Server_GetBlendingInterface = NULL;// HPB40
SAVEGAMECOMMENTFN other_SaveGameComment;// XDM3038c

pfnXHL_Classify_t				XDM_Classify = NULL;
pfnXHL_EntityIs_t				XDM_EntityIs = NULL;
pfnXHL_EntityRelationship_t		XDM_EntityRelationship = NULL;
pfnXHL_EntityObjectCaps_t		XDM_EntityObjectCaps = NULL;
pfnXHL_CanHaveItem_t			XDM_CanHaveItem = NULL;
pfnXHL_GetActiveGameRules_t		XDM_GetActiveGameRules = NULL;

extern DLL_FUNCTIONS other_gFunctionTable;
extern NEW_DLL_FUNCTIONS other_gNewFunctionTable;// XDM

//cvar_t g_Architecture = { "sv_dllarch", "i486", FCVAR_SERVER|FCVAR_SPONLY, 0.0f, NULL };// XDM3035
char g_Architecture[16];// XBM: server DLL postfix


#ifndef __linux__
// Required DLL entry point
int WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	DBG_PRINTF("XBM: DllMain(%d)\n", fdwReason);
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		memset(&other_gFunctionTable, 0, sizeof(DLL_FUNCTIONS));
		memset(&other_gNewFunctionTable, 0, sizeof(NEW_DLL_FUNCTIONS));
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		WaypointInit();// Frees waypoint data
		FreeNameFuncGlobals();  // Free exported symbol table

		if (h_Library)
		{
			FreeLibrary(h_Library);
			h_Library = NULL;
		}
		if (h_global_argv)
		{
			GlobalUnlock(h_global_argv);
			GlobalFree(h_global_argv);
			h_global_argv = NULL;
		}
	}
	return TRUE;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Copy engine's function pointer to LOCAL g_engfuncs for everyday use and provide proxy pointers to the MOD DLL
// Input  : *pengfuncsFromEngine - 
//			*pGlobals - 
//-----------------------------------------------------------------------------
extern "C" void DLLEXPORT STDCALL GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals)
{
	char mod_name[32];
	char game_dir[MAX_PATH];
	char game_dll_filename[MAX_PATH];

	gpGlobals = pGlobals;

	// Patented X-Half-Life(tm) Half-Life(r)(tm) server version check algorithm(c)!
	g_iProtocolVersion = 46;// HL1110-, safest, default
	int exe_build = 0;
	const char *version_string = (*pengfuncsFromEngine->pfnCVarGetString)("sv_version");// "sv_version" is "0123456789abcdefghijklmnopqrstu,48,4554"
	if (version_string)
	{
		//char PatchVersion[32];// from XDM/steam.inf, max 32 chars
		//fail	sscanf(version_string, "%s,%d,%d", PatchVersion, &g_iProtocolVersion, &exe_build);// == 3?
		const char *protocol_ver_str = strchr(version_string, ',');// safe: HL is smart enough to filter all ',' from these values
		if (protocol_ver_str)
			if (sscanf(protocol_ver_str, ",%d,%d", &g_iProtocolVersion, &exe_build) != 2)
				printf("XBM: error parsing engine version string\n");
	}

	// now use proper preset
	size_t engineFunctionsSize;
#if defined(SVDLL_NEWFUNCTIONS)// now it's safe to define it for all HL versions
	if (g_iProtocolVersion >= 48)// HL1121
		engineFunctionsSize = sizeof(enginefuncs_t);
	else if (g_iProtocolVersion == 47)// HL1120
		engineFunctionsSize = offsetof(enginefuncs_t, pfnQueryClientCvarValue);
	else// HL1110
		engineFunctionsSize = offsetof(enginefuncs_t, pfnSequenceGet);
#else
		engineFunctionsSize = sizeof(enginefuncs_t);
#endif

	char *str = UTIL_VarArgs("XBM: GiveFnptrsToDll(): using enginefuncs_t size %d (assuming protocol %d)\n", engineFunctionsSize, g_iProtocolVersion);
	(*pengfuncsFromEngine->pfnServerPrint)(str);
	printf(str);

	memcpy(&g_engfuncs, pengfuncsFromEngine, engineFunctionsSize);
	// Now we can call engine API functions
	if (engineFunctionsSize < sizeof(enginefuncs_t))
		SERVER_PRINT("XBM will provide unavailable engine function stubs to the mod DLL\n");

	// find the directory name of the currently running MOD...
	GET_GAME_DIR(game_dir);
	game_dir[MAX_PATH-1] = '\0';
	int pos = 0;
	if (strstr(game_dir, "/") != NULL)
	{
		pos = strlen(game_dir) - 1;
		// scan backwards till first directory separator...
		while ((pos) && (game_dir[pos] != '/'))
			--pos;
		
		if (pos == 0)// Error getting directory name!
			ALERT(at_warning, "XBM: Error determining MOD directory name!\n");

		++pos;
	}
	strcpy(mod_name, &game_dir[pos]);
	game_dll_filename[0] = 0;
	memset(g_Architecture, 0, sizeof(g_Architecture));
	char *pArcFileName = "dlls/XBM.arc";
	//strcpy(g_Architecture, "i386"); don't add architecture by default
	int len = 0;
	byte *pBuf = LOAD_FILE_FOR_ME(pArcFileName, &len);
	if (pBuf)
	{
		strncpy(g_Architecture, (char *)pBuf, min(len,sizeof(g_Architecture)));
		g_Architecture[15] = '\0';
		len = strlen(g_Architecture);// for later use
		FREE_FILE(pBuf);
		pBuf = NULL;
	}
	else
		SERVER_PRINT(UTIL_VarArgs("XBM: Unable to load %s\n", pArcFileName));

	if (_stricmp(mod_name, "valve") == 0)
		mod_id = GAME_VALVE_DLL;
	else if (_stricmp(mod_name, "XDM") == 0)// XBM
		mod_id = GAME_XDM_DLL;
	else
		mod_id = GAME_UNKNOWN;

	char *pDLLName = NULL;
#if defined (SVDLL_NEWFUNCTIONS)// this method is unavailable in older engine versions
	if ((*pengfuncsFromEngine->pfnCheckParm)("-modsvdll", &pDLLName) > 0)
		_snprintf(game_dll_filename, MAX_PATH, "%s/dlls/%s", mod_name, pDLLName);
	else
#endif
	{
		// we couldn't get the DLL name from command line argumants
		if (mod_id == GAME_XDM_DLL)
			pDLLName = "XDM";
		else
			pDLLName = "hl";

#if defined (_WIN32)
		_snprintf(game_dll_filename, MAX_PATH, "%s/dlls/%s%s%s.dll", mod_name, pDLLName, g_Architecture[0]!=0?"_":"", g_Architecture);
#else
		_snprintf(game_dll_filename, MAX_PATH, "%s/dlls/%s%s%s.so", mod_name, pDLLName, g_Architecture[0]!=0?"_":"", g_Architecture);
#endif
	}

	if (game_dll_filename[0])
	{
		//SERVER_PRINT(game_dll_filename);
		conprintf(1, "XBM: loading \"%s\"\n\n", game_dll_filename);
#if defined (_WIN32)
		h_Library = LoadLibrary(game_dll_filename);
#else
		h_Library = dlopen(game_dll_filename, RTLD_NOW);
#endif
	}
	else
	{
		ALERT(at_error, "XBM: Mod dll not found or unsupported!\n");
		return;
	}

	if (h_Library == NULL)
	{
		ALERT(at_error, "XBM: Unable to load \"%s\"!\n", game_dll_filename);
		pGlobals->maxClients = 0;
		END_SECTION("_oem_end_logo");//(*g_engfuncs.pfnEndSection)
		memset(pengfuncsFromEngine, 0, sizeof(enginefuncs_t));
		return;
	}

#if defined (_WIN32)
	h_global_argv = GlobalAlloc(GMEM_SHARE, GLOBAL_ARGV_SIZE);
	g_argv = (char *)GlobalLock(h_global_argv);
#else
	g_argv = (char *)h_global_argv;
#endif

	other_GiveFnptrsToDll = (GIVEFNPTRSTODLL)GetProcAddress(h_Library, "GiveFnptrsToDll");
	if (other_GiveFnptrsToDll == NULL)// Can't find GiveFnptrsToDll!
		ALERT(at_error, "XBM: Can't get GiveFnptrsToDll!\n");

	other_GetEntityAPI = (APIFUNCTION)GetProcAddress(h_Library, "GetEntityAPI");
	if (other_GetEntityAPI == NULL)// Can't find GetEntityAPI!
		ALERT(at_warning, "XBM: Can't get GetEntityAPI!\n");

	other_GetEntityAPI2 = (APIFUNCTION2)GetProcAddress(h_Library, "GetEntityAPI2");
	if (other_GetEntityAPI2 == NULL)// Can't find GetEntityAPI2!
		conprintf(1, "XBM: Can't get GetEntityAPI2!\n");

	other_GetNewDLLFunctions = (NEW_DLL_FUNCTIONS_FN)GetProcAddress(h_Library, "GetNewDLLFunctions");
	if (other_GetNewDLLFunctions == NULL)// Can't find GetNewDLLFunctions!
		conprintf(1, "XBM: Can't get GetNewDLLFunctions!\n");

	other_Server_GetBlendingInterface = (SERVER_GETBLENDINGINTERFACE)GetProcAddress(h_Library, "Server_GetBlendingInterface");
	if (other_Server_GetBlendingInterface == NULL)// purely optional
		conprintf(1, "XBM: Can't get GetBlendingInterface!\n");

	other_SaveGameComment = (SAVEGAMECOMMENTFN)GetProcAddress(h_Library, "SV_SaveGameComment");
	if (other_SaveGameComment == NULL)// purely optional
		conprintf(1, "XBM: Can't get SV_SaveGameComment!\n");

	// XDM3036: extended server DLL API from XHL
	if (mod_id == GAME_XDM_DLL)
	{
		conprintf(1, "XBM: getting XHL-specific API...\n");
		XDM_Classify			= (pfnXHL_Classify_t)GetProcAddress(h_Library, "Classify");
		XDM_EntityIs			= (pfnXHL_EntityIs_t)GetProcAddress(h_Library, "EntityIs");
		XDM_EntityRelationship	= (pfnXHL_EntityRelationship_t)GetProcAddress(h_Library, "EntityRelationship");
		XDM_EntityObjectCaps	= (pfnXHL_EntityObjectCaps_t)GetProcAddress(h_Library, "EntityObjectCaps");
		XDM_CanHaveItem			= (pfnXHL_CanHaveItem_t)GetProcAddress(h_Library, "CanHaveItem");
		XDM_GetActiveGameRules	= (pfnXHL_GetActiveGameRules_t)GetProcAddress(h_Library, "GetActiveGameRules");
	}

#ifndef __linux__
	if (!LoadSymbols(game_dll_filename))// Load exported symbol table
		return;
#endif

	// Now replace original engine function pointers with local proxies
	pengfuncsFromEngine->pfnCmd_Args = Cmd_Args;
	pengfuncsFromEngine->pfnCmd_Argv = Cmd_Argv;
	pengfuncsFromEngine->pfnCmd_Argc = Cmd_Argc;
	pengfuncsFromEngine->pfnPrecacheModel = pfnPrecacheModel;
	pengfuncsFromEngine->pfnPrecacheSound = pfnPrecacheSound;
	pengfuncsFromEngine->pfnSetModel = pfnSetModel;
	pengfuncsFromEngine->pfnModelIndex = pfnModelIndex;
	pengfuncsFromEngine->pfnModelFrames = pfnModelFrames;
	pengfuncsFromEngine->pfnSetSize = pfnSetSize;
	pengfuncsFromEngine->pfnChangeLevel = pfnChangeLevel;
	pengfuncsFromEngine->pfnGetSpawnParms = pfnGetSpawnParms;
	pengfuncsFromEngine->pfnSaveSpawnParms = pfnSaveSpawnParms;
	pengfuncsFromEngine->pfnVecToYaw = pfnVecToYaw;
	pengfuncsFromEngine->pfnVecToAnglesHL = pfnVecToAngles;
	pengfuncsFromEngine->pfnMoveToOrigin = pfnMoveToOrigin;
	pengfuncsFromEngine->pfnChangeYaw = pfnChangeYaw;
	pengfuncsFromEngine->pfnChangePitch = pfnChangePitch;
	pengfuncsFromEngine->pfnFindEntityByString = pfnFindEntityByString;
	pengfuncsFromEngine->pfnGetEntityIllum = pfnGetEntityIllum;
	pengfuncsFromEngine->pfnFindEntityInSphere = pfnFindEntityInSphere;
	pengfuncsFromEngine->pfnFindClientInPVS = pfnFindClientInPVS;
	pengfuncsFromEngine->pfnEntitiesInPVS = pfnEntitiesInPVS;
	pengfuncsFromEngine->pfnMakeVectors = pfnMakeVectors;
	pengfuncsFromEngine->pfnAngleVectors = pfnAngleVectors;
	pengfuncsFromEngine->pfnCreateEntity = pfnCreateEntity;
	pengfuncsFromEngine->pfnRemoveEntity = pfnRemoveEntity;
	pengfuncsFromEngine->pfnCreateNamedEntity = pfnCreateNamedEntity;
	pengfuncsFromEngine->pfnMakeStatic = pfnMakeStatic;
	pengfuncsFromEngine->pfnEntIsOnFloor = pfnEntIsOnFloor;
	pengfuncsFromEngine->pfnDropToFloor = pfnDropToFloor;
	pengfuncsFromEngine->pfnWalkMove = pfnWalkMove;
	pengfuncsFromEngine->pfnSetOrigin = pfnSetOrigin;
	pengfuncsFromEngine->pfnEmitSound = pfnEmitSound;
	pengfuncsFromEngine->pfnEmitAmbientSound = pfnEmitAmbientSound;
	pengfuncsFromEngine->pfnTraceLine = pfnTraceLine;
	pengfuncsFromEngine->pfnTraceToss = pfnTraceToss;
	pengfuncsFromEngine->pfnTraceMonsterHull = pfnTraceMonsterHull;
	pengfuncsFromEngine->pfnTraceHull = pfnTraceHull;
	pengfuncsFromEngine->pfnTraceModel = pfnTraceModel;
	pengfuncsFromEngine->pfnTraceTexture = pfnTraceTexture;
	pengfuncsFromEngine->pfnTraceSphere = pfnTraceSphere;
	pengfuncsFromEngine->pfnGetAimVector = pfnGetAimVector;
	pengfuncsFromEngine->pfnServerCommand = pfnServerCommand;
	pengfuncsFromEngine->pfnServerExecute = pfnServerExecute;
	pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;
	pengfuncsFromEngine->pfnParticleEffect = pfnParticleEffect;
	pengfuncsFromEngine->pfnLightStyle = pfnLightStyle;
	pengfuncsFromEngine->pfnDecalIndex = pfnDecalIndex;
	pengfuncsFromEngine->pfnPointContents = pfnPointContents;
	pengfuncsFromEngine->pfnMessageBegin = pfnMessageBegin;
	pengfuncsFromEngine->pfnMessageEnd = pfnMessageEnd;
	pengfuncsFromEngine->pfnWriteByte = pfnWriteByte;
	pengfuncsFromEngine->pfnWriteChar = pfnWriteChar;
	pengfuncsFromEngine->pfnWriteShort = pfnWriteShort;
	pengfuncsFromEngine->pfnWriteLong = pfnWriteLong;
	pengfuncsFromEngine->pfnWriteAngle = pfnWriteAngle;
	pengfuncsFromEngine->pfnWriteCoord = pfnWriteCoord;
	pengfuncsFromEngine->pfnWriteString = pfnWriteString;
	pengfuncsFromEngine->pfnWriteEntity = pfnWriteEntity;
	pengfuncsFromEngine->pfnCVarRegister = pfnCVarRegister;
	pengfuncsFromEngine->pfnCVarGetFloat = pfnCVarGetFloat;
	pengfuncsFromEngine->pfnCVarGetString = pfnCVarGetString;
	pengfuncsFromEngine->pfnCVarSetFloat = pfnCVarSetFloat;
	pengfuncsFromEngine->pfnCVarSetString = pfnCVarSetString;
	pengfuncsFromEngine->pfnPvAllocEntPrivateData = pfnPvAllocEntPrivateData;
	pengfuncsFromEngine->pfnPvEntPrivateData = pfnPvEntPrivateData;
	pengfuncsFromEngine->pfnFreeEntPrivateData = pfnFreeEntPrivateData;
	pengfuncsFromEngine->pfnSzFromIndex = pfnSzFromIndex;
	pengfuncsFromEngine->pfnAllocString = pfnAllocString;
	pengfuncsFromEngine->pfnGetVarsOfEnt = pfnGetVarsOfEnt;
	pengfuncsFromEngine->pfnPEntityOfEntOffset = pfnPEntityOfEntOffset;
	pengfuncsFromEngine->pfnEntOffsetOfPEntity = pfnEntOffsetOfPEntity;
	pengfuncsFromEngine->pfnIndexOfEdict = pfnIndexOfEdict;
	pengfuncsFromEngine->pfnPEntityOfEntIndex = pfnPEntityOfEntIndex;
	pengfuncsFromEngine->pfnFindEntityByVars = pfnFindEntityByVars;
	pengfuncsFromEngine->pfnGetModelPtr = pfnGetModelPtr;
	pengfuncsFromEngine->pfnRegUserMsg = pfnRegUserMsg;
	pengfuncsFromEngine->pfnAnimationAutomove = pfnAnimationAutomove;
	pengfuncsFromEngine->pfnGetBonePosition = pfnGetBonePosition;
	pengfuncsFromEngine->pfnFunctionFromName = pfnFunctionFromName;
	pengfuncsFromEngine->pfnNameForFunction = pfnNameForFunction;
	pengfuncsFromEngine->pfnClientPrintf = pfnClientPrintf;
	pengfuncsFromEngine->pfnServerPrint = pfnServerPrint;
	pengfuncsFromEngine->pfnCmd_Args = Cmd_Args;
	pengfuncsFromEngine->pfnCmd_Argv = Cmd_Argv;
	pengfuncsFromEngine->pfnCmd_Argc = Cmd_Argc;
	pengfuncsFromEngine->pfnGetAttachment = pfnGetAttachment;
	pengfuncsFromEngine->pfnCRC32_Init = pfnCRC32_Init;
	pengfuncsFromEngine->pfnCRC32_ProcessBuffer = pfnCRC32_ProcessBuffer;
	pengfuncsFromEngine->pfnCRC32_ProcessByte = pfnCRC32_ProcessByte;
	pengfuncsFromEngine->pfnCRC32_Final = pfnCRC32_Final;
	pengfuncsFromEngine->pfnRandomLong = pfnRandomLong;
	pengfuncsFromEngine->pfnRandomFloat = pfnRandomFloat;
	pengfuncsFromEngine->pfnSetView = pfnSetView;
	pengfuncsFromEngine->pfnTime = pfnTime;
	pengfuncsFromEngine->pfnCrosshairAngle = pfnCrosshairAngle;
	pengfuncsFromEngine->pfnLoadFileForMe = pfnLoadFileForMe;
	pengfuncsFromEngine->pfnFreeFile = pfnFreeFile;
	pengfuncsFromEngine->pfnEndSection = pfnEndSection;
	pengfuncsFromEngine->pfnCompareFileTime = pfnCompareFileTime;
	pengfuncsFromEngine->pfnGetGameDir = pfnGetGameDir;
	pengfuncsFromEngine->pfnCvar_RegisterVariable = pfnCvar_RegisterVariable;
	pengfuncsFromEngine->pfnFadeClientVolume = pfnFadeClientVolume;
	pengfuncsFromEngine->pfnSetClientMaxspeed = pfnSetClientMaxspeed;
	pengfuncsFromEngine->pfnCreateFakeClient = pfnCreateFakeClient;
	pengfuncsFromEngine->pfnRunPlayerMove = pfnRunPlayerMove;
	pengfuncsFromEngine->pfnNumberOfEntities = pfnNumberOfEntities;
	pengfuncsFromEngine->pfnGetInfoKeyBuffer = pfnGetInfoKeyBuffer;
	pengfuncsFromEngine->pfnInfoKeyValue = pfnInfoKeyValue;
	pengfuncsFromEngine->pfnSetKeyValue = pfnSetKeyValue;
	pengfuncsFromEngine->pfnSetClientKeyValue = pfnSetClientKeyValue;
	pengfuncsFromEngine->pfnIsMapValid = pfnIsMapValid;
	pengfuncsFromEngine->pfnStaticDecal = pfnStaticDecal;
	pengfuncsFromEngine->pfnPrecacheGeneric = pfnPrecacheGeneric;
	pengfuncsFromEngine->pfnGetPlayerUserId = pfnGetPlayerUserId;
	pengfuncsFromEngine->pfnBuildSoundMsg = pfnBuildSoundMsg;
	pengfuncsFromEngine->pfnIsDedicatedServer = pfnIsDedicatedServer;
	pengfuncsFromEngine->pfnCVarGetPointer = pfnCVarGetPointer;
	pengfuncsFromEngine->pfnGetPlayerWONId = pfnGetPlayerWONId;

	// SDK 2.0 additions...
	pengfuncsFromEngine->pfnInfo_RemoveKey = pfnInfo_RemoveKey;
	pengfuncsFromEngine->pfnGetPhysicsKeyValue = pfnGetPhysicsKeyValue;
	pengfuncsFromEngine->pfnSetPhysicsKeyValue = pfnSetPhysicsKeyValue;
	pengfuncsFromEngine->pfnGetPhysicsInfoString = pfnGetPhysicsInfoString;
	pengfuncsFromEngine->pfnPrecacheEvent = pfnPrecacheEvent;
	pengfuncsFromEngine->pfnPlaybackEvent = pfnPlaybackEvent;
	pengfuncsFromEngine->pfnSetFatPVS = pfnSetFatPVS;
	pengfuncsFromEngine->pfnSetFatPAS = pfnSetFatPAS;
	pengfuncsFromEngine->pfnCheckVisibility = pfnCheckVisibility;
	pengfuncsFromEngine->pfnDeltaSetField = pfnDeltaSetField;
	pengfuncsFromEngine->pfnDeltaUnsetField = pfnDeltaUnsetField;
	pengfuncsFromEngine->pfnDeltaAddEncoder = pfnDeltaAddEncoder;
	pengfuncsFromEngine->pfnGetCurrentPlayer = pfnGetCurrentPlayer;
	pengfuncsFromEngine->pfnCanSkipPlayer = pfnCanSkipPlayer;
	pengfuncsFromEngine->pfnDeltaFindField = pfnDeltaFindField;
	pengfuncsFromEngine->pfnDeltaSetFieldByIndex = pfnDeltaSetFieldByIndex;
	pengfuncsFromEngine->pfnDeltaUnsetFieldByIndex = pfnDeltaUnsetFieldByIndex;
	pengfuncsFromEngine->pfnSetGroupMask = pfnSetGroupMask;
	pengfuncsFromEngine->pfnCreateInstancedBaseline = pfnCreateInstancedBaseline;
	pengfuncsFromEngine->pfnCvar_DirectSet = pfnCvar_DirectSet;
	pengfuncsFromEngine->pfnForceUnmodified = pfnForceUnmodified;
	pengfuncsFromEngine->pfnGetPlayerStats = pfnGetPlayerStats;
	pengfuncsFromEngine->pfnAddServerCommand = pfnAddServerCommand;

	// SDK 2.3 additions...
	pengfuncsFromEngine->pfnVoice_GetClientListening = pfnVoice_GetClientListening;
	pengfuncsFromEngine->pfnVoice_SetClientListening = pfnVoice_SetClientListening;
	pengfuncsFromEngine->pfnGetPlayerAuthId = pfnGetPlayerAuthId;

#if defined (SVDLL_NEWFUNCTIONS)
	// MOD DLL calls these only if they were designed to use protocol 47+. We don't provide any crash-proof mechanism as it's useless anyway.
	//if (g_iProtocolVersion >= 47)
	//{
		// These are NOT stubs!
		pengfuncsFromEngine->pfnSequenceGet = pfnSequenceGet;
		pengfuncsFromEngine->pfnSequencePickSentence = pfnSequencePickSentence;
		pengfuncsFromEngine->pfnGetFileSize = pfnGetFileSize;
		pengfuncsFromEngine->pfnGetApproxWavePlayLen = pfnGetApproxWavePlayLen;
		pengfuncsFromEngine->pfnIsCareerMatch = pfnIsCareerMatch;
		pengfuncsFromEngine->pfnGetLocalizedStringLength = pfnGetLocalizedStringLength;
		pengfuncsFromEngine->pfnRegisterTutorMessageShown = pfnRegisterTutorMessageShown;
		pengfuncsFromEngine->pfnGetTimesTutorMessageShown = pfnGetTimesTutorMessageShown;
		pengfuncsFromEngine->ProcessTutorMessageDecayBuffer = pfnProcessTutorMessageDecayBuffer;// HL20130901 weird names
		pengfuncsFromEngine->ConstructTutorMessageDecayBuffer = pfnConstructTutorMessageDecayBuffer;
		pengfuncsFromEngine->ResetTutorMessageDecayData = pfnResetTutorMessageDecayData;
		//if (g_iProtocolVersion >= 48)
		//{
			pengfuncsFromEngine->pfnQueryClientCvarValue = pfnQueryClientCvarValue;
			pengfuncsFromEngine->pfnQueryClientCvarValue2 = pfnQueryClientCvarValue2;
			pengfuncsFromEngine->pfnCheckParm = pfnCheckParm;// HL20130901
		//}
	//}
#endif

	// give the engine functions to the other DLL...
	if (other_GiveFnptrsToDll)
		(*other_GiveFnptrsToDll)(pengfuncsFromEngine, pGlobals);
}


//-----------------------------------------------------------------------------
// Purpose: Old version, used only if GetEntityAPI2 is not available
// Input  : *pFunctionTable - empty function table to be filled by game DLL
//			interfaceVersion - engine interface version
// Output : int 0 = failure
//-----------------------------------------------------------------------------
extern "C" EXPORT int GetEntityAPI(DLL_FUNCTIONS *pFunctionTable, int interfaceVersion)
{
#if defined (_DEBUG)
	if (pFunctionTable == NULL)
	{
		printf("XBM: GetEntityAPI(): pFunctionTable == NULL!\n");
		return 0;
	}
#endif
	if (interfaceVersion != INTERFACE_VERSION)
	{
		printf("XBM: GetEntityAPI(): incompatible interface version %d! (local %d)\n", interfaceVersion, INTERFACE_VERSION);
		return 0;
	}

	// pass engine callback function table to engine...
	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));

	if (other_GetEntityAPI == NULL)
	{
		printf("XBM: game DLL does not have GetEntityAPI!\n");
		return 0;
	}
	// pass other DLLs engine callbacks to function table...
	//if (!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION))
	//	return 0;  // error initializing function table!!!

	return (*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION);
}

//-----------------------------------------------------------------------------
// Purpose: Called first in new engine versions, allows DLL to return version
// Input  : *pFunctionTable - empty function table to be filled by game DLL
//			*interfaceVersion - in: engine interface version for check,
//					out: must be set after that to game DLL inverface version
// Output : int 0 = failure
//-----------------------------------------------------------------------------
extern "C" EXPORT int GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
#if defined (_DEBUG)
	if (pFunctionTable == NULL || interfaceVersion == NULL)
	{
		printf("XBM: GetEntityAPI2(): bad arguments!\n");
		return 0;
	}
#endif
	if (*interfaceVersion != INTERFACE_VERSION)
	{
		printf("XBM: GetEntityAPI2(): incompatible interface version %d! (local %d)\n", *interfaceVersion, INTERFACE_VERSION);
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return 0;// should we?
	}

	// pass engine callback function table to engine...
	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));// XDM3035 this was a stupid FAIL, because engine DOES NOT call GetEntityAPI if it finds GetEntityAPI2 first!

	if (other_GetEntityAPI2 == NULL)
	{
		printf("XBM: game DLL does not have GetEntityAPI2!\n");
		return 0;// should we?
	}

	// This part is tricky: we overwrite function pointer array, but the game dll will try to overwrite it again, so supply custom array instead
	return (*other_GetEntityAPI2)(&other_gFunctionTable, interfaceVersion);
}


//-----------------------------------------------------------------------------
// Purpose: This API is optional, so don't crash if game DLL does not support it
// Input  : *pFunctionTable - empty function table to be filled by game DLL
//			*interfaceVersion - in: engine interface version for check,
//					out: must be set after that to game DLL inverface version
// Output : int 0 = failure
//-----------------------------------------------------------------------------
extern "C" EXPORT int GetNewDLLFunctions(NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
#if defined (_DEBUG)
	if (pFunctionTable == NULL || interfaceVersion == NULL)
	{
		printf("XBM: GetNewDLLFunctions(): bad arguments!\n");
		return 0;
	}
#endif
	if (*interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		printf("XBM: GetNewDLLFunctions(): incompatible interface version %d! (local %d)\n", *interfaceVersion, NEW_DLL_FUNCTIONS_VERSION);
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return 0;
	}

	//if (other_GetNewDLLFunctions == NULL)
	//	other_GetNewDLLFunctions = (GETNEWDLLFUNCTIONS)GetProcAddress(h_Library, "GetNewDLLFunctions");
	//if (other_GetNewDLLFunctions == NULL)// Can't find GetNewDLLFunctions!
	//	ALERT(at_warning, "XBM: Can't get GetNewDLLFunctions!\n" );
	/*printf("XBM: GetNewDLLFunctions(): size = %d, version = %d\n", sizeof(gNewFunctionTable), *interfaceVersion);

	memcpy(pFunctionTable, &gNewFunctionTable, sizeof(gNewFunctionTable));*/
	if (other_GetNewDLLFunctions == NULL)// child mod server DLL does not have GetNewDLLFunctions API
	{
		printf("XBM: game DLL does not require GetNewDLLFunctions.\n");
		return 0;// TESTME: what will HL do?
	}

	// This part is tricky: we overwrite function pointer array, but the game dll will try to overwrite it again, so supply custom array instead
	return (*other_GetNewDLLFunctions)(/*other_gNewFunctionTable*/pFunctionTable, interfaceVersion);
}


//-----------------------------------------------------------------------------
// Purpose: This API is optional, so don't crash if game DLL does not support it
// Input  : version - 
//			**ppinterface - 
//			*pstudio - 
//			(*rotationmatrix - 
// Output : int 0 = failure
//-----------------------------------------------------------------------------
int EXPORT STDCALL Server_GetBlendingInterface(int version, struct sv_blending_interface_s **ppinterface, struct engine_studio_api_s *pstudio, float (*rotationmatrix)[3][4], float (*bonetransform)[MAXSTUDIOBONES][3][4])
{
	//static SERVER_GETBLENDINGINTERFACE other_Server_GetBlendingInterface = NULL;
	static bool missing = false;

	// if the blending interface has been formerly reported as missing, give up
	if (missing)
		return 0;

	// do we NOT know if the blending interface is provided? if so, look for its address
	//if (other_Server_GetBlendingInterface == NULL)
	//	other_Server_GetBlendingInterface = (SERVER_GETBLENDINGINTERFACE)GetProcAddress(h_Library, "Server_GetBlendingInterface");

	// have we NOT found it ?
	if (other_Server_GetBlendingInterface == NULL)
	{
		//conprintf(1, "XBM: Can't get GetBlendingInterface!\n" );
		printf("XBM: game DLL does not require GetBlendingInterface.\n");
		missing = true;// then mark it as missing, no use to look for it again in the future
		return 0;// and give up
	}

	// else call the function that provides the blending interface on request
	return (*other_Server_GetBlendingInterface)(version, ppinterface, pstudio, rotationmatrix, bonetransform);
}

//-----------------------------------------------------------------------------
// Purpose: This API is optional, so don't crash if game DLL does not support it
// Warning: This funciton is called often.
// Input  : *pszBuffer - output buffer
//			iSizeBuffer - output buffer size
//-----------------------------------------------------------------------------
extern "C" void EXPORT STDCALL SV_SaveGameComment(char *pszBuffer, int iSizeBuffer)
{
	if (other_SaveGameComment == NULL)
	{
		//conprintf(1, "XBM: Can't get SaveGameComment!\n" );
		//printf("XBM: game DLL does not require SaveGameComment.\n");
		// STUB! We cannot un-export this function if current mod does not have it, so we have to emulate it here or the engine will get empty buffer.
		strncpy(pszBuffer, STRING(gpGlobals->mapname), iSizeBuffer-1);
		pszBuffer[iSizeBuffer-1] = 0;
		return;// and give up
	}

	// else call the function that provides the interface on request
	return (*other_SaveGameComment)(pszBuffer, iSizeBuffer);
}
