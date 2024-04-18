#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "skill.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "globals.h"
#include "game.h"// XDM:
#include "maprules.h"// spawnflags
#include "client.h"// ClientPrecache
#include "entconfig.h"
#include "pm_materials.h"// Materials
#include "func_break.h"
#include "sound.h"
#include "soundent.h"
#include "nodes.h"
#include "shared_resources.h"
#include "shake.h"

// Half-Life-compatible world entity. Not much of an entity, but lots of initialization code here.

DLL_GLOBAL CWorld *g_pWorld = NULL;// XDM3038: maybe EHANDLE?
DLL_GLOBAL char g_szMapName[MAX_MAPNAME];// XDM3038c: string_t will point here?
extern const char *g_szStandardLightStyles[];

// XDM: old stuff moved to effects_new.cpp
// XDM3037a: LinkUserMessages() is now in globals.cpp

TYPEDESCRIPTION	CWorld::m_SaveData[] =
{
	DEFINE_FIELD(CWorld, m_iRoomType, FIELD_INTEGER),
	DEFINE_ARRAY(CWorld, m_rgiszAmmoTypes, FIELD_STRING, MAX_AMMO_SLOTS),
	DEFINE_ARRAY(CWorld, m_rgAmmoMaxCarry, FIELD_UINT32, MAX_AMMO_SLOTS),
};

IMPLEMENT_SAVERESTORE(CWorld, CBaseEntity);

LINK_ENTITY_TO_CLASS(worldspawn, CWorld);

//-----------------------------------------------------------------------------
// Purpose: CWorld spawns first when map is being loaded.
//-----------------------------------------------------------------------------
void CWorld::Spawn(byte restore)
{
	ASSERT(entindex() == 0);// XDM3035a: in case some moron creates another "worldspawn"
	if (entindex() != 0)
	{
		SERVER_PRINT("ERROR: another world entity is being created!\n");
		pev->flags = FL_KILLME;// DispatchSpawn() will return -1
		return;
	}

	if (restore)// XDM3038c: set this global so every entity may know
		SetBits(gpGlobals->serverflags, FSERVER_RESTORE);
	else
		ClearBits(gpGlobals->serverflags, FSERVER_RESTORE);

	g_pWorld = this;// XDM3034: safer, Precache() gets called more often

	// HACKFIX: restores case of badly entered map name
	char szMapPath[MAX_PATH];
	_snprintf(szMapPath, MAX_PATH, "maps/%s.bsp", STRING(gpGlobals->mapname));
	szMapPath[MAX_PATH-1] = '\0';
	if (!UTIL_FixFileName(szMapPath, MAX_PATH))
	{
		SERVER_PRINTF("==== ERROR: UTIL_FixFileName(\"%s\") FAILED!!! ====\n", szMapPath);
		//pev->flags = FL_KILLME;// although DispatchSpawn() properly returns -1, engine does not expect it.
		UTIL_FAIL();
		return;
	}
	// FAIL on subfolders! UTIL_StripFileName(szMapPath, g_szMapName, MAX_MAPNAME); We must extract 
	//if (strcmp(STRING(gpGlobals->mapname), g_szMapName) != 0)
	if (strstr(szMapPath, STRING(gpGlobals->mapname)) == NULL)
	{
		//gpGlobals->mapname = MAKE_STRING(g_szMapName);// engine does not deallocate this, so it's OK
		SERVER_PRINTF("==== ERROR: map name has bad case! Map: \"%s\" ====\n", szMapPath);// g_szMapName);
		/* does not have a chance to be sent MESSAGE_BEGIN(MSG_ALL, svc_stufftext, NULL, NULL);
			WRITE_STRING("Map name error\n");
			WRITE_STRING(szMapPath);
		MESSAGE_END();*/
		UTIL_FAIL();
		//CHANGE_LEVEL(g_szMapName, NULL);
		return;
	}

	pev->angles.Clear();// XDM3035a: or the world's gonna...
#if defined(MOVEWITH)
	m_pAssistLink = NULL;
#endif
	m_pFirstAlias = NULL;

	size_t i = 0;
	for (i=0; i<=MAX_TEAMS; ++i)// XDM3038a: MAX_TEAMS+1
		g_pLastSpawn[i] = NULL;

	g_iWeaponBoxCount = 0;// XDM3035
	m_iNumSecrets = 0;// XDM3038c
	gpGlobals->found_secrets = 0;// XDM3038c
	for (i=0; i<=MAX_CLIENTS; ++i)//gpGlobals->maxClients; ++i)// update entire array
		g_ClientShouldInitialize[i] = 1;// XDM3037: world spawns before connecting any clients

	Precache();

	// XDM: from CBasePlayer::Precache()
	if (sv_nodegraphdisable.value <= 0.0f)// XDM3035a
	{
		if (WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet)
		{
			if (!WorldGraph.FSetGraphPointers())
				conprintf(1, "** Graph pointers were not set!\n");
			else
				conprintf(1, "** Graph Pointers Set!\n");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Precache all common game resources here
//-----------------------------------------------------------------------------
void CWorld::Precache(void)
{
#if 1
	CVAR_SET_STRING("sv_gravity", UTIL_dtos1(DEFAULT_GRAVITY));//"800"); // 67ft/sec
	CVAR_SET_STRING("sv_stepsize", "18");
#else
	CVAR_SET_STRING("sv_gravity", "384"); // 32ft/sec
	CVAR_SET_STRING("sv_stepsize", "24");
#endif

	//CVAR_SET_STRING("room_type", "0");// clear DSP
	CVAR_SET_FLOAT("room_type", m_iRoomType);// XDM3038c

	CVAR_SET_STRING("sv_maxvelocity", "65535");// XDM
	CVAR_SET_STRING("tracerspeed", "65535");

	//g_Language = (int)CVAR_GET_FLOAT("sv_language");

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

#if defined(_DEBUG_INFILOOPS)
	sv_dbg_maxcollisionloops.value = gpGlobals->maxEntities+1;// XDM3038a
#endif
	mp_maprules.value = 0.0f;
	mp_maxteams.value = 0.0f;// XDM3038a
	CVAR_DIRECT_SET(&mp_maprules, "0");
	CVAR_DIRECT_SET(&mp_maxteams, "0");

	if (sv_generategamecfg.value > 0.0f)
		CONFIG_GenerateFromTemplate("settings.scr", "game.cfg", false);

	SERVER_COMMAND("exec game.cfg\n");
	SERVER_EXECUTE();

	ENTCONFIG_ExecMapPreConfig();// XDM3035b: give user a chance to pre-define some variables for specific map

	// Decide which game type to use. May be affected by previously executed configs.
	int iNewGameType = DetermineGameType();

	// Set up game rules after executing all configs
	if (g_pGameRules)
	{
		if (!g_pGameRules->FPersistBetweenMaps() || g_pGameRules->GetGameType() != iNewGameType)
		{
			SERVER_PRINT("Destroying old game rules\n");// XDM3036: don't keep even persistent rules if new map defines different game type
			delete g_pGameRules;
			g_pGameRules = NULL;
		}
	}

	if (pev->gravity == 0.0f)// XDM3035c: prevent default editor values from corrupting the game
		pev->gravity = 1.0f;
	else if (pev->gravity > 10.0f)// XDM3038: HACK to accept some bogus maps (assuming cvar-like scale)
	{
		SERVER_PRINTF("Warning: absolute gravity value (%g) detected in world! Converting to relative.\n", pev->gravity);
		pev->gravity /= DEFAULT_GRAVITY;// mapper probably used standard HL scale, revert to relative!
	}

	if (pev->friction == 0.0f)
		pev->friction = 1.0f;

	if (g_pGameRules == NULL)
		g_pGameRules = InstallGameRules(iNewGameType);

	if (g_pGameRules)
		g_pGameRules->Initialize();// XDM: this applies to both new and persistent rules
	else
		SERVER_PRINT("Game rules: Initialization failed!\n");

	if (pSoundEnt)// XDM3037
		pSoundEnt->Initialize();

	SENTENCEG_Init();

	SERVER_PRINT("Precaching common resources:\n");// XDM

	// the area based ambient sounds MUST be the first precache_sounds
	// sounds used from C physics code
	PRECACHE_SOUND("common/null.wav");// clears sound channels
	PRECACHE_SOUND("common/bodyhit1.wav");// XDM3037
	PRECACHE_SOUND("common/bodysplat.wav");// XDM
	PRECACHE_SOUND("common/watersplash.wav");
	/*PRECACHE_SOUND("common/npc_step1.wav");// XDM3038c: moved to CBaseMonster::Precache()
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");*/

	SERVER_PRINT("Precaching materials...\n");
	// material/texture hit sounds
	MaterialSoundPrecache(matGlass);
	MaterialSoundPrecache(matWood);
	MaterialSoundPrecache(matMetal);
	MaterialSoundPrecache(matFlesh);
	MaterialSoundPrecache(matCinderBlock);
	MaterialSoundPrecache(matCeilingTile);
	//MaterialSoundPrecache(matComputer);			// same as matGlass
	//MaterialSoundPrecache(matUnbreakableGlass);	// same as matGlass
	//MaterialSoundPrecache(matRocks);				// same as matCinderBlock

	size_t i = 0;
	size_t c = NUM_BODYDROP_SOUNDS;//sizeof(gSoundsDropBody);// does not work!!!!
	for (i = 0; i < c; ++i)
		PRECACHE_SOUND((char *)gSoundsDropBody[i]);

	c = NUM_RICOCHET_SOUNDS;//ARRAYSIZE(gSoundsRicochet);
	for (i = 0; i < c; ++i)
		PRECACHE_SOUND((char *)gSoundsRicochet[i]);

	c = NUM_SPARK_SOUNDS;//ARRAYSIZE(gSoundsSparks);
	for (i = 0; i < c; ++i)
		PRECACHE_SOUND((char *)gSoundsSparks[i]);

	// XDM3037: glass sounds for frozen gibbage
	c = NUM_BREAK_SOUNDS;
	for (i = 0; i < c; ++i)
		PRECACHE_SOUND((char *)gBreakSounds[matGlass][i]);

	/* no! too many unneeded resources!
	c = NUM_MATERIALS;
	for(i = 0; i < c; ++i)
		UTIL_PrecacheMaterial(&gMaterials[i]);*/

	ClientPrecache();// Precache common resources used by players

	PrecacheSharedResources();

	conprintf(1, "Restoring saved ammo types...\n");
	for (i = 0; i < MAX_AMMO_SLOTS; ++i)// register ammo types as they were in previous game so indexes are preserved
	{
		if (!FStringNull(m_rgiszAmmoTypes[i]))
			AddAmmoToRegistry(STRING(m_rgiszAmmoTypes[i]), m_rgAmmoMaxCarry[i]);
	}

	W_Precache();// precache weapon and ammo resources

	conprintf(1, "Saving ammo types...\n");
	for (i = 0; i < MAX_AMMO_SLOTS; ++i)// now lists are merged, save and replace everything
	{
		if (g_AmmoInfoArray[i].name[0] == '\0')
			m_rgiszAmmoTypes[i] = iStringNull;
		else
			m_rgiszAmmoTypes[i] = MAKE_STRING(g_AmmoInfoArray[i].name);

		m_rgAmmoMaxCarry[i] = g_AmmoInfoArray[i].iMaxCarry;
	}
	// TODO: while(fgets("scripts/sv_entprecache.lst")) { UTIL_PrecacheOther(str); }

	PrecacheEvents();// XDM3038c

	SERVER_PRINT("Loading light styles...\n");// XDM

	// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
	for (i = 0; g_szStandardLightStyles[i]; ++i)
		LIGHT_STYLE(i, (char *)g_szStandardLightStyles[i]);

	// styles 32-62 are assigned by the light program for switchable lights
	// 63 testing
	LIGHT_STYLE(63, "a");

	SERVER_PRINT("Done precaching.\n");// XDM

	// We need to have server cvars set (from server cfg) at this point!
	ENTCONFIG_Init();// XDM: load custom entities form <mapname>.ent file

	// Init the WorldGraph for monster navigation
	if (sv_nodegraphdisable.value <= 0.0f)// XDM3035a
	{
		WorldGraph.InitGraph();
		if (g_pGameRules && g_pGameRules->FAllowMonsters())// XDM3035
		{
			if (WorldGraph.CheckNODFile(STRINGV(gpGlobals->mapname)))// make sure the .NOD file is newer than the .BSP file.
			{
				if (!WorldGraph.FLoadGraph(STRINGV(gpGlobals->mapname)))// Load the node graph for this level
				{
					//conprintf(1, "*Error opening .NOD file\n");
					WorldGraph.AllocNodes();// couldn't load, so alloc and prepare to build a graph.
				}
				else
					conprintf(1, "\n*Graph Loaded!\n");
			}
			else
				WorldGraph.AllocNodes();// NOD file is not present, or is older than the BSP file.
		}
	}

	if (m_iMaxVisibleRange > 0)
		CVAR_SET_FLOAT("sv_zmax", m_iMaxVisibleRange);// copied to movevars
	else
		CVAR_SET_FLOAT("sv_zmax", 4096);

	/* XDM3038c: v_dark is bad if (FBitSet(pev->spawnflags, SF_WORLD_DARK))// this stuff works only in single
	{
		ClearBits(pev->spawnflags, SF_WORLD_DARK);// XDM3038c: clear so it won't work on restore
		CVAR_SET_FLOAT("v_dark", 1.0);
	}
	else
		CVAR_SET_FLOAT("v_dark", 0.0);*/
}

// Better not override entvars here
void CWorld::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "skyname"))
	{
		CVAR_SET_STRING("sv_skyname", pkvd->szValue);// Sent over net now.
		if (StringInList(pkvd->szValue, "scripts/sky_no_airstrike.txt"))
		{
			SetBits(pev->spawnflags, SF_WORLD_NOAIRSTRIKE);
			SERVER_PRINTF("Map skyname \"%s\" blacklisted for airstrikes.\n", pkvd->szValue);
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		// HACK: DO NOT USE GLOBALS! Engine chokes on it! gpGlobals->cdAudioTrack = atoi(pkvd->szValue);
		m_iAudioTrack = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		pev->scale = atof(pkvd->szValue) * (1.0/8.0);// engine reads this
		CVAR_SET_FLOAT("sv_wateramp", pev->scale);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "MaxRange"))
	{
		m_iMaxVisibleRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "chaptertitle"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);// may be used by the engine?
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "startdark"))
	{
		pkvd->fHandled = TRUE;
		if (atoi(pkvd->szValue) > 0)
			SetBits(pev->spawnflags, SF_WORLD_DARK);
	}
	else if (FStrEq(pkvd->szKeyName, "newunit"))// Single player only. Clear save directory if set
	{
		if (atoi(pkvd->szValue) > 0)
		{
			SetBits(pev->spawnflags, SF_WORLD_NEWUNIT);// XDM3038c
			CVAR_SET_FLOAT("sv_newunit", 1);
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "gametitle"))
	{
		if (atoi(pkvd->szValue))
			SetBits(pev->spawnflags, SF_WORLD_TITLE);

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "gameprefix"))// XDM3038a: string, because indexes are unstable
	{
		if (pkvd->szValue && *pkvd->szValue)
			m_iszGamePrefix = ALLOC_STRING(pkvd->szValue);
		else
			m_iszGamePrefix = iStringNull;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "mapteams"))// XDM3037: unused
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "maxteams"))// XDM3037
	{
		m_iMaxMapTeams = atoi(pkvd->szValue);// XDM3035c: no problems with save/restore
		pkvd->fHandled = TRUE;
	}
	else if (strncmp(pkvd->szKeyName, "teamstartdelay", 14) == 0)// XDM3038a: up to 5 values
	{
		TEAM_ID team = atoi(pkvd->szKeyName + 14);
		if (team >= 0 && team <= MAX_TEAMS)
		{
			m_iTeamDelay[team] = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
	}
	else if (FStrEq(pkvd->szKeyName, "defaultteam"))
	{
		if (atoi(pkvd->szValue) > 0)
			SetBits(pev->spawnflags, SF_WORLD_FORCETEAM);

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "roomtype"))// XDM
	{
		m_iRoomType = atoi(pkvd->szValue);// XDM3038c: save/restore?
		if (g_ServerActive)
			CVAR_SET_FLOAT("room_type", m_iRoomType);// XDM3038c: apply immediately

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "startsuit"))
	{
		if (atoi(pkvd->szValue) > 0)
			SetBits(pev->spawnflags, SF_WORLD_STARTSUIT);

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "nomonsters"))
	{
		if (atoi(pkvd->szValue) > 0)
		{
			SetBits(pev->spawnflags, SF_WORLD_NOMONSTERS);
			conprintf(1, "World policy: explicitly DENY monsters.\n");
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "monstersrespawnpolicy"))
	{
		int iPolicy = atoi(pkvd->szValue);
		if (iPolicy == POLICY_ALLOW)
		{
			CVAR_DIRECT_SET(&mp_monstersrespawn, "1");
			conprintf(1, "World policy: explicitly ALLOW monsters to respawn.\n");
		}
		else if (iPolicy == POLICY_DENY)
		{
			CVAR_DIRECT_SET(&mp_monstersrespawn, "0");
			conprintf(1, "World policy: explicitly DENY monsters to respawn.\n");
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "airstrikepolicy"))// XDM3038a
	{
		int iPolicy = atoi(pkvd->szValue);
		if (iPolicy == POLICY_ALLOW)
		{
			ClearBits(pev->spawnflags, SF_WORLD_NOAIRSTRIKE);
			conprintf(1, "World policy: explicitly ALLOW airstrikes.\n");
		}
		else if (iPolicy == POLICY_DENY)
		{
			SetBits(pev->spawnflags, SF_WORLD_NOAIRSTRIKE);
			conprintf(1, "World policy: explicitly DENY airstrikes.\n");
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "compiler"))
	{
		conprintf(2, "Info: map was compiled using %s\n", pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "wad"))
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "worldtype"))
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "mapversion"))
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "version"))
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "light"))
		pkvd->fHandled = TRUE;
	else if (FStrEq(pkvd->szKeyName, "copyright"))
		pkvd->fHandled = TRUE;
	else
		CBaseEntity::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: A really nice way to send global data to all players
// Note   : UNDONE!!!! What if a player is asking for data more than one time?!
// Input  : *pClient - 
//			msgtype - 
//			sendcase - 
// Output : int - number of messages sent
//-----------------------------------------------------------------------------
int CWorld::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	int sent = 0;
	if (msgtype == MSG_ONE && pClient)// a single client asking for update
	{
		if (sendcase != SCD_CLIENTRESTORE)// don't show titles upon restoring a game
		{
			if (!FStringNull(pev->netname))// XDM3037: map chapter title
			{
				ClientPrint(pClient->pev, HUD_PRINTHUD, STRING(pev->netname));
				++sent;
			}
			if (FBitSet(pev->spawnflags, SF_WORLD_TITLE) || (IsMultiplayer() && mp_showgametitle.value > 0.0f))
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgShowGameTitle, NULL, pClient->edict());
					WRITE_BYTE(0);
					//WRITE_STRING(STRING(m_iszTitleSprite));// not needed if we have custom HUD text file
				MESSAGE_END();
				++sent;
			}
			if (FBitSet(pev->spawnflags, SF_WORLD_DARK)/* && IsMultiplayer()*/)// only in multiplayer, where v_dark does not work
			{
				UTIL_ScreenFade(pClient, g_vecZero, 1.5f, IsMultiplayer()?(mp_respawntime.value + 1.0f):3.0f, 255, FFADE_IN | FFADE_MODULATE);
				++sent;
			}
			if (!FStringNull(pev->noise))// map audio track, NOT when restoring!
			{
				pClient->PlayAudioTrack(STRING(pev->noise), MPSVCMD_PLAYFILELOOP, 0);// XDM3038c: LOOP
				++sent;
			}
			else if (m_iAudioTrack > 1)//(gpGlobals->cdAudioTrack > 1)
			{
				char buffer[32];
				_snprintf(buffer, 32, "%d", m_iAudioTrack);//gpGlobals->cdAudioTrack);// XDM3038a: better
				pClient->PlayAudioTrack(buffer, MPSVCMD_PLAYTRACK, 0);
				++sent;
			}
		}
	}
	return sent;
}

//-----------------------------------------------------------------------------
// Purpose: Called before destructor
//-----------------------------------------------------------------------------
void CWorld::OnFreePrivateData(void)
{
	ClearBits(gpGlobals->serverflags, FSERVER_RESTORE);// XDM3037
	if (IsMultiplayer())// Fix all temporary users in mp
	{
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "DelayedUse")) != NULL)
		{
			pEntity->m_pGoalEnt = NULL;
			//UTIL_Remove(pEntity);
		}
	}
	//memset(g_ClientShouldInitialize, 0, sizeof(
	int i;
	// SHL, G-Cont
	CBaseEntity *pEntity = NULL;
	for (i=1; i <= gpGlobals->maxEntities; ++i)
	{
		edict_t *pEntityEdict = INDEXENT(i);
		if (UTIL_IsValidEntity(pEntityEdict) && !FStringNull(pEntityEdict->v.globalname))
		{
			pEntity = CBaseEntity::Instance(pEntityEdict);
			if (pEntity)
				pEntity->PrepareForTransfer();
		}
	}
	CBaseEntity::OnFreePrivateData();// XDM3038c
	//should we?	g_pWorld = NULL;
}
