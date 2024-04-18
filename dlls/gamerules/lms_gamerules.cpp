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
#include "lms_gamerules.h"
#include "globals.h"
#include "pm_shared.h"
#include "voice_gamemgr.h"


extern CVoiceGameMgr g_VoiceGameMgr;

//-----------------------------------------------------------------------------
// Purpose: Runs every server frame, should handle any timer tasks, periodic events, etc.
//-----------------------------------------------------------------------------
void CGameRulesLMS::StartFrame(void)
{
	if (m_iGameState == GAME_STATE_WAITING)
	{
		m_fRemainingTime = mp_jointime.value - (gpGlobals->time - m_fStartTime);// so SendLeftUpdates() will send time remaining to join the match
		uint32 c = CountPlayers();
		if (c > 1)// don't do anything if there's only one client
		{
			bool bStart = false;
			if ((int)c >= gpGlobals->maxClients)// everyone's here, we can skip jointime
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
				// Start the game!
				GameSetState(GAME_STATE_SPAWNING);
				//RoundStart();
				CBasePlayer *pPlayer = NULL;
				for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
				{
					pPlayer = UTIL_ClientByIndex(i);
					if (pPlayer)
					{
						if (FBitSet(pPlayer->m_iGameFlags, EGF_PRESSEDREADY) || mp_lms_allowspectators.value <= 0.0f)// XDM3038a
							pPlayer->ObserverStop();// Spawn() is here
					}
				}
				m_flIntermissionEndTime = 0.0f;
				m_bReadyButtonsHit = 0;// ?
				GameSetState(GAME_STATE_ACTIVE);
				// Everyone connecting after this point won't be allowed into this round
			}
			else
				g_VoiceGameMgr.Update(gpGlobals->frametime);// don't run parent::StartFrame()
		}
	}
	else
		CGameRulesMultiplay::StartFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Server is activated, all entities spawned
// Input  : *pEdictList - 
//			edictCount - 
//			clientMax - 
//-----------------------------------------------------------------------------
/*void CGameRulesLMS::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
// don't set GAME_STATE_ACTIVE;
}*/

//-----------------------------------------------------------------------------
// Purpose: Initialize HUD (client data) for a client
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesLMS::InitHUD(CBasePlayer *pPlayer)
{
//	DBG_GR_PRINT("CGameRulesLMS::InitHUD(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	CGameRulesMultiplay::InitHUD(pPlayer);

	if (m_iGameState == GAME_STATE_WAITING)
	{
		if (pPlayer->IsObserver())
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#LMS_INTRO_WAITING");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Individual function for each player
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
/*void CGameRulesLMS::PlayerThink(CBasePlayer *pPlayer)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::PlayerThink(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (m_iGameState == GAME_STATE_ACTIVE)
	{
		if (pPlayer->IsObserver())// XDM3038a: catch those who failed to spawn
		{
			if ((gpGlobals->time - pPlayer->m_flLastSpawnTime) >= GetPlayerRespawnDelay(pPlayer))
				pPlayer->ObserverStop();
		}
	}
	CGameRulesMultiplay::PlayerThink(pPlayer);
}*/

//-----------------------------------------------------------------------------
// Purpose: A player got killed, run logic
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CGameRulesLMS::PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	DBG_GR_PRINT("CGameRulesMultiplay::PlayerKilled(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->netname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");
	CGameRulesMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);

	if (!IsGameOver() && GetScoreLimit() > 0 && pVictim->m_iDeaths >= GetScoreLimit())
	{
		pVictim->ObserverStart(pVictim->pev->origin, pVictim->pev->angles, pKiller?OBS_CHASE_FREE:OBS_ROAMING, pKiller);
		MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ALL:MSG_BROADCAST), gmsgGREvent);
			WRITE_BYTE(GAME_EVENT_PLAYER_OUT);
			WRITE_SHORT(pVictim->entindex());
			WRITE_SHORT((int)pVictim->pev->frags);
		MESSAGE_END();
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: Controls ObserverStart process
// Input  : *pPlayer - 
// Output : true - allow, false - deny
//-----------------------------------------------------------------------------
bool CGameRulesLMS::PlayerStartObserver(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesLMS::PlayerStartObserver(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (m_iGameState == GAME_STATE_ACTIVE)
	{
		if (pPlayer->m_iSpawnState < SPAWNSTATE_SPAWNED)// This player joined too late
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#LMS_JOIN_LATE");// Sorry, wait until this round ends
			return false;
		}
		//else if (!PlayerUseSpawnSpot(pPlayer, false))
	}
	return CGameRulesMultiplay::PlayerStartObserver(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: Controls observer status change process
// Input  : *pPlayer - 
// Output : true - allow, false - deny
//-----------------------------------------------------------------------------
bool CGameRulesLMS::PlayerStopObserver(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesLMS::PlayerStopObserver(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (!FPlayerCanRespawn(pPlayer))// player wants to join the game
		return false;

	return CGameRulesMultiplay::PlayerStopObserver(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Some players cannot respawn
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesLMS::FPlayerCanRespawn(const CBasePlayer *pPlayer)
{
	if (GetScoreLimit() > 0 && pPlayer->m_iDeaths >= GetScoreLimit())// XDM3038a: allow spawning if no limit is set
		return false;

	//noif (m_iGameState != GAME_STATE_SPAWNING)
	//	return false;

	return CGameRulesMultiplay::FPlayerCanRespawn(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: Should player become observer right from the very beginning
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesLMS::FPlayerStartAsObserver(CBasePlayer *pPlayer)
{
	if (m_iGameState == GAME_STATE_WAITING)// join time, everyone connects
	{
		return true;
	}
	else if (m_iGameState == GAME_STATE_ACTIVE)
	{
		if (pPlayer->m_iSpawnState < SPAWNSTATE_SPAWNED)// This player never spawned, don't allow him to enter the game
			return true;// if a player joins the game with 0 deaths a second before it ends, it's an instant win cheat!
		else if (!FPlayerCanRespawn(pPlayer))// XDM3038a: player died last allowed time, make him respawn as spectator
			return true;// never actually get here
	}

	//if (m_iGameState == GAME_STATE_SPAWNING || m_iGameState == GAME_STATE_ACTIVE)
	//	return false;

	return CGameRulesMultiplay::FPlayerStartAsObserver(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Get player with the best score (least deaths)
// Input  : teamIndex - 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *CGameRulesLMS::GetBestPlayer(TEAM_ID teamIndex)
{
	CBasePlayer *pPlayer = NULL;
	CBasePlayer *pBestPlayer = NULL;
	int score = 0;// can be negative
	int bestscore = INT_MIN;// XDM3037: in case there are only losers around
	uint32 limit = GetScoreLimit();//(int)mp_fraglimit.value;
	uint32 bestloses = limit;
	float bestlastscoretime = gpGlobals->time + SCORE_AWARD_TIME;// we use this to determine the first player to achieve his score. Allow + some seconds forward
	m_iLastCheckedNumActivePlayers = 0;
	m_iLastCheckedNumFinishedPlayers = 0;
	m_iRemainingScore = 0;// remaining score = how many players*times best player should score
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			if (pPlayer->m_iDeaths < limit && IsActivePlayer(pPlayer))// XDM3037a
			{
				++m_iLastCheckedNumActivePlayers;
				m_iRemainingScore += (limit - pPlayer->m_iDeaths);// XDM3036
				score = (int)pPlayer->pev->frags;
				if (pPlayer->m_iDeaths < bestloses)
				{
					bestscore = score;
					bestloses = pPlayer->m_iDeaths;
					bestlastscoretime = pPlayer->m_fNextScoreTime;
					pBestPlayer = pPlayer;
				}
				else if (pPlayer->m_iDeaths == bestloses)
				{
					if (score > bestscore)
					{
						bestscore = score;
						//==bestloses = pPlayer->m_iDeaths;
						bestlastscoretime = pPlayer->m_fNextScoreTime;
						pBestPlayer = pPlayer;
					}
					else if (score == bestscore)// TODO: the first one to achieve this score shoud win! lesser index/join time?
					{
						if (pPlayer->m_fNextScoreTime < bestlastscoretime)
						{
							//==bestscore = score;
							//==bestloses = pPlayer->m_iDeaths;
							bestlastscoretime = pPlayer->m_fNextScoreTime;
							pBestPlayer = pPlayer;
						}
					}
				}
			}
			else
				++m_iLastCheckedNumFinishedPlayers;
		}
	}
	if (pBestPlayer)
	{
		if (bestloses == limit)// nobody earned a single point, no winners.
			pBestPlayer = NULL;
		else
			m_iRemainingScore -= (limit - pBestPlayer->m_iDeaths);// XDM3037a: don't count own lives

		//if (pBestPlayer)
		//	best_player_index = pBestPlayer->entindex();
	}
	DBG_GR_PRINT("CGameRulesLMS::GetBestPlayer(%d): %d %s\n", teamIndex, pBestPlayer?pBestPlayer->entindex():0, pBestPlayer?STRING(pBestPlayer->pev->netname):"");
	return pBestPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: Check if the game has reached one of its limits and END the match
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesLMS::CheckLimits(void)
{
	CBasePlayer *pBestPlayer = g_pGameRules->GetBestPlayer(g_pGameRules->GetBestTeam());
	if (pBestPlayer == NULL)
		return false;

	CLIENT_INDEX best_player_index = pBestPlayer->entindex();
	if (m_iLastCheckedNumActivePlayers == 1)// winner
	{
		SERVER_PRINTF("GAME: ended by life limit (player %d \"%s\" wins with %d deaths)\n", best_player_index, STRING(pBestPlayer->pev->netname), pBestPlayer->m_iDeaths);
		IntermissionStart(pBestPlayer, NULL/*m_pLastVictim may contain random person*/);
		return true;
	}
	if (m_iLeader > 0 && m_iLeader != best_player_index && pBestPlayer->pev->frags > 0)
	{
		m_iLeader = best_player_index;
		if (pBestPlayer->pev->frags > 1)// XDM3035a: don't interrupt "first score" announcement?
		{
		MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
			WRITE_BYTE(GAME_EVENT_TAKES_LEAD);
			WRITE_SHORT(m_iLeader);
			WRITE_SHORT(pBestPlayer->pev->team);
		MESSAGE_END();
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check if it is ok to end the game by force (time limit)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesLMS::CheckEndConditions(void)
{
//	DBG_GR_PRINT("CGameRulesLMS::CheckEndConditions()\n");
	size_t numActivePlayers = 0;
	CBasePlayer *pPlayer;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			if (FPlayerIsActive(pPlayer))
				++numActivePlayers;
		}
	}
	if (numActivePlayers == 1)
		return true;// everything is ended according to rules

	return false;//	return CGameRulesMultiplay::CheckEndConditions();// we'd better use overtime (if allowed)
}

//-----------------------------------------------------------------------------
// Purpose: Score limit for this game type is determined by mp_fraglimit value
//-----------------------------------------------------------------------------
/*int CGameRulesLMS::GetScoreLimit(void)
{
	return iMaxDeaths
}*/
