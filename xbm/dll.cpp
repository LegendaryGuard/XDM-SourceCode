//====================================================================
// Purpose: DLL loading, DLL API functions
//====================================================================
#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"
#include "entity_state.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "bot.h"
#include "bot_func.h"
#include "bot_cvar.h"
#include "weapondef.h"
//#include "pm_shared.h"

extern APIFUNCTION other_GetEntityAPI;
extern APIFUNCTION2 other_GetEntityAPI2;
extern NEW_DLL_FUNCTIONS_FN other_GetNewDLLFunctions;
extern SERVER_GETBLENDINGINTERFACE other_Server_GetBlendingInterface;// HPB40
extern SAVEGAMECOMMENTFN other_SaveGameComment;// XDM3038c

extern enginefuncs_t g_engfuncs;
extern char *g_argv;

DLL_FUNCTIONS other_gFunctionTable;// pointers to mod DLL functions
NEW_DLL_FUNCTIONS other_gNewFunctionTable;// pointers to mod DLL functions
// XDM3038a: DLL_GLOBAL const Vector g_vecZero(0.0f,0.0f,0.0f);
DLL_GLOBAL short mod_id = GAME_UNKNOWN;
DLL_GLOBAL short IsDedicatedServer = 0;
DLL_GLOBAL short g_ServerActive = false;
DLL_GLOBAL playermove_t *pmove = NULL;

int g_iModelIndexAnimglow01 = 0;
int g_iModelIndexLaser = 0;

int g_iGetWeaponDataPlayerIndex = 0;// XDM3038b

int fake_arg_count = 0;
float bot_check_time = 60.0;
//int min_bots = -1;
//int max_bots = -1;
short num_bots = 0;
short prev_num_bots = 0;
//edict_t *clients[MAX_PLAYERS+1];// XDM3037a: nobody uses this
edict_t *listenserver_edict = NULL;

bool g_BotStop = false;
bool g_GameRules = false;

bool isFakeClientCommand = false;
bool spawn_time_reset = false;

// XDM3037a
char g_szConfigFileName[] = "botmatch.cfg";
float respawn_time = 0.0;

// TheFatal's method for calculating the msecval
int msecnum = 0;
float msecdel = 0.0f;
float msecval = 0.0f;
// Now these are global


//cvar_t sv_bot					= { "HPB_bot", "" };
cvar_t g_bot_allow_commands		= { "bot_allow_commands",		"1",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_reaction			= { "bot_reaction",				"2",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_skill				= { "bot_skill",				"2",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_strafe_percent		= { "bot_strafe_percent",		"20",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_chat_enable		= { "bot_chat_enable",			"1",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_chat_percent		= { "bot_chat_percent",			"10",		FCVAR_SERVER|FCVAR_EXTDLL };
/* XDM3038: useless cvar_t g_bot_chat_tag_percent	= { "bot_chat_tag_percent",		"80",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_chat_drop_percent	= { "bot_chat_drop_percent",	"10",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_chat_lower_percent	= { "bot_chat_lower_percent",	"50",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_chat_swap_percent	= { "bot_chat_swap_percent",	"10",		FCVAR_SERVER|FCVAR_EXTDLL };*/
cvar_t g_bot_taunt_percent		= { "bot_taunt_percent",		"20",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_whine_percent		= { "bot_whine_percent",		"10",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_grenade_time		= { "bot_grenade_time",			"15",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_logo_percent		= { "bot_logo_percent",			"40",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_logo_custom		= { "bot_logo_custom",			"1",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_dont_shoot			= { "bot_dont_shoot",			"0",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_check_lightlevel	= { "bot_check_lightlevel",		"0",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_use_flashlight		= { "bot_use_flashlight",		"0",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_use_entities		= { "bot_use_entities",			"1",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_follow_actions		= { "bot_follow_actions",		"1",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_follow_distance	= { "bot_follow_distance",		"100",		FCVAR_SERVER|FCVAR_EXTDLL };// XDM3038
cvar_t g_bot_default_model		= { "bot_default_model",		"helmet",	FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_minbots			= { "bot_minbots",				"0",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_maxbots			= { "bot_maxbots",				"32",		FCVAR_SERVER|FCVAR_EXTDLL };
#if defined (_DEBUG_BOTDLL)
cvar_t g_bot_debug				= { "bot_debug",				"1",		FCVAR_SERVER|FCVAR_EXTDLL };
#else
cvar_t g_bot_debug				= { "bot_debug",				"0",		FCVAR_SERVER|FCVAR_EXTDLL };
#endif
cvar_t g_botmatch				= { "botmatch",					"0",		FCVAR_SERVER|FCVAR_EXTDLL };
#if defined (_DEBUG)
cvar_t g_bot_superdebug			= { "bot_superdebug",			"0",		FCVAR_SERVER|FCVAR_EXTDLL };
cvar_t g_bot_test1				= { "bot_test1",				"0",		FCVAR_SERVER|FCVAR_EXTDLL };
#endif
#if defined (DEBUG_USERMSG)
cvar_t g_dbg_disable_usermsg	= { "dbg_disable_usermsg",		"0", FCVAR_SERVER|FCVAR_EXTDLL };
#endif
//cvar_t *g_pBotmatch = &g_botmatch;
cvar_t	*g_pCvarDeveloper = NULL;
cvar_t	*g_pmp_footsteps = NULL;
cvar_t 	*g_pmp_friendlyfire = NULL;
cvar_t 	*g_pmp_noshooting = NULL;
cvar_t 	*g_pmp_revengemode = NULL;
cvar_t 	*g_pmp_teammines = NULL;
cvar_t 	*g_psv_gravity = NULL;
cvar_t 	*g_psv_maxspeed = NULL;

#if defined (_DEBUG)
void XBM_SUPERDBG_PRINTF(const char *szMsg)
{
	if (g_bot_superdebug.value > 0.0f)
		SERVER_PRINT(szMsg);
}
#else
#define XBM_SUPERDBG_PRINTF// do nothing in release build
#endif

void Cmd_KickBots(void)
{
	for (int bot_index = 0; bot_index < MAX_PLAYERS; ++bot_index)
	{
		if (IsActiveBot(bot_index))
		{
			char cmd[64];
			_snprintf(cmd, 63, "kick \"%s\"\n", STRING(bots[bot_index].pEdict->v.netname));// "kick # %d" doesn't work 8( ENTINDEX(bots[bot_index].pEdict));
			cmd[63] = 0;
			bots[bot_index].autospawn_state = RESPAWN_NO;// don't attempt to autospawn
			SERVER_COMMAND(cmd);  // kick the bot using "kick # id"
			--num_bots;
		}
	}
	SERVER_EXECUTE();
}

void Cmd_BotCreate(void)
{
	if (gpGlobals->maxClients <= 1)
	{
		conprintf(1, "Botmatch is only available in multiplayer games.\n");
		return;
	}
	if (g_botmatch.value <= 0)
	{
		conprintf(1, "Botmatch is not allowed (%s is %g).\n", g_botmatch.name, g_botmatch.value);
		return;
	}
	const char *arg1 = CMD_ARGV(1);
	const char *arg2 = CMD_ARGV(2);
	if (!arg1 || !arg2)
	{
		conprintf(1, "Usage: %s <model> <name> [skill] [topcolor] [bottomcolor] [skin] [reaction time] [strafe %] [chat %] [taunt %] [whine %] [team name]\n", Cmd_Argv(0));
		return;
	}
	int skill = -1, topcolor = -1, bottomcolor = -1, skin = 0;
	int reaction_time = -1, strafe_prc = -1, chat_prc = -1, taunt_prc = -1, whine_prc = -1;
	const char *arg3 = CMD_ARGV(3);
	const char *arg4 = CMD_ARGV(4);
	const char *arg5 = CMD_ARGV(5);
	const char *arg6 = CMD_ARGV(6);
	const char *arg7 = CMD_ARGV(7);
	const char *arg8 = CMD_ARGV(8);
	const char *arg9 = CMD_ARGV(9);
	const char *arg10 = CMD_ARGV(10);
	const char *arg11 = CMD_ARGV(11);
	if (arg3 && *arg3) skill = atoi(arg3);
	if (arg4 && *arg4) topcolor = atoi(arg4);
	if (arg5 && *arg5) bottomcolor = atoi(arg5);
	if (arg6 && *arg6) skin = atoi(arg6);
	if (arg7 && *arg7) reaction_time = atoi(arg7);
	if (arg8 && *arg8) strafe_prc = atoi(arg8);
	if (arg9 && *arg9) chat_prc = atoi(arg9);
	if (arg10 && *arg10) taunt_prc = atoi(arg10);
	if (arg11 && *arg11) whine_prc = atoi(arg11);

	if (g_ServerActive)// XDM3037a: don't create bots if the server is starting up
		BOT_CREATE(-1, (char *)arg1, (char *)arg2, skill, topcolor, bottomcolor, skin, reaction_time, strafe_prc, chat_prc, taunt_prc, whine_prc, (char *)Cmd_Argv(12));
	else
		BOT_INIT(-1, (char *)arg1, (char *)arg2, skill, topcolor, bottomcolor, skin, reaction_time, strafe_prc, chat_prc, taunt_prc, whine_prc, (char *)Cmd_Argv(12));

	// XDM3037a: OBSOLETE	bot_cfg_pause_time = gpGlobals->time + 0.25f;
}

void Cmd_Waypoint(void)
{
	const char *arg1 = CMD_ARGV(1);
	if (!arg1)
	{
		conprintf(1, "Usage: %s <on|off|load|save|update>\n", Cmd_Argv(0));
		return;
	}

	if (FStrEq(arg1, "on"))
	{
		g_waypoint_on = true;
		conprintf(1, "Waypoints are ON\n");
	}
	else if (FStrEq(arg1, "off"))
	{
		g_waypoint_on = false;
		conprintf(1, "Waypoints are OFF\n");
	}
	/*else if (FStrEq(arg1, "add"))
	{
		if (!g_waypoint_on)
			g_waypoint_on = true;  // turn waypoints on if off

		WaypointAdd(pEntity);
	}
	else if (FStrEq(arg1, "delete"))
	{
		if (!g_waypoint_on)
			g_waypoint_on = true;  // turn waypoints on if off

		WaypointDelete(pEntity);
	}*/
	else if (FStrEq(arg1, "save"))
	{
		WaypointSave();
		conprintf(1, "Waypoints saved\n");
	}
	else if (FStrEq(arg1, "load"))
	{
		if (WaypointLoad())
			conprintf(1, "Waypoints loaded\n");
	}
	else if (FStrEq(arg1, "info"))
	{
		WaypointPrintSummary();
		//WaypointPrintInfo(pEntity);
	}
	else if (FStrEq(arg1, "update"))
	{
		conprintf(1, "Updating waypoint tags...\n");
		WaypointUpdate();
		conprintf(1, "...update done! (don't forget to save!)\n");
	}
	else
	{
		if (g_waypoint_on)
			conprintf(1, "Waypoints are ON\n");
		else
			conprintf(1, "Waypoints are OFF\n");
	}
}

// XDM3038: dbg
// ARGV(0) is Cmd_BotCmd(), ARGV(1) is bot index, ARGV(2) is the bot's command
// WARNING! Potential recursion point!
void Cmd_BotCmd(void)
{
	if ((!IsDedicatedServer) && (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f))
	{
		if (CMD_ARGC() >= 3 && CMD_ARGC() <= FAKE_CMD_ARGS+2)
		{
			edict_t *pClientEd = UTIL_ClientByIndex(atoi(CMD_ARGV(1)));
			if (pClientEd)
			{
				int bot_index = UTIL_GetBotIndex(pClientEd);
				if (bot_index >= 0 && IsActiveBot(bot_index))
				{
					const char *pcmd = CMD_ARGV(2);
					const char *arg1 = CMD_ARGV(3);
					const char *arg2 = CMD_ARGV(4);
					const char *arg3 = CMD_ARGV(5);
					// WARNING! Cmd_Argv() will return recursive trash here!!! Cache pointers first!
					FakeClientCommand(pClientEd, pcmd, arg1, arg2, arg3);
				}
			}
		}
		else
			SERVER_PRINT(UTIL_VarArgs("Usage: %s <client index> <cmd> [\"arg1\"] [\"arg2\"] [\"arg3\"]\n", CMD_ARGV(0)));
	}
}





//-----------------------------------------------------------------------------
// EntityAPI functions
//-----------------------------------------------------------------------------


void GameDLLInit(void)
{
	SERVER_PRINT(UTIL_VarArgs("Initializing XBM (based on HPB bot) server DLL (%s build %s)\n", BUILD_DESC, __DATE__));

	//cvar_t *pHostGameLoaded = CVAR_GET_POINTER("host_gameloaded");// Xash detection
	//if (pHostGameLoaded)
	//	SERVER_PRINT("Xash detected\n");
	g_pCvarDeveloper = CVAR_GET_POINTER("developer");

	SERVER_PRINT("Registering commands and variables...\n");
	//CVAR_REGISTER(&sv_bot);
	CVAR_REGISTER(&g_bot_allow_commands);
	CVAR_REGISTER(&g_bot_reaction);
	CVAR_REGISTER(&g_bot_skill);
	CVAR_REGISTER(&g_bot_strafe_percent);
	CVAR_REGISTER(&g_bot_chat_enable);
	CVAR_REGISTER(&g_bot_chat_percent);
	/* XDM3038: useless	CVAR_REGISTER(&g_bot_chat_tag_percent);
	CVAR_REGISTER(&g_bot_chat_drop_percent);
	CVAR_REGISTER(&g_bot_chat_lower_percent);
	CVAR_REGISTER(&g_bot_chat_swap_percent);*/
	CVAR_REGISTER(&g_bot_taunt_percent);
	CVAR_REGISTER(&g_bot_whine_percent);
	CVAR_REGISTER(&g_bot_grenade_time);
	CVAR_REGISTER(&g_bot_logo_percent);
	CVAR_REGISTER(&g_bot_logo_custom);
	CVAR_REGISTER(&g_bot_dont_shoot);
	CVAR_REGISTER(&g_bot_check_lightlevel);
	CVAR_REGISTER(&g_bot_use_flashlight);
	CVAR_REGISTER(&g_bot_use_entities);
	CVAR_REGISTER(&g_bot_follow_actions);
	CVAR_REGISTER(&g_bot_follow_distance);// XDM3038
	CVAR_REGISTER(&g_bot_default_model);
	CVAR_REGISTER(&g_bot_minbots);
	CVAR_REGISTER(&g_bot_maxbots);
	CVAR_REGISTER(&g_bot_debug);
	CVAR_REGISTER(&g_botmatch);
#if defined (_DEBUG)
	CVAR_REGISTER(&g_bot_superdebug);
	CVAR_REGISTER(&g_bot_test1);
#endif

	ADD_SERVER_COMMAND("bot_create", Cmd_BotCreate);
	ADD_SERVER_COMMAND("bot_kickall", Cmd_KickBots);
	ADD_SERVER_COMMAND("waypoint", Cmd_Waypoint);// XDM3037
	ADD_SERVER_COMMAND("bot_clientcmd", Cmd_BotCmd);// XDM3038: debug

	IsDedicatedServer = IS_DEDICATED_SERVER();

	SERVER_PRINT("Initializing...\n");
	//int i;
	//for (i=0; i<=MAX_PLAYERS; ++i)
	// XDM3037a		clients[i] = NULL;

	// initialize the bots array of structures...
	memset(bots, 0, sizeof(bot_t)*MAX_PLAYERS);
	// ^ respawn state is now RESPAWN_NO
	BotLogoInit();
	if (!BotWeaponInit())// XDM3035
	{
		SERVER_PRINT("BotWeaponInit() failed!\n");
		return;
	}
	LoadBotChat();

	SERVER_PRINT("Done. XBM DLL initialized.\n");
	(*other_gFunctionTable.pfnGameInit)();

	g_pmp_footsteps = CVAR_GET_POINTER("mp_footsteps");
	ASSERT(g_pmp_footsteps != NULL);
	g_pmp_friendlyfire = CVAR_GET_POINTER("mp_friendlyfire");
	ASSERT(g_pmp_friendlyfire != NULL);
	g_pmp_noshooting = CVAR_GET_POINTER("mp_noshooting");
	ASSERT(g_pmp_noshooting != NULL);
	g_pmp_revengemode = CVAR_GET_POINTER("mp_revengemode");
	ASSERT(g_pmp_revengemode != NULL);
	g_pmp_teammines = CVAR_GET_POINTER("mp_teammines");
	ASSERT(g_pmp_teammines != NULL);
	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	ASSERT(g_psv_gravity != NULL);
	g_psv_maxspeed = CVAR_GET_POINTER("sv_maxspeed");
	ASSERT(g_psv_maxspeed != NULL);
	//if (mod_id == GAME_XDM_DLL)// XDM3035
	//	g_pBotmatch = CVAR_GET_POINTER("bot_allow");
	//else
	//	g_pBotmatch = &g_botmatch;
}

int DispatchSpawn(edict_t *pent)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchSpawn(%d %s)\n", ENTINDEX(pent), STRING(pent->v.classname)));
#endif
	if (gpGlobals->deathmatch)
	{
		if (strcmp(STRING(pent->v.classname), "worldspawn") == 0)
		{
			// do level initialization stuff here...
			SERVER_PRINT("XBM: Initializing waypoints...\n");
			WaypointInit();
			WaypointLoad();
			PRECACHE_SOUND("weapons/xbow_hit1.wav");// waypoint add
			PRECACHE_SOUND("weapons/mine_activate.wav");// waypoint delete
			PRECACHE_SOUND("common/wpn_hudoff.wav");// path add/delete start
			PRECACHE_SOUND("common/wpn_hudon.wav");// path add/delete done
			PRECACHE_SOUND("common/wpn_moveselect.wav");// path add/delete cancel
			PRECACHE_SOUND("common/wpn_denyselect.wav");// path add/delete error
			PRECACHE_SOUND("player/sprayer.wav");// logo spray sound
			g_iModelIndexAnimglow01 = PRECACHE_MODEL("sprites/animglow01.spr");
			g_iModelIndexLaser = PRECACHE_MODEL("sprites/laserbeam.spr");
			g_GameRules = true;
			//is_team_play = false;
			//checked_teamplay = false;
			// XDM3037a: OBSOLETE	bot_cfg_pause_time = 0.0;
			respawn_time = 0.0;
			spawn_time_reset = false;
			prev_num_bots = num_bots;
			num_bots = 0;
			bot_check_time = gpGlobals->time + 60.0f;
		}
	}
	return (*other_gFunctionTable.pfnSpawn)(pent);
}

void DispatchThink(edict_t *pent)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchThink(%d %s)\n", ENTINDEX(pent), STRING(pent->v.classname)));
#endif
	(*other_gFunctionTable.pfnThink)(pent);
}

void DispatchUse(edict_t *pentUsed, edict_t *pentOther)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchUse(%d %s, %d %s)\n", ENTINDEX(pentUsed), STRING(pentUsed->v.classname), ENTINDEX(pentOther), STRING(pentOther->v.classname)));
#endif
	(*other_gFunctionTable.pfnUse)(pentUsed, pentOther);
}

void DispatchTouch(edict_t *pentTouched, edict_t *pentOther)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchTouch(%d %s, %d %s)\n", ENTINDEX(pentTouched), STRING(pentTouched->v.classname), ENTINDEX(pentOther), STRING(pentOther->v.classname)));
#endif
	(*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
}

void DispatchBlocked(edict_t *pentBlocked, edict_t *pentOther)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchBlocked(%d %s, %d %s)\n", ENTINDEX(pentBlocked), STRING(pentBlocked->v.classname), ENTINDEX(pentOther), STRING(pentOther->v.classname)));
#endif
	(*other_gFunctionTable.pfnBlocked)(pentBlocked, pentOther);
}

void DispatchKeyValue(edict_t *pent, KeyValueData *pkvd)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchKeyValue(%d %s)\n", pent->pvPrivateData?ENTINDEX(pent):-1, STRING(pent->v.classname)));
#endif
	(*other_gFunctionTable.pfnKeyValue)(pent, pkvd);
}

void DispatchSave(edict_t *pent, SAVERESTOREDATA *pSaveData)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchSave(%d %s, %s, %s)\n", ENTINDEX(pent), STRING(pent->v.classname), pSaveData->szCurrentMapName, pSaveData->szLandmarkName));
#endif
	(*other_gFunctionTable.pfnSave)(pent, pSaveData);
}

int DispatchRestore(edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchRestore(%d %s, %s, %s, %d)\n", ENTINDEX(pent), STRING(pent->v.classname), pSaveData->szCurrentMapName, pSaveData->szLandmarkName, globalEntity));
#endif
	return (*other_gFunctionTable.pfnRestore)(pent, pSaveData, globalEntity);
}

void DispatchObjectCollisionBox(edict_t *pent)
{
//#if defined (_DEBUG)
// crash on invalid object	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("DispatchObjectCollisionBox(%d %s)\n", ENTINDEX(pent), STRING(pent->v.classname)));
//#endif
	(*other_gFunctionTable.pfnSetAbsBox)(pent);
}

void SaveWriteFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("SaveWriteFields(%s %s, %s, -, %s, %d)\n", pSaveData->szCurrentMapName, pSaveData->szLandmarkName, pname, pFields->fieldName, fieldCount));
#endif
	(*other_gFunctionTable.pfnSaveWriteFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveReadFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("SaveReadFields(%s %s, %s, -, %s, %d)\n", pSaveData->szCurrentMapName, pSaveData->szLandmarkName, pname, pFields->fieldName, fieldCount));
#endif
	(*other_gFunctionTable.pfnSaveReadFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveGlobalState(SAVERESTOREDATA *pSaveData)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("SaveGlobalState(%s %s)\n", pSaveData->szCurrentMapName, pSaveData->szLandmarkName));
#endif
	(*other_gFunctionTable.pfnSaveGlobalState)(pSaveData);
}

void RestoreGlobalState(SAVERESTOREDATA *pSaveData)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("RestoreGlobalState(%s %s)\n", pSaveData->szCurrentMapName, pSaveData->szLandmarkName));
#endif
	(*other_gFunctionTable.pfnRestoreGlobalState)(pSaveData);
}

void ResetGlobalState(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("ResetGlobalState()\n");
#endif
	(*other_gFunctionTable.pfnResetGlobalState)();
}

BOOL ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
//#if defined (_DEBUG)
//	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ClientConnect(%d %s, %s, %s, %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), pszName, pszAddress, szRejectReason));
//#endif
	DBG_PRINTF("ClientConnect(%s[%d], %s, %s)\n", STRING(pEntity->v.classname), ENTINDEX(pEntity), pszName, pszAddress);
	if (gpGlobals->deathmatch)
	{
		// check if this client is the listen server client
		if (strcmp(pszAddress, "loopback") == 0)
			listenserver_edict = pEntity;// save the edict of the listen server client...

		// check if this is NOT a bot joining the server...
		/* XDM3037: makes no sence because server reports FULL and no connection is made
		if (strcmp(pszAddress, "127.0.0.1") != 0)// !(pEntity->v.flags & FL_FAKECLIENT)
		{
			// don't try to add bots for 60 seconds, give client time to get added
			bot_check_time = gpGlobals->time + 60.0f;

			int count = UTIL_CountBots();
			// if there are currently more than the minimum number of bots running then kick one of the bots off the server...
			if ((count > min_bots) && (min_bots != -1))
			{
				for (short i=0; i < MAX_PLAYERS; ++i)
				{
					if (IsActiveBot(i))  // is this slot used?
					{
						char cmd[80];
						sprintf(cmd, "kick \"%s\"\n", bots[i].name);
						//sprintf(cmd, "kick # %d\n", i+1);
						SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
						break;
					}
				}
			}
		}*/
	}
	return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
}

void ClientDisconnect(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ClientDisconnect(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif

	if (g_iGetWeaponDataPlayerIndex == ENTINDEX(pEntity))// XDM3038c: IMPORTANT!
		g_iGetWeaponDataPlayerIndex = 0;// re-detect!

	//if (gpGlobals->deathmatch)// this may get out of control
	//{
		// XDM3037a	if (clients[i] == pEntity)
		//	clients[i] = NULL;//cli = i;

		bot_t *pBot;
		short index = UTIL_GetBotIndex(pEntity);
		if (index >= 0)
		{
			pBot = &bots[index];
			if (pBot)
			{
				conprintf(1, "XBM: bot %hd \"%s\" disconnected.\n", index, pBot->name);
				pBot->pBotUser = NULL;
				// someone kicked this bot off of the server...
				BotSpawnInit(pBot);// XDM: this resets bot data to default values
				pBot->pEdict = NULL;// XDM
				pBot->is_used = false;// this slot is now free to use
				// XDM3038c: OBSOLETE pBot->f_kick_time = gpGlobals->time;// save the kicked time
				pEntity->v.origin.Clear();
				pEntity->v.velocity.Clear();
				pEntity->v.nextthink = 0;
				pEntity->v.movetype = MOVETYPE_NONE;
				pEntity->v.solid = SOLID_NOT;
				pEntity->v.health = 0;
				pEntity->v.frags = 0;
				pEntity->v.weapons = 0;
				pEntity->v.takedamage = DAMAGE_NO;
				ClearBits(pEntity->v.flags, FL_FAKECLIENT);// XDM3037: otherwise next joining player will have it
			}
		}
	//}
	(*other_gFunctionTable.pfnClientDisconnect)(pEntity);
}

void ClientKill(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ClientKill(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	(*other_gFunctionTable.pfnClientKill)(pEntity);
}

void ClientPutInServer(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ClientPutInServer(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	(*other_gFunctionTable.pfnClientPutInServer)(pEntity);
// XDM3037a	clients[ENTINDEX(pEntity)] = pEntity;
}

void ClientCommand(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ClientCommand(%d %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname)));
#endif
	const char *pcmd = Cmd_Argv(0);
	const char *arg1 = Cmd_Argv(1);
	const char *arg2 = Cmd_Argv(2);
	// only allow custom commands if deathmatch mode and NOT dedicated server and client sending command is the listen server client...
	if ((gpGlobals->deathmatch > 0) && (!IsDedicatedServer) && (pEntity == listenserver_edict))
	{
		if (FStrEq(pcmd, "addbot"))
		{
			conprintf(1, "Use 'bot_create' instead of '%s'\n", pcmd);
			return;
		}
		else if (FStrEq(pcmd, "bot_use"))
		{
			if (!arg1 || !arg2)
			{
				ALERT(at_notice, "Usage: %s <use type> <bot name>\n", pcmd);
				return;
			}

			int utype = atoi(arg1);
			for (short i=0; i<MAX_PLAYERS; ++i)
			{
				if (IsActiveBot(i) && _stricmp(arg2, bots[i].name) == 0)
				{
					bot_t *pBot = &bots[i];
					if (pBot->use_type == utype)
						BotUseCommand(pBot, pEntity, BOT_USE_NONE);
					else
						BotUseCommand(pBot, pEntity, utype);
				}
			}
			return;
		}
		/*else if (FStrEq(pcmd, "debug_engine"))
		{
			if ((arg1 != NULL) && (*arg1 != 0))
			{
				int temp = atoi(arg1);
				if(temp)
					debug_engine = 1;
				else
					debug_engine = 0;
			}

			if (debug_engine)
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "debug_engine ENABLED!\n");
			else
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "debug_engine DISABLED!\n");

			return;
		}*/
		else if (FStrEq(pcmd, "waypoint_add"))
		{
			if (!g_waypoint_on)
				g_waypoint_on = true;  // turn waypoints on if off

			WaypointAdd(pEntity);
			return;
		}
		else if (FStrEq(pcmd, "waypoint_delete"))
		{
			if (!g_waypoint_on)
				g_waypoint_on = true;  // turn waypoints on if off

			WaypointDelete(pEntity);
			return;
		}
		else if (FStrEq(pcmd, "info"))
		{
			WaypointPrintInfo(pEntity);
			return;
		}
		else if (FStrEq(pcmd, "autowaypoint"))
		{
			if (FStrEq(arg1, "on"))
			{
				g_auto_waypoint = true;
				g_waypoint_on = true;  // turn this on just in case
			}
			else if (FStrEq(arg1, "off"))
			{
				g_auto_waypoint = false;
			}

			if (g_auto_waypoint)
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "autowaypoint is ON\n");
			else
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "autowaypoint is OFF\n");

			return;
		}
		else if (FStrEq(pcmd, "pathwaypoint"))
		{
			if (FStrEq(arg1, "on"))
			{
				g_path_waypoint = true;
				g_waypoint_on = true;  // turn this on just in case
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "pathwaypoint is ON\n");
			}
			else if (FStrEq(arg1, "off"))
			{
				g_path_waypoint = false;
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "pathwaypoint is OFF\n");
			}
			else if (FStrEq(arg1, "enable"))
			{
				g_path_waypoint_enable = true;
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "pathwaypoint is ENABLED\n");
			}
			else if (FStrEq(arg1, "disable"))
			{
				g_path_waypoint_enable = false;
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "pathwaypoint is DISABLED\n");
			}
			else if (FStrEq(arg1, "create1"))
			{
				WaypointCreatePath(pEntity, 1);
			}
			else if (FStrEq(arg1, "create2"))
			{
				WaypointCreatePath(pEntity, 2);
			}
			else if (FStrEq(arg1, "remove1"))
			{
				WaypointRemovePath(pEntity, 1);
			}
			else if (FStrEq(arg1, "remove2"))
			{
				WaypointRemovePath(pEntity, 2);
			}
			return;
		}
		else if (FStrEq(pcmd, "botobserver"))
		{
			if ((arg1 != NULL) && (*arg1 != 0))
			{
				int temp = atoi(arg1);
				if (temp)
					g_observer_mode = true;
				else
					g_observer_mode = false;
			}
			if (g_observer_mode)
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
			else
				ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

			return;
		}
//#if _DEBUG
		else if (g_pCvarDeveloper->value > 0)
		{
			if (FStrEq(pcmd, "botstop"))
			{
				g_BotStop = true;
				return;
			}
			else if (FStrEq(pcmd, "botstart"))
			{
				g_BotStop = false;
				return;
			}
			/*else if (FStrEq(pcmd, "bot_gamemode"))
			{
				conprintf(1, "XBM: game mode %d dm %g svflags %g\n", g_iGameType, gpGlobals->deathmatch, gpGlobals->serverflags);
				return;
			}*/
		}
//#endif
	}
	(*other_gFunctionTable.pfnClientCommand)(pEntity);
}

void ClientUserInfoChanged(edict_t *pEntity, char *infobuffer)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ClientUserInfoChanged(%d %s %s, %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname), infobuffer));
#endif
	(*other_gFunctionTable.pfnClientUserInfoChanged)(pEntity, infobuffer);
}

void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ServerActivate(%d %d)\n", edictCount, clientMax));
#endif
//	SERVER_PRINT("XBM ServerActivate\n");
	g_intermission = false;
	if (g_ServerActive == false)
	{
		if (g_iGameType != GT_SINGLE || gpGlobals->maxClients > 1)// XDM3038: not in singleplayer
		{
			char str[256];
			_snprintf(str, 256, "exec \"%s\"\n", g_szConfigFileName);// XDM3037a: global config
			str[255] = '\0';
			SERVER_COMMAND(str);
			_snprintf(str, 256, "exec \"maps/%s_%s\"\n", STRING(gpGlobals->mapname), g_szConfigFileName);// XDM3037: map-specific config
			str[255] = '\0';
			SERVER_COMMAND(str);
			SERVER_EXECUTE();
		}
		g_ServerActive = true;// set after executing cfgs

		msecnum = 0;
		msecdel = 0;
		msecval = 0;
		/*wtf	short bot_index;
		short count = 0;
		// mark the bots as needing to be respawned...
		for (bot_index = 0; bot_index < MAX_PLAYERS; ++bot_index)
		{
			if (count >= prev_num_bots)
			{
				bots[bot_index].is_used = false;
				bots[bot_index].autospawn_state = RESPAWN_NO;
				bots[bot_index].f_kick_time = 0.0;
			}
			if (bots[bot_index].is_used)// is this slot used?
			{
				bots[bot_index].autospawn_state = RESPAWN_NEED_TO_SPAWN;
				count++;
			}
			// check for any bots that were very recently kicked...
			if ((bots[bot_index].f_kick_time + 5.0) > (gpGlobals->time - gpGlobals->frametime))
			{
				bots[bot_index].autospawn_state = RESPAWN_NEED_TO_SPAWN;
				count++;
			}
			else
				bots[bot_index].f_kick_time = 0.0;  // reset to prevent false spawns later
		}*/

		// set the respawn time
		if (IsDedicatedServer)
			respawn_time = gpGlobals->time + 5.0f;
		else
			respawn_time = gpGlobals->time + 10.0f;

		//client_update_time = gpGlobals->time + 10.0;  // start updating client data again
		bot_check_time = gpGlobals->time + 60.0f;

		/* UNDONE: entity that can be used to measure light level in a specific point
		edict_t *g_pLightLevelChecker = CREATE_ENTITY();// HACK
		if (g_pLightLevelChecker)
		{
			g_pLightLevelChecker->v.model = MAKE_STRING("models/testsphere.mdl");
			g_pLightLevelChecker->v.solid = SOLID_NOT;
			g_pLightLevelChecker->v.effects = EF_NODRAW;
		}*/
	}
	g_iGetWeaponDataPlayerIndex = 0;// XDM3038b
	(*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);
}

/* XDM3038: UNDONE
int UTIL_MeasureLight(const Vecot &vecOrigin)
{
	if (g_pLightLevelChecker)
	{
		g_pLightLevelChecker->v.origin = vecOrigin;
		SET_ORIGIN(g_pLightLevelChecker, vecOrigin);
		return GETENTITYILLUM(g_pLightLevelChecker);
	}
	return 127;// ?
}*/

void ServerDeactivate(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("ServerDeactivate()\n");
#endif
	if (g_ServerActive)
		g_ServerActive = false;
	(*other_gFunctionTable.pfnServerDeactivate)();
	g_iGetWeaponDataPlayerIndex = 0;// XDM3038b
}

void PlayerPreThink(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("PlayerPreThink(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	(*other_gFunctionTable.pfnPlayerPreThink)(pEntity);
}

void PlayerPostThink(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("PlayerPostThink(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	if (pEntity->v.flags & FL_FAKECLIENT)
	{
		bot_t *pBot = UTIL_GetBotPointer(pEntity);
		if (pBot)
			BotPostThink(pBot);
	}
	(*other_gFunctionTable.pfnPlayerPostThink)(pEntity);
}

//float previous_time = -1.0f;

void StartFrame(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("StartFrame()\n");
#endif
#if defined (DEBUG_USERMSG)
	for (short msg_dest = 0; msg_dest <= MSG_SPEC; ++msg_dest)
	{
		if (g_UserMessagesStats.count[msg_dest] != 0)// anything
		{
			// r - reliable, u - unreliable
			conprintf(1, "XBM:UMSG: Bu %d Or %d Br %d I %d Vu %d Au %d Vr %d Ar %d Ou %d S %d\n",
				g_UserMessagesStats.count[MSG_BROADCAST],
				g_UserMessagesStats.count[MSG_ONE],
				g_UserMessagesStats.count[MSG_ALL],
				g_UserMessagesStats.count[MSG_INIT],
				g_UserMessagesStats.count[MSG_PVS],
				g_UserMessagesStats.count[MSG_PAS],
				g_UserMessagesStats.count[MSG_PVS_R],
				g_UserMessagesStats.count[MSG_PAS_R],
				g_UserMessagesStats.count[MSG_ONE_UNRELIABLE],
				g_UserMessagesStats.count[MSG_SPEC]);

			for (; msg_dest <= MSG_SPEC; ++msg_dest)
				g_UserMessagesStats.count[msg_dest] = 0;

			break;
		}
	}
#endif

	if (gpGlobals->deathmatch)
	{
		// adjust the millisecond delay based on the frame rate interval...
		if (msecdel <= gpGlobals->time)
		{
			msecdel = gpGlobals->time + 0.5f;
			if (msecnum > 0)
				msecval = (float)(450/msecnum);

			msecnum = 0;
		}
		else
			msecnum++;

		if (msecval < 1)// don't allow msec to be less than 1...
			msecval = 1;
		else if (msecval > 100.0f)// ...or greater than 100
			msecval = 100.0f;

		if (g_ServerActive)// XDM3037a
		{
#if defined (_DEBUG)
			CLIENT_INDEX player_index;
			short bot_index, numactivebots, numplayers;
#else
			static CLIENT_INDEX player_index;
			static short bot_index, numactivebots, numplayers;
#endif
			numactivebots = 0;
			numplayers = 0;

			edict_t *pPlayer;
			for (player_index = 1; player_index <= gpGlobals->maxClients; ++player_index)
			{
				pPlayer = INDEXENT(player_index);
				if (pPlayer && !pPlayer->free && FBitSet(pPlayer->v.flags, FL_CLIENT))
				{
					if (FBitSet(pPlayer->v.flags, FL_FAKECLIENT))
					{
						bot_index = UTIL_GetBotIndex(pPlayer);
						if (IsActiveBot(bot_index))
						{
							if (bots[bot_index].autospawn_state == RESPAWN_IDLE)
							{
								if (bots[bot_index].pEdict->free)
								{
									bots[bot_index].is_used = false;
									conprintf(2, "XBM: Warning! Bot %d edict was freed!\n", bot_index);
									continue;
								}
								if (g_BotStop == false)
									BotThink(&bots[bot_index]);// bots must think even while spectating
							}
							// old	if (g_intermission)
							//	bots[bot_index].pEdict->v.button |= (IN_ATTACK|IN_JUMP);// XDM3037: press ready buttons

							++numactivebots;
						}
					}
					else
					{
						WaypointThink(pPlayer);
					}
					++numplayers;
				}
			}

			if (numactivebots > num_bots)
				num_bots = numactivebots;

			// CREATE QUEUED BOTS
			if (g_botmatch.value > 0.0f)// XDM3035c: server may suddenly disallow bots
			{
				// Are we currently respawning bots and is it time to spawn one yet?
				if ((respawn_time > 1.0f) && (respawn_time <= gpGlobals->time) &&
					(numplayers < gpGlobals->maxClients) && (num_bots < min(gpGlobals->maxClients-1, (int)g_bot_maxbots.value)))// XDM3038c: ALWAYS KEEP ONE SLOT for connecting player! Otherwise nobody is able to connect!
				{
					short index = 0;
					// find bot needing to be added to the game
					while ((index < MAX_PLAYERS) && (bots[index].autospawn_state != RESPAWN_NEED_TO_SPAWN || IsActiveBot(index)))
						++index;

					if (index < MAX_PLAYERS)
					{
						//if (bots[index].use_type != BOT_USE_NONE)// XDM: don't remember holding position of old map or user that may not exist anymore
							bots[index].use_type = BOT_USE_NONE;
							bots[index].pBotUser = NULL;// !

						bots[index].is_used = false;// free up this slot
						if (bots[index].model[0] && bots[index].name[0])
						{
							bots[index].autospawn_state = RESPAWN_IS_SPAWNING;
							if (BOT_CREATE(index, bots[index].model, bots[index].name, bots[index].bot_skill, bots[index].top_color, bots[index].bottom_color, bots[index].model_skin, bots[index].reaction, bots[index].strafe_percent, bots[index].chat_percent, bots[index].taunt_percent, bots[index].whine_percent, ""))
							{
								respawn_time = gpGlobals->time + BOT_JOIN_DELAY;
								bot_check_time = gpGlobals->time + 5.0f;
							}
							else
								bots[index].autospawn_state = RESPAWN_NO;
						}
						else
							bots[index].autospawn_state = RESPAWN_NO;

						if (bots[index].autospawn_state == RESPAWN_NO)// skip bad bot
						{
							respawn_time = gpGlobals->time;
							bot_check_time = gpGlobals->time + 0.1f;
						}
						// CONSIDER: we could erase all remaining initialized, but unused bots, but what if g_bot_maxbots or maxplayers changes? Better keep them just in case.
					}
					else
						respawn_time = 0.0;
				}
			}

			if (g_GameRules)
			{
				/*// XDM3037a: OBSOLETE	if (need_to_open_cfg)  // have we open .cfg file yet?
				{
					need_to_open_cfg = false;  // only do this once!!!
					char filename[256];
					UTIL_BuildFileName(filename, (char *)g_szConfigFileName, NULL);
					bot_cfg_fp = fopen(filename, "r");
					if (bot_cfg_fp == NULL)
						conprintf(1, "XBM: file not found %s!\n", g_szConfigFileName);
					else
						conprintf(1, "XBM: Executing %s\n", g_szConfigFileName);

					if (IsDedicatedServer)
						bot_cfg_pause_time = gpGlobals->time + 5.0f;
					else
						bot_cfg_pause_time = gpGlobals->time + 10.0f;
				}*/

				if (!IsDedicatedServer && !spawn_time_reset)
				{
					if (listenserver_edict != NULL)
					{
						if (IsAlive(listenserver_edict))
						{
							spawn_time_reset = true;
							if (respawn_time >= 1.0)
								respawn_time = min(respawn_time, gpGlobals->time + 1.0f);

	// XDM3037a: OBSOLETE						if (bot_cfg_pause_time >= 1.0)
	//							bot_cfg_pause_time = min(bot_cfg_pause_time, gpGlobals->time + 1.0f);
						}
					}
				}
	// XDM3037a			if ((bot_cfg_fp) && (bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= gpGlobals->time))
	// XDM3037a: corrupts pointer				ProcessBotCfgFile();// process .cfg file options...
			}// g_GameRules
		}// g_ServerActive
		//previous_time = gpGlobals->time;
	}
	(*other_gFunctionTable.pfnStartFrame)();
}

void ParmsNewLevel(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("ParmsNewLevel()\n");
#endif
	(*other_gFunctionTable.pfnParmsNewLevel)();
}

void ParmsChangeLevel(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("ParmsChangeLevel()\n");
#endif
	(*other_gFunctionTable.pfnParmsChangeLevel)();
	g_iGetWeaponDataPlayerIndex = 0;// XDM3038b
}

const char *GetGameDescription(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("GetGameDescription()\n");
#endif
	return (*other_gFunctionTable.pfnGetGameDescription)();
}

void PlayerCustomization(edict_t *pEntity, struct customization_s *pCust)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("PlayerCustomization(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	/* XDM3035c: TODO	if (pCust)
	{
		bot_t *pBot = UTIL_GetBotPointer(pEntity);
		if (pBot)
		{
			if (pCust->resource.type == t_decal)
				strcpy(pBot->logo_name, pCust->resource.szFileName);
				?pBot->logo_index = pCust->resource.nIndex;
				?pBot->logo_index = pCust->nUserData2; // Second int is max # of frames.
		}
	}*/
	(*other_gFunctionTable.pfnPlayerCustomization)(pEntity, pCust);
}

void SpectatorConnect(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("SpectatorConnect(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	(*other_gFunctionTable.pfnSpectatorConnect)(pEntity);
}

void SpectatorDisconnect(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("SpectatorDisconnect(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	(*other_gFunctionTable.pfnSpectatorDisconnect)(pEntity);
}

void SpectatorThink(edict_t *pEntity)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("SpectatorThink(%d %s %s)\n", ENTINDEX(pEntity), STRING(pEntity->v.classname), STRING(pEntity->v.netname)));
#endif
	(*other_gFunctionTable.pfnSpectatorThink)(pEntity);
}

void Sys_Error(const char *error_string)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("Sys_Error(%s)\n", error_string));
#endif
	(*other_gFunctionTable.pfnSys_Error)(error_string);
}

void PM_Move(struct playermove_s *ppmove, int server)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("PM_Move(%d)\n", server));
#endif
	(*other_gFunctionTable.pfnPM_Move)(ppmove, server);
}

static bool pm_proxy_initialized = false;

void PM_Init(struct playermove_s *ppmove)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("PM_Init()\n");
#endif
	ASSERT(pm_proxy_initialized == false);

	pmove = ppmove;
	pmove->Con_DPrintf("PM_Init(client %d) (proxy)\n", ppmove->player_index);
	pm_proxy_initialized = true;

	(*other_gFunctionTable.pfnPM_Init)(ppmove);
}

char PM_FindTextureType(char *name)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("PM_FindTextureType(%s)\n", name));
#endif
	return (*other_gFunctionTable.pfnPM_FindTextureType)(name);
}

void SetupVisibility(edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("SetupVisibility(%d %s, %d %s %s)\n", pViewEntity?ENTINDEX(pViewEntity):0, pViewEntity?STRING(pViewEntity->v.classname):"", ENTINDEX(pClient), STRING(pClient->v.classname), STRING(pClient->v.netname)));
#endif
	(*other_gFunctionTable.pfnSetupVisibility)(pViewEntity, pClient, pvs, pas);
}

void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("UpdateClientData(%d %s %s, %d)\n", ENTINDEX(ent), STRING(ent->v.classname), STRING(ent->v.netname), sendweapons));
#endif
	(*other_gFunctionTable.pfnUpdateClientData)(ent, sendweapons, cd);

	bot_t *pBot = UTIL_GetBotPointer(ent);
	if (pBot)
	{
		pBot->current_weapon.iId = cd->m_iId;
		//pBot->current_weapon.iClip = ???
		pBot->current_weapon.weapon_select_table_index = GetWeaponSelectIndex(cd->m_iId);
	}
}

int AddToFullPack(struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet)
{
// TOO SLOW #if defined (_DEBUG)
//	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("AddToFullPack(%d, %d %s, %d %s %s, %d, %d)\n", e, ENTINDEX(ent), STRING(ent->v.classname), ENTINDEX(host), STRING(host->v.classname), STRING(host->v.netname), hostflags, player));
//#endif
	return (*other_gFunctionTable.pfnAddToFullPack)(state, e, ent, host, hostflags, player, pSet);
}

void CreateBaseline(int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("CreateBaseline(%d, %d, %d %s, %d)\n", player, eindex, ENTINDEX(entity), STRING(entity->v.classname), playermodelindex));
#endif
	(*other_gFunctionTable.pfnCreateBaseline)(player, eindex, baseline, entity, playermodelindex, player_mins, player_maxs);
}

void RegisterEncoders(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("RegisterEncoders()\n");
#endif
	(*other_gFunctionTable.pfnRegisterEncoders)();
}

#if defined (EXPERIMENTAL_BOT_WEAPON_DATA)
static weapon_data_t botweaponinfo[MAX_WEAPONS];// XDM3038c: place to store data that will be filled by game DLL for bots
#endif

// WARNING: engine DOES NOT CALL THIS FOR BOTS! So we emulate.
int GetWeaponData(struct edict_s *player, struct weapon_data_s *info)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("GetWeaponData(%d %s %s)\n", ENTINDEX(player), STRING(player->v.classname), STRING(player->v.netname)));
#endif

	int iRet = (*other_gFunctionTable.pfnGetWeaponData)(player, info);
#if defined (EXPERIMENTAL_BOT_WEAPON_DATA)
	int playerindex = ENTINDEX(player);
	if (g_iGetWeaponDataPlayerIndex == 0)// first call ever
		g_iGetWeaponDataPlayerIndex = playerindex;// remember some client

	// XDM3038b: call this only once per frame (this function may be called for many real clients)
	// HL and other mods have different meaning for m_iWeaponState
	if (mod_id == GAME_XDM_DLL && g_iGetWeaponDataPlayerIndex == playerindex && iRet > 0 && player)// we detect that this function is called for remembered player again (not for other players)
	{
		size_t weaponindex;
		for (size_t index=0; index < MAX_PLAYERS; ++index)// now emulate weapon data updates for all bots
		{
			bot_t *pBot = &bots[index];
			if (pBot && pBot->is_used && pBot->pEdict)
			{
				if ((*other_gFunctionTable.pfnGetWeaponData)(pBot->pEdict, botweaponinfo) == 0)// emulate call to the game DLL
					continue;

				for (weaponindex = 0; weaponindex < MAX_WEAPONS; ++weaponindex)
				{
					if (botweaponinfo[weaponindex].m_iWeaponState == wstate_current || botweaponinfo[weaponindex].m_iWeaponState == wstate_current_ontarget)
					{
						pBot->current_weapon.iId = botweaponinfo[weaponindex].m_iId;
						pBot->current_weapon.iClip = botweaponinfo[weaponindex].m_iClip;
						//pBot->current_weapon.iState = botweaponinfo[weaponindex].m_iWeaponState;
						pBot->current_weapon.weapon_select_table_index = GetWeaponSelectIndex(botweaponinfo[weaponindex].m_iId);
					}
					pBot->m_iWeaponState[botweaponinfo[weaponindex].m_iId] = botweaponinfo[weaponindex].m_iWeaponState;
				}
			}
		}
	}
#endif
	return iRet;
}

void CmdStart(const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("CmdStart(%d %s %s)\n", ENTINDEX(player), STRING(player->v.classname), STRING(player->v.netname)));
#endif
	usercmd_s *pCmd = (usercmd_s *)cmd;// HACK
	bot_t *pBot = UTIL_GetBotPointer(player);
	if (pBot)
	{
		pCmd->weaponselect = pBot->m_iQueuedSelectWeaponID;
		pCmd->upmove = pBot->cmd_upmove;
	}
	(*other_gFunctionTable.pfnCmdStart)(player, pCmd, random_seed);
}

void CmdEnd(const edict_t *player)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("CmdEnd(%d %s %s)\n", ENTINDEX(player), STRING(player->v.classname), STRING(player->v.netname)));
#endif
	(*other_gFunctionTable.pfnCmdEnd)(player);
}

int ConnectionlessPacket(const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("ConnectionlessPacket(%s)\n", args));
#endif
	return (*other_gFunctionTable.pfnConnectionlessPacket)(net_from, args, response_buffer, response_buffer_size);
}

int GetHullBounds(int hullnumber, float *mins, float *maxs)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("GetHullBounds(%d)\n", hullnumber));
#endif
	return (*other_gFunctionTable.pfnGetHullBounds)(hullnumber, mins, maxs);
}

void CreateInstancedBaselines(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("CreateInstancedBaselines()\n");
#endif
	(*other_gFunctionTable.pfnCreateInstancedBaselines)();
}

int InconsistentFile(const edict_t *player, const char *filename, char *disconnect_message)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF(UTIL_VarArgs("CmdStart(%d %s %s, %s)\n", ENTINDEX(player), STRING(player->v.classname), STRING(player->v.netname), filename));
#endif
	return (*other_gFunctionTable.pfnInconsistentFile)(player, filename, disconnect_message);
}

int AllowLagCompensation(void)
{
#if defined (_DEBUG)
	XBM_SUPERDBG_PRINTF("AllowLagCompensation()\n");
#endif
	return (*other_gFunctionTable.pfnAllowLagCompensation)();
}

DLL_FUNCTIONS gFunctionTable =
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
	DispatchObjectCollisionBox,	//pfnAbsBox

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

	GetGameDescription,			//pfnGetGameDescription	 Returns string describing current .dll game.
	PlayerCustomization,		//pfnPlayerCustomization	Notifies .dll of new customization for player.

	SpectatorConnect,			//pfnSpectatorConnect		Called when spectator joins server
	SpectatorDisconnect,		//pfnSpectatorDisconnect	Called when spectator leaves the server
	SpectatorThink,				//pfnSpectatorThink		  Called when spectator sends a command packet (usercmd_t)

	Sys_Error,					//pfnSys_Error			 Called when engine has encountered an error

	PM_Move,					//pfnPM_Move
	PM_Init,					//pfnPM_Init				Server version of player movement initialization
	PM_FindTextureType,			//pfnPM_FindTextureType

	SetupVisibility,			//pfnSetupVisibility		  Set up PVS and PAS for networking for this client
	UpdateClientData,			//pfnUpdateClientData		 Set up data sent only to specific client
	AddToFullPack,				//pfnAddToFullPack
	CreateBaseline,				//pfnCreateBaseline		  Tweak entity baseline for network encoding, allows setup of player baselines, too.
	RegisterEncoders,			//pfnRegisterEncoders		Callbacks for network encoding
	GetWeaponData,				//pfnGetWeaponData
	CmdStart,					//pfnCmdStart
	CmdEnd,						//pfnCmdEnd
	ConnectionlessPacket,		//pfnConnectionlessPacket
	GetHullBounds,				//pfnGetHullBounds
	CreateInstancedBaselines,	//pfnCreateInstancedBaselines
	InconsistentFile,			//pfnInconsistentFile
	AllowLagCompensation,		//pfnAllowLagCompensation
};

// XBM does not use NEW_DLL_FUNCTIONS so we don't have to intercept them
/* TODO: uncomment if you want to use them
void OnFreeEntPrivateData(edict_t *pEnt)
{
	if (other_gNewFunctionTable.pfnOnFreeEntPrivateData)
		(*other_gNewFunctionTable.pfnOnFreeEntPrivateData)(pEnt);
}

void GameDLLShutdown(void)
{
	if (other_gNewFunctionTable.pfnGameShutdown)
		(*other_gNewFunctionTable.pfnGameShutdown)();
}

int ShouldCollide(edict_t *pentTouched, edict_t *pentOther)
{
	if (other_gNewFunctionTable.pfnShouldCollide)
		return (*other_gNewFunctionTable.pfnShouldCollide)(pentTouched, pentOther);
}

#if defined (HL1120)
void CvarValue(const edict_t *pEnt, const char *value)
{
	if (other_gNewFunctionTable.pfnCvarValue)
		(*other_gNewFunctionTable.pfnCvarValue)(pEnt, value);
}

void CvarValue2(const edict_t *pEnt, int requestID, const char *cvarName, const char *value)
{
	if (other_gNewFunctionTable.pfnCvarValue2)
		(*other_gNewFunctionTable.pfnCvarValue2)(pEnt, requestID, cvarName, value);
}
#endif

NEW_DLL_FUNCTIONS gNewFunctionTable =
{
	OnFreeEntPrivateData,	//pfnOnFreeEntPrivateData
	GameDLLShutdown,		//pfnGameShutdown
	ShouldCollide,			//pfnShouldCollide
#if defined (HL1120)// XDM3035: these causes API glitches in earlier HL engine such as ClientCommand will be never called
	CvarValue,				//pfnCvarValue
	CvarValue2,				//pfnCvarValue2
#endif
};
*/

//-----------------------------------------------------------------------------
// Acts as engine for the Mod dll. Provides CMD and ARGS!
// WARNING: UNDONE: this really should cache a chain of fake commands and ONLY EXECUTE THEM at the beginning of the next frame!!!
//-----------------------------------------------------------------------------
void FakeClientCommand(edict_t *pBot, const char *pcmd, const char *arg1, const char *arg2, const char *arg3)
{
	memset(g_argv, 0, GLOBAL_ARGV_SIZE);
	isFakeClientCommand = true;

	if (g_bot_debug.value > 0.0f)
		conprintf(2, "FakeClientCommand(%s[%d], %s %s %s)\n", STRING(pBot->v.netname), ENTINDEX(pBot), pcmd, arg1, arg2, arg3);

	fake_arg_count = 0;
	if (pcmd == NULL || *pcmd == 0)
		return;

	strncpy(&g_argv[0], pcmd, FAKE_CMD_ARG_LEN);
	g_argv[FAKE_CMD_ARG_LEN-1] = '\0';
	++fake_arg_count;

	if (arg1 != NULL && *arg1 != 0)
	{
		strncpy(&g_argv[FAKE_CMD_ARG_LEN], arg1, FAKE_CMD_ARG_LEN);
		g_argv[FAKE_CMD_ARG_LEN*2-1] = '\0';
		++fake_arg_count;
	}
	if (arg2 != NULL && *arg2 != 0)
	{
		strncpy(&g_argv[FAKE_CMD_ARG_LEN*2], arg2, FAKE_CMD_ARG_LEN);
		g_argv[FAKE_CMD_ARG_LEN*3-1] = '\0';
		++fake_arg_count;
	}
	if (arg3 != NULL && *arg3 != 0)
	{
		strncpy(&g_argv[FAKE_CMD_ARG_LEN*3], arg3, FAKE_CMD_ARG_LEN);
		g_argv[FAKE_CMD_ARG_LEN*4-1] = '\0';
		++fake_arg_count;
	}
	// allow the MOD DLL to execute the ClientCommand...
	ClientCommand(pBot);
	isFakeClientCommand = false;
}

const char *Cmd_Args(void)
{
	if (isFakeClientCommand)
		return &g_argv[FAKE_CMD_ARG_LEN];// does not include the command itself
	else
		return (*g_engfuncs.pfnCmd_Args)();
}

const char *Cmd_Argv(int argc)
{
	if (isFakeClientCommand)
	{
		if (argc < FAKE_CMD_ARGS)// XDM3038a: no more damn stupid monkey code!
			return &g_argv[FAKE_CMD_ARG_LEN*argc];
		else
			return NULL;
	}
	else
		return (*g_engfuncs.pfnCmd_Argv)(argc);
}

int Cmd_Argc(void)
{
	if (isFakeClientCommand)
		return fake_arg_count;
	else
		return (*g_engfuncs.pfnCmd_Argc)();
}
