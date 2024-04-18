//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "maprules.h"
#include "game.h"
#include "skill.h"
#include "items.h"
#include "voice_gamemgr.h"
#include "hltv.h"
#include "globals.h"
#include "mapcycle.h"
#include "pm_shared.h"// observer modes
#include "shake.h"
#include <time.h>
 
// Interface to allow voice engine query game rules
class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
		if (!g_pGameRules || (g_pGameRules->IsTeamplay() && g_pGameRules->PlayerRelationship(pListener, pTalker) != GR_TEAMMATE))
			return false;

		return true;
	}
};

CVoiceGameMgr g_VoiceGameMgr;
static CMultiplayGameMgrHelper g_GameMgrHelper;

// These must be saved between rules changes
static char g_szPreviousMapCycleFile[256] = {0};
static mapcycle_t g_MapCycle = {NULL, NULL};
// Must end with 0-terminator
int g_iszScoreAnnounceValues[] = {1000, 500, 400, 300, 200, 100, 50, 30, 20, 10, 5, 3, 2, 1, 0};
// Seconds
int g_iszTimeAnnounceValues[] = {6000, 3000, 1200, 600, 300, 180, 60, 30, 10, 0};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameRulesMultiplay::CGameRulesMultiplay() : CGameRules()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	m_pLastVictim = NULL;
	m_pIntermissionEntity1 = NULL;
	m_pIntermissionEntity2 = NULL;

	m_flIntermissionEndTime = 0.0f;
	m_flIntermissionStartTime = 0.0f;
	m_bReadyButtonsHit = false;

	m_iRemainingScore = 0;
	m_fRemainingTime = 0.0f;
	m_iRemainingScoreSent = 0;// XDM
	m_iRemainingTimeSent = 0;

	m_iFirstScoredPlayer = CLIENT_INDEX_INVALID;// XDM3035
	m_iLeader = CLIENT_INDEX_INVALID;// XDM3035
	//m_iAddItemsCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameRulesMultiplay::~CGameRulesMultiplay()
{
	m_flIntermissionEndTime = 0;
	m_flIntermissionStartTime = 0;
	m_pLastVictim = NULL;
	m_pIntermissionEntity1 = NULL;
	m_pIntermissionEntity2 = NULL;
	m_iRemainingScoreSent = 0;
	m_iRemainingTimeSent = 0;
	//DestroyMapCycle(&g_MapCycle);// XDM3038a: keep between rules
}

//-----------------------------------------------------------------------------
// Purpose: First thing called after constructor. Initialize all data, cvars, etc.
// Warning: Called from CWorld::Precache(), so all other entities are not spawned yet!
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::Initialize(void)
{
	DBG_GR_PRINT("CGameRulesMultiplay::Initialize()\n");
	if (IS_DEDICATED_SERVER())
	{
		const char *servercfgfile = CVAR_GET_STRING("servercfgfile");
		if (servercfgfile && servercfgfile[0])
		{
			char szCommand[256];
			conprintf(1, "Executing dedicated server config file\n");
			sprintf(szCommand, "exec %s\n", servercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
	else
	{
		const char *lservercfgfile = CVAR_GET_STRING("lservercfgfile");
		if (lservercfgfile && lservercfgfile[0])
		{
			char szCommand[256];
			conprintf(1, "Executing listen server config file\n");
			sprintf(szCommand, "exec %s\n", lservercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
	SERVER_EXECUTE();// XDM3037: apply configs NOW!

	m_pLastVictim = NULL;
	m_pIntermissionEntity1 = NULL;
	m_pIntermissionEntity2 = NULL;
	m_flIntermissionEndTime = 0.0f;
	m_flIntermissionStartTime = 0.0f;
	m_iRemainingScore = 0;
	m_fRemainingTime = 0.0f;
	m_iRemainingScoreSent = 0;
	m_iRemainingTimeSent = 0;
	m_iLastCheckedNumActivePlayers = 0;
	m_iFirstScoredPlayer = CLIENT_INDEX_INVALID;
	m_iLeader = CLIENT_INDEX_INVALID;
	m_bReadyButtonsHit = false;
	m_bSendStats = false;

	CGameRules::Initialize();
	// XDM3038a: these two better be set after sv_playermaxhealth is validated
	// Since these may be changed later, more checks must be done before use
	if (mp_playerstartarmor.value < 0.0f)
	{
		mp_playerstartarmor.value = 0.0f;
		CVAR_DIRECT_SET(&mp_playerstartarmor, "0");
	}
	if (mp_playerstarthealth.value < 1.0f)
	{
		mp_playerstarthealth.value = GetPlayerMaxHealth();// default to maximum
		CVAR_DIRECT_SET(&mp_playerstarthealth, UTIL_dtos1((int)mp_playerstarthealth.value));
	}

	//InitAddDefault();

	// XDM3035 crash recovery CONSIDER: write config using template: server.scr ?
	g_pServerAutoFile = LoadFile(g_szServerAutoFileName, "w+");// Opens an empty file for both reading and writing. If the given file exists, its contents are destroyed.
	if (g_pServerAutoFile)
	{
		fseek(g_pServerAutoFile, 0L, SEEK_SET);
#if defined (_WIN32)
		_tzset();
#endif
		//char bufdate[16];
		//char buftime[16];
		//_strdate(bufdate);
		//_strtime(buftime);
		time_t crt_time;
		time(&crt_time);
		char szTime[32];
		strftime(szTime, 32, "%Y%m%d @ %H%M", localtime(&crt_time));
		fprintf(g_pServerAutoFile, "// SERVER \"%s\" RUNNING \"%s\" ON MAP \"%s\"\n", CVAR_GET_STRING("hostname"), g_pGameRules->GetGameDescription(), STRING(gpGlobals->mapname));
		fprintf(g_pServerAutoFile, "// This file was generated %s\n", szTime);//fprintf(g_pServerAutoFile, "// This file was generated %s @ %s\n", bufdate, buftime);
		fprintf(g_pServerAutoFile, "echo \" *** Running server recovery config file.\"\n");
		//fprintf(g_pServerAutoFile, "maxplayers %d\nmap %s\n", maxplayers, szNextMap);
		//?fprintf(g_pServerAutoFile, "mp_gamerules %d\n", (int)mp_gamerules.value);
		fprintf(g_pServerAutoFile, "map %s\n", STRING(gpGlobals->mapname));
		fflush(g_pServerAutoFile);// IMPORTANT: write to disk
		fclose(g_pServerAutoFile);
		g_pServerAutoFile = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client entered a console command
// Warning: Don't allow cheats
// Input  : *pPlayer - client
//			*pcmd - command line, use CMD_ARGC() and CMD_ARGV()
// Output : Returns true if the command was recognized.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	if (FStrEq(pcmd, "follownext"))
	{
		if (pPlayer->IsObserver())
			pPlayer->ObserverFindNextPlayer(false);
	}
	else if (FStrEq(pcmd, "followprev"))
	{
		if (pPlayer->IsObserver())
			pPlayer->ObserverFindNextPlayer(true);
	}
	else if (FStrEq(pcmd, "follow"))
	{
		if (pPlayer->IsObserver())
		{
			CBasePlayer *pTarget = UTIL_ClientByIndex(atoi(CMD_ARGV(1)));
			if (pTarget)
			{
				// XDM3037
				if (!g_pGameRules->IsTeamplay() ||
					(mp_specteammates.value == 0.0f || (pPlayer->GetLastTeam() == TEAM_NONE || pTarget->pev->team == pPlayer->GetLastTeam())))
				{
					pPlayer->ObserverSetTarget(pTarget);
				}
			}
		}
	}
	else if (FStrEq(pcmd, "spectate"))// this exact command is required and called by the engine
	{
		if (FBitSet(pPlayer->pev->flags, FL_PROXY))// HL20130901
		{
			pPlayer->ObserverStart(pPlayer->pev->origin, pPlayer->pev->angles, OBS_CHASE_LOCKED, NULL);
		}
		else if (!IsGameOver())
		{
			if (g_pGameRules->FAllowSpectatorChange(pPlayer))
			{
				// XDM3038a: we have to disable toggling by this command, because it may be called at unpredictable moments
				//if (pPlayer->IsObserver())// DO NOT allow players to ObserverStart() more than once! Last team will be erased and player will be able to track anyone.
				//	pPlayer->ObserverStop();
				//else if (g_pGameRules->FAllowSpectators())
				if (!pPlayer->IsObserver() && g_pGameRules->FAllowSpectators())
					if (pPlayer->ObserverStart(pPlayer->pev->origin, pPlayer->pev->angles, OBS_ROAMING, NULL))
						SetBits(pPlayer->m_iGameFlags, EGF_SPECTOGGLED);// XDM3038c
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#SPECCHANGENOTALLOWED");
		}
	}
	else if (FStrEq(pcmd, "specmode"))// doesn't conflict with client's spec_mode
	{
		if (pPlayer->IsObserver())
			pPlayer->ObserverSetMode(atoi(CMD_ARGV(1)));
	}
	else if (FStrEq(pcmd, "joingame"))
	{
		if (!IsGameOver())
		{
			if (FAllowSpectatorChange(pPlayer))
			{
				if (pPlayer->IsObserver())
					pPlayer->ObserverStop();
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#SPECCHANGENOTALLOWED");
		}
	}
	else if (g_psv_cheats->value > 0.0f && DeveloperCommand(pPlayer, pcmd))
	{
		// developer command was used
	}
#if defined (_DEBUG)
	else if (FStrEq(pcmd, "whois"))
	{
		if (CMD_ARGC() > 1)
		{
			CBasePlayer *pTarget = UTIL_ClientByIndex(atoi(CMD_ARGV(1)));
			if (pTarget)
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Client: %s (%s) team %s\n", STRING(pTarget->pev->netname), GETPLAYERAUTHID(pTarget->edict()), GET_INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(pTarget->edict()), "team"));
		}
	}
	else if (FStrEq(pcmd, "dumpplayers"))
	{
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, " ID (name) frags deaths team (team name)\n");
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			CBasePlayer *plr = UTIL_ClientByIndex(i);
			if (plr)
				ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " > %d (%s)\t fr %d dh %d\tteam %d (%s)\n", i, STRING(plr->pev->netname), (int)plr->pev->frags, plr->m_iDeaths, plr->pev->team, GetTeamName(plr->pev->team));
		}
	}
	else if (FStrEq(pcmd, "sv_cmd_gm"))
	{
		if (g_psv_cheats && g_psv_cheats->value > 0.0f && g_pCvarDeveloper && g_pCvarDeveloper->value > 0.0f/* && pPlayer == UTIL_ClientByIndex(1)*/)
		{
			CBasePlayer *cl = UTIL_ClientByIndex(atoi(CMD_ARGV(1)));
			if (cl)
			{
				char *msg = NULL;
				cl->pev->flags ^= FL_GODMODE;
				if (FBitSet(cl->pev->flags, FL_GODMODE))
					msg = "activated";
				else
					msg = "deactivated";

				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "God mode for %s %s\n", STRING(cl->pev->netname), msg);
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Client not found.\n");
		}
	}
#endif // _DEBUG
	else if (g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
	{
	}
	/*else if (BotmatchCommand(pPlayer, pcmd))
	{
	}*/
	else
		return CGameRules::ClientCommand(pPlayer, pcmd);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: User info changed // BUGBUG: top/bottomcolor are left forced after teamplay
// Input  : *pPlayer - 
//			*infobuffer - 
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer)
{
	CGameRules::ClientUserInfoChanged(pPlayer, infobuffer);
}


//-----------------------------------------------------------------------------
// Purpose: Get player with the best score   BUGBUG! Why is this called in CTF and DOM?!!
// Warning: Must be identical to client scoreboard code!
// Input  : team - may be TEAM_NONE
// Output : CBasePlayer *
//-----------------------------------------------------------------------------
CBasePlayer *CGameRulesMultiplay::GetBestPlayer(TEAM_ID teamIndex)
{
	if (g_pGameRules->IsTeamplay())// XDM3037: should never be true
	{
		if (teamIndex == TEAM_NONE)
			return NULL;
	}
	CBasePlayer *pPlayer = NULL;
	CBasePlayer *pBestPlayer = NULL;
	int score = 0;// CAN be negative!
	int bestscore = INT_MIN;// XDM3037: in case there are only losers around
	uint32 bestloses = UINT_MAX;
	float bestlastscoretime = gpGlobals->time + SCORE_AWARD_TIME;// we use this to determine the first player to achieve his score. Allow + some seconds forward
	//m_iRemainingScore
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			if (pPlayer->IsObserver())// XDM3035: don't award spectators!
				continue;
			if (teamIndex != TEAM_NONE && pPlayer->pev->team != teamIndex)
				continue;// skip if particular team specified and it didn't match

			score = (int)pPlayer->pev->frags;
			if (score > bestscore)
			{
				bestscore = score;
				bestloses = pPlayer->m_iDeaths;
				bestlastscoretime = pPlayer->m_fNextScoreTime;
				pBestPlayer = pPlayer;
			}
			else if (score == bestscore)
			{
				if (pPlayer->m_iDeaths < bestloses)
				{
					//==bestscore = score;
					bestloses = pPlayer->m_iDeaths;
					bestlastscoretime = pPlayer->m_fNextScoreTime;
					pBestPlayer = pPlayer;
				}
				else if (pPlayer->m_iDeaths == bestloses)// TODO: the first one to achieve this score shoud win! lesser index/join time?
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
	}

	if (bestscore <= 0)// XDM3035c: nobody earned a single point, no winners. Can't be < 0.
	{
		if (mp_overtime.value > 0 || !g_pGameRules->IsTeamplay())// XDM3037: in teamplay it matters not, just for the camera TODO: (g_pGameRules->FUseExtraScore)?
			pBestPlayer = NULL;
	}
	DBG_GR_PRINT("CGameRulesMultiplay::GetBestPlayer(%hd): %d %s\n", teamIndex, pBestPlayer?pBestPlayer->entindex():0, pBestPlayer?STRING(pBestPlayer->pev->netname):"");
	return pBestPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: Runs every server frame, should handle any timer tasks, periodic events, etc.
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::StartFrame(void)
{
	//DBG_GR_PRINT("CGameRulesMultiplay::StartFrame()\n");

	g_VoiceGameMgr.Update(gpGlobals->frametime);

	if (IsGameOver())// intermission in progress
	{
		if (m_bSendStats)// XDM3038c
		{
			for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
			{
				CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
				if (pPlayer)
				{
					MESSAGE_BEGIN(MSG_ALL, gmsgPlayerStats, NULL, NULL);
						WRITE_BYTE(i);
						WRITE_BYTE(STAT_NUMELEMENTS);
						for (short c = 0; c < STAT_NUMELEMENTS; ++c)
							WRITE_LONG(pPlayer->m_Stats[c]);

					MESSAGE_END();
				}
			}
			m_bSendStats = false;
		}

		if (m_bReadyButtonsHit == false)// XDM3036: now all players must press ready
			m_bReadyButtonsHit = CheckPlayersReady();

		// check to see if we should change levels now
		if (m_bReadyButtonsHit || (m_flIntermissionEndTime != 0.0f && m_flIntermissionEndTime < gpGlobals->time))
		{
			m_flIntermissionEndTime = 0.0f;
			ChangeLevel();// intermission is over
		}
		//return;
	}
	else if (m_iGameState == GAME_STATE_WAITING)// XDM3038a
	{
		m_fRemainingTime = mp_jointime.value - (gpGlobals->time - m_fStartTime);// so SendLeftUpdates() will send time remaining to join the match
	}
	else if (m_iGameState == GAME_STATE_LOADING)
	{
		// do nothing
		m_iRemainingScore = 0;
		m_fRemainingTime = 0;
	}
	else//	if (CheckLimits() == false)
	{
		// time limit check must be done every frame
		float flTimeLimit = GetTimeLimit();// XDM3038
		if (flTimeLimit > 0.0f)
		{
			float fTimePassed = gpGlobals->time - m_fStartTime;
			if (fTimePassed >= flTimeLimit)
			{
				if (m_fRemainingTime != 0.0f)// only once
					SERVER_PRINTF("GAME: time limit reached (%g seconds)\n", flTimeLimit);

				if (CheckEndConditions() == false && mp_overtime.value > 0)// XDM3038: start overtime if it is REALLY necessary
				{
					if (m_fRemainingTime != 0.0f)// only once
					{
						MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ALL:MSG_BROADCAST), gmsgGREvent);
							WRITE_BYTE(GAME_EVENT_OVERTIME);// XDM3037a
							WRITE_SHORT((short)mp_overtime.value);
							WRITE_SHORT(0);
						MESSAGE_END();
						m_fRemainingTime = 0.0f;
					}
				}
				else
				{
					m_fRemainingTime = 0.0f;
					SERVER_PRINT("GAME: ended by time limit\n");
					EndMultiplayerGame();// same as IntermissionStart(g_pGameRules->GetBestPlayer(g_pGameRules->GetBestTeam()), NULL);
					return;
				}
			}
			m_fRemainingTime = (flTimeLimit > fTimePassed ? (flTimeLimit - fTimePassed) : 0.0f);// XDM3037a: fixed
		}
		else
			m_fRemainingTime = 0.0f;

		SendLeftUpdates();
	}
}

//-----------------------------------------------------------------------------
// Purpose: This function sends rapid gamerules-related updates, optimize and pack as much as possible!
// Safe to be called from StartFrame (e.g. once per frame) only!
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::SendLeftUpdates(void)
{
	static uint32 iLimitScorePrev = 0;
	static uint32 iLimitTimePrev = 0;
	static uint32 iLimitDeathPrev = 0;// XDM3038a: NOT LMS!
// SPAM DBG_GR_PRINT("CGameRulesMultiplay::SendLeftUpdates()\n");
	if (IsGameOver())
		return;// intermission has already been triggered, so ignore.

	// Everything here is converted to uint values which can be reliably compared and sent over network
	uint32 score_limit = g_pGameRules->GetScoreLimit();
	uint32 score_remaining = g_pGameRules->GetScoreRemaining();
	uint32 death_limit = mp_deathlimit.value;// XDM3038a
	uint32 time_limit = GetTimeLimit();
	uint32 time_remaining = 0;
	if (m_fRemainingTime > 0)
		time_remaining = (uint32)m_fRemainingTime;// no point virtualizing it // quantized by 1 second

//	DBG_GR_PRINT("CGameRulesMultiplay::SendLeftUpdates() 1\n");
	// some ideas
//WTF? this function returns crap	if (IS_DEDICATED_SERVER())// XDM3035
	// Client can't read mp_timeleft cvar when connected to hlds

	// prevent sending 200 messages/second and also track major changes
	short send = 0;

	// HACK: since we don't have any OnCVarChange() callback from the engine, we can only assume when something changes
	if (score_limit != iLimitScorePrev)// server admin has changed the score limit
	{
		send |= UPDATE_LEFT_SCORE | UPDATE_LEFT_FORCE;
		iLimitScorePrev = score_limit;
	}
	else if (time_limit != iLimitTimePrev)// server admin has changed the time limit
	{
		send |= UPDATE_LEFT_TIME | UPDATE_LEFT_FORCE;
		iLimitTimePrev = time_limit;// XDM3038
	}
	else if (death_limit != iLimitDeathPrev)// server admin has changed the time limit
	{
		send |= UPDATE_LEFT_OTHER | UPDATE_LEFT_FORCE;
		iLimitDeathPrev = death_limit;// XDM3038
	}
	else// limits unchanged, check for "remaining" values
	{
		if ((score_remaining != m_iRemainingScoreSent) || (score_remaining != 0 && m_iRemainingScoreSent == 0))
			send |= UPDATE_LEFT_SCORE;
		// not else!
		if ((time_remaining != m_iRemainingTimeSent) || (time_remaining != 0 && m_iRemainingTimeSent == 0))
			send |= UPDATE_LEFT_TIME;
	}

	if ((send & UPDATE_LEFT_FORCE) != 0)
		UpdateGameMode(NULL);// send new limits to everyone; this does not include "remaining" values

	// Updates when remaining score change
	CVAR_DIRECT_SET(&mp_scoreleft, UTIL_VarArgs("%u", score_remaining));
	// Updates once per second
	CVAR_DIRECT_SET(&mp_timeleft, UTIL_VarArgs("%u", time_remaining));

//	DBG_GR_PRINT("CGameRulesMultiplay::SendLeftUpdates(send = %hd) 2\n", send);
	if (send > 0 && g_ServerActive)
	{
		if ((send & UPDATE_LEFT_FORCE) != 0 ||
			((score_remaining > 0) && (send & UPDATE_LEFT_SCORE) != 0 && Array_FindInt(g_iszScoreAnnounceValues, score_remaining) != -1) ||
			((time_limit > 0) && (send & UPDATE_LEFT_TIME) != 0 && Array_FindInt(g_iszTimeAnnounceValues, time_remaining) != -1))
		{
			TEAM_ID best_team_index = GetBestTeam();
			DBG_GR_PRINT("CGameRulesMultiplay::SendLeftUpdates(): gmsgGRInfo(%d %hd %u %u)\n", m_iLeader, best_team_index, score_remaining, time_remaining);
			//ASSERT(score_remaining < score_limit);
			MESSAGE_BEGIN(MSG_ALL, gmsgGRInfo);
				WRITE_BYTE(m_iLeader);
				WRITE_BYTE(best_team_index);
				WRITE_SHORT(score_remaining);// TODO: WRITE_LONG ?
				WRITE_LONG(time_remaining);
			MESSAGE_END();
			m_iRemainingScoreSent = score_remaining;
			m_iRemainingTimeSent  = time_remaining;
		}
	}
//	DBG_GR_PRINT("CGameRulesMultiplay::SendLeftUpdates() END\n");

	//fail (	g_pWorld->pev->frags = score_remaining;// XDM3035: attempt to send these to all clients through entity
	//fail (	g_pWorld->pev->impacttime = time_remaining;
}

//-----------------------------------------------------------------------------
// Purpose: Should players respawn immediately?
// Output : Returns true or false
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FForceRespawnPlayer(CBasePlayer *pPlayer)
{
	if (mp_forcerespawn.value > 0)
	{
		if (pPlayer->m_fDiedTime > 0.0f && (gpGlobals->time >= (pPlayer->m_fDiedTime + mp_forcerespawntime.value)))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: players are not allowed to materialize if the game is over
// Output : Returns true or false
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FPlayerStartAsObserver(CBasePlayer *pPlayer)
{
	return IsGameOver();
}

//-----------------------------------------------------------------------------
// Purpose: Check if all active players pressed ready (+USE) buttons
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::CheckPlayersReady(void)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::CheckPlayersReady()\n");
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			if (m_iGameState == GAME_STATE_WAITING || FBitSet(pPlayer->m_iGameFlags, EGF_JOINEDGAME))// XDM3038a: players never joined at LMS/teammenu start!
			{
				if (!FBitSet(pPlayer->m_iGameFlags, EGF_PRESSEDREADY))// XDM3037a: faster
					return false;
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when player picks up a new weapon
// Input  : *pPlayer -
//			*pWeapon -
// Output : Returns true or false
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::FShouldSwitchWeapon(%d %s)\n", pPlayer?pPlayer->entindex():0, STRING(pPlayer->pev->netname));
	if (!pWeapon || !pWeapon->CanDeploy())// maybe && pPlayer->CanDeployItem(pWeapon)? or not...
		return false;// that weapon can't deploy anyway.

	if (pPlayer->m_pActiveItem == NULL)
		return true;// player doesn't have an active item!

	if (!pPlayer->m_pActiveItem->CanHolster())
		return false;// can't put away the active item.

#if defined(SERVER_WEAPON_SLOTS)
	if (pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight())
		return true;
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: player have found a secret
// Input  : *pPlayer - activator
//			*pSecret - the trigger
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::OnPlayerFoundSecret(CBasePlayer *pPlayer, CBaseEntity *pSecret)
{
	DBG_GR_PRINT("CGameRulesMultiplay::OnPlayerFoundSecret(%d %s, %d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), pSecret?pSecret->entindex():0, pSecret?STRING(pSecret->pev->classname):"");
	MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
		WRITE_BYTE(GAME_EVENT_SECRET);
		WRITE_SHORT(pPlayer->entindex());
		WRITE_SHORT(pSecret?pSecret->entindex():0);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize HUD (client data) for a client
// Input  : *pPlayer - dest
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::InitHUD(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesMultiplay::InitHUD(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	UpdateGameMode(pPlayer->edict());

	//if (sv_lognotice.value > 0)
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" entered the game\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), GET_INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(pPlayer->edict()), "team"));

	if (!IsTeamplay())// XDM3038: clear out client's team info
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict());
			WRITE_BYTE(0);
		MESSAGE_END();
	}

	// loop through all active players and send their score info to the new client
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pClient = UTIL_ClientByIndex(i);
		if (pClient)
		{
			// Send this client's score info to me
			MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pPlayer->edict());
				WRITE_BYTE(i);// client number
				WRITE_SHORT((int)pClient->pev->frags);// In case someone needs score more than +-32768, change this to LONG
				WRITE_SHORT((int)pClient->m_iDeaths);
			MESSAGE_END();
			// Send this client's spectator state to me
			MESSAGE_BEGIN(MSG_ONE, gmsgSpectator, NULL, pPlayer->edict());
				WRITE_BYTE(i);
				WRITE_BYTE(pClient->pev->iuser1);// XDM: was != 0
			MESSAGE_END();
		}
	}

	// XDM3038a: so client can start counting (moved from before gmsgScoreInfo)
	//conprintf(1, "CGameRulesMultiplay::InitHUD: gmsgGRInfo(%d %d %d %g)\n", m_iLeader, GetBestTeam(), GetScoreRemaining(), m_fRemainingTime);
	MESSAGE_BEGIN(((sv_reliability.value > 0)?MSG_ONE:MSG_ONE_UNRELIABLE), gmsgGRInfo, NULL, pPlayer->edict());
		WRITE_BYTE(m_iLeader);
		WRITE_BYTE(GetBestTeam());
		WRITE_SHORT(GetScoreRemaining());
		if (m_iGameState == GAME_STATE_WAITING)// XDM3038a: 20150813 client code will set this as join time
			WRITE_LONG((int)mp_jointime.value);// XDM3037a
		else if (m_iGameState == GAME_STATE_SPAWNING)
			WRITE_LONG(GetTimeLimit());
		else
			WRITE_LONG((int)m_fRemainingTime);
	MESSAGE_END();

	if (!pPlayer->m_fGameHUDInitialized)// XDM: this may get called from spectator code
	{
		// send the server name
		MESSAGE_BEGIN(MSG_ONE, gmsgServerName, NULL, pPlayer->edict());
			WRITE_STRING(CVAR_GET_STRING("hostname"));
		MESSAGE_END();
		SendMOTDToClient(pPlayer);
	}

	if (IsGameOver())
	{
		MESSAGE_BEGIN(MSG_ONE, svc_intermission, NULL, pPlayer->edict());
		MESSAGE_END();
		// XDM3035c
		pPlayer->pev->framerate = 0;// stop!
		pPlayer->pev->velocity.Clear();
		pPlayer->pev->speed = 0;
		pPlayer->pev->iuser1 = OBS_INTERMISSION;
		if (m_pIntermissionEntity1)
		{
			pPlayer->m_hObserverTarget = m_pIntermissionEntity1;// XDM3035c: use his PVS
			pPlayer->pev->iuser2 = m_pIntermissionEntity1->entindex();
		}
		else
			pPlayer->pev->iuser2 = 0;

		if (m_pIntermissionEntity2)
			pPlayer->pev->iuser3 = m_pIntermissionEntity2->entindex();
		else
			pPlayer->pev->iuser3 = 0;
	}
	else// always if (mp_firetargets_player.value > 0)
	{
		// XDM3037a: display team selection menu or MOTD. TEAM INFORMATION MUST BE SENT TO CLIENT SIDE BEFOREHAND!
		if (pPlayer->m_iSpawnState <= SPAWNSTATE_SPAWNED && !pPlayer->m_fGameHUDInitialized)// first time
		{
			// Bots SHOULD recieve this!!
			int iMenuID = ShouldShowStartMenu(pPlayer);
			//if (iMenuID != MENU_CLOSEALL)
			//{
				DBG_PRINTF("SV: player %d (%s): sending menu %d open signal\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), iMenuID);
				MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pPlayer->edict());// dest: all
					WRITE_BYTE(iMenuID);
					WRITE_BYTE(iMenuID==MENU_CLOSEALL?MENUFLAG_CLOSE:0);
				MESSAGE_END();
			//}
		}
		FireTargets("game_playerjoin", pPlayer, pPlayer, USE_TOGGLE, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Game rules may require the player to choose team or something
// Input  : *pPlayer - 
// Output : int - menu index, 0 = none
//-----------------------------------------------------------------------------
int CGameRulesMultiplay::ShouldShowStartMenu(CBasePlayer *pPlayer)
{
	return MENU_INTRO;
}

//-----------------------------------------------------------------------------
// Purpose: Server is activated, all entities spawned
// Input  : *pEdictList - 
//			edictCount - 
//			clientMax - 
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	CGameRules::ServerActivate(pEdictList, edictCount, clientMax);

	if (!IsRoundBased())// in round-based matches this is called at real round start
		FireTargets("game_roundstart", g_pWorld, g_pWorld, USE_TOGGLE, 0);// XDM3038
}

//-----------------------------------------------------------------------------
// Purpose: A network client is connecting. DO NOT CONVERT TO CBasePlayer!!!
// If ClientConnected returns FALSE, the connection is rejected and the user is provided the reason specified in svRejectReason
// Only the client's name and remote address are provided to the dll for verification.
// Input  : *pEntity -
//			*pszName -
//			*pszAddress -
//			szRejectReason -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	DBG_GR_PRINT("CGameRulesMultiplay::ClientConnected(%d %s)\n", pszName, pszAddress);
	pEntity->v.origin.z = -4095;// XDM3037a
	//pEntity->v.solid = SOLID_NOT;
	SetBits(pEntity->v.effects, EF_NOINTERP|EF_NODRAW);// XDM3035a
	if (!FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_SCORE))// XDM3038a: does it do anything?
		pEntity->v.frags = 0;

	pEntity->v.team = TEAM_NONE;// XDM: IMPORTANT !!!!!!
	pEntity->v.euser4 = NULL;// XDM3038: for COOP (landmark)
	g_VoiceGameMgr.ClientConnected(pEntity);

	ClientPrint(NULL, HUD_PRINTTALK, "+ #CL_JOIN\n", pszName);

	//if (g_DisplayTitle || mp_showgametitle.value > 0.0f)
	//	pPlayer->m_bDisplayTitle = true;

	UpdateGameMode(pEntity);// test
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Client has been disconnected
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::ClientDisconnected(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesMultiplay::ClientDisconnected(%d %s)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"");
	if (g_ServerActive && !IsGameOver())// always && (mp_firetargets_player.value > 0))
		FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

	ClientRemoveFromGame(pPlayer);// XDM3038c
	CLIENT_INDEX iPlayer = pPlayer->entindex();
	// XDM: this may be called from ObserverStart()
	if (!FBitSet(pPlayer->pev->flags, FL_SPECTATOR))
	{
		if (!FStringNull(pPlayer->pev->netname))
			ClientPrint(NULL, HUD_PRINTTALK, "- #CL_LEAVE\n", STRING(pPlayer->pev->netname));

		pPlayer->StatsSave();
		if (!FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_STATS))// XDM3038a
			pPlayer->StatsClear();

		if (m_iFirstScoredPlayer == iPlayer)// XDM3037a: now we supply some invalid non-zero index
			m_iFirstScoredPlayer = CLIENT_INDEX_INVALID;// XDM3038: TESTME: 0 must count as invalid index
	}
	pPlayer->pev->iuser1 = 0;
	pPlayer->pev->iuser2 = 0;
	pPlayer->pev->iuser3 = 0;
	pPlayer->pev->iuser4 = 0;
	pPlayer->pev->euser1 = NULL;
	pPlayer->pev->euser2 = NULL;
	pPlayer->pev->euser3 = NULL;
	pPlayer->pev->euser4 = NULL;// XDM3038: for COOP (landmark)
	pPlayer->m_iDeaths = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Remove client from scoreboard, disable interaciton, teams, etc. Used by spectators mostly.
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::ClientRemoveFromGame(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesMultiplay::ClientRemoveFromGame(%d %s)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"");
	if (pPlayer == NULL)
		return;
	CLIENT_INDEX iPlayer = pPlayer->entindex();
	if (pPlayer->HasWeapons())
	{
		pPlayer->PackImportantItems();
		pPlayer->RemoveAllItems(true, true);// destroy all of the players weapons and items
	}
	DeactivateMines(pPlayer);// XDM3035
	DeactivateSatchels(pPlayer);// XDM3035
	CBasePlayer *pClient = NULL;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		if (i == iPlayer)
			continue;

		pClient = UTIL_ClientByIndex(i);
		if (pClient)
		{
			//if (pClient == pPlayer)
			//	continue;

			// If a spectator was chasing this player, move him/her onto the next player
			if (pClient->IsObserver() && pClient->m_hObserverTarget == pPlayer)
				pClient->ObserverFindNextPlayer(false);

			// Don't let this freed index be remembered anywhere! Otherwise players won't be able to get revenge, etc.
			if (pClient->m_hLastKiller == pPlayer)
				pClient->m_hLastKiller = NULL;
			if (pClient->m_hLastVictim == pPlayer)
				pClient->m_hLastVictim = NULL;
		}
	}
	if (m_iLeader == iPlayer)// XDM3038: same here
		m_iLeader = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Check if the game has reached one of its limits and END the match
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::CheckLimits(void)
{
	DBG_GR_PRINT("CGameRulesMultiplay::CheckLimits()\n");
	uint32 ifraglimit = GetScoreLimit();
	if (ifraglimit > 0)
	{
		uint32 score_remaining = 0;
		CBasePlayer *pBestPlayer = g_pGameRules->GetBestPlayer(g_pGameRules->GetBestTeam());
		if (pBestPlayer)
		{
			CLIENT_INDEX best_player_index = pBestPlayer->entindex();
			if ((int)pBestPlayer->pev->frags >= ifraglimit)// NOTANERROR: frags can be negative, limit can't!
			{
				SERVER_PRINTF("GAME: ended by score limit (player %d \"%s\" wins with score %d)\n", best_player_index, STRING(pBestPlayer->pev->netname), (int)pBestPlayer->pev->frags);
				IntermissionStart(pBestPlayer, NULL/*m_pLastVictim may contain random person*/);
				return true;
			}

			if (m_iLeader > 0 && m_iLeader != best_player_index && pBestPlayer->pev->frags > 0)
			{
				m_iLeader = best_player_index;
				if (pBestPlayer->pev->frags > 1)// XDM3035a: don't interrupt "first score" announcement
				{
				MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
					WRITE_BYTE(GAME_EVENT_TAKES_LEAD);
					WRITE_SHORT(m_iLeader);
					WRITE_SHORT(pBestPlayer->pev->team);
				MESSAGE_END();
				}
			}
			score_remaining = ifraglimit - (int)pBestPlayer->pev->frags;
		}
		else
			score_remaining = ifraglimit;

		m_iRemainingScore = score_remaining;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check if it is ok to end the game by force (time limit)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::CheckEndConditions(void)
{
	DBG_GR_PRINT("CGameRulesMultiplay::CheckEndConditions()\n");
	if (g_pGameRules->GetBestPlayer(g_pGameRules->GetBestTeam()) != NULL)
		return true;// we have a potential winner, game can end according to rules

	return false;// we'd better use overtime (if allowed)
}

//-----------------------------------------------------------------------------
// Purpose: How much damage should falling players take
// Input  : *pPlayer -
// Output : float
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetPlayerFallDamage(CBasePlayer *pPlayer)
{
	if (pPlayer->m_fFrozen || mp_falldamage.value > 0.0f)// XDM3035a
	{
		// XDM3038c: WTF?! pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return (pPlayer->m_flFallVelocity - PLAYER_MAX_SAFE_FALL_SPEED) * DAMAGE_FOR_FALL_SPEED;
	}
	return PLAYER_CONSTANT_FALL_DAGAMGE;
}

//-----------------------------------------------------------------------------
// Purpose: Can this player take damage from this attacker?
// Input  : *pPlayer - target
//			*pAttacker -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::FPlayerCanTakeDamage(%d %s, %d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), pAttacker?pAttacker->entindex():0, pAttacker?STRING(pAttacker->pev->classname):"");
	if (IsGameOver())// XDM3035: nobody wants to see score changed after the end!
		return false;

	if (pAttacker && !pAttacker->IsBSPModel())// world can inflict damage any time
	{
		if ((gpGlobals->time - pPlayer->m_flLastSpawnTime) <= max(0.0f, mp_spawnprotectiontime.value))
		{
			if (!FBitSet(pPlayer->m_afButtonLast, BUTTONS_FIRE) &&
				!FBitSet(pPlayer->m_afButtonPressed, BUTTONS_FIRE))// XDM3035c: testme
			{
				//DBG_PRINTF("CGameRulesMultiplay::FPlayerCanTakeDamage(%d %s, %d %s): Spawn protection.\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), pAttacker?pAttacker->entindex():0, pAttacker?STRING(pAttacker->pev->classname):"");
				return false;
			}
		}

		if (g_pGameRules->GetCombatMode() != GAME_COMBATMODE_NORMAL && pAttacker->IsPlayer())// XDM3038a: can shoot, but can't hurt
			return false;
		//if (g_pGameRules->GetCombatMode() == GAME_COMBATMODE_NOSHOOTING && (pAttacker->IsPlayer() || pAttacker->IsMonster()))// XDM3038a: cannot shoot, don't let monsters hurt players?
		//	return false;

		if (mp_revengemode.value > 1.0f)// mode 2: players cannot hurt anyone until they get revenge!
		{
			if (pAttacker->IsPlayer())
			{
				CBasePlayer *pAttackerPlayer = (CBasePlayer *)pAttacker;
				if (pAttackerPlayer->m_hLastKiller.Get())// killer may have disconnected/invalidated
				{
					CBaseEntity *pLastKiller = pAttackerPlayer->m_hLastKiller;
					if (pLastKiller != pAttackerPlayer && pLastKiller != pPlayer)
					{
						if (pAttackerPlayer->m_flNextChatTime <= gpGlobals->time)
						{
							ClientPrint(pAttackerPlayer->pev, HUD_PRINTCENTER, "#REVENGE_NEED", STRING(pLastKiller->pev->netname));//"Can't hurt anyone! Get revenge!\n");
							pAttackerPlayer->m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;
						}
						return false;
					}
				}
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: A player about to spawn (first part). Called by CBasePlayer::Spawn() at the very beginning.
// Note   : Called for everyone, can prevent player from spawning.
// Input  : *pPlayer -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::PlayerSpawnStart(CBasePlayer *pPlayer)// XDM3038a: 
{
	DBG_GR_PRINT("CGameRulesMultiplay::PlayerSpawnStart(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (FPlayerStartAsObserver(pPlayer))
		return false;

	SetBits(pPlayer->m_iGameFlags, EGF_JOINEDGAME);// XDM3038a
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Player is spawning
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::PlayerSpawn(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesMultiplay::PlayerSpawn(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	ClearBits(pPlayer->m_iGameFlags, EGF_PRESSEDREADY);// XDM3038a

	if (!pPlayer->IsObserver())
	{
		UpdateGameMode(pPlayer->edict());// XDM3035c: some of game values may change during the game.

		pPlayer->m_Stats[STAT_SPAWN_COUNT]++;// XDM3037

		bool addDefault = false;
		SetBits(pPlayer->pev->weapons, 1<<WEAPON_SUIT);// hack, but better than creating and giving actual item

		if (sv_usemapequip.value >= 1.0f)// 1 = map equip only
		{
			if (sv_usemapequip.value == 2.0f)// 2 = add both, 3 = add default when map says so
				addDefault = true;

			CBaseEntity	*pWeaponEntity = NULL;
			while ((pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip")) != NULL)
			{
				pWeaponEntity->Touch(pPlayer);
				if (FBitSet(pWeaponEntity->pev->spawnflags, SF_PLAYEREQUIP_ADDTODEFAULT) && sv_usemapequip.value >= 3.0f)
					addDefault = true;
			}
		}
		else
			addDefault = true;

		if (addDefault)
		{
			/*size_t nItems = */PlayerAddDefault(pPlayer);
			//if (nItems > 0 && sv_lognotice.value > 0)// XDM3037a: if no WEAPONS meant to be added, don't add the crowbar too
			//	UTIL_LogPrintf("\"%s<%i><%s><%s>\" got \"%d\" default items\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), g_pGameRules->GetTeamName(pPlayer->pev->team), nItems);
		}

		PLAYBACK_EVENT_FULL(0, pPlayer->edict(), g_usPlayerSpawn, 0, pPlayer->pev->origin, pPlayer->pev->angles, 2, 0, pPlayer->pev->team, 0, 0, 0);
		if (mp_firetargets_player.value > 0)
			FireTargets("game_playerspawn", pPlayer, pPlayer, USE_TOGGLE, 0);// XDM: moved here from CBasePlayer::UpdateClientData()
/* too early #if !defined(SERVER_WEAPON_SLOTS)
		// since g_SilentItemPickup suppresses all pickup messages
		pPlayer->SelectNextBestItem(pPlayer->m_pActiveItem);// useless: client iWeaponBits == 0
#endif*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Individual function for each player
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::PlayerThink(CBasePlayer *pPlayer)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::PlayerThink(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
	if (IsGameOver() || m_iGameState == GAME_STATE_WAITING)// XDM3037a: some game rules require this check to start
	{
		// check for button presses
		if (FBitSet(pPlayer->m_afButtonPressed, BUTTONS_READY))
		{
			if (!FBitSet(pPlayer->m_iGameFlags, EGF_PRESSEDREADY))// XDM3036
			{
				SetBits(pPlayer->m_iGameFlags, EGF_PRESSEDREADY);
				MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ALL:MSG_BROADCAST), gmsgGREvent);
					WRITE_BYTE(GAME_EVENT_PLAYER_READY);
					WRITE_SHORT(pPlayer->entindex());
					WRITE_SHORT(1);
				MESSAGE_END();
			}
		}
		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
	/*else if (check_camping)
	{
		something like this? better track the whole path, because player may go away and return back near m_vecLastCheckedOrigin or jump around one spot
		if ((pPlayer->m_fCheckCampingTime <= gpGlobals->time)
		{
			if ((pPlayer->pev->origin - pPlayer->m_vecLastCheckedOrigin).Length() < 100)
			{
				// also, if not stuck?
				MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
					WRITE_BYTE(GAME_EVENT_PLAYER_CAMPING);
					WRITE_SHORT(pPlayer->entindex());
					WRITE_SHORT(0);
				MESSAGE_END();
			}
			pPlayer->m_vecLastCheckedOrigin = pPlayer->pev->origin;
			pPlayer->m_fCheckCampingTime = gpGlobals->time + 5.0f;
		}
	}*/
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		SERVER_PRINT("CGameRulesMultiplay::PlayerThink() EXCEPTION!!!\n");
		DBG_FORCEBREAK
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Controls ObserverStart process
// Input  : *pPlayer - 
// Output : true - allow, false - deny
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::PlayerStartObserver(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesMultiplay::PlayerStartObserver(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));

	CBasePlayer *pClient = NULL;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pClient = UTIL_ClientByIndex(i);
		if (pClient)
		{
			if (pClient == pPlayer)
				continue;

			// If a spectator was chasing this player, move him/her onto the next player
			if (pClient->IsObserver() && pClient->m_hObserverTarget == pPlayer)
				pClient->ObserverFindNextPlayer(false);

			// Don't let this freed index be remembered anywhere!
			if (pClient->m_hLastKiller == pPlayer)
				pClient->m_hLastKiller = NULL;
			if (pClient->m_hLastVictim == pPlayer)
				pClient->m_hLastVictim = NULL;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: Controls observer status change process
// Input  : *pPlayer - 
// Output : true - allow, false - deny
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::PlayerStopObserver(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesMultiplay::PlayerStopObserver(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	return FPlayerCanRespawn(pPlayer);// XDM3038c: players become spectators upon using up their respawn limit
}

//-----------------------------------------------------------------------------
// Purpose: Is the player actually PLAYING the game and NOT finished/spectating
// Note   : XDM3038: this is a gameplay logic rather than technical check
// Input  : *pPlayer -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FPlayerIsActive(CBasePlayer *pPlayer)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::FPlayerIsActive(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (pPlayer)
	{
		if (pPlayer->IsObserver())
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Can this player respawn NOW?
// Input  : *pPlayer -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FPlayerCanRespawn(const CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesMultiplay::FPlayerCanRespawn(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (IsGameOver())
		return false;
	//if (m_iGameState == GAME_STATE_LOADING)
	//	return false;
	if (mp_deathlimit.value > 0.0f)// XDM3038a
	{
		if (pPlayer->m_iDeaths >= (int)mp_deathlimit.value)
			return false;
	}
	float rd = GetPlayerRespawnDelay(pPlayer);
	if (rd > 0.0f)// XDM3035
	{
		if ((gpGlobals->time - pPlayer->m_fDiedTime) < rd)
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Player is spawning, add default items to his inventory
// Input  : *pPlayer - 
// Output : int - number of items added
//-----------------------------------------------------------------------------
size_t CGameRulesMultiplay::PlayerAddDefault(CBasePlayer *pPlayer)//, bool bCountItems)
{
	DBG_GR_PRINT("CGameRulesMultiplay::PlayerAddDefault(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (mp_adddefault.value <= 0)
		return 0;
	else if (mp_adddefault.value == 2.0f && FBitSet(pPlayer->m_iGameFlags, EGF_ADDEDDEFITEMS))
		return 0;

	size_t iAddItemsCount = 0;
	const char *pString = mp_defaultitems.string;// XDM3035: support for multiple items
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (pString && strcmp(pString, "empty") != 0)// FCVAR_PRINTABLEONLY does not allow empty strings and changes them to "empty"
		{
			size_t i = 0;
			size_t iItemStart = 0;
			size_t iItemLength = 0;
			char item_classname[32];//MAX_ENTITY_STRING_LENGTH];
			g_SilentItemPickup = true;// XDM3038: make unaccepted items disappear!
#if defined (USE_EXCEPTIONS)
			try
			{
#endif
			while (/*NO! we need last zero! pString[i] &&*/ i < 512 && iAddItemsCount < MAX_ADD_DEFAULT_ITEMS)
			{
				if (isspace(pString[i]))// skip whitespace
				{
				}
				else if (pString[i] == ';' || pString[i] == '\0')// end of item // cannot use ispunct() because it counts '_'
				{
					if (/*iItemStart > 0 IT CAN BE! && */iItemLength > 0)// XDM3037: "item1;item2;\0" could cause erroneous iteration
					{
						iItemLength = i-iItemStart;
						if (iItemLength > 1)
						{
							strncpy(item_classname, pString+iItemStart, iItemLength);
							item_classname[iItemLength] = 0;
							if (pPlayer->GiveNamedItem(item_classname))
							{
								//if (bCountItems)
									++iAddItemsCount;
							}
						}
						iItemStart = 0;// reset
						iItemLength = 0;
					}
					if (pString[i] == '\0')// || pString[i+1] == '\0')
					{
						//i = 512;
						break;// finished!
					}
				}
				else if (pString[i])
				{
					if (iItemLength == 0)// don't check if iItemStart == 0 because it CAN be 0!
					{
						iItemStart = i;
						iItemLength = 1;
					}
				}
				++i;
			}// while
#if defined (USE_EXCEPTIONS)
			}
			catch (...)
			{
				g_SilentItemPickup = false;
			}
#endif
			g_SilentItemPickup = false;
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		g_SilentItemPickup = false;
		conprintf(1, "ERROR: CGameRulesMultiplay::PlayerAddDefault() exception!\n");
		DBG_FORCEBREAK
	}
#endif
	SetBits(pPlayer->m_iGameFlags, EGF_ADDEDDEFITEMS);
	return iAddItemsCount;
}

//-----------------------------------------------------------------------------
// Purpose: How many points awarded to anyone that kills this entity
// Warning: CONCEPT: relationship SHOULD NOT be checked here! Just amount of points
// Input  : *pAttacker -
//			*pKilled -
// Output : int
//-----------------------------------------------------------------------------
int CGameRulesMultiplay::IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled)
{
	DBG_GR_PRINT("CGameRulesMultiplay::IPointsForKill(%d %s, %d %s)\n", pAttacker?pAttacker->entindex():0, pAttacker?STRING(pAttacker->pev->classname):"", pKilled?pKilled->entindex():0, pKilled?STRING(pKilled->pev->classname):"");
	if (pKilled)
	{
		if (pKilled->IsPlayer())
			return SCORE_AWARD_NORMAL;
		else if (pKilled->IsMonster() && mp_monsterfrags.value > 0.0f)
			return SCORE_AWARD_NORMAL;
	}
	return 0;// no score for anything other than players
}

//-----------------------------------------------------------------------------
// Purpose: Determines relationship between given player and entity
// Warning: DO NOT CALL IRelationship() from here!
// Input  : *pPlayer - subject
//			*pTarget - object
// Output : int GR_NEUTRAL
//-----------------------------------------------------------------------------
int CGameRulesMultiplay::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::PlayerRelationship(%d %s, %d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->classname), pTarget?pTarget->entindex():0, pTarget?STRING(pTarget->pev->classname):"");
	if (pTarget)
	{
		if (pTarget->IsPlayer())
			return GR_ENEMY;

		//if (pTarget->IsBSPModel())// XDM3035: ?
		//	return GR_NEUTRAL;
	}
	return GR_NEUTRAL;
}

//-----------------------------------------------------------------------------
// Purpose: Add score and awards UNDONE: monsters can score??
// Input  : *pWinner - NULL means everyone
//			score -
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::AddScore(CBaseEntity *pWinner, int score)
{
	DBG_GR_PRINT("CGameRulesMultiplay::AddScore(%d %s, %d)\n", pWinner?pWinner->entindex():0, pWinner?STRING(pWinner->pev->netname):"EVERYONE", score);
	if (/*pWinner == NULL || */score == 0)
		return false;
	if (IsGameOver())
		return false;

	int plindex;
	if (pWinner)
	{
		if (pWinner->IsPlayer())
		{
			plindex = pWinner->entindex();
		}
		else// award an NPC
		{
			pWinner->pev->frags += (float)score;
			return true;
		}
	}
	else
		plindex = 0;// invalid index for a client

	CBasePlayer *pPlayer;
	for (CLIENT_INDEX player_index = 1; player_index <= gpGlobals->maxClients; ++player_index)
	{
		if (plindex == 0 || player_index == plindex)//if (pWinner == NULL || pLucky == pWinner)
		{
			//CLIENT_INDEX player_index = pPlayer->entindex();
			pPlayer = UTIL_ClientByIndex(player_index);
			pPlayer->pev->frags += (float)score;
			// XDM3038: most important update: score
			MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
				WRITE_BYTE(player_index);
				WRITE_SHORT((int)pPlayer->pev->frags);// In case someone needs score more than +-32768, change this to LONG
				WRITE_SHORT((int)pPlayer->m_iDeaths);
			MESSAGE_END();

			// COMBOS (killing spree, etc.)
			if (pPlayer->IsAlive())// XDM3038: dead player can get score updates but can't get combos
			{
				pPlayer->m_iScoreCombo += 1;//score; XDM3038c: sometimes we get more than a single point of award! // keep increasing this no matter what
				div_t dcombo = div(pPlayer->m_iScoreCombo, SCORE_AWARD_COMBO);// divide
				if (dcombo.quot > 0 && dcombo.rem == 0 && dcombo.quot <= SCORE_COMBO_MAX)
				{
					MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
						WRITE_BYTE(GAME_EVENT_COMBO);
						WRITE_SHORT(player_index);
						WRITE_SHORT(dcombo.quot);
					MESSAGE_END();
					/* is this important enough?
					MESSAGE_BEGIN(MSG_SPEC, svc_director);
						WRITE_BYTE(9);// command length in bytes
						WRITE_BYTE(DRC_CMD_EVENT);
						WRITE_SHORT(player_index);// index number of primary entity
						WRITE_SHORT(0);// index number of secondary entity
						WRITE_LONG(7 | DRC_FLAG_FACEPLAYER);// eventflags (priority and flags)
					MESSAGE_END();*/
					if (sv_lognotice.value > 0)
						UTIL_LogPrintf("\"%s<%i><%s><%s>\" got combo \"%d\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), g_pGameRules->GetTeamName(pPlayer->pev->team), dcombo.quot);
				}
				pPlayer->m_Stats[STAT_CURRENT_COMBO] = dcombo.quot;// XDM3038a
				if (pPlayer->m_Stats[STAT_BEST_COMBO] < dcombo.quot)// XDM3038a
					pPlayer->m_Stats[STAT_BEST_COMBO] = dcombo.quot;

				// N-kill AWARDS (double, triple, multi, etc.)
				if (pPlayer->m_fNextScoreTime >= gpGlobals->time)
					pPlayer->m_iLastScoreAward++;// increase
				else
					pPlayer->m_iLastScoreAward = 1;// start over

				pPlayer->m_Stats[STAT_CURRENT_SCORE_AWARD] = pPlayer->m_iLastScoreAward;// XDM3037
				if (pPlayer->m_iLastScoreAward > 1 && pPlayer->m_iLastScoreAward <= SCORE_AWARD_MAX)
				{
					MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, gmsgGREvent, NULL, pPlayer->edict());
						WRITE_BYTE(GAME_EVENT_AWARD);
						WRITE_SHORT(player_index);
						WRITE_SHORT(pPlayer->m_iLastScoreAward);
					MESSAGE_END();
				}
				if (pPlayer->m_Stats[STAT_BEST_SCORE_AWARD] < pPlayer->m_iLastScoreAward)// XDM3037
					pPlayer->m_Stats[STAT_BEST_SCORE_AWARD] = pPlayer->m_iLastScoreAward;

				pPlayer->m_fNextScoreTime = gpGlobals->time + SCORE_AWARD_TIME;

				if (m_iFirstScoredPlayer == CLIENT_INDEX_INVALID)// WARNING! This will be reset when client leaves the game! TODO: FIXME
				{
					MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
						WRITE_BYTE(GAME_EVENT_FIRST_SCORE);
						WRITE_SHORT(player_index);
						WRITE_SHORT((int)pPlayer->pev->frags);
					MESSAGE_END();
					m_iFirstScoredPlayer = player_index;
					pPlayer->m_Stats[STAT_FIRST_SCORE_AWARD]++;// XDM3037
				}
			}
			pPlayer->m_Stats[STAT_SCORE] = (int)pPlayer->pev->frags;// XDM3037
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Score limit for this game type
// Output : uint32
//-----------------------------------------------------------------------------
uint32 CGameRulesMultiplay::GetScoreLimit(void)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::GetScoreLimit()\n");
	return (uint32)mp_fraglimit.value;
}

//-----------------------------------------------------------------------------
// Purpose: How much score remaining until end of game
// Output : uint32
//-----------------------------------------------------------------------------
uint32 CGameRulesMultiplay::GetScoreRemaining(void)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::GetScoreRemaining()\n");
	return m_iRemainingScore;
}

//-----------------------------------------------------------------------------
// Purpose: Time limit for this game type
// Output : uint32 seconds!
//-----------------------------------------------------------------------------
uint32 CGameRulesMultiplay::GetTimeLimit(void)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::GetTimeLimit()\n");
	if (mp_timelimit.value < 0.0f)// XDM3038: TODO: revisit, use better conversion?
		mp_timelimit.value = 0.0f;

	return (uint32)(mp_timelimit.value*60.0f);// CVar is in minutes, this should be THE ONLY place where we touch it!
}

//-----------------------------------------------------------------------------
// Purpose: A player got killed, run logic
// Input  : *pVictim - a player that was killed
//			*pKiller - a player, monster or whatever it can be
//			*pInflictor - the actual entity that did the damage (weapon, projectile, etc.)
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	ASSERT(pVictim != NULL);
	if (pVictim == NULL)
		return;
	DBG_GR_PRINT("CGameRulesMultiplay::PlayerKilled(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->netname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");
	//conprintf(1, " +++ %s WAS KILLED +++\n", STRING(pVictim->pev->netname));

	m_pLastVictim = pVictim;// XDM3035
	//pVictim->m_iDeaths++; moved to CBasePlayer
	pVictim->m_Stats[STAT_DEATH_COUNT]++;// XDM3037
	ClearBits(pVictim->m_iGameFlags, EGF_PRESSEDREADY);// XDM3038a

	CBasePlayer *pPlayerKiller = NULL;
	CBaseMonster *pMonsterKiller = NULL;
	CLIENT_INDEX iKiller = 0;
	CLIENT_INDEX iVictim = pVictim->entindex();
	bool allykill = false;

	if (pKiller)
	{
		if (pKiller->IsPlayer())
			pPlayerKiller = (CBasePlayer *)pKiller;
		// both can be true		else
		pMonsterKiller = pKiller->MyMonsterPointer();

		if (pMonsterKiller && pPlayerKiller == NULL)// XDM3037: a true monster
		{
			if (pMonsterKiller->m_hOwner.Get() && pMonsterKiller->m_hOwner->IsPlayer())// works on behalf of a player
			{
				pKiller = pMonsterKiller->m_hOwner;// so we think that monster's owner should get awarded
				pPlayerKiller = (CBasePlayer *)pKiller;
			}
		}

		iKiller = pKiller->entindex();

		// pVictim was killed by the same entity twice!
		if (pVictim->m_hLastKiller == pKiller)// is not NULL already so don't need to check
		{
			//no if (pVictim->m_iLastScoreAward == 0)// pVictim did not kill anyone after respawning
			{
			MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);// let everyone laugh?
				WRITE_BYTE(GAME_EVENT_LOSECOMBO);
				WRITE_SHORT(iKiller);
				WRITE_SHORT(iVictim);
			MESSAGE_END();
			pVictim->m_Stats[STAT_FAIL_COUNT]++;// XDM3037
			if (pPlayerKiller)
				pPlayerKiller->m_Stats[STAT_TROLL_COUNT]++;// XDM3037
			}
			// multiple?	pVictim->m_hLastKiller = NULL;
		}
		else
			pVictim->m_hLastKiller = pKiller;// remember new killer

		if (pMonsterKiller)// must be valid for monsters AND players
			pMonsterKiller->m_hLastVictim = pVictim;

		if (pPlayerKiller)
		{
			if (pVictim->m_iScoreCombo > SCORE_AWARD_COMBO)// at least one combo
			{
				MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
					WRITE_BYTE(GAME_EVENT_COMBO_BREAKER);
					WRITE_SHORT(iKiller);
					WRITE_SHORT(iVictim);
				MESSAGE_END();
				pPlayerKiller->m_Stats[STAT_COMBO_BREAKER_COUNT]++;// XDM3037
			}
		}

		if (pVictim->pev == pKiller->pev)// killed self
		{
			AddScore(pKiller, -IPointsForKill(pKiller, pVictim));// -
			pVictim->m_Stats[STAT_SUICIDE_COUNT]++;// XDM3037

			if (pPlayerKiller)
				pPlayerKiller->m_hLastVictim = NULL;

			if (mp_revengemode.value > 1.0f)// XDM3037: if a player kills himself, he gets erased as anyone's killer
			{
				for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
				{
					CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
					if (pPlayer)
					{
						if (pPlayer != pKiller && pPlayer->m_hLastKiller == pKiller)
						{
							pPlayer->m_hLastKiller = NULL;// reset
							MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ONE:MSG_ONE_UNRELIABLE), gmsgGREvent, NULL, pPlayer->edict());
								WRITE_BYTE(GAME_EVENT_REVENGE_RESET);
								WRITE_SHORT(i);
								WRITE_SHORT(iKiller);
							MESSAGE_END();
							//ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#REVENGE_RESET", STRING(pKiller->pev->netname));
						}
					}
				}
			}
		}
		else if (pPlayerKiller && !pPlayerKiller->IsObserver())// && pKiller->IsPlayer()) // XDM3035: don't award spectators!
		{
			// pKiller's killer is now a victim. Check BEFORE awarding!
			if (pPlayerKiller->m_hLastKiller == pVictim)
			{
				if (pPlayerKiller->m_iLastScoreAward == 0)// this is the FIRST victim after respawn
				{
					MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ALL:MSG_BROADCAST), gmsgGREvent);// let everyone know
						WRITE_BYTE(GAME_EVENT_REVENGE);
						WRITE_SHORT(iKiller);
						WRITE_SHORT(iVictim);
					MESSAGE_END();
					pPlayerKiller->m_Stats[STAT_REVENGE_COUNT]++;// XDM3037
					if (mp_revengemode.value > 0.0f)
						AddScore(pKiller, SCORE_AWARD_NORMAL);// extra score for killing your killer!
				}
				pPlayerKiller->m_hLastKiller = NULL;// only once! (critical for mp_revengemode)
			}
			// if a player dies in a deathmatch game and the killer is a client, award the killer some points
			int relation = g_pGameRules->PlayerRelationship(pPlayerKiller, pVictim);
			if (relation == GR_NOTTEAMMATE || relation == GR_ENEMY)
			{
				AddScore(pKiller, IPointsForKill(pKiller, pVictim));// +
			}
			else if (relation == GR_TEAMMATE || relation == GR_ALLY)
			{
				allykill = true;
				AddScore(pKiller, -IPointsForKill(pKiller, pVictim));// -
				pPlayerKiller->m_Stats[STAT_TEAMKILL_COUNT]++;// XDM3037
			}
			if (mp_firetargets_player.value > 0)
				FireTargets("game_playerkill", pKiller, pKiller, USE_TOGGLE, 0);

			if (pVictim->m_LastHitGroup == HITGROUP_HEAD)// XDM3037: headshot
			{
				pPlayerKiller->m_Stats[STAT_HEADSHOT_COUNT]++;
				MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ONE:MSG_ONE_UNRELIABLE), gmsgGREvent, NULL, pPlayerKiller->edict());
					WRITE_BYTE(GAME_EVENT_HEADSHOT);
					WRITE_SHORT(iKiller);
					WRITE_SHORT(iVictim);
				MESSAGE_END();
			}

			if ((pVictim->Center() - pPlayerKiller->Center()).Length() > DISTANTSHOT_DISTANCE)// XDM3038b
			{
				pPlayerKiller->m_Stats[STAT_DISTANTSHOT_COUNT]++;
				MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ONE:MSG_ONE_UNRELIABLE), gmsgGREvent, NULL, pPlayerKiller->edict());
					WRITE_BYTE(GAME_EVENT_DISTANTSHOT);
					WRITE_SHORT(iKiller);
					WRITE_SHORT(iVictim);
				MESSAGE_END();
			}
		}
		else if (/*pKiller && */pKiller->IsMonster())
		{
			if (g_pGameRules->IsCoOp())// XDM
				AddScore(pKiller, IPointsForKill(pKiller, pVictim));// award the killer monster
		}
		else// killed by the world
		{
			pVictim->m_hLastKiller = NULL;
			//AddScore(pVictim, -1);
		}// not fair when killed by spawn spot
	}
	else
		pVictim->m_hLastKiller = NULL;

	pVictim->m_iLastScoreAward = 0;// XDM3035: reset awards
	pVictim->m_iScoreCombo = 0;// XDM3035: reset combo
	pVictim->m_Stats[STAT_CURRENT_COMBO] = 0;// XDM3037

	// XDM3038a: pVictim->m_iDeaths ALWAYS CHANGES!
	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(iVictim);
		WRITE_SHORT((int)pVictim->pev->frags);// In case someone needs score more than +-32768, change this to LONG
		WRITE_SHORT((int)pVictim->m_iDeaths);
	MESSAGE_END();

	// killers score, if it's a player
	if (pPlayerKiller)
	{
		// XDM3038: message moved to AddScore()
		// let the killer paint another decal as soon as he'd like.
		pPlayerKiller->m_flNextDecalTime = gpGlobals->time;

		if (pKiller != pVictim)
			pPlayerKiller->m_Stats[STAT_PLAYERKILL_COUNT]++;// XDM3038c
	}

	if (pVictim->GetInventoryItem(WEAPON_SATCHEL))
		DeactivateSatchels(pVictim);

	// UNDONE: in some cases this will help regaining trains, in other it will break the gameplay.
	//CBaseDelay *pTrain = pVictim->GetControlledTrain();
	//if (pTrain)
	//	pTrain->Use(pVictim, pVictim, USE_OFF, 0.0f);// STOP the train

	// XDM3037: moved to the end
	DeathNotice(pVictim, pKiller, pInflictor);

	if (mp_firetargets_player.value > 0)
		FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

	CheckLimits();// XDM3037a: moved from AddScore because score is not always being added, but endgame may be triggered by something else

	if (mp_deathlimit.value > 0.0f)// XDM3038a: otherwise player will just lie around dead, which is not fun at all...
	{
		if (pVictim->m_iDeaths >= (int)mp_deathlimit.value)
			pVictim->ObserverStart(pVictim->pev->origin, pVictim->pev->angles, pKiller?OBS_CHASE_FREE:OBS_ROAMING, pKiller);
	}
}

//-----------------------------------------------------------------------------
// Purpose: A monster got killed, run logic
// Input  : *pVictim - a monster that was killed
//			*pKiller - a player, monster or whatever it can be
//			*pInflictor - the actual entity that did the damage (weapon, projectile, etc.)
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
//	DBG_GR_PRINT("CGameRulesMultiplay::MonsterKilled(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->classname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");
	if (/* impossible pKiller != pVictim && */pKiller->IsPlayer())
		((CBasePlayer *)pKiller)->m_Stats[STAT_MONSTERKILL_COUNT]++;// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: Notify players about someone's death
// Input  : *pVictim - can be anything
//			*pKiller - can be NULL
//			*pInflictor - weapon, projectile, door, train...
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::DeathNotice(CBaseEntity *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	ASSERT(pVictim != NULL);
	if (pVictim == NULL)
		return;
	//DBG_GR_PRINT("CGameRulesMultiplay::DeathNotice(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->classname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");
	DBG_GR_PRINT("CGameRulesMultiplay::DeathNotice(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->classname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");
	CBasePlayer *pPlayerKiller = NULL;// only if it IS player
	CBasePlayer *pPlayerVictim;
	int killer_index = 0;// ENTINDEXes
	int victim_index = pVictim->entindex();
#if !defined (OLD_DEATHMSG)
	int weaponindex = WEAPON_NONE;
#endif
	const char *killer_weapon_name = "";// XDM3038a

	if (pVictim->IsPlayer())// get player pointer
		pPlayerVictim = (CBasePlayer *)pVictim;
	else
		pPlayerVictim = NULL;

	if (pKiller)
	{
		killer_index = pKiller->entindex();
		if (pKiller->IsPlayer())// get player pointer
			pPlayerKiller = (CBasePlayer *)pKiller;

		// determine killer_weapon_name
#if defined (OLD_DEATHMSG)
		if (pKiller->IsMonster())// XDM: HACK because we can't send monster classname and it's weapon name at once
			killer_weapon_name = STRING(pKiller->pev->classname);
		else
#endif
		if (pInflictor)
		{
			if (pPlayerKiller && pInflictor == pKiller && pPlayerKiller->m_pActiveItem)// If the inflictor is the killer, then it must be their current weapon doing the damage
			{
#if defined (OLD_DEATHMSG)
				killer_weapon_name = pPlayerKiller->m_pActiveItem->GetWeaponName();
#else
				weaponindex = pPlayerKiller->m_pActiveItem->GetID();
#endif
			}
			else// a projectile, door, train...
			{
				killer_weapon_name = STRING(pInflictor->pev->classname);// it's just that easy
#if !defined (OLD_DEATHMSG)
				if (pInflictor->IsPlayerItem())// must be safe to cast!
					weaponindex = ((CBasePlayerItem *)pInflictor)->GetID();
#endif
			}
		}
		else if (!pPlayerKiller)
			killer_weapon_name = STRING(pKiller->pev->classname);

		// strip the monster_* or weapon_* from the inflictor's classname
		size_t offset = strbegin(killer_weapon_name, "weapon_");
		if (offset == 0)
		{
			offset = strbegin(killer_weapon_name, "func_");
			if (offset == 0 && !pKiller->IsMonster())
				offset = strbegin(killer_weapon_name, "monster_");
		}
		/*if ((offset = strbegin(killer_weapon_name, "weapon_") != 0) ||
			(offset = strbegin(killer_weapon_name, "func_") != 0) ||
			(!pKiller->IsMonster() && (offset = strbegin(killer_weapon_name, "monster_") != 0)))*/
			killer_weapon_name += offset;
	}
	else if (pInflictor)
		killer_weapon_name = STRING(pInflictor->pev->classname);// it's just that easy

	// TODO: merge gmsgGREvent into gmsgDeathMsg?
	MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
		WRITE_SHORT(killer_index);// killer XDM3037a: now 2 bytes!
		WRITE_SHORT(victim_index);// victim
#if !defined (OLD_DEATHMSG)
		WRITE_BYTE(pKiller?EntityIs(pKiller->edict()):0);
		WRITE_BYTE(pVictim?EntityIs(pVictim->edict()):0);
		WRITE_BYTE(PlayerRelationship(pKiller, pVictim));
		WRITE_BYTE(weaponindex);// provide string if 0
#endif
		WRITE_STRING(killer_weapon_name);// what they were killed by (should this be a string?)
	MESSAGE_END();

	if (FAllowSpectators())
	{
		MESSAGE_BEGIN(MSG_SPEC, svc_director);
			WRITE_BYTE(9);// command length in bytes
			WRITE_BYTE(DRC_CMD_EVENT);// player killed
			WRITE_SHORT(victim_index);// index number of primary entity
			if (pInflictor)
				WRITE_SHORT(pInflictor->entindex());// index number of secondary entity
			else
				WRITE_SHORT(killer_index);// index number of secondary entity

			WRITE_LONG(7 | DRC_FLAG_DRAMATIC);// eventflags (priority and flags)
		MESSAGE_END();
	}

	if (sv_lognotice.value > 0)// XDM
	{
		if (pKiller && pVictim == pKiller)// killed self
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()),
				g_pGameRules->GetTeamName(pVictim->pev->team), killer_weapon_name);
		}
		else if (pPlayerKiller)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING(pKiller->pev->netname), GETPLAYERUSERID(pKiller->edict()), GETPLAYERAUTHID(pKiller->edict()),
				g_pGameRules->GetTeamName(pKiller->pev->team),
				STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()),
				g_pGameRules->GetTeamName(pVictim->pev->team),
				killer_weapon_name);
		}
		else if (pKiller && pKiller->IsMonster())
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" was killed by %s [%d]\n",
				STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()),
				g_pGameRules->GetTeamName(pVictim->pev->team),
				STRING(pKiller->pev->classname), killer_index);
		}
		else if (killer_weapon_name)// killed by the world
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" was destoryed by \"%s\" (world)\n",
				STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()),
				g_pGameRules->GetTeamName(pVictim->pev->team), killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" died\n",
				STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()),
				g_pGameRules->GetTeamName(pVictim->pev->team));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Player picked up a weapon
// Input  : *pPlayer -
//			*pWeapon -
//-----------------------------------------------------------------------------
/*void CGameRulesMultiplay::PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
}*/

//-----------------------------------------------------------------------------
// Purpose: After what time weapon may spawn?
// Input  : *pWeapon -
// Output : float - delay in seconds
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetWeaponRespawnDelay(const CBasePlayerItem *pWeapon)
{
	if (mp_weaponstay.value > 0.0f)
	{
		if (!FBitSet(pWeapon->iFlags(), ITEM_FLAG_LIMITINWORLD))
			return 0.0f;// weapon respawns almost instantly
	}
	return mp_weaponrespawntime.value;
}

//-----------------------------------------------------------------------------
// Purpose: Weapon tries to respawn, calculate delay
// Input  : *pWeapon -
// Output : float - the DELAY after which it can try to spawn again (0 == now, -1 == fail)
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::OnWeaponTryRespawn(CBasePlayerItem *pWeapon)
{
	if (pWeapon)// when respawning, Spawn was not called yet! && pWeapon->GetID())
	{
		if (FBitSet(pWeapon->iFlags(), ITEM_FLAG_LIMITINWORLD))
		{
			if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
				return 0.0f;

			// We're past the entity tolerance level, so delay the respawn
			return GetWeaponRespawnDelay(pWeapon);// XDM3038c: FIX
		}
		else
			return 0.0f;
	}
	return -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
// Input  : *pWeapon -
// Output : Vector - absolute coordinates
//-----------------------------------------------------------------------------
Vector CGameRulesMultiplay::GetWeaponRespawnSpot(CBasePlayerItem *pWeapon)
{
	return pWeapon->m_vecSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: World models will be scaled by this factor. Don't use direcly!
//			Call UTIL_GetWeaponWorldScale() instead!
// Output : float - scale
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetWeaponWorldScale(void)
{
	if (sv_weaponsscale.value > 0.0f)// XDM3035b
		return clamp(sv_weaponsscale.value, 1.0f, WEAPON_WORLD_SCALE);
	else
		return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Can this player respawn NOW?
// Input  : *pMonster -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FMonsterCanRespawn(const CBaseMonster *pMonster)
{
	DBG_GR_PRINT("CGameRulesMultiplay::FMonsterCanRespawn(%d %s)\n", pMonster->entindex(), STRING(pMonster->pev->netname));
	if (IsGameOver())
		return false;
	//if (m_iGameState == GAME_STATE_LOADING)
	//	return false;

	/* float rd = GetMonsterRespawnDelay(pMonster);
	if (rd > 0.0f)// XDM3035
	{
		if ((gpGlobals->time - pMonster->m_fDiedTime) < rd)
			return false;
	}*/
	if (mp_monstersrespawn.value > 0.0f)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a After what delay may this player respawn?
// Input  : *pPlayer - can be NULL
// Output : mp_respawntime
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetMonsterRespawnDelay(const CBaseMonster *pMonster)
{
	return mp_monstersrespawntime.value;
}

//-----------------------------------------------------------------------------
// Purpose: Should this weapon respawn according to game rules?
// Input  : *pWeapon -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FWeaponShouldRespawn(const CBasePlayerItem *pWeapon)
{
	if (mp_weaponrespawntime.value <= 0.0f)
		return false;

	return true;// don't do item internal-only checks here!
}

//-----------------------------------------------------------------------------
// Purpose: Can this player have specified item?
// Input  : *pPlayer -
//			*pItem -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem)
{
	if (mp_weaponstay.value > 0)
	{
		if (FBitSet(pItem->iFlags(), ITEM_FLAG_LIMITINWORLD))
			return CGameRules::CanHavePlayerItem(pPlayer, pItem);

		if (pPlayer->GetInventoryItem(pItem->GetID()))// player has weapon with same ID
			return false;
	}
	return CGameRules::CanHavePlayerItem(pPlayer, pItem);
}

//-----------------------------------------------------------------------------
// Purpose: Can this player drop specified item?
// Warning: Assumes the item is in player's inventory!
// Input  : *pPlayer - player instance
//			*pItem - existing item instance
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::CanDropPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem)
{
	if (!CGameRules::CanDropPlayerItem(pPlayer, pItem))
		return false;
	if (FBitSet(pItem->pev->spawnflags, SF_NOREFERENCE) && (mp_dropdefault.value <= 0.0f))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer -
//			*pItem -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::CanHaveItem(CBasePlayer *pPlayer, CBaseEntity *pItem)
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: currently used by weaponboxes
// Input  : *pPlayer -
//			*pEntity -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::CanPickUpOwnedThing(CBasePlayer *pPlayer, CBaseEntity *pEntity)
{
	if (pEntity)
	{
		CBaseEntity *pOwner = pEntity->m_hOwner;
		if (pOwner == NULL || !pOwner->IsPlayer())// don't care if owned by non-player
			return true;

		if (mp_pickuppolicy.value == PICKUP_POLICY_OWNER)
		{
			if (pOwner == pPlayer)
				return true;
		}
		else if (mp_pickuppolicy.value == PICKUP_POLICY_FRIENDS)
		{
			if (pOwner == pPlayer)
				return true;
			int relation = PlayerRelationship(pPlayer, pOwner);
			if (relation == GR_TEAMMATE || relation == GR_ALLY)
				return true;
		}
		else if (mp_pickuppolicy.value == PICKUP_POLICY_ENEMIES)
		{
			if (pOwner == pPlayer)
				return true;
			int relation = PlayerRelationship(pPlayer, pOwner);
			if (relation == GR_NOTTEAMMATE || relation == GR_ENEMY)
				return true;
		}
		else// PICKUP_POLICY_EVERYONE
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer -
//			*pItem -
//-----------------------------------------------------------------------------
/*void CGameRulesMultiplay::PlayerGotItem(CBasePlayer *pPlayer, CBaseEntity *pItem)
{
}*/

//-----------------------------------------------------------------------------
// Purpose: Should this item respawn according to game rules?
// Input  : *pItem -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FItemShouldRespawn(const CBaseEntity *pItem)
{
	if (mp_itemrespawntime.value <= 0.0f)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: After which delay should this Item respawn?
// Input  : *pItem -
// Output : float - delay in seconds
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetItemRespawnDelay(const CBaseEntity *pItem)
{
	return mp_itemrespawntime.value;
}

//-----------------------------------------------------------------------------
// Purpose: Some game variations may choose to randomize spawn locations
// Input  : *pItem -
// Output : Vector
//-----------------------------------------------------------------------------
const Vector &CGameRulesMultiplay::GetItemRespawnSpot(CBaseEntity *pItem)
{
	if (pItem)
		return pItem->m_vecSpawnSpot;

	return g_vecZero;
}

//-----------------------------------------------------------------------------
// Purpose: Player picked up some ammo
// Input  : *pPlayer -
//			*szName -
//			iCount -
//-----------------------------------------------------------------------------
//void CGameRulesMultiplay::PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, const int &iCount)
//{
//}

//-----------------------------------------------------------------------------
// Purpose: Entity restrictions may apply here
// Warning: Optimize for speed! Fastest checks first!
// Input  : *pEntity -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::IsAllowedToSpawn(CBaseEntity *pEntity)
{
	if (pEntity->IsPlayer())
		return true;

	if (pEntity->IsMonster() && !FAllowMonsters())
		return false;

	if (pEntity->IsPlayerWeapon())
	{
		if (mp_noweapons.value > 0.0f)// XDM3037
			return false;

		if (mp_allowpowerfulweapons.value <= 0.0f)
		{
			CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)pEntity;
			if (FBitSet(pWeapon->iFlags(), ITEM_FLAG_SUPERWEAPON))
				return false;
		}
	}

	if (mp_nofriction.value > 0.0f && FClassnameIs(pEntity->pev, "func_friction"))// obsolete?
		return false;

	//if (FClassnameIs(pEntity->pev, "env_fade"))// XDM: thissux!
	//	return false;

	if (mp_bannedentities.string && *mp_bannedentities.string)// XDM3037a: semicolon-separated list of classnames not allowed in this game
	{
		char *pFound = strstr(mp_bannedentities.string, STRING(pEntity->pev->classname));
		if (pFound)
		{
			size_t l = strlen(STRING(pEntity->pev->classname));
			if (*(pFound + l) == ';' || *(pFound + l) == '\0')
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Should this entity be shown on the minimap?
// Input  : *pEntity - 
//			*pPlayer - map owner
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FShowEntityOnMap(CBaseEntity *pEntity, CBasePlayer *pPlayer)
{
	if (pEntity->IsGameGoal())// conflicts with old display mechanism, requires eflag to be sent to client side
		return true;
	if (!pEntity->ShowOnMap(pPlayer))// XDM3038c
		return false;
	if (pEntity->IsProjectile())// || !pEntity->IsAlive())// XDM3038a
		return false;

	// TODO: TESTME if (!pEntity->ShouldBeSentTo(pPlayer))
	//	return false;

	if (pEntity->IsMonster() || pEntity->IsPlayer())
	{
		if (!pEntity->IsAlive())// XDM3038a
			return false;

		int iClass = pEntity->Classify();
		if (iClass == CLASS_NONE || iClass == CLASS_INSECT || iClass == CLASS_GIB || iClass == CLASS_BARNACLE)// XDM3038b
			return false;

		int relation = PlayerRelationship(pPlayer, pEntity);
		if (relation == GR_NOTTEAMMATE || relation == GR_ENEMY)
		{
			if (mp_allowenemiesonmap.value > 0)
			{
				if ((pEntity->pev->origin - pPlayer->pev->origin).Length() > sv_radardist.value)// XDM3038a
					return false;

				return true;
			}
		}
		else// if (relation == GR_TEAMMATE || relation == GR_ALLY || relation == GR_NEUTRAL)
			return true;
	}
	return true;// XDM3038c: allow entities to be shown on the map if requested
}

//-----------------------------------------------------------------------------
// Purpose: Any conditions inhibiting the respawning of this ammo?
// Input  : *pAmmo -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAmmoShouldRespawn(const CBasePlayerAmmo *pAmmo)
{
	if (mp_ammorespawntime.value <= 0.0f)
		return false;

	return true;// don't do item internal checks here!
}

//-----------------------------------------------------------------------------
// Purpose: Ammo respawn time
// Input  : *pAmmo -
// Output : float - delay in seconds
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetAmmoRespawnDelay(const CBasePlayerAmmo *pAmmo)
{
	return mp_ammorespawntime.value;
}

//-----------------------------------------------------------------------------
// Purpose: Wall-mounted charger renew time
// Output : float - delay in seconds
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetChargerRechargeDelay(void)
{
	return mp_itemrespawntime.value;
}

//-----------------------------------------------------------------------------
// Purpose: Which weapons should be packed and dropped
// Input  : *pPlayer -
// Output : int GR_PLR_DROP_GUN_NO
//-----------------------------------------------------------------------------
short CGameRulesMultiplay::DeadPlayerWeapons(CBasePlayer *pPlayer)
{
	int d = (int)mp_weapondrop.value;
	if (d == GR_PLR_DROP_GUN_NO)
		return GR_PLR_DROP_GUN_NO;
	else if (d == GR_PLR_DROP_GUN_ALL)
		return GR_PLR_DROP_GUN_ALL;

	return GR_PLR_DROP_GUN_ACTIVE;
}

//-----------------------------------------------------------------------------
// Purpose: Which ammo should be packed and dropped
// Input  : *pPlayer -
// Output : int GR_PLR_DROP_AMMO_NO
//-----------------------------------------------------------------------------
short CGameRulesMultiplay::DeadPlayerAmmo(CBasePlayer *pPlayer)
{
	int d = (int)mp_ammodrop.value;
	if (d == GR_PLR_DROP_AMMO_NO)
		return GR_PLR_DROP_AMMO_NO;
	else if (d == GR_PLR_DROP_AMMO_ALL)
		return GR_PLR_DROP_AMMO_ALL;

	return GR_PLR_DROP_AMMO_ACTIVE;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: how to use spawn spots
// Output : int
//-----------------------------------------------------------------------------
int CGameRulesMultiplay::SpawnSpotUsePolicy(void)
{
	if (mp_spawnpointforce.value > 0.0)
		return SPAWNSPOT_FORCE_CLEAR;

	return CGameRules::SpawnSpotUsePolicy();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: allow some tweaks
// Output : int
//-----------------------------------------------------------------------------
int CGameRulesMultiplay::GetPlayerMaxHealth(void)
{
	return min((int)sv_playermaxhealth.value, MAX_ABS_PLAYER_HEALTH);// hardcoded maximum
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: allow some tweaks
// Output : int
//-----------------------------------------------------------------------------
int CGameRulesMultiplay::GetPlayerMaxArmor(void)
{
	return min((int)sv_playermaxarmor.value, MAX_ABS_PLAYER_ARMOR);// hardcoded maximum
}

//-----------------------------------------------------------------------------
// Purpose: Should texture hit sounds be played?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::PlayTextureSounds(void)
{
	return (mp_texsnd.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Should footstep sounds be played?
// Input  : *pPlayer -
//			fvol -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::PlayFootstepSounds(CBasePlayer *pPlayer, float fvol)
{
	if (g_pmp_footsteps && g_pmp_footsteps->value <= 0.0f)
		return false;

	if (pPlayer->IsOnLadder() || pPlayer->pev->velocity.Length2D() > 220.0f)
		return true;  // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Allow players to use flashlight?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAllowFlashlight(void)
{
	return (mp_flashlight.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Are spectators allowed?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAllowSpectators(void)
{
		return (mp_allowspectators.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Allow monsters in game?
// Output : returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAllowMonsters(void)
{
	return (mp_allowmonsters.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Are effects allowed on this server?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAllowEffects(void)
{
	return (mp_effects.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Is dynamic music (events, not playlist!) allowed on this server?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAllowMapMusic(void)
{
	if (IsGameOver())
		return false;
	else
		return (mp_allowmusicevents.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Should server always precache all possible ammo types?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FPrecacheAllAmmo(void)
{
	return (mp_precacheammo.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Should server always precache all possible items?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FPrecacheAllItems(void)
{
	return (mp_precacheitems.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Should server always precache all possible weapons?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FPrecacheAllWeapons(void)
{
	return (mp_precacheweapons.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Server policy
// Output : game_combat_mode_e
//-----------------------------------------------------------------------------
short CGameRulesMultiplay::GetCombatMode(void)
{
	if (IsGameOver())
		return GAME_COMBATMODE_NODAMAGE;
	else
		return min(GAME_COMBATMODE_NOSHOOTING, (int)mp_noshooting.value);
}

//-----------------------------------------------------------------------------
// Purpose: Are players allowed to switch to spectator mode in game? (cheat!)
// Input  : *pPlayer - can be NULL
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAllowSpectatorChange(CBasePlayer *pPlayer)
{
	if (pPlayer && FBitSet(pPlayer->pev->flags, FL_PROXY) && pPlayer->IsObserver())
		return false;// XDM3038a: don't allow HLTV proxy to become a player

	else if (!IsGameOver() && FAllowSpectators())
		return (mp_spectoggle.value > 0.0f);

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038 Are players allowed to shoot while climbing?
// Input  : *pPlayer - can be NULL
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::FAllowShootingOnLadder(CBasePlayer *pPlayer)
{
	return (mp_laddershooting.value > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a After what delay may this player respawn?
// Input  : *pPlayer - can be NULL
// Output : mp_respawntime
//-----------------------------------------------------------------------------
float CGameRulesMultiplay::GetPlayerRespawnDelay(const CBasePlayer *pPlayer)
{
	return mp_respawntime.value;
}

//-----------------------------------------------------------------------------
// Purpose: EndOfGame. Show score board, camera shows the winner
// Input  : *pWinner -
//			*pInFrameEntity - some secondary entity to show in focus
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::IntermissionStart(CBasePlayer *pWinner, CBaseEntity *pInFrameEntity)
{
	DBG_GR_PRINT("CGameRulesMultiplay::IntermissionStart(%s, %s)\n", pWinner?STRING(pWinner->pev->netname):"NULL", pInFrameEntity?STRING(pInFrameEntity->pev->targetname):"NULL");

	if (IsGameOver())
		return;  // intermission has already been triggered, so ignore.

	conprintf(2, "CGameRulesMultiplay::IntermissionStart(%s, %s): %s\n", pWinner?STRING(pWinner->pev->netname):"NULL", pInFrameEntity?STRING(pInFrameEntity->pev->targetname):"NULL", STRING(gpGlobals->mapname));
	m_pIntermissionEntity1 = pWinner;
	m_pIntermissionEntity2 = pInFrameEntity;
	// bounds check
	int inttime = (int)mp_chattime.value;// XDM
	if (inttime < 1)
		CVAR_DIRECT_SET(&mp_chattime, "1");
	else if (inttime > MAX_INTERMISSION_TIME)
		CVAR_DIRECT_SET(&mp_chattime, UTIL_dtos1(MAX_INTERMISSION_TIME));

	inttime = (int)mp_chattime.value;
	m_flIntermissionEndTime = gpGlobals->time + inttime;
	m_flIntermissionStartTime = gpGlobals->time;

	//g_fGameOver = true;
	m_bGameOver = true;
	m_bReadyButtonsHit = false;
	m_bSendStats = true;

	MESSAGE_BEGIN(MSG_ALL, svc_intermission);
	MESSAGE_END();

	CLIENT_INDEX winner_index;
	if (pWinner)
	{
		winner_index = pWinner->entindex();
		SetBits(pWinner->pev->flags, FL_DRAW_ALWAYS);// XDM3035a: send even if outside of PVS
		pWinner->pev->effects = EF_BRIGHTLIGHT;// XDM3035c: forget all other effects
		//pWinner->pev->scale = 1.0f;
		pWinner->pev->rendermode = kRenderNormal;// XDM3038c: stop all fading effects
		pWinner->pev->renderamt = 255;
		pWinner->pev->rendercolor.Set(255,255,255);
		pWinner->pev->renderfx = kRenderFxNone;
		m_iLeader = winner_index;// XDM3035a: update this!
		// TODO:? find all clients outside winner's PVS and mark them with EF_NODRAW FNullEnt(FIND_CLIENT_IN_PVS(edict()))
	}
	else
		winner_index = 0;

	TEAM_ID winner_team = pWinner?pWinner->pev->team:GetBestTeam();

	// spectator mode to move camera
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			if (pPlayer->IsOnTrain())
			{
				CBaseDelay *pTrain = pPlayer->GetControlledTrain();
				if (pTrain)
					pTrain->Use(pPlayer, pPlayer, USE_OFF, 0.0f);// STOP the train itself
			}
			UTIL_SetOrigin(pPlayer, pPlayer->pev->origin);
			pPlayer->pev->framerate = 0;// stop!
			pPlayer->pev->velocity.Clear();
			pPlayer->pev->speed = 0;
			//pPlayer->pev->gaitsequence = 0;// XDM3037: no. this will clear current pose
			pPlayer->pev->iuser1 = OBS_INTERMISSION;
			if (pWinner)
			{
				pPlayer->m_hObserverTarget = pWinner;// XDM3035c: use his PVS
				pPlayer->pev->iuser2 = winner_index;
				pPlayer->UpdateStatusBar();// XDM3037: since it won't get called the usual way
			}
			else
			{
				pPlayer->m_hObserverTarget = pPlayer;// XDM3037: watch over self
				pPlayer->pev->iuser2 = i;
			}

			if (pInFrameEntity)
				pPlayer->pev->iuser3 = pInFrameEntity->entindex();
			else
				pPlayer->pev->iuser3 = 0;

			ClearBits(pPlayer->m_iGameFlags, EGF_PRESSEDREADY);// XDM3038a: we could send GAME_EVENT_PLAYER_READY 0, but it's better to just reset data for everyone on client side in IntermissionStart()

			/*if (pPlayer != pWinner && !pPlayer->IsBot())// winner stats will be sent to everyone later
			{
			MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ONE:MSG_ONE_UNRELIABLE), gmsgPlayerStats, NULL, pPlayer->edict());
				WRITE_BYTE(i);
				WRITE_BYTE(STAT_NUMELEMENTS);
				for (short c = 0; c < STAT_NUMELEMENTS; ++c)
					WRITE_LONG(pPlayer->m_Stats[c]);

			MESSAGE_END();
			}*/
		}
	}
	GameSetState(GAME_STATE_FINISHED);// XDM3038b: moved here so botmatch DLL wil catch up

	/*if (pWinner)// winner stats to everyone
	{
		MESSAGE_BEGIN(((sv_reliability.value > 1)?MSG_ALL:MSG_BROADCAST), gmsgPlayerStats);
			WRITE_BYTE(winner_index);
			WRITE_BYTE(STAT_NUMELEMENTS);
			for (short c = 0; c < STAT_NUMELEMENTS; ++c)
				WRITE_LONG(pWinner->m_Stats[c]);

		MESSAGE_END();
	}*/

	// XDM3035 TESTME?
	if (FAllowSpectators())
	{
	MESSAGE_BEGIN(MSG_SPEC, svc_director);
		WRITE_BYTE(9);// command length in bytes
		WRITE_BYTE(DRC_CMD_EVENT);
		WRITE_SHORT(winner_index);// index number of primary entity
		WRITE_SHORT(pInFrameEntity?pInFrameEntity->entindex():0);// index number of secondary entity
		WRITE_LONG(15 | DRC_FLAG_FINAL);// eventflags (priority and flags)
	MESSAGE_END();
	}
	// XDM3038a: after intermission info
	MESSAGE_BEGIN(MSG_ALL, gmsgGRInfo);
		WRITE_BYTE(winner_index);
		WRITE_BYTE(winner_team);
		WRITE_SHORT(0);
		WRITE_LONG(inttime);
	MESSAGE_END();

	if (pWinner)
	{
		UTIL_LogPrintf("ENDGAME %s at %s Winner: \"%s<%i><%s><%s>\", team %hd\n",
			g_pGameRules->GetGameDescription(), STRING(gpGlobals->mapname),
			STRING(pWinner->pev->netname), GETPLAYERUSERID(pWinner->edict()), GETPLAYERAUTHID(pWinner->edict()),
			g_pGameRules->GetTeamName(pWinner->pev->team), pWinner->pev->team);
	}
	else
		UTIL_LogPrintf("ENDGAME %s at %s\n", g_pGameRules->GetGameDescription(), STRING(gpGlobals->mapname));

	ClientPrint(NULL, HUD_PRINTTALK, "* #MP_WIN_TEXT", pWinner?(g_pGameRules->IsTeamplay()?GetTeamName(winner_team):STRING(pWinner->pev->netname)):"#NOBODY");

	FireTargets("game_roundend", g_pWorld, g_pWorld, USE_TOGGLE, 0);

	// TODO?	UpdateGameMode(NULL);
	UTIL_ScreenFadeAll(Vector(255,255,255), 1.0f, 0.25f, 191, FFADE_IN);// XDM3035: cool effect and also disables all other fade effects for everyone

#if defined (_DEBUG)
	DumpInfo();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Instant end of the game with intermission
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::EndMultiplayerGame(void)
{
	DBG_GR_PRINT("CGameRulesMultiplay::EndMultiplayerGame()\n");
	if (IsGameOver())
		return;

	TEAM_ID bestteam = g_pGameRules->GetBestTeam();
	IntermissionStart(g_pGameRules->GetBestPlayer(bestteam), NULL/*m_pLastVictim may contain random person*/);
}

//-----------------------------------------------------------------------------
// Purpose: Server is changing to a new level, check mapcycle.txt for map name and setup info
//  UPDATE: XDM3034: search for the current map in list and continue from there!
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::ChangeLevel(void)
{
	DBG_GR_PRINT("CGameRulesMultiplay::ChangeLevel()\n");
	char szMapCycleFile[MAX_PATH];
	char szNextMap[MAX_MAPNAME];
	char szFirstMapInList[MAX_MAPNAME];
	char szCommands[MAX_RULE_BUFFER];
	char szRules[MAX_RULE_BUFFER];
	const char *mapcfile = CVAR_GET_STRING("mapcyclefile");

	if (mp_gamerules.value > 0)// XDM: otherwise, auto-select game type
	{
		gametype_prefix_t *pPrefix = FindGamePrefix(g_pGameRules->GetGameType());
		if (pPrefix)
		{
			_snprintf(szFirstMapInList, MAX_MAPNAME, "%s_000\0", pPrefix->prefix);
			szFirstMapInList[MAX_MAPNAME-1] = '\0';
			//_snprintf(szMapCycleFile, MAX_PATH, "mapcycle.%s\0", pPrefix->prefix);
			//_strlwr(szMapCycleFile);
			size_t l = strlen(mapcfile);
			if (l > 0)
			{
				strncpy(szMapCycleFile, mapcfile, l);// "mapcycle.txt"
				for (l-=1; l > 0; --l)// search starting from the end
				{
					if (szMapCycleFile[l] == '.')
					{
						++l;// step back to keep the dot
						szMapCycleFile[l] = '\0';
						break;
					}
				}
				// l is at writing position by now
				size_t j = 0;// all this stuff is written to copy "prefix" in lower case
				do
				{
					szMapCycleFile[l] = tolower(pPrefix->prefix[j]);
					if (pPrefix->prefix[j] == '\0')// include terminator, break after copying
						break;

					++j;
					++l;
				}
				while (l < MAX_PATH);
				szMapCycleFile[MAX_PATH-1] = '\0';// safety
				mapcfile = szMapCycleFile;
			}
		}
		else
		{
			strcpy(szFirstMapInList, "DM_000");// the absolute default level
			szFirstMapInList[MAX_MAPNAME-1] = '\0';
			mapcfile = NULL;
		}
	}
	//if (mapcfile == NULL)
	//	mapcfile = (char *)CVAR_GET_STRING("mapcyclefile");

	ASSERT(mapcfile != NULL);
	if (mapcfile == NULL)
		return;

	SERVER_PRINTF("SV: Using map cycle file \"%s\"\n", mapcfile);

	szCommands[0] = '\0';
	szRules[0] = '\0';

	//uint32 minplayers = 0, maxplayers = 0;
	uint32 curplayers = CountPlayers();
	bool do_cycle = true;
	// Has the map cycle filename changed?
	if (_stricmp(mapcfile, g_szPreviousMapCycleFile) || g_MapCycle.items == NULL)
	{
		strncpy(g_szPreviousMapCycleFile, mapcfile, 256);// XDM: buffer overrun protection
		DestroyMapCycle(&g_MapCycle);
		if (ReloadMapCycleFile(mapcfile, &g_MapCycle) <= 0 || g_MapCycle.items == NULL)
		{
			SERVER_PRINTF("Map cycle: warning! Unable to load map cycle file \"%s\"!\n", mapcfile);
			do_cycle = false;
		}
		// NOTE: we don't need to specially search for current map in freshly loaded list because it is already done by ReloadMapCycleFile()
	}
	else// XDM3035: we did not change the list file but played an out-of-order (but may be still in list) map, so try to find it and continue playing
	{
		mapcycle_item_t *found = Mapcycle_Find(&g_MapCycle, STRING(gpGlobals->mapname));
		if (found)
		{
			g_MapCycle.next_item = found->next;
			SERVER_PRINTF("Map cycle: \"%s\" found in list, next map is \"%s\".\n", found->mapname, found->next?found->next->mapname:"(null)");
		}
		//else
		//	g_MapCycle.next_item = g_MapCycle.items;// restart the cycle?
	}

	if (do_cycle && g_MapCycle.items)
	{
		mapcycle_item_s *item = NULL;
		//bool keeplooking = false;
		//bool found = false;
		// Assume current map
		if (g_MapCycle.items)
		{
			strncpy(szFirstMapInList, g_MapCycle.items->mapname, MAX_MAPNAME);
			szFirstMapInList[MAX_MAPNAME-1] = '\0';
		}
		else
			memset(szFirstMapInList, 0, sizeof(szFirstMapInList));

		memset(szNextMap, 0, sizeof(szNextMap));
		// Traverse list
		for (item = g_MapCycle.next_item; item != NULL && item->next != g_MapCycle.next_item; item = item->next)
		{
			//keeplooking = false;
			ASSERT(item != NULL);
			if (MapIsBanned(item->mapname))
			{
				SERVER_PRINTF("Map cycle: \"%s\" is banned, skipping.\n", item->mapname);
				continue;
			}
			if (item->minplayers > 0 && curplayers < item->minplayers)
			{
				SERVER_PRINTF("Map cycle: \"%s\" requires at least %u players, skipping.\n", item->mapname, item->minplayers);
				continue;//keeplooking = true;
			}
			if (item->maxplayers > 0 && curplayers > item->maxplayers)
			{
				SERVER_PRINTF("Map cycle: \"%s\" allows up to %u players, skipping.\n", item->mapname, item->maxplayers);
				continue;//keeplooking = true;
			}
			break;
		}
		//if (!found)
		//	item = g_MapCycle.next_item;

		// Increment next item pointer
		if (item)
		{
			g_MapCycle.next_item = item->next;
			// Perform logic on current item
			strncpy(szNextMap, item->mapname, MAX_MAPNAME);
			ExtractCommandString(item->rulebuffer, szCommands);
			strncpy(szRules, item->rulebuffer, MAX_RULE_BUFFER);
		}
		else
			conprintf(1, "CGameRulesMultiplay::ChangeLevel() item == NULL!\n");
	}

	if (!IS_MAP_VALID(szNextMap))// someone may have deleted the map while the server is runnning
	{
		SERVER_PRINTF("Map cycle: \"%s\" is invalid, returning to \"%s\"\n", szNextMap, szFirstMapInList);
		strncpy(szNextMap, szFirstMapInList, MAX_MAPNAME);
		szNextMap[MAX_MAPNAME-1] = '\0';
	}

	//g_fGameOver = true;
	m_bGameOver = true;
	m_pIntermissionEntity1 = NULL;
	m_pIntermissionEntity2 = NULL;
	m_flIntermissionEndTime = 0.0f;
	strcpy(m_szLastMap, STRING(gpGlobals->mapname));// XDM3038a: now it works for all game rules
	SERVER_PRINTF("CHANGE LEVEL: %s\n", szNextMap);//conprintf(1, "CHANGE LEVEL: %s\n", szNextMap);
	GameSetState(GAME_STATE_LOADING);// XDM3037a
	//if (minplayers > 0 || maxplayers > 0)
	//	conprintf(1, "PLAYER COUNT: min %i max %i current %i, RULES: %s\n", minplayers, maxplayers, curplayers, szRules);

	// XDM3035 crash recovery
	//g_pServerAutoFile ?

	CHANGE_LEVEL(szNextMap, NULL);
	if (strlen(szCommands) > 0)
		SERVER_COMMAND(szCommands);
}

#define MAX_MOTD_CHUNK		MAX_USER_MSG_DATA-2// chars per packet
//#define MAX_MOTD_LENGTH   1280// (MAX_MOTD_CHUNK * 4) XDM3035: reduced a little

//-----------------------------------------------------------------------------
// Purpose: Send MOTD to client in pieces. XDM3038a: does not show it!
// Input  : *pClient -
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::SendMOTDToClient(CBasePlayer *pClient)
{
	DBG_GR_PRINT("CGameRulesMultiplay::SendMOTDToClient(%d %s)\n", pClient->entindex(), STRING(pClient->pev->netname));

	// read from the MOTD.txt file
	int length = 0;
	char *pFileName = (char *)CVAR_GET_STRING("motdfile");
	char *pFileList;// TODO: replace this shit with normal fread()
	char *aFileList = pFileList = (char *)LOAD_FILE_FOR_ME(pFileName, &length);
//	ASSERT(pFileList != NULL);
	if (pFileList == NULL)
	{
		SERVER_PRINTF("Error loading MOTD file \"%s\"!\n", pFileName);
		return;
	}
	size_t char_count = 0;
	char chunk[MAX_MOTD_CHUNK+1];
	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts
	while (pFileList && *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}
		char_count += strlen(chunk);

		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, pClient->edict());
			WRITE_BYTE(*pFileList ? FALSE : TRUE);// FALSE means there is still more message to come
			WRITE_STRING(chunk);
		MESSAGE_END();
	}
	FREE_FILE(aFileList);
}

//-----------------------------------------------------------------------------
// Purpose: Is this server full?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesMultiplay::ServerIsFull(void)
{
	DBG_GR_PRINT("CGameRulesMultiplay::ServerIsFull()\n");
	if (CountPlayers() >= (uint32)gpGlobals->maxClients)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Dump debug info to console
//-----------------------------------------------------------------------------
void CGameRulesMultiplay::DumpInfo(void)
{
	CGameRules::DumpInfo();// must be first
	conprintf(1, "Players:\nidx\tteam\tfrg\tdth\tname\tcombo\n");
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)// XDM
		{
			conprintf(1, "%d\t%d\t%d\t%u\t(%s)\t%d\n", i,
				pPlayer->pev->team,
				(int)pPlayer->pev->frags,
				pPlayer->m_iDeaths,
				STRING(pPlayer->pev->netname),
				pPlayer->m_iScoreCombo);
		}
	}
	CBasePlayer *pBestPlayer = g_pGameRules->GetBestPlayer(g_pGameRules->GetBestTeam());
	if (pBestPlayer)
		conprintf(1, " Best Player: %d (%s)\n", pBestPlayer->entindex(), STRING(pBestPlayer->pev->netname));
	else
		conprintf(1, " Best Player: none\n");
}
