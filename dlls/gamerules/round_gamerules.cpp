//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "round_gamerules.h"
#include "maprules.h"
#include "globals.h"
#include "pm_shared.h"// observer modes
#include "voice_gamemgr.h"

extern CVoiceGameMgr	g_VoiceGameMgr;

//
// UNDONE TODO STUB TESTME
//
// Suitable mostly for asymmetric game rules like Assault
//
// TODO: force maximum of 2 teams?
// TODO: find a good way to remember and exchange teams

// Main idea is: teams have different goals (like attacking or defending the base), when round finishes,
// restart map and shift teams so players who used to play on team 1 will automatically join team 2.
// The purpose of all Round()-related functions is mostly to have data collected and managed between rounds and
// clearly distinct level changes and rounds on the same map.

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameRulesRoundBased::CGameRulesRoundBased() : CGameRulesTeamplay()
{
// possible way to clearly reset all entities is to reload the map using "m_bPersistBetweenMaps" flag to keep this game rules instance
// at the same time we can keep global stuff intact
//	m_bPersistBetweenMaps = false;

	m_fRoundStartTime = 0.0f;
	m_iRoundsCompleted = 0;
	m_iRoundState = ROUND_STATE_WAITING;
}

//-----------------------------------------------------------------------------
// Purpose: First thing called after constructor. Initialize all data, cvars, etc.
//-----------------------------------------------------------------------------
void CGameRulesRoundBased::Initialize(void)
{
	m_fRoundStartTime = 0.0f;
	m_flIntermissionEndTime = 0.0f;
	m_iRoundState = ROUND_STATE_WAITING;

	if (FPersistBetweenMaps())// this instance was transfered from previous round on the same map
	{
		if (m_szLastMap[0] && _stricmp(m_szLastMap, STRING(gpGlobals->mapname)) == 0)
		{
			if (m_iRoundsCompleted > 0)
			{
				SERVER_PRINT("CGameRulesRoundBased::Initialize(): continuing on the same map.\n");
				//m_bSwitchTeams = true;
			}
		}
	}
	CGameRulesTeamplay::Initialize();

	if (mp_round_minplayers.value < 2.0f)
	{
		mp_round_minplayers.value = 2.0f;
		CVAR_DIRECT_SET(&mp_round_minplayers, "2");
	}

	ASSERT(m_iNumTeams > 0);
	m_iNumRounds = m_iNumTeams - 1;// TODO: detect number of rounds on this map here
}

//-----------------------------------------------------------------------------
// Purpose: Useless, because entvars will be cleared after this
// Input  : *pEntity - 
//			*pszName - 
//			*pszAddress - 
//			szRejectReason[128] - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*bool CGameRulesRoundBased::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	bool ret = CGameRulesTeamplay::ClientConnected(pEntity, pszName, pszAddress, szRejectReason);
	if (ret)// don't allow players to spawn
	{
		pEntity->v.flags |= FL_SPECTATOR;// XDM3035a: mark to spawn as spectator
// Entity data does not exist yet!
// NO!		CBaseEntity *pSpawnSpot = EntSelectSpectatorPoint(NULL);
//		pPlayer->ObserverStart(pSpawnSpot->pev->origin, pSpawnSpot->pev->angles);
	}
	return ret;
}*/

//-----------------------------------------------------------------------------
// Purpose: client has been disconnected
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
/*void CGameRulesRoundBased::ClientDisconnected(CBasePlayer *pPlayer)
{
	CGameRulesTeamplay::ClientDisconnected(pPlayer);
}*/

//-----------------------------------------------------------------------------
// Purpose: Initialize client HUD
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesRoundBased::InitHUD(CBasePlayer *pPlayer)
{
	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#ROUND\n", UTIL_dtos1(m_iRoundsCompleted+1));
	CGameRulesTeamplay::InitHUD(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Runs every server frame, should handle any timer tasks, periodic events, etc.
//-----------------------------------------------------------------------------
void CGameRulesRoundBased::StartFrame(void)
{
	if (m_iRoundState == ROUND_STATE_WAITING)
	{
		uint32 c = CountPlayers();
		if (c >= (int)mp_round_minplayers.value)// don't do anything if there's only one client
		{
			bool bStart = false;
			if (c >= gpGlobals->maxClients)// everyone's here, we can skip jointime
			{
				// game starts when everyone presses ready buttons
				m_bReadyButtonsHit = CheckPlayersReady();
				bStart = (m_bReadyButtonsHit > 0);
			}
			if (!bStart)
			{
				if (gpGlobals->time - m_fStartTime > mp_jointime.value)// force game start after join time runs out
					bStart = true;
			}
			if (bStart)
			{
				m_bReadyButtonsHit = false;
				m_flIntermissionEndTime = 0.0f;
				RoundStart();
			}
		}
		else
			g_VoiceGameMgr.Update(gpGlobals->frametime);// don't run parent::StartFrame()
	}
	else if (m_iRoundState == ROUND_STATE_ACTIVE)
	{
		if ((m_fRoundStartTime != 0.0f) && gpGlobals->time >= (m_fRoundStartTime + mp_round_time.value*60.0f))
		{
			SERVER_PRINT("GAME: ended by round time limit\n");
			EndMultiplayerGame();
		}
		else
			CGameRulesTeamplay::StartFrame();// CheckLimits() is there
	}
	else
		CGameRulesTeamplay::StartFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Disallow damage during round change, intermission, etc.
// Input  : *pPlayer -
//			*pAttacker -
// Output : Returns true if can
//-----------------------------------------------------------------------------
bool CGameRulesRoundBased::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
	if (m_iRoundState == ROUND_STATE_ACTIVE)
		return CGameRulesTeamplay::FPlayerCanTakeDamage(pPlayer, pAttacker);

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: A player is spawning
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
/*bool CGameRulesRoundBased::PlayerSpawnStart(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesRoundBased::PlayerSpawnStart(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (pPlayer->m_iSpawnState == SPAWNSTATE_CONNECTED)// first time
	{
		if (m_flIntermissionEndTime == 0.0f)// Somebody joined first. Start counting time to round start.
			m_flIntermissionEndTime = gpGlobals->time + mp_chattime.value;// TODO: check limits?

		if (m_iRoundState == ROUND_STATE_WAITING)// we have list to choose from // bots can't select team
		{
			pPlayer->pev->team = TEAM_NONE;
			//called from RemoveAllItems	g_pGameRules->InitHUD(pPlayer);// send team list to this client (UpdateClientData() will not get called for spectator)
			/ *	CBaseEntity *pSpawnSpot = SpawnPointEntSelectSpectator(NULL);
			if (pSpawnSpot)
				pPlayer->ObserverStart(pSpawnSpot->pev->origin, pSpawnSpot->pev->angles, OBS_ROAMING, NULL);
			else
				pPlayer->ObserverStart(pPlayer->pev->origin, pPlayer->pev->angles, OBS_ROAMING, NULL);

			if (!pPlayer->IsBot())
			{
			MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pPlayer->edict());// dest: all
				WRITE_BYTE(MENU_TEAM);
				WRITE_BYTE(0);
			MESSAGE_END();
			//CLIENT_COMMAND(pPlayer->edict(), "chooseteam\n");
			}* /
			return false;
		}
		else
			return CGameRulesTeamplay::PlayerSpawnStart(pPlayer);
	}
	else// not first time
	{
		if (m_iRoundState == ROUND_STATE_ACTIVE)// spawn once
		{
			return CGameRulesMultiplay::PlayerSpawnStart(pPlayer);
		}
	}
	return false;// not sure
}*/

//-----------------------------------------------------------------------------
// Purpose: Can this player respawn NOW?
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesRoundBased::FPlayerCanRespawn(const CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesRoundBased::FPlayerCanRespawn(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (m_iRoundState != ROUND_STATE_SPAWNING)
		return false;

	return CGameRulesTeamplay::FPlayerCanRespawn(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: should player become observer right from the very beginning
// Input  : *pPlayer - 
// Output : bool
//-----------------------------------------------------------------------------
bool CGameRulesRoundBased::FPlayerStartAsObserver(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesRoundBased::FPlayerStartAsObserver(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (m_iRoundState == ROUND_STATE_WAITING)
	{
		return true;
	}
	else if (m_iRoundState == ROUND_STATE_ACTIVE)
	{
		if (pPlayer->m_iSpawnState < SPAWNSTATE_SPAWNED && mp_round_allowlatejoin.value <= 0.0f)// This player never spawned, don't allow him to enter the game
			return true;
	}
	return CGameRulesTeamplay::FPlayerStartAsObserver(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Game rules may require the player to choose team or something
// Input  : *pPlayer - 
// Output : int - menu index, 0 = none
//-----------------------------------------------------------------------------
int CGameRulesRoundBased::ShouldShowStartMenu(CBasePlayer *pPlayer)// XDM3038a
{
	if (m_iRoundState == ROUND_STATE_WAITING)// don't bother showing menu to the players who connects after game start
	{
		if (m_iRoundsCompleted == 0)// only allow to choose team once at the beginning
			return MENU_TEAM;
		//else
		//	return MENU_CLOSEALL;//MENU_INTRO;
	}
	// For players who connect in the middle of the game, just show the standard MOTD
	return MENU_INTRO;//return CGameRulesTeamplay::ShouldShowStartMenu(pPlayer);//MENU_CLOSEALL;
}

//-----------------------------------------------------------------------------
// Purpose: RoundStart
//-----------------------------------------------------------------------------
void CGameRulesRoundBased::RoundStart(void)
{
	DBG_GR_PRINT("CGameRulesRoundBased::RoundStart()\n");
	if (m_iRoundState == ROUND_STATE_ACTIVE)
		return;

	// Save/restore? We just restart the whole map and no probs (^_^)/

	m_iRoundState = ROUND_STATE_SPAWNING;
	CBasePlayer *pPlayer = NULL;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			if (m_iRoundsCompleted > 0)// this map was played at least once
			{
				// shift team indexes for all players or entities?
				// Players and teams must have all their scores saved!
				TEAM_ID newteam = pPlayer->pev->team + 1;
				if (newteam > GetNumberOfTeams())
					newteam = TEAM_1;

				AddPlayerToTeam(pPlayer, newteam);
			}
			pPlayer->ObserverStop();// Spawn() is here
		}
	}
	m_fRoundStartTime = gpGlobals->time;
	m_iRoundState = ROUND_STATE_ACTIVE;
	GameSetState(GAME_STATE_ACTIVE);

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
		WRITE_BYTE(GAME_EVENT_ROUND_START);
		WRITE_SHORT(m_iRoundsCompleted);
		WRITE_SHORT(GetRoundsLimit());// round time?
	MESSAGE_END();

	SERVER_PRINTF("GAME: round %u started\n", m_iRoundsCompleted);
	FireTargets("game_roundstart", g_pWorld, g_pWorld, USE_TOGGLE, m_iRoundsCompleted);
}

//-----------------------------------------------------------------------------
// Purpose: RoundEnd (local intermission)
// Warning: Do not call this directly! Only through EndMultiplayerGame()!
//-----------------------------------------------------------------------------
void CGameRulesRoundBased::RoundEnd(void)
{
	DBG_GR_PRINT("CGameRulesRoundBased::RoundEnd()\n");
	if (m_iRoundState == ROUND_STATE_FINISHED)
		return;

	SERVER_PRINTF("GAME: round %u ended\n", m_iRoundsCompleted);
	m_iRoundState = ROUND_STATE_FINISHED;// disables damage
	GameSetState(GAME_STATE_FINISHED);

	AddScoreToTeam(GetBestTeam(), 1);// who won this round

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
		WRITE_BYTE(GAME_EVENT_ROUND_END);
		WRITE_SHORT(m_iRoundsCompleted);
		WRITE_SHORT(GetRoundsLimit());// round time?
	MESSAGE_END();

	FireTargets("game_roundend", g_pWorld, g_pWorld, USE_TOGGLE, m_iRoundsCompleted);

	m_iRoundsCompleted++;

	if (m_iRoundsCompleted == GetRoundsLimit())
	{
		TEAM_ID winteam = GetBestTeamByScore();
		ClientPrint(NULL, HUD_PRINTCENTER, "#RGR_GAMEEND", GetTeamName(winteam));
	}
}

//-----------------------------------------------------------------------------
// Purpose: After each round best team is selected by normal score and
// awarded with 1 extra score. In the end, extra scores are compared.
// TODO: TESTME
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesRoundBased::GetBestTeam(void)
{
	if (m_iRoundsCompleted == GetRoundsLimit())
		return GetBestTeamByScore();// redirect to proper counter
	else
		return CGameRulesTeamplay::GetBestTeam();
}

//-----------------------------------------------------------------------------
// Purpose: User-defined number of rounds to play on this map.
// Can be based on team switching, number of objectives, etc.
// Output : int 0 == unlimited
//-----------------------------------------------------------------------------
uint32 CGameRulesRoundBased::GetRoundsLimit(void)
{
	return m_iNumRounds;//(uint32)(mp_rounds.value);
	//return MaxTeams();
}

//-----------------------------------------------------------------------------
// Purpose: Overridden. Called when limits are reached.
//-----------------------------------------------------------------------------
void CGameRulesRoundBased::EndMultiplayerGame(void)
{
	DBG_GR_PRINT("CGameRulesRoundBased::EndMultiplayerGame()\n");
	RoundEnd();
	CGameRulesTeamplay::EndMultiplayerGame();// IntermissionStart() and stuff
}

//-----------------------------------------------------------------------------
// Purpose: Overridden. Parent CGameRules will call this after intermission
// and we must catch it and decide if it's end of game, or just end of round.
//-----------------------------------------------------------------------------
void CGameRulesRoundBased::ChangeLevel(void)
{
	DBG_GR_PRINT("CGameRulesRoundBased::ChangeLevel()\n");
	// Reload current map without destroying this instance of game rules class
	// This way we can retain all desired gameplay data while completely restoring the entire world
	if (m_iRoundsCompleted < GetRoundsLimit())
	{
		m_iPersistBetweenMaps = GR_PERSIST | GR_PERSIST_KEEP_EXTRASCORE;// keep instance of this
		CHANGE_LEVEL(STRINGV(gpGlobals->mapname), NULL);// restart level
	}
	else// all rounds are finished, load next map
	{
		m_iPersistBetweenMaps = 0;// new map, new score
		//endless loop g_pGameRules->EndMultiplayerGame();
		TEAM_ID bestteam = g_pGameRules->GetBestTeam();
		IntermissionStart(g_pGameRules->GetBestPlayer(bestteam), NULL/*m_pLastVictim may contain random person*/);
	}
}
