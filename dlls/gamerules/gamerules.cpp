//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//
// WARNING: always keep in mind that players may connect and disconnect at any impossible moment!
// WARNING: players respawning mechanism differs from entities
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "globals.h"
#include "skill.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "maprules.h"
#include "teamplay_gamerules.h"
#include "coop_gamerules.h"// XDM
#include "dom_gamerules.h"
#include "ctf_gamerules.h"
#include "round_gamerules.h"
#include "lms_gamerules.h"
#include "entconfig.h"

// Maps with certain prefixes indicate they were designed for these game rules:
// Prefixes for same rules must be sorted by most-to-least preferred, so reverse algorithm can return the first encountered prefix.
gametype_prefix_t gGameTypePrefixes[] =
{
	{"DM",		GT_DEATHMATCH},
	{"CO",		GT_COOP},
	{"CTF",		GT_CTF},
	{"DOM",		GT_DOMINATION},
	{"LMS",		GT_LMS},
	{"TM",		GT_TEAMPLAY},
	{"RM",		GT_ROUND},
	{"AS",		GT_ROUND},// assault
	{"OP4CTF",	GT_CTF},
	{NULL,		0}// IMPORTANT! Must end with a null-terminator.
};

DLL_GLOBAL CGameRules *g_pGameRules = NULL;


//-----------------------------------------------------------------------------
// Purpose: Default constructor
//-----------------------------------------------------------------------------
CGameRules::CGameRules()
{
	//m_iGameType = 0;
	m_iGameMode = 0;
	GameSetState(GAME_STATE_WAITING);// XDM3037a
	m_fStartTime = 0;//gpGlobals->time;
	m_iPersistBetweenMaps = 0;
	m_bGameOver = false;
	memset(m_szLastMap, 0, MAX_MAPNAME);// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameRules::~CGameRules()
{
	//m_iGameType = 255;// ?
}

//-----------------------------------------------------------------------------
// Purpose: GetGameType
// Output : short
//-----------------------------------------------------------------------------
short CGameRules::GetGameType(void)
{
	conprintf(0, "CGameRules: ERROR: PURE VIRTUAL GAME RULES AND NO TYPE!\n");
	DBG_FORCEBREAK
	return 255;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a GameSetState
// Input  : iGameState - 
//-----------------------------------------------------------------------------
void CGameRules::GameSetState(short iGameState)
{
	conprintf(2, "CGameRules::GameSetState(%hd)\n", iGameState);// debug
	if (m_iGameState != iGameState)
	{
		if (iGameState == GAME_STATE_ACTIVE)
		{
			m_fStartTime = gpGlobals->time;// XDM3037a: IMPORTANT: start counting time from this moment
			//OnGameStart();
			//GAME_EVENT_START?
		}
		/*else if (iGameState == GAME_STATE_FINISHED)
		{
			m_fStartTime = 0;// XDM3037a: ?
			GAME_EVENT_END?
			OnGameStop();
		}*/
		m_iGameState = iGameState;
		UpdateGameMode(NULL);// ?
	}
	else
		SERVER_PRINTF("CGameRules::GameSetState(%hd): state is already set!\n", iGameState);
}

//-----------------------------------------------------------------------------
// Purpose: First thing called after constructor. Initialize all data, cvars, etc.
// Warning: Must be called by all derived classes!
// Warning: Called from CWorld::Precache(), so all other entities are not spawned yet!
//-----------------------------------------------------------------------------
void CGameRules::Initialize(void)
{
	conprintf(2, "CGameRules::Initialize()\n");
	g_iMapHasLandmarkedSpawnSpots = 0;// XDM3038b: this must be reset before any netities are created
	if (FPersistBetweenMaps() && m_bGameOver)// persist flag is ON, and game ove is ON (previous match is over)
		SERVER_PRINTF("Game rules (%hd) transfered between maps\n", GetGameType());
	else
		memset(m_szLastMap, 0, MAX_MAPNAME);// XDM3038a: this is a member variable, it was just allocated so clear it.

	// Since these may be changed later, more checks must be done before use
	if (sv_playermaxhealth.value < 1.0f)// XDM3037
	{
		sv_playermaxhealth.value = MAX_PLAYER_HEALTH;// default to normal
		CVAR_DIRECT_SET(&sv_playermaxhealth, UTIL_dtos1((int)sv_playermaxhealth.value));
	}
	if (sv_playermaxarmor.value < 1.0f)// XDM3038a
	{
		sv_playermaxarmor.value = 0.0f;
		CVAR_DIRECT_SET(&sv_playermaxarmor, "0");
	}
	if (sv_radardist.value <= 0.0f)// XDM3038a
	{
		sv_radardist.value = 512;
		CVAR_DIRECT_SET(&sv_radardist, "512");
	}
	m_fStartTime = gpGlobals->time;// XDM3038a: WARNING! Must be set here because some rules persist between maps!
	m_bGameOver = false;
	RefreshSkillData();// XDM: do this after loading CFG files
}

//-----------------------------------------------------------------------------
// Purpose: load the SkillData struct with the proper values based on the skill level
//-----------------------------------------------------------------------------
void CGameRules::RefreshSkillData(void)
{
	int iSkill = (int)CVAR_GET_FLOAT("skill");
	if (iSkill < SKILL_EASY)
		iSkill = SKILL_EASY;
	else if (iSkill > SKILL_HARD)
		iSkill = SKILL_HARD; 

	CVAR_SET_FLOAT("skill", (float)iSkill);
	conprintf(0, " Skill level: %d\n", iSkill);
	SkillUpdateData(iSkill);
}

//-----------------------------------------------------------------------------
// Purpose: Should this entity be shown on the minimap?
// Input  : *pEntity - 
//			*pPlayer - map owner
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::FShowEntityOnMap(CBaseEntity *pEntity, CBasePlayer *pPlayer)
{
	if (pEntity->IsPlayer())
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called when current weapon becomes unusable.
// Updated: Get method DOES NOT SELECT anything
// Warning: XDM3037: This is a DEFAULT method, which does not consider player's individual weapon priority list
// Input  : *pPlayer - inventory owner
//			*pCurrentWeapon - WARNING: may be active, already detached from player or NULL!
// Output : *pBest may be NULL if no weapons
//-----------------------------------------------------------------------------
CBasePlayerItem *CGameRules::GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon)
{
	CBasePlayerItem *pCheck = NULL;
	CBasePlayerItem *pBest = NULL;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight = -1;
	//if (pCurrentWeapon)
	//	iBestWeight = pCurrentWeapon->iWeight();
	for (int i = WEAPON_NONE+1; i < PLAYER_INVENTORY_SIZE; ++i)
	{
		pCheck = pPlayer->GetInventoryItem(i);
		if (pCheck)
		{
			if (pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon && pCheck->CanDeploy())// don't reselect the weapon we're trying to get rid of
			{
				iBestWeight = pCheck->iWeight();// if this weapon is useable, flag it as the best
				pBest = pCheck;
			}
			/*else if (pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon)// this weapon has the same priority
			{
				if (pCheck->CanDeploy())
				{
					pBest = pCheck;
				}
			}*/
		}
	}
//#if defined (_DEBUG)
//	conprintf(2, "CGameRules::GetNextBestWeapon(): %d\n", pBest?pBest->GetID():WEAPON_NONE);
//#endif
	return pBest;
}

//-----------------------------------------------------------------------------
// Purpose: Do NOT get into player's inventory here! Use as entity filter!
// Input  : *pPlayer - 
//			iAmmoIndex - 
//			iMaxCarry - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*bool CGameRules::CanHaveAmmo(CBasePlayer *pPlayer, const int &iAmmoIndex)
{
	return true;// player has room for more of this type of ammo
}*/

//-----------------------------------------------------------------------------
// Purpose: Is this spawn spot valid for specified player?
// Warning: Should only be called through global SpawnPointValidate()
// Input  : *pPlayer - 
//			pSpot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::ValidateSpawnPoint(CBaseEntity *pPlayer, CBaseEntity *pSpot)// XDM3038a
{
//	DBG_GR_PRINT("CGameRules::ValidateSpawnPoint(%d %s, %d %s)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"", pSpot?pSpot->entindex():0, pSpot?STRING(pSpot->pev->classname):"");
	if (pSpot)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Move player to a proper spawn point
// Warning: IsObserver() is not effective yet because ObserverStart() will be called afterwards.
// Input  : *pPlayer - 
//			bSpectator - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CGameRules::PlayerUseSpawnSpot(CBasePlayer *pPlayer, bool bSpectator)
{
//	DBG_GR_PRINT("CGameRules::PlayerUseSpawnSpot(%d %s, bSpectator %d)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"", bSpectator?1:0);
	CBaseEntity *pSpawnSpot = NULL;
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		pSpawnSpot = SpawnPointEntSelect(pPlayer, bSpectator);
#if defined (USE_EXCEPTIONS)
	}
	catch(...)
	{
		SERVER_PRINT("SpawnPointEntSelect() exception!\n");
		conprintf(1, "CGameRules::SpawnPointEntSelect(%d, spec:%d) exception!\n", pPlayer->entindex(), bSpectator?1:0);
		pSpawnSpot = NULL;
	}
#endif
	if (pSpawnSpot)
	{
		pPlayer->pev->oldorigin = pPlayer->pev->origin;// XDM3037a: ensure we save this for special effects

		if (pSpawnSpot->IsBSPModel())// XDM3035c: spawn inside trigger
			pPlayer->pev->origin = RandomVector(pSpawnSpot->pev->absmin, pSpawnSpot->pev->absmax);
		else
			pPlayer->pev->origin = pSpawnSpot->pev->origin;

		pPlayer->pev->origin.z += 1.0f;// hack?
		pPlayer->pev->velocity = pSpawnSpot->pev->velocity;// XDM3035c: TESTME! g_vecZero;
		pPlayer->pev->angles = pSpawnSpot->pev->angles;
		pPlayer->pev->punchangle.Clear();
		pPlayer->pev->v_angle.Clear();
		pPlayer->pev->fixangle = TRUE;
		ClearBits(pPlayer->pev->flags, FL_ONGROUND);// XDM3038c
		//UTIL_SetOrigin(pPlayer, pPlayer->pev->origin);// XDM3038c: set by Materialize() <-- Spawn()

		if (!bSpectator && !FStringNull(pSpawnSpot->pev->target))// XDM3038a
			FireTargets(STRING(pSpawnSpot->pev->target), pPlayer, pPlayer, USE_TOGGLE, 0);
	}
	else
		conprintf(1, "CGameRules::PlayerUseSpawnSpot(%d) failed!\n", pPlayer->entindex());

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Can this player have specified item?
// Input  : *pPlayer - player instance
//			*pItem - item instance
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem)
{
	if (!pPlayer->IsAlive())// only living players can have items
		return false;

	if (!g_SilentItemPickup && pPlayer->m_fInitHUD)// XDM: HACK: prevent player from touching in-world weapons during HUD initialization
		return false;

	if (pPlayer->GetInventoryItem(pItem->GetID()))// player has an item of this type (ID)
	{
		CBasePlayerWeapon *pWeapon = pItem->GetWeaponPtr();
		if (pWeapon)// actually iz wepn
		{
			int iIndex = pWeapon->PrimaryAmmoIndex();
			// we can't carry anymore ammo for this gun. We can only have the gun if we aren't already carrying one of this type
			// if (pWeapon->pszAmmo1() && CanHaveAmmo(pPlayer, GetAmmoIndexFromRegistry(pWeapon->pszAmmo1())))// XDM: don't use pWeapon->PrimaryAmmoIndex()!!!
			if (iIndex >= 0 && (pPlayer->AmmoInventory(iIndex) < g_AmmoInfoArray[iIndex].iMaxCarry))
				return true;

			iIndex = pWeapon->SecondaryAmmoIndex();
			//if (pWeapon->pszAmmo2() && CanHaveAmmo(pPlayer, GetAmmoIndexFromRegistry(pWeapon->pszAmmo2())))
			if (iIndex >= 0 && (pPlayer->AmmoInventory(iIndex) < g_AmmoInfoArray[iIndex].iMaxCarry))
				return true;
			// Weapon doesn't use any ammo, don't take another if you already have it.
		}
	}
	else
		return true;

	// NOTE: will fall through to here if GetItemInfo doesn't fill the struct!
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Can this player drop specified item?
// Warning: Assumes the item is in player's inventory!
// Input  : *pPlayer - player instance
//			*pItem - existing item instance
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::CanDropPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem)
{
	if (pPlayer == NULL || pItem == NULL)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Add score and awards UNDONE: monsters can score??
// Input  : *pWinner - NULL means everyone
//			score - can be negative :D
//-----------------------------------------------------------------------------
bool CGameRules::AddScore(CBaseEntity *pWinner, int score)
{
	//conprintf(1, "CGameRules::AddScore(%d, %d)\n", pWinner?pWinner->entindex():0, score);
	if (IsGameOver())
		return false;

	if (pWinner)
	{
		pWinner->pev->frags += (float)score;
	}
	else// XDM3038a: let's have fun here
	{
		conprintf(2, "CGameRules::AddScore(EVERYONE, %d)\n", score);
		CBasePlayer *pLucky;
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			pLucky = UTIL_ClientByIndex(i);
			if (pLucky)
				pLucky->pev->frags += (float)score;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Real number of active players on server
// Warning: DOES NOT CHECK FOR SPECTATORS!
// Output : uint32
//-----------------------------------------------------------------------------
uint32 CGameRules::CountPlayers(void)
{
	uint32 c = 0;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		if (UTIL_ClientByIndex(i) != NULL)
			++c;
	}
	return c;
}

//-----------------------------------------------------------------------------
// Purpose: Should autoaim track this target?
// Input  : *pPlayer - 
//			*pTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::FShouldAutoAim(CBasePlayer *pPlayer, CBaseEntity *pTarget)
{
	if (pPlayer == pTarget)
		return false;

	int relation = PlayerRelationship(pPlayer, pTarget);// XDM3038
	if (relation == GR_NOTTEAMMATE || relation == GR_ENEMY)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sends current game rules to a client
// Input  : *pPlayer - dest
//-----------------------------------------------------------------------------
void CGameRules::UpdateGameMode(edict_t *pePlayer)
{
	DBG_GR_PRINT("CGameRules::UpdateGameMode(client %d)\n", ENTINDEX(pePlayer));
	if (!g_ServerActive)
		return;

	unsigned char gameflags = 0;// XDM3035a: some useful extensions

	if (g_pGameRules->GetCombatMode() == GAME_COMBATMODE_NOSHOOTING)
		gameflags |= GAME_FLAG_NOSHOOTING;

	if (mp_allowcamera.value > 0.0f)
		gameflags |= GAME_FLAG_ALLOW_CAMERA;

	if (g_pGameRules->FAllowSpectators())
		gameflags |= GAME_FLAG_ALLOW_SPECTATORS;

	if (mp_specteammates.value <= 0.0f)// XDM3037
		gameflags |= GAME_FLAG_ALLOW_SPECTATE_ALL;

	if (mp_allowminimap.value > 0)// XDM3037a
		gameflags |= GAME_FLAG_ALLOW_OVERVIEW;

	if (mp_allowhighlight.value > 0)// XDM3038c
		gameflags |= GAME_FLAG_ALLOW_HIGHLIGHT;

	if (sv_overdrive.value > 0.0f)
		gameflags |= GAME_FLAG_OVERDRIVE;

	if (mp_teambalance.value > 0.0f)// XDM3037a
		gameflags |= GAME_FLAG_BALANCETEAMS;

	if (pePlayer)
		MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pePlayer);
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgGameMode);

		WRITE_BYTE(g_pGameRules->GetGameType());
		WRITE_BYTE(m_iGameMode);
		WRITE_BYTE(m_iGameState);// XDM3037a
		WRITE_BYTE(gSkillData.iSkillLevel);// XDM3035a
		WRITE_BYTE(gameflags);// XDM3035a is this a good place for these? (can be changed by the server on-the-fly)
		WRITE_BYTE((int)mp_revengemode.value);
	if (IsGameOver())
		WRITE_LONG((int)mp_chattime.value);
	else
		WRITE_LONG(GetTimeLimit());// XDM3038: now in seconds (max 2147483647s = ~596523.2h = ~24855d = ~68y)

		WRITE_LONG(GetScoreLimit());// XDM3035a: was (int)mp_scorelimit.value
		WRITE_LONG((int)mp_deathlimit.value);// XDM3038a
		WRITE_BYTE(GetRoundsLimit());
		WRITE_BYTE(GetRoundsPlayed());
		WRITE_BYTE(GetPlayerMaxHealth());// XDM3037
		WRITE_BYTE(GetPlayerMaxArmor());// XDM3038
		WRITE_FLOAT(m_fStartTime);// XDM3038a
	MESSAGE_END();
	//m_iGameFlags = gameflags;
}

//-----------------------------------------------------------------------------
// Purpose: Client entered a console command
// Warning: Don't allow cheats
// Input  : *pPlayer - client
//			*pcmd - command line, use CMD_ARGC() and CMD_ARGV()
// Output : Returns true if the command was recognized.
//-----------------------------------------------------------------------------
bool CGameRules::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	if (FStrEq(pcmd, "showstats"))
	{
		if (pPlayer->m_flNextChatTime <= gpGlobals->time)// a little hacky: reuse this for DoS prevention
		{
			//if (FBitSet(pPlayer->m_iGameFlags, EGF_SPAWNED))
			MESSAGE_BEGIN(MSG_ONE, gmsgPlayerStats, NULL, pPlayer->edict());
				WRITE_BYTE(pPlayer->entindex());
				WRITE_BYTE(STAT_NUMELEMENTS);
				for (short c = 0; c < STAT_NUMELEMENTS; ++c)
					WRITE_LONG(pPlayer->m_Stats[c]);

			MESSAGE_END();
			pPlayer->m_flNextChatTime = gpGlobals->time + 2.0f;
		}
	}
	else
		return false;//	return CGameRules::ClientCommand(pPlayer, pcmd);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update player skin immediately (without restart). Warning! Should not interfere with teamplay!
// Input  : *pPlayer - 
//			*infobuffer - 
//-----------------------------------------------------------------------------
void CGameRules::ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer)
{
	const char *strskin = GET_INFO_KEY_VALUE(infobuffer, "skin");
	if (strskin)
		pPlayer->pev->skin = atoi(strskin);
}

//-----------------------------------------------------------------------------
// Purpose: World models will be scaled by this factor. Don't use direcly!
//			Call UTIL_GetWeaponWorldScale() instead!
// Output : float
//-----------------------------------------------------------------------------
float CGameRules::GetWeaponWorldScale(void)
{
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Is game over?
// Output : Returns TRUE on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRules::IsGameOver(void)
{
	return m_bGameOver;
}

//-----------------------------------------------------------------------------
// Purpose: the one and only way
//-----------------------------------------------------------------------------
//void CGameRules::SetGameType(short gametype)
//{
//		m_iGameType = gametype;
//}

//-----------------------------------------------------------------------------
// Purpose: Server is activated, all entities spawned
// Input  : *pEdictList - 
//			edictCount - 
//			clientMax - 
//-----------------------------------------------------------------------------
void CGameRules::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	DBG_GR_PRINT("CGameRules::ServerActivate(..., %d, %d)\n", edictCount, clientMax);
	if (FStartImmediately())// XDM3038: some game rules do not start immediately (LMS)
		GameSetState(GAME_STATE_ACTIVE);// XDM3037a
	else// GameSetState() already calls UpdateGameMode()
		UpdateGameMode(NULL);// XDM3038: more reliability for proxy DLLs (but HL will sometimes complain PF_MessageEnd_I:  Unknown User Msg 84) 
}

//-----------------------------------------------------------------------------
// Purpose: Dump debug info to console
//-----------------------------------------------------------------------------
void CGameRules::DumpInfo(void)
{
	SERVER_PRINT(UTIL_VarArgs("Game Rules: %s\nType: %hd, Mode: %hd, CombatMode: %hd, Start time: %g\nIsMustiplayer: %s, IsTeamplay: %s, IsRoundBased: %s, IsGameOver: %s, PersistBetweenMaps: %hd\n",
		GetGameDescription(), GetGameType(), GetGameMode(), GetCombatMode(), GetStartTime(),
		IsMultiplayer()?"yes":"no", IsTeamplay()?"yes":"no", IsRoundBased()?"yes":"no", IsGameOver()?"yes":"no", m_iPersistBetweenMaps));
}






//-----------------------------------------------------------------------------
// Purpose: Search map for player enemies
// Output : int
//-----------------------------------------------------------------------------
/* useless because monsters are not added yet
int CheckMapForMonsters(void)
{
	// Prepare ALL entities for the game
	int entindex = 1;
	edict_t *pEdict = NULL;
	do
	{
		pEdict = INDEXENT(entindex);
		++entindex;
		if (pEdict)
		{
			if (!pEdict->free)
			{
				CBaseEntity *pEntity = NULL;
				pEntity = CBaseEntity::Instance(pEdict);
				if (pEntity)
				{
					if (pEntity->IsMonster())
					{
						if (g_iRelationshipTable[pEntity->Classify()][CLASS_PLAYER] > R_NO)// dislike or hate players
							return 1;// don't count, just return
					}
				}
			}
		}
	} while (entindex < gpGlobals->maxEntities);
	return 0;
}*/

//-----------------------------------------------------------------------------
// Purpose: Get default map prefix data for specified game type
// Input  : game_type
// Output : gametype_prefix_t *
//-----------------------------------------------------------------------------
gametype_prefix_t *FindGamePrefix(int game_type)
{
	size_t prefix = 0;
	while (gGameTypePrefixes[prefix].prefix)
	{
		if (gGameTypePrefixes[prefix].gametype == game_type)
			return &gGameTypePrefixes[prefix];

		++prefix;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Detect game type by game type-specific prefix string (case-insensitive)
// Input  : pString - string to compare, can be longer than prefix - only the beginning is compared
// Output : int GT_SINGLE
//-----------------------------------------------------------------------------
int DetectGameTypeByPrefix(const char *pString)
{
	DBG_PRINTF("DetectGameTypeByPrefix(%s)\n", pString);
	if (pString)
	{
		size_t prefix = 0, i;
		bool success;
		while (gGameTypePrefixes[prefix].prefix)// pick a line from association table
		{
			i = 0;
			success = true;
			while (gGameTypePrefixes[prefix].prefix[i])// compare strings
			{
				if (pString[i] == '\0' || toupper(pString[i]) != toupper(gGameTypePrefixes[prefix].prefix[i]))// case-insensitive comparison
				{
					success = false;
					break;// string ended earlier or comparison failed
				}
				++i;
			}
			if (success && pString[i] == '_')// "CO_Columns" is valid, "Columns" is invalid
			{
				return gGameTypePrefixes[prefix].gametype;
				break;
			}
			++prefix;
		}
	}
	return GT_SINGLE;
}

//-----------------------------------------------------------------------------
// Purpose: DetermineGameType based on 'deathmatch, mp_maprules' and map name
// Output : int GT_SINGLE
//-----------------------------------------------------------------------------
int DetermineGameType(void)
{
	DBG_PRINTF("DetermineGameType()\n");
	int iGT = (int)mp_gamerules.value;

	if (gpGlobals->deathmatch <= 0)
	{
		iGT = GT_SINGLE;
	}
	else
	{
		int iAutoGT = GT_SINGLE;// invalid value
		if (mp_maprules.value > GT_SINGLE)// this var should be aded to maps/<mapname>_pre.cfg for every map
			iAutoGT = (int)mp_maprules.value;// overrides everything else
		else
			iAutoGT = DetectGameTypeByPrefix(STRING(gpGlobals->mapname));// XDM3038a

		if (iAutoGT == GT_DEATHMATCH)
		{
			if (mp_teamplay.value > 0.0f)// teamplay selected by default
				iAutoGT = GT_TEAMPLAY;
		}
		else if (iAutoGT == GT_SINGLE)// not detected
		{
			DBG_PRINTF("DetermineGameType(): game rules were not detected by map name.\n");
			if (g_pWorld && !FStringNull(g_pWorld->m_iszGamePrefix))// XDM3038a: look inside map to be 100% sure
			{
				iAutoGT = DetectGameTypeByPrefix(STRING(g_pWorld->m_iszGamePrefix));// XDM3038a
				if (iAutoGT == GT_SINGLE)// still not detected
					SERVER_PRINTF("Warning: unknown game type \"%s\" specified inside map \"%s\"!\n", STRING(g_pWorld->m_iszGamePrefix), STRING(gpGlobals->mapname));
			}
		}

		if (iAutoGT == GT_SINGLE)// still not detected
		{
			if (mp_teamplay.value > 0.0f)// teamplay selected by default
				iAutoGT = GT_TEAMPLAY;
			else
				iAutoGT = GT_DEATHMATCH;

			DBG_PRINTF("DetermineGameType(): map rules were not detected automatically. Using %d.\n", iAutoGT);
		}

		if (iGT <= GT_SINGLE)// use auto-detected rules
		{
			iGT = iAutoGT;
		}
		else// use rules selected by server administrator
		{
			if (iAutoGT > GT_SINGLE && iGT != iAutoGT)// rules that were detected differ from those specified by the administrator
				SERVER_PRINTF("Warning: selected game rules (%d) do not match autodetected game rules (%d) for map \"%s\".\n", iGT, iAutoGT, STRING(gpGlobals->mapname));
		}
	}
	return iGT;
}

//-----------------------------------------------------------------------------
// Purpose: Decide wich game rules to use this time
// Warning: Not called with persistent game rules
// Input  : game_type - GT_SINGLE, etc.
// Output : CGameRules
//-----------------------------------------------------------------------------
CGameRules *InstallGameRules(int game_type)
{
	DBG_PRINTF("InstallGameRules(%d)\n", game_type);
	CGameRules *pNewGameRules = NULL;// XDM

	gpGlobals->deathmatch = 0;
	gpGlobals->coop = 0;
	gpGlobals->teamplay = 0;

	switch (game_type)
	{
	case GT_SINGLE:
		{
			pNewGameRules = new CGameRulesSinglePlay;
		}
		break;
	case GT_COOP:
		{
			pNewGameRules = new CGameRulesCoOp;
			gpGlobals->coop = 1.0;
		}
		break;
	case GT_DEATHMATCH:
		{
			pNewGameRules = new CGameRulesMultiplay;
		}
		break;
	case GT_LMS:
		{
			pNewGameRules = new CGameRulesLMS;
		}
		break;
	case GT_TEAMPLAY:
		{
			pNewGameRules = new CGameRulesTeamplay;
		}
		break;
	case GT_CTF:
		{
			pNewGameRules = new CGameRulesCTF;
		}
		break;
	case GT_DOMINATION:
		{
			pNewGameRules = new CGameRulesDomination;
		}
		break;
	case GT_ROUND:
		{
			pNewGameRules = new CGameRulesRoundBased;
		}
		break;
	/*?	case GT_ASSAULT:
		{
			pNewGameRules = new CGameRulesAssault;
		}
		break;*/
	default:
		{
			SERVER_PRINTF("InstallGameRules() Error! Unknown game type %d!\n", game_type);
			//iGT = GT_DEATHMATCH;
			pNewGameRules = new CGameRulesMultiplay;
		}
		break;
	}

	SERVER_PRINTF("\n ** GAME TYPE: %hd:%s **\n", pNewGameRules->GetGameType(), pNewGameRules->GetGameDescription());
	//pNewGameRules->m_iGameType = game_type;
	gpGlobals->deathmatch = (float)game_type;// XDM: for external DLLs

	if (pNewGameRules->IsTeamplay())
		gpGlobals->teamplay = 1.0f;
	else
		gpGlobals->teamplay = 0.0f;

	return pNewGameRules;
}




//-----------------------------------------------------------------------------
// Purpose: Is this a real, playable team? // TEAM_NONE must be invalid here!
// Input  : &team_id - TEAM_ID
// Output : Returns true if TEAM_1 to TEAM_4 and active
//-----------------------------------------------------------------------------
bool IsActiveTeam(const TEAM_ID &team_id)
{
	if (g_pGameRules)
		return (team_id > TEAM_NONE && team_id <= g_pGameRules->GetNumberOfTeams());

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Real player, connected and not a spectator?
// Input  : *pPlayerEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsActivePlayer(CBaseEntity *pPlayerEntity)
{
	if (pPlayerEntity && pPlayerEntity->IsPlayer() && !FStringNull(pPlayerEntity->pev->netname))
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pPlayerEntity;
		if (!pPlayer->IsObserver())// UNDONE: intermission!
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Real player, connected and not a spectator?
// Input  : *ent - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsActivePlayer(const CLIENT_INDEX &idx)
{
	return IsActivePlayer(UTIL_ClientByIndex(idx));
}

//-----------------------------------------------------------------------------
// Purpose: Is this a possible index for a player?
// Input  : idx - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsValidPlayerIndex(const CLIENT_INDEX &idx)
{
	if (idx >= 1 && idx <= gpGlobals->maxClients)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: This player was active at least once in the game!
// Input  : *pPlayerEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool WasActivePlayer(CBaseEntity *pPlayerEntity)
{
	if (pPlayerEntity && pPlayerEntity->IsPlayer() && !FStringNull(pPlayerEntity->pev->netname))
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pPlayerEntity;
		if (pPlayer->m_iSpawnState >= SPAWNSTATE_CONNECTED)// Count players who EVER was an active player!! Including current spectators!!!!
		{
			if (FBitSet(pPlayer->m_iGameFlags, EGF_JOINEDGAME))
				return true;
		}
	}
	return false;
}


#define CMDCREATE_ENTKVMAX			8// number of kv pairs
#define CMDCREATE_ENTKVBUFLEN		32// buffer length

extern EHANDLE g_WriteEntityToFile;

//CBaseEntity *pPrepareEntity;// XDM
/* Moved to CBasePlayer
Vector g_MemOrigin;// coords
Vector g_MemAngles;
bool g_CoordsRemembered = false;
TraceResult g_PickTrace;
CBaseEntity *g_pPickEntity = NULL;*/

//-----------------------------------------------------------------------------
// Purpose: Mouse pick
// Input  : *pPlayer - 
//			*pcmd - 
//-----------------------------------------------------------------------------
void Cmd_Dev_MPick(CBasePlayer *pPlayer, const char *pcmd)
{
	if (!pPlayer->RightsCheckBits((1<<USER_RIGHTS_ADMINISTRATOR) | (1<<USER_RIGHTS_DEVELOPER)))
		return;

	memset(&pPlayer->m_PickTrace, 0, sizeof(TraceResult));
	if (CMD_ARGC() >= CMDARG_PICK_END_Z)
	{
		Vector vecSrc(atof(CMD_ARGV(CMDARG_PICK_START_X)), atof(CMD_ARGV(CMDARG_PICK_START_Y)), atof(CMD_ARGV(CMDARG_PICK_START_Z)));
		Vector vecEnd(atof(CMD_ARGV(CMDARG_PICK_END_X)), atof(CMD_ARGV(CMDARG_PICK_END_Y)), atof(CMD_ARGV(CMDARG_PICK_END_Z)));
/*#if defined (_DEBUG)
		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, svc_temp_entity, vecSrc, pPlayer->edict());
			WRITE_BYTE(TE_BEAMPOINTS);
			WRITE_COORD(vecSrc.x); THIS SHOWS GARBAGE!!!
			WRITE_COORD(vecSrc.y);
			WRITE_COORD(vecSrc.z);
			WRITE_COORD(vecEnd.x);
			WRITE_COORD(vecEnd.y);
			WRITE_COORD(vecEnd.z);
			WRITE_SHORT(g_iModelIndexLaser);
			WRITE_BYTE(0);	// framestart
			WRITE_BYTE(16);	// framerate
			WRITE_BYTE(100);// life
			WRITE_BYTE(10);	// width
			WRITE_BYTE(0);	// noise
			WRITE_BYTE(207);// r
			WRITE_BYTE(207);// g
			WRITE_BYTE(255);// b
			WRITE_BYTE(255);// brightness
			WRITE_BYTE(10);	// speed
		MESSAGE_END();
#endif*/
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, dont_ignore_glass, pPlayer->edict(), &pPlayer->m_PickTrace);
		if (pPlayer->m_PickTrace.flFraction != 1.0f && !FNullEnt(pPlayer->m_PickTrace.pHit))
		{
			pPlayer->m_pPickEntity = CBaseEntity::Instance(pPlayer->m_PickTrace.pHit);
			if (pPlayer->m_pPickEntity)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgPickedEnt, NULL, pPlayer->edict());
					WRITE_SHORT(pPlayer->m_pPickEntity->entindex());
					WRITE_COORD(pPlayer->m_PickTrace.vecEndPos.x);
					WRITE_COORD(pPlayer->m_PickTrace.vecEndPos.y);
					WRITE_COORD(pPlayer->m_PickTrace.vecEndPos.z);
				MESSAGE_END();
			}
#if defined (_DEBUG)
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "SV: pick: null entity.\n");
#endif
		}
#if defined (_DEBUG)
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "SV: pick: trace fails.\n");
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Mouse move
// Input  : *pPlayer - 
//			*pcmd - 
//-----------------------------------------------------------------------------
void Cmd_Dev_MMove(CBasePlayer *pPlayer, const char *pcmd)
{
	if (!pPlayer->RightsCheckBits((1<<USER_RIGHTS_ADMINISTRATOR) | (1<<USER_RIGHTS_DEVELOPER)))
		return;

	if (CMD_ARGC() >= CMDARG_MOVE_ORIGIN+1)// <entindex> <\"x y z\" origin>
	{
		int mventindex = atoi(CMD_ARGV(CMDARG_MOVE_ENTINDEX));
		Vector mvorigin;
		if (StringToVec(CMD_ARGV(CMDARG_MOVE_ORIGIN), mvorigin))
		{
			CBaseEntity *pEntity = UTIL_EntityByIndex(mventindex);
			if (pEntity)
			{
				ClearBits(pEntity->pev->flags, FL_ONGROUND);
				UTIL_SetOrigin(pEntity, mvorigin);
				pEntity->pev->origin = mvorigin;
				ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "SV: move: moved %s[%d] to (%g %g %g).\n", STRING(pEntity->pev->classname), mventindex, mvorigin.x, mvorigin.y, mvorigin.z);
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "SV: move: entity not found.\n");
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "SV: move: bad arguments.\n");
	}
	else
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "SV: move: bad number of arguments.\n");
}

//-----------------------------------------------------------------------------
// Purpose: Mouse create
// Input  : *pPlayer - 
//			*pcmd - 
//-----------------------------------------------------------------------------
void Cmd_Dev_MCreate(CBasePlayer *pPlayer, const char *pcmd)
{
	if (!pPlayer->RightsCheckBits((1<<USER_RIGHTS_ADMINISTRATOR) | (1<<USER_RIGHTS_DEVELOPER)))
		return;

	int argc = CMD_ARGC();
	if (argc >= CMDARG_CREATE_ORIGIN+1)// <classname> <bool create 0/1> <\"x y z\" origin> <\"x y z\" angles> [key1:value1] ...
	{
		Vector origin;
		if (StringToVec(CMD_ARGV(CMDARG_CREATE_ORIGIN), origin))
		{
			Vector angles;
			int create = atoi(CMD_ARGV(CMDARG_CREATE_CREATE));// XDM3038b

			if (argc < CMDARG_CREATE_ANGLES+1 || !StringToVec(CMD_ARGV(CMDARG_CREATE_ANGLES), angles))
				angles.Clear();

			CBaseEntity *pEntity = NULL;
			if (create > 0)
			{
				pEntity = CBaseEntity::Create((char *)CMD_ARGV(CMDARG_CREATE_CLASSNAME), origin, angles, NULL);
				if (pEntity == NULL)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Unable to create %s!\n", CMD_ARGV(CMDARG_CREATE_CLASSNAME));
			}
			//for (int zzz=0; zzz<argc; ++zzz)
			//	ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "%d: %s", zzz, CMD_ARGV(zzz));

			if (create == 0 || pEntity != NULL)
			{
				//g_WriteEntityToFile = pEntity;
				size_t kn = 0;// number of keys
				size_t j,k;
				const char *kvstring;
				char knames[CMDCREATE_ENTKVMAX][CMDCREATE_ENTKVBUFLEN];// actual data storage
				char kvalues[CMDCREATE_ENTKVMAX][CMDCREATE_ENTKVBUFLEN];
				char *pnames[CMDCREATE_ENTKVMAX];// array of pointers
				char *pvalues[CMDCREATE_ENTKVMAX];
				KeyValueData kvd;
				knames[0][0] = '\0';
				kvalues[0][0] = '\0';
				if (create == 0)// if we didn't create the entity, add classname, origin into KV list
				{
					strncpy(knames[kn], "classname", CMDCREATE_ENTKVBUFLEN);
					strncpy(kvalues[kn], CMD_ARGV(CMDARG_CREATE_CLASSNAME), CMDCREATE_ENTKVBUFLEN);
					pnames[kn] = knames[kn];// get pointers
					pvalues[kn] = kvalues[kn];
					++kn;
					strncpy(knames[kn], "origin", CMDCREATE_ENTKVBUFLEN);
					_snprintf(kvalues[kn], CMDCREATE_ENTKVBUFLEN, "%g %g %g", origin.x, origin.y, origin.z);
					pnames[kn] = knames[kn];// get pointers
					pvalues[kn] = kvalues[kn];
					++kn;
					if (!angles.IsZero())
					{
						strncpy(knames[kn], "angles", CMDCREATE_ENTKVBUFLEN);
						_snprintf(kvalues[kn], CMDCREATE_ENTKVBUFLEN, "%g %g %g", angles.x, angles.y, angles.z);
						pnames[kn] = knames[kn];// get pointers
						pvalues[kn] = kvalues[kn];
						++kn;
					}
				}
				if (argc >= CMDARG_CREATE_KV1)
				{
					for (int i=CMDARG_CREATE_KV1; i<argc; ++i)
					{
						kvstring = CMD_ARGV(i);// NAME:VALUE string
						kvd.szClassName = (char *)CMD_ARGV(CMDARG_CREATE_CLASSNAME);
						j = 0;// sscanf(CMD_ARGV(i), "%s:%s", buf1, buf2);
						k = 0;
						while (k < CMDCREATE_ENTKVBUFLEN && kvstring[j] != ':' && kvstring[j] != '\0')// split name
						{
							knames[kn][k] = kvstring[j];//buf1[k] = kvstring[j];
							if (knames[kn][k] == '{' || knames[kn][k] == '}')// security
								knames[kn][k] = '_';// print warning?

							++j; ++k;
						}
						if (k == 0)// empty key!
							continue;

						knames[kn][k] = '\0';// terminate!
						++j;// skip ':'
						k = 0;// restart index in a new buffer
						while (k < CMDCREATE_ENTKVBUFLEN && kvstring[j] != '\0')// and value
						{
							kvalues[kn][k] = kvstring[j];
							if (kvalues[kn][k] == '{' || kvalues[kn][k] == '}')// security
								kvalues[kn][k] = '_';// print warning?

							++j; ++k;
						}
						if (k == 0)// empty value!
							continue;

						kvalues[kn][k] = '\0';// terminate!
						kvd.szKeyName = knames[kn];//buf1;
						kvd.szValue = kvalues[kn];//buf2;
						kvd.fHandled = FALSE;
						if (pEntity)
							DispatchKeyValue(pEntity->edict(), &kvd);// apply immediately

						if (create == 0 || kvd.fHandled == TRUE)// only write to file if valid
						{
							pnames[kn] = knames[kn];// get pointers
							pvalues[kn] = kvalues[kn];
							++kn;
							if (kn >= CMDCREATE_ENTKVMAX)
								break;
						}
					}
					if (pEntity)
						ENTCONFIG_WriteEntity(pEntity->pev, pnames, pvalues, kn);// now we can provide array of pointers
					else
						ENTCONFIG_WriteEntityVirtual(pnames, pvalues, kn);
				}
				else if (pEntity)
					ENTCONFIG_WriteEntity(pEntity->pev);

				//g_WriteEntityToFile = NULL;
				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Added %s\n", CMD_ARGV(CMDARG_CREATE_CLASSNAME));
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Failed to create %s!\n", CMD_ARGV(CMDARG_CREATE_CLASSNAME));
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Unable to create %s without origin!\n", CMD_ARGV(CMDARG_CREATE_CLASSNAME));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create player start currently used by game rules
// Input  : *pPlayer - 
//			*pcmd - 
//-----------------------------------------------------------------------------
void Cmd_Dev_CreatePlayerStart(CBasePlayer *pPlayer, const char *pcmd)
{
	if (/*g_psv_cheats && g_psv_cheats->value > 0.0f && */pPlayer->RightsCheckBits((1<<USER_RIGHTS_ADMINISTRATOR) | (1<<USER_RIGHTS_DEVELOPER)))
	{
		int argc = CMD_ARGC();
		if (argc == 2 && strcmp(CMD_ARGV(1), "--help") == 0)// special option: display help
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s [team# 0==all] [landmark <type#>] [spawnflags]\nTypes: 0:RANDOM, 1:MAPSTART, 2:MAPEND, 3+:MAPJUNCTION\nSpawnflag '2' allows start with no landmark.\n", pcmd);
			return;
		}
		if (argc == CMDARG_CREATEPLSTART_LANDMARK+1)// if user specifies landmark name, he also must specify type
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "You must specify spot type (number) if you provide landmark name. See \"%s --help\".\n", pcmd);
			return;
		}
		const char *spotname = g_pSpotList[g_iSpawnPointTypePlayers].classname;
		CBaseEntity *pStart = CBaseEntity::Create(spotname, pPlayer->pev->origin, pPlayer->pev->angles);//pStart = CBaseEntity::Create(SUPERDEFAULT_MP_START, org, ang);
		if (pStart)
		{
			if (argc >= CMDARG_CREATEPLSTART_LANDMARK+1)
			{
				char knames[CMDARG_CREATEPLSTART_NUMARGS][CMDCREATE_ENTKVBUFLEN];// actual data storage
				char kvalues[CMDARG_CREATEPLSTART_NUMARGS][CMDCREATE_ENTKVBUFLEN];
				char *pnames[CMDARG_CREATEPLSTART_NUMARGS];// array of pointers
				char *pvalues[CMDARG_CREATEPLSTART_NUMARGS];
				int iValue;
				size_t kn = 0;// number of keys
				KeyValueData kvd;// apply values on the fly
				knames[0][0] = '\0';
				kvalues[0][0] = '\0';
				pnames[0] = NULL;
				pvalues[0] = NULL;
				if (argc >= CMDARG_CREATEPLSTART_TEAM+1)
				{
					iValue = atoi(CMD_ARGV(CMDARG_CREATEPLSTART_TEAM));
					strncpy(knames[kn], "team", CMDCREATE_ENTKVBUFLEN);
					//strncpy(kvalues[kn], CMD_ARGV(CMDARG_CREATEPLSTART_TEAM), CMDCREATE_ENTKVBUFLEN);
					_snprintf(kvalues[kn], CMDCREATE_ENTKVBUFLEN, "%d", iValue);// security
					pnames[kn] = knames[kn];// get pointers
					pvalues[kn] = kvalues[kn];
					++kn;
				}
				if (argc >= CMDARG_CREATEPLSTART_LANDMARK+1)
				{
					strncpy(knames[kn], "landmark", CMDCREATE_ENTKVBUFLEN);
					strncpy(kvalues[kn], CMD_ARGV(CMDARG_CREATEPLSTART_LANDMARK), CMDCREATE_ENTKVBUFLEN);
					pnames[kn] = knames[kn];// get pointers
					pvalues[kn] = kvalues[kn];
					++kn;
				}
				if (argc >= CMDARG_CREATEPLSTART_TYPE+1)
				{
					iValue = atoi(CMD_ARGV(CMDARG_CREATEPLSTART_TYPE));
					strncpy(knames[kn], "watertype", CMDCREATE_ENTKVBUFLEN);
					//strncpy(kvalues[kn], CMD_ARGV(CMDARG_CREATEPLSTART_TYPE), CMDCREATE_ENTKVBUFLEN);
					_snprintf(kvalues[kn], CMDCREATE_ENTKVBUFLEN, "%d", iValue);// security
					pnames[kn] = knames[kn];// get pointers
					pvalues[kn] = kvalues[kn];
					++kn;
				}
				if (argc >= CMDARG_CREATEPLSTART_SPAWNFLAGS+1)
				{
					iValue = atoi(CMD_ARGV(CMDARG_CREATEPLSTART_SPAWNFLAGS));// like SF_PLAYERSTART_ALLOWSTART
					strncpy(knames[kn], "spawnflags", CMDCREATE_ENTKVBUFLEN);
					_snprintf(kvalues[kn], CMDCREATE_ENTKVBUFLEN, "%d", iValue);// security
					pnames[kn] = knames[kn];// get pointers
					pvalues[kn] = kvalues[kn];
					++kn;
				}
				for (size_t i=0; i<kn; ++i)// apply these values right away
				{
					kvd.szKeyName = knames[i];
					kvd.szValue = kvalues[i];
					kvd.fHandled = FALSE;
					DispatchKeyValue(pStart->edict(), &kvd);
					//if (kvd.fHandled == FALSE)
					//	ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: warning: key \"%s\" (\"%s\") was not accepted!\n", pcmd, kvd.szKeyName, kvd.szValue);
				}
				ENTCONFIG_WriteEntity(pStart->pev, pnames, pvalues, kn);// now we can provide array of pointers
			}
			else
				ENTCONFIG_WriteEntity(pStart->pev);

			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " +\"%s\" saved at: %g %g %g, %g %g %g\n", STRING(pStart->pev->classname), pPlayer->pev->origin.x,pPlayer->pev->origin.y,pPlayer->pev->origin.z, pPlayer->pev->angles.x,pPlayer->pev->angles.y,pPlayer->pev->angles.z);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: error creating \"%s\"!\n", pcmd, spotname);
	}
	else
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s not allowed\n", pcmd);
}





//-----------------------------------------------------------------------------
// Purpose: Developer commands. May be considered cheats. Sort by usability!
// Note   : Sort commands: most popular first!
// Note   : It is pointless to use alert levels other than 0 because these are console commands.
// Input  : *pPlayer - 
//			*pcmd - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool DeveloperCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	if (g_pCvarDeveloper && g_pCvarDeveloper->value <= 0.0f)
		return false;
	if (!pPlayer->RightsCheckBits((1<<USER_RIGHTS_ADMINISTRATOR) | (1<<USER_RIGHTS_DEVELOPER)))// XDM3038c
	{
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Permission denied for \"%s\".\n", pcmd);
		return false;
	}

	if (FStrEq(pcmd, ".p"))// Pick an entity and send its info to client. We need this because there's no way to reliably trace entities on client side.
	{
		Cmd_Dev_MPick(pPlayer, pcmd);
	}
	else if (FStrEq(pcmd, ".m"))// Move entity
	{
		Cmd_Dev_MMove(pPlayer, pcmd);
	}
	else if (FStrEq(pcmd, ".c"))// Place entity on map, save it into map patch file
	{
		Cmd_Dev_MCreate(pPlayer, pcmd);
	}
	else if (FStrEq(pcmd, "giveall"))
	{
		if (g_psv_cheats && g_psv_cheats->value > 0.0f)
		{
			g_SilentItemPickup = true;
			if (!IsMultiplayer())
			{
				pPlayer->pev->weapons |= (1<<WEAPON_SUIT);//don't!	GiveNamedItem("item_suit");
				if (sv_precacheitems.value > 0)
					pPlayer->GiveNamedItem("item_longjump");
			}
			size_t i;
			for (i = 0; i < MAX_WEAPONS; ++i)// XDM3038c
			{
				if (g_ItemInfoArray[i].iId == WEAPON_NONE)
					continue;
				if (g_ItemInfoArray[i].szName[0])
					pPlayer->GiveNamedItem(g_ItemInfoArray[i].szName);
			}
			for (i = 0; i < MAX_AMMO_SLOTS; ++i)
				pPlayer->GiveAmmo(MaxAmmoCarry(i), i);

			g_SilentItemPickup = false;
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: not alowed\n", pcmd);
	}
	else if (FStrEq(pcmd, "givesuit"))
	{
		if (g_psv_cheats && g_psv_cheats->value > 0.0f)
		{
			SetBits(pPlayer->pev->weapons, 1<<WEAPON_SUIT);
			pPlayer->m_fFlashBattery = MAX_FLASHLIGHT_CHARGE;
		}
	}
	else if (FStrEq(pcmd, "give"))
	{
		if (g_psv_cheats && g_psv_cheats->value > 0.0f)
		{
			if (CMD_ARGC() > 1)
				pPlayer->GiveNamedItem(CMD_ARGV(1));// XDM3038: now it's more simple
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <classname>\n", pcmd);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: not alowed\n", pcmd);
	}
	else if (FStrEq(pcmd, "allammo"))
	{
		if (g_psv_cheats && g_psv_cheats->value > 0.0f)
		{
#if defined(OLD_WEAPON_AMMO_INFO)
			for (short i = 0; i < MAX_AMMO_SLOTS; ++i)
				pPlayer->GiveAmmo(MAX_AMMO, i, MAX_AMMO);
#else
			for (int i = 0; i < MAX_AMMO_SLOTS; ++i)
				pPlayer->GiveAmmo(MAX_AMMO, i);
#endif
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: not alowed\n", pcmd);
	}
	else if (FStrEq(pcmd, "make"))// main creation function
	{
		if (g_psv_cheats && g_psv_cheats->value > 0.0f)
		{
			if (CMD_ARGC() > 1)
			{
				size_t n;
				if (CMD_ARGC() > 2)
					n = atoi(CMD_ARGV(2));
				else
					n = 1;

				for (size_t i = 0; i < n; ++i)
					CBaseEntity::Create(CMD_ARGV(1), pPlayer->pev->origin + gpGlobals->v_forward*48.0f*(float)(i+1), pPlayer->pev->angles, g_vecZero, NULL, SF_NORESPAWN);// XDM3038: safe
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <classname> [number]\nExample: %s item_healthkit 3\n", pcmd, pcmd);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: not alowed\n", pcmd);
	}
	else if (FStrEq(pcmd, "throw"))
	{
		if (g_psv_cheats && g_psv_cheats->value > 0.0f)
		{
			if (CMD_ARGC() > 1)
			{
				Vector fwd;
				AngleVectors(pPlayer->pev->angles, fwd, NULL, NULL);
				CBaseEntity::Create(CMD_ARGV(1), pPlayer->pev->origin + gpGlobals->v_forward*48.0f, pPlayer->pev->angles, fwd*300.0f, NULL, SF_NORESPAWN);// XDM3038: safe
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <classname>\n", pcmd);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: not alowed\n", pcmd);
	}
	else if (FStrEq(pcmd, "fov"))
	{
		if (CMD_ARGC() > 1)
			pPlayer->pev->fov/*m_iFOV*/ = atof(CMD_ARGV(1));
		else
			CLIENT_PRINTF(pPlayer->edict(), print_console, UTIL_VarArgs("server: FOV = %g\n", pPlayer->pev->fov));//(int)pPlayer->m_iFOV));
	}
	else if (FStrEq(pcmd, "searchents"))// searchents <stringname> <string>
	{
		if (CMD_ARGC() > 2)
		{
			size_t count = 0;
			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityByString(pEntity, CMD_ARGV(1), CMD_ARGV(2))) != NULL)
			{
				//if (FBitSet(pEntity->pev->spawnflags, SF_NOREFERENCE) && g_pCvarDeveloper && g_pCvarDeveloper->value < 2)
				//	continue;
				int a; const char *args[32];
				for (a=0; a<CMD_ARGC(); ++a) {args[a] = CMD_ARGV(a);} args[a] = NULL;
				Cmd_EntityAction(3, CMD_ARGC(), args, pEntity, pPlayer, true);
				++count;
			}
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " %u entities total\n", count);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <entity string> <value> [action [options]]\nExample: %s classname func_door use 3\nNote: use \"searchtarget\" to search by target!\n", pcmd, pcmd);
	}
	else if (FStrEq(pcmd, "searchtype"))// searchents <type ID>
	{
		if (CMD_ARGC() > 1)
		{
			edict_t *pEdict;
			size_t count = 0;
			int i;
			int iType = atoi(CMD_ARGV(1));
			for (i = 0; i < gpGlobals->maxEntities; ++i, ++pEdict)
			{
				pEdict = INDEXENT(i);
				if (UTIL_IsValidEntity(pEdict))
				{
					if (EntityIs(pEdict) == iType)
					{
						int a; const char *args[32];
						for (a=0; a<CMD_ARGC(); ++a) {args[a] = CMD_ARGV(a);} args[a] = NULL;
						Cmd_EntityAction(2, CMD_ARGC(), args, CBaseEntity::Instance(pEdict), pPlayer, true);
						++count;
					}
				}
			}
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " %u entities total\n", count);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <entity type> [action [options]]\nTypes: 0=ALL,PLAYER,MONSTER,HUMAN,GAMEGOAL,PROJECTILE,PLRWEAPON,PLRITEM,PICKUP,PUSHABLE,BREAKABLE,TRIGGER,OTHER\n", pcmd);
	}																																   
	else if (FStrEq(pcmd, "searchtarget"))// special OOP-aware version
	{
		if (CMD_ARGC() > 1)
		{
			edict_t *pEdict;
			size_t count = 0;
			int i;
			for (i = 0; i < gpGlobals->maxEntities; ++i, ++pEdict)
			{
				pEdict = INDEXENT(i);
				if (UTIL_IsValidEntity(pEdict))
				{
					CBaseEntity *pEntity = CBaseEntity::Instance(pEdict);
					if (pEntity && pEntity->HasTarget(MAKE_STRING(CMD_ARGV(1))))
					{
						int a; const char *args[32];
						for (a=0; a<CMD_ARGC(); ++a) {args[a] = CMD_ARGV(a);} args[a] = NULL;
						Cmd_EntityAction(2, CMD_ARGC(), args, pEntity, pPlayer, true);
						++count;
					}
				}
			}
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " %u entities total\n", count);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <target> [action [options]]\nExample: %s 1 set netname test\nChecks if entity has specified target, works not only for pev->target.\n", pcmd, pcmd);
	}																																   
	else if (FStrEq(pcmd, "searchradius"))																							   
	{																																   
		if (CMD_ARGC() > 1)																											   
		{																															   
			size_t count = 0;
			float radius = atof(CMD_ARGV(1));
			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityInSphere(pEntity, pPlayer->Center(), radius)) != NULL)
			{
				//if (FBitSet(pEntity->pev->spawnflags, SF_NOREFERENCE) && g_pCvarDeveloper && g_pCvarDeveloper->value < 2)
				//	continue;
				int a; const char *args[32];
				for (a=0; a<CMD_ARGC(); ++a) {args[a] = CMD_ARGV(a);} args[a] = NULL;
				Cmd_EntityAction(2, CMD_ARGC(), args, pEntity, pPlayer, true);
				++count;
			}
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " %u entities total\n", count);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <radius> [action [options]]\nExample: %s 128 hl show\n", pcmd, pcmd);
	}
	else if (FStrEq(pcmd, "searchindex"))
	{
		if (CMD_ARGC() > 1)
		{
			CBaseEntity *pEntity = UTIL_EntityByIndex(atoi(CMD_ARGV(1)));
			if (pEntity)
			{
				int a; const char *args[32];
				for (a=0; a<CMD_ARGC(); ++a) {args[a] = CMD_ARGV(a);} args[a] = NULL;
				Cmd_EntityAction(2, CMD_ARGC(), args, pEntity, pPlayer, true);
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, " nothing found\n");
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <entindex> [action [options]]\nExample: %s 1 show\n", pcmd, pcmd);
	}
	else if (FStrEq(pcmd, "searchforward"))
	{
		if (CMD_ARGC() > 0)
		{
			CBaseEntity *pEntity = UTIL_FindEntityForward(pPlayer);
			if (pEntity)
			{
				int a; const char *args[32];
				for (a=0; a<CMD_ARGC(); ++a) {args[a] = CMD_ARGV(a);} args[a] = NULL;
				Cmd_EntityAction(1, CMD_ARGC(), args, pEntity, pPlayer, true);
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, " nothing found\n");
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s [action [options]]\nExample: %s info\n", pcmd, pcmd);
	}
	else if (FStrEq(pcmd, "pickedent"))
	{
		if (CMD_ARGC() > 0)
		{
			CBaseEntity *pEntity = pPlayer->m_pPickEntity;
			if (pEntity)
			{
				int a; const char *args[32];
				for (a=0; a<CMD_ARGC(); ++a) {args[a] = CMD_ARGV(a);} args[a] = NULL;
				Cmd_EntityAction(1, CMD_ARGC(), args, pEntity, pPlayer, true);
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, " no entity picked\n");
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s [action [options]]\nExample: %s info\n", pcmd, pcmd);
	}
	/*else if (FStrEq(pcmd, "hideitems"))// OBSOLETE: use searchtype instead // for overview screenshots
	{
		if (CMD_ARGC() > 1)
		{
			int i = 1;
			edict_t *pEdict = INDEXENT(i);
			bool bHide = (atoi(CMD_ARGV(1)) > 0);
			for (; i < gpGlobals->maxEntities; ++i, pEdict++)
			{
				if (!UTIL_IsValidEntity(pEdict))
					continue;
				if (FBitSet(pEdict->v.spawnflags, SF_NOREFERENCE))// XDM3037a
					continue;
				CBaseEntity *pEntity = CBaseEntity::Instance(pEdict);
				if (pEntity)
				{
					if (pEntity->IsPickup())// redundant || pEntity->IsPlayerItem() || pEntity->IsPlayerWeapon())// XDM3038
					{
						if (bHide)
							SetBits(pEdict->v.effects, EF_NODRAW);
						else
							ClearBits(pEdict->v.effects, EF_NODRAW);
					}
				}
			}
		}
		else
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "usage: %s <0/1>\nDANGEROUS! Affects respawning items!\n", pcmd);
	}*/
	/*else if (FStrEq(pcmd, "hidemonsters"))// OBSOLETE: use searchtype instead // for overview screenshots
	{
		if (CMD_ARGC() > 1)
		{
			int i = 1;
			edict_t *pEdict = INDEXENT(i);
			bool bHide = (atoi(CMD_ARGV(1)) > 0);
			for (; i < gpGlobals->maxEntities; ++i, pEdict++)
			{
				if (!UTIL_IsValidEntity(pEdict))
					continue;
				if (FBitSet(pEdict->v.spawnflags, SF_NOREFERENCE))// XDM3037a
					continue;
				CBaseEntity *pEntity = CBaseEntity::Instance(pEdict);
				if (pEntity)
				{
					if (pEntity->IsMonster())
					{
						if (bHide)
							SetBits(pEdict->v.effects, EF_NODRAW);
						else
							ClearBits(pEdict->v.effects, EF_NODRAW);
					}
				}
			}
		}
		else
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "usage: %s <0/1>\nDANGEROUS! Affects respawning monsters!\n", pcmd);
	}*/
	else if (FStrEq(pcmd, "fixfalling"))// EXPERIMENTAL: fix entities that are falling through the floor
	{
		int i = 1;
		uint32 count = 0;
		edict_t *pEdict = INDEXENT(i);
		Vector vecNewOrigin;
		ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "Fixing entities below %g...\n", (float)-MAX_ABS_ORIGIN);
		for (; i < gpGlobals->maxEntities; ++i, ++pEdict)
		{
			if (!UTIL_IsValidEntity(pEdict))
				continue;
			if (FBitSet(pEdict->v.spawnflags, SF_NOREFERENCE))
				continue;
			if (FBitSet(pEdict->v.effects, EF_NODRAW))
				continue;

			if (pEdict->v.origin.z < -MAX_ABS_ORIGIN)// FAIL! if (pEdict->v.origin.z < g_pWorld->pev->absmin.z)
			{
				ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " fixing: %s[%d] at %g %g %g\n", STRING(pEdict->v.classname), ENTINDEX(pEdict), pEdict->v.origin.x, pEdict->v.origin.y, pEdict->v.origin.z);
				ClearBits(pEdict->v.flags, FL_ONGROUND);
				vecNewOrigin = pEdict->v.origin;
				vecNewOrigin.z = g_pWorld->pev->absmax.z;// start from the top
				SET_ORIGIN(pEdict, vecNewOrigin);
				++count;
			}
		}
		ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " %u entities fixed.\n", count);
	}
	else if (FStrEq(pcmd, "showweapons"))// [clientindex] ?? BUGBUG: this command only prints to server console
	{
		CBasePlayer *pTargetPlayer;
		if (CMD_ARGC() > 1 && pPlayer->RightsCheckBits((1<<USER_RIGHTS_ADMINISTRATOR) | (1<<USER_RIGHTS_DEVELOPER)))
			pTargetPlayer = UTIL_ClientByIndex(atoi(CMD_ARGV(1)));
		else
			pTargetPlayer = pPlayer;

		if (pTargetPlayer)
		{
			for (int i = WEAPON_NONE; i < PLAYER_INVENTORY_SIZE; ++i)
			{
				CBasePlayerItem *pItem = pTargetPlayer->GetInventoryItem(i);
				if (pItem)
					pItem->ReportState(1);
			}
		}
	}
	else if (FStrEq(pcmd, "showammoslots"))
	{
		ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "Registered ammo types: %d\n", giAmmoIndex);
		for (size_t i = 0; i < MAX_AMMO_SLOTS; ++i)
		{
			if (g_AmmoInfoArray[i].name[0] == 0)// 1st char is empty
				continue;

			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "%d: %s (max: %hd)\n", i, g_AmmoInfoArray[i].name, g_AmmoInfoArray[i].iMaxCarry);
		}
	}
	else if (FStrEq(pcmd, "createplayerstart"))
	{
		Cmd_Dev_CreatePlayerStart(pPlayer, pcmd);
	}
	else if (FStrEq(pcmd, "testfountain"))
	{
		if (CMD_ARGC() >= 3)
		{
			Vector forward, right, up, offset;
			// ok, but requires quotes StringToVec(CMD_ARGV(3), offset); and we want to avoid quoted arguments here
			if (CMD_ARGC() == 6)// 4
				offset.Set(atof(CMD_ARGV(3)), atof(CMD_ARGV(4)), atof(CMD_ARGV(5)));
				//if (!StringToVec(CMD_ARGV(3), offset))
				//	ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "%s: bad vector\n", pcmd);

			AngleVectors(pPlayer->pev->angles, forward, right, up);
			UTIL_BloodStream(pPlayer->pev->origin + offset.x*right + offset.y*forward + offset.z*up, forward, atoi(CMD_ARGV(1)), atoi(CMD_ARGV(2)));
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <color index> <speed> [\"X Y Z offset\"]\n", pcmd);
	}
	/* OBSOLETE else if (FStrEq(pcmd, "useent"))
	{
		if (CMD_ARGC() >= 3)
		{
			USE_TYPE ut = USE_SET;
			switch (atoi(CMD_ARGV(2)))
			{
			default: ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "Invalid use type! Using USE_SET (%d).\n", USE_SET); break;
			case USE_OFF: ut = USE_OFF; break;
			case USE_ON: ut = USE_ON; break;
			case USE_SET: ut = USE_SET; break;
			case USE_TOGGLE: ut = USE_TOGGLE; break;
			case USE_KILL: ut = USE_KILL; break;
			}
			FireTargets(CMD_ARGV(1), pPlayer, pPlayer, ut, 1.0);
		}
		else
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "usage: %s <targetname> <int usetype 0-4>\n", pcmd);
	}*/
	else if (FStrEq(pcmd, "setkeyvalue"))// kept for compatibility
	{
		if (pPlayer->m_pPickEntity)
		{
			CBaseEntity *pEntity = pPlayer->m_pPickEntity;
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: using picked entity: %s\n", pcmd, STRING(pEntity->pev->classname));
			KeyValueData kvd;
			kvd.szClassName = STRINGV(pEntity->pev->classname);
			kvd.szKeyName = (char *)CMD_ARGV(1);
			kvd.szValue = (char *)CMD_ARGV(2);
			kvd.fHandled = FALSE;
			DispatchKeyValue(pEntity->edict(), &kvd);
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "%s: selected entity required\n", pcmd);
	}
	/*else if (FStrEq(pcmd, "getcolormap"))
	{
		ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "pev->colormap = %d\n", pPlayer->pev->colormap);
	}
	else if (FStrEq(pcmd, "showedicts"))// try using 'stats' and 'entities'
	{
		edict_t *e = NULL;
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			e = UTIL_ClientEdictByIndex(i);
			if (e != NULL)
				ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "Found: #%d %s.\n", i, STRING(e->v.netname));
		}
	}*/
	else if (FStrEq(pcmd, "getcoord"))// Show or remember my current coordinates+angles
	{
		char str[128];
		sprintf(str, "Origin: %g %g %g\nAngles: %g %g %g\n",
			pPlayer->pev->origin.x, pPlayer->pev->origin.y, pPlayer->pev->origin.z,
			pPlayer->pev->angles.x, pPlayer->pev->angles.y, pPlayer->pev->angles.z);
		ClientPrint(pPlayer->pev, HUD_PRINTCENTER, str);

		if (g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f)
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, str);

		if (CMD_ARGV(1))
		{
			pPlayer->m_MemOrigin = pPlayer->pev->origin;
			pPlayer->m_MemAngles = pPlayer->pev->angles;
			pPlayer->m_CoordsRemembered = true;
		}
		if (CMD_ARGV(2))
			ALERT(at_logged, str);
	}
	else if (FStrEq(pcmd, "mycoordedit"))// Manually set "remembered" coordinates
	{
		if (CMD_ARGC() >= 4)
		{
			pPlayer->m_MemOrigin.x = (float)atof(CMD_ARGV(1));// have to scan coordinates as arguments because clients have hard time sending quotes
			pPlayer->m_MemOrigin.y = (float)atof(CMD_ARGV(2));
			pPlayer->m_MemOrigin.z = (float)atof(CMD_ARGV(3));
			if (CMD_ARGC() == 7)
			{
				pPlayer->m_MemAngles.x = (float)atof(CMD_ARGV(4));
				pPlayer->m_MemAngles.y = (float)atof(CMD_ARGV(5));
				pPlayer->m_MemAngles.z = (float)atof(CMD_ARGV(6));
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Coordinates saved: origin, angles\n");
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Coordinates saved: origin\n");

			pPlayer->m_CoordsRemembered = true;
		}
		else if (CMD_ARGC() == 2)
		{
			const char *pArg1 = CMD_ARGV(1);
			if (FStrEq(pArg1, "0") == 0)
			{
				pPlayer->m_CoordsRemembered = false;
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Coordinates erased.\n");
			}
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "usage: %s <origin x y z> [angles x y z] or just \"0\" to erase.\n", pcmd);
	}
	/* OBSOLETE: use getcoord.	else if (FStrEq(pcmd, "mycoordshow"))
	{
		ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "Origin: %g %g %g\tAngles: %g %g %g\n", g_MemOrigin.x, g_MemOrigin.y, g_MemOrigin.z, g_MemAngles.x, g_MemAngles.y, g_MemAngles.z);
	}*/
	else if (FStrEq(pcmd, "entsetcoord"))// cannot be replaced by "searchforward moveto" because it operates on a picked entity
	{
		if (pPlayer->m_CoordsRemembered)
		{
			CBaseEntity *pEntity = NULL;
			if (pPlayer->m_pPickEntity)
			{
				pEntity = pPlayer->m_pPickEntity;
				ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "%s: using picked entity: %s\n", pcmd, STRING(pEntity->pev->classname));
			}
			else
			{
				pEntity = UTIL_FindEntityForward(pPlayer);
				if (pEntity)
					ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "%s: using entity in front: %s\n", pcmd, STRING(pEntity->pev->classname));
			}
			if (pEntity)
			{
				if (pEntity->IsPushable())
				{
					char str[128];
					sprintf(str, "Moving entity to\norigin: %g %g %g\nangles: %g %g %g\n",
						pPlayer->m_MemOrigin.x, pPlayer->m_MemOrigin.y, pPlayer->m_MemOrigin.z,
						pPlayer->m_MemAngles.x, pPlayer->m_MemAngles.y, pPlayer->m_MemAngles.z);
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, str);
					ClearBits(pEntity->pev->flags, FL_ONGROUND);
					UTIL_SetOrigin(pEntity, pPlayer->m_MemOrigin);
					pEntity->pev->angles = pPlayer->m_MemAngles;
					//g_CoordsRemembered = false;
				}
				else
					ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Entiy is not pushable!\n");
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "No entity picked and no entities in front of you.\n");
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Remember coordinates first.\n");
	}
	else if (FStrEq(pcmd, "dbg_sendevent"))// very dangerous! may cause crashes!
	{
		if (CMD_ARGC() > 7)
			PLAYBACK_EVENT_FULL(FEV_HOSTONLY, pPlayer->edict(), atoi(CMD_ARGV(1)), 0.0, pPlayer->pev->origin, pPlayer->pev->angles,
				(float)atof(CMD_ARGV(2)), (float)atof(CMD_ARGV(3)), atoi(CMD_ARGV(4)), atoi(CMD_ARGV(5)), atoi(CMD_ARGV(6)), atoi(CMD_ARGV(7)));
		else
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "usage: %s <eventname> <fparam1 fparam2 iparam1 iparam2 bparam1 bparam2>\n", pcmd);
	}
	else if (FStrEq(pcmd, "dbg_showll"))
	{
		ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "pev->light_level = %d, GetEntityIllum = %d\n", pPlayer->pev->light_level, GETENTITYILLUM(pPlayer->edict()));
	}
	else
		return false;

	return true;
}
