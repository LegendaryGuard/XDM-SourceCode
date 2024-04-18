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
#include "teamplay_gamerules.h"
#include "maprules.h"
#include "game.h"
#include "color.h"// XDM3037a
#include "colors.h"// XDM
#include "globals.h"
#include "voice_gamemgr.h"


extern CVoiceGameMgr g_VoiceGameMgr;

//-----------------------------------------------------------------------------
// Purpose: Gets team color from CVars or predefined array. Slow! Do NOT use frequently!
// Warning: May be called when g_pGameRules or g_pWorld == NULL!
// Input  : team - TEAM_ID
//			&r &g &b -
//-----------------------------------------------------------------------------
void GetTeamColor(TEAM_ID team, byte &r, byte &g, byte &b)
{
	if (team <= TEAM_NONE || team > MAX_TEAMS)
	{
#if defined (_DEBUG)
		if (team > TEAM_NONE)
			conprintf(1, "GetTeamColor() bad team ID %hd!\n", team);
#endif
		team = 0;
	}

	if (IsMultiplayer())// XDM3037: IsTeamplay())
	{
		cvar_t *pCvar = NULL;
		if (team == TEAM_1)
			pCvar = &mp_teamcolor1;
		else if (team == TEAM_2)
			pCvar = &mp_teamcolor2;
		else if (team == TEAM_3)
			pCvar = &mp_teamcolor3;
		else if (team == TEAM_4)
			pCvar = &mp_teamcolor4;

		if (pCvar)
		{
			if (StringToRGB(pCvar->string, r,g,b))
				return;// success
		}
	}

	// fall back to predefined values
#if defined (_DEBUG)
	if (team > TEAM_NONE)
		conprintf(1, "GetTeamColor() is using predefined value!\n");
#endif
	r = g_iTeamColors[team][0];
	g = g_iTeamColors[team][1];
	b = g_iTeamColors[team][2];
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameRulesTeamplay::CGameRulesTeamplay() : CGameRulesMultiplay()
{
	m_DisableDeathMessages = false;
	m_DisableDeathPenalty = false;

	// clean up
	memset(m_Teams, 0, sizeof(m_Teams));
	m_iNumTeams = 0;
	//m_bTeamLimit = false;
	m_LeadingTeam = TEAM_NONE;

	for (TEAM_ID ti = 0; ti <MAX_TEAMS+1; ++ti)// XDM3035
		m_hBaseEntities[ti] = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: First thing called after constructor. Initialize all data, cvars, etc.
// Warning: Called from CWorld::Precache(), so all other entities are not spawned yet!
// Note   : We need this because g_pGameRules cannot be used from constructor.
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::Initialize(void)
{
	DBG_GR_PRINT("CGameRulesTeamplay::Initialize()\n");

	CGameRulesMultiplay::Initialize();// load server configs!!

	if (m_iPersistBetweenMaps == 0)// brand new GameRules
	{
		memset(m_Teams, 0, sizeof(m_Teams));// XDM3035 ?
		m_iNumTeams = 0;// ?
		ASSERT(CreateNewTeam(" ") == TEAM_NONE);// 0th team is TEAM_NONE, used by spectators and unconnected players

		char szTeamList[MAX_TEAMS*MAX_TEAMNAME_LENGTH];
		memset(szTeamList, 0, MAX_TEAMS*MAX_TEAMNAME_LENGTH);
		strncpy(szTeamList, mp_teamlist.string, MAX_TEAMS*MAX_TEAMNAME_LENGTH-1);
		szTeamList[MAX_TEAMS*MAX_TEAMNAME_LENGTH-1]=0;

		// don't use just this::MaxTeams() because it always returns value for CGameRulesTeamplay!
		short iMaxTeams;
		if (g_pWorld && g_pWorld->m_iMaxMapTeams > 0)// was defined
			iMaxTeams = clamp(g_pWorld->m_iMaxMapTeams, 2, g_pGameRules->MaxTeams());// XDM3037: map can limit number of teams too
		else if (mp_maxteams.value > 0)// XDM3038a: server limits number of teams
			iMaxTeams = clamp(mp_maxteams.value, 2, g_pGameRules->MaxTeams());
		else// default to what game type allows
			iMaxTeams = g_pGameRules->MaxTeams();

		mp_maxteams.value = iMaxTeams;// XDM3038a: maybe reset to zero?

		char *pName = szTeamList;// parse string provided by user; names are separated with semicolon.
		pName = strtok(pName, ";");// WARNING! this destroys the original string!
		while (pName != NULL && *pName && m_iNumTeams <= iMaxTeams)// + 1
		{
			if (GetTeamIndex(pName) == TEAM_NONE)// team does not exist
				CreateNewTeam(pName);

			pName = strtok(NULL, ";");
		}
	}
	else //if (FBitSet(m_iPersistBetweenMaps, GR_PERSIST))// teams were kept from a previous map/round, choose what data should be cleared
	{
		for (TEAM_ID ti = 0; ti < m_iNumTeams; ++ti)
		{
			if (!FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_EXTRASCORE))
				m_Teams[ti].extrascore = 0;
			if (!FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_SCORE))
				m_Teams[ti].score = 0;
			if (!FBitSet(m_iPersistBetweenMaps, GR_PERSIST_KEEP_LOSES))
				m_Teams[ti].loses = 0;
		}
	}

	if (m_iNumTeams > 1)// 0th team already created
	{
		//m_bTeamLimit = true;
		if (mp_defaultteam.string[0] != 0 && !IsValidTeam(mp_defaultteam.string))// check defaultteam to be valid // XDM3038: only if there's something
		{
			SERVER_PRINTF("%s invalid ('%s'), resetting.\n", mp_defaultteam.name, mp_defaultteam.string);
			CVAR_DIRECT_SET(&mp_defaultteam, "");
		}
		UTIL_LogPrintf("Teamplay initialized, %hd teams. Default team: %s. Max teams: map %d, rules: %hd\n", GetNumberOfTeams(), mp_defaultteam.string, g_pWorld?g_pWorld->m_iMaxMapTeams:0, g_pGameRules->MaxTeams());
	}

	RecountTeams(false);
}

//-----------------------------------------------------------------------------
// Purpose: Team base, e.g. flag return point or something
// Input  : team - ID
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CGameRulesTeamplay::GetTeamBaseEntity(TEAM_ID teamIndex)
{
	DBG_GR_PRINT("CGameRulesTeamplay::GetTeamBaseEntity(%hd)\n", teamIndex);
	if (IsValidTeam(teamIndex))
	{
		if (m_hBaseEntities[teamIndex].IsValid())
			return m_hBaseEntities[teamIndex];
		else
			SERVER_PRINTF("CGameRulesTeamplay::GetTeamBaseEntity(%hd) error: base entity was invalidated!\n", teamIndex);
	}
	return NULL;// g_pWorld?
}

//-----------------------------------------------------------------------------
// Purpose: Set team base, e.g. flag return point or something
// Input  : team - ID
//			*pEntity -
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::SetTeamBaseEntity(TEAM_ID teamIndex, CBaseEntity *pEntity)
{
	DBG_GR_PRINT("CGameRulesTeamplay::SetTeamBaseEntity(%hd %s)\n", teamIndex, STRING(pEntity->pev->classname));
	if (IsValidTeam(teamIndex) && UTIL_IsValidEntity(pEntity))
		m_hBaseEntities[teamIndex] = pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize HUD (client data) for a client
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::InitHUD(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::InitHUD(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	// now we can use messages to send client info
	if (!pPlayer->IsBot())// Send down team names BEFORE gmsgTeamInfo
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict());
			WRITE_BYTE(m_iNumTeams);// WARNING! +1 for 0th team!
		for (TEAM_ID ti = 0; ti < m_iNumTeams; ++ti)
		{
			WRITE_BYTE(m_Teams[ti].color[0]);
			WRITE_BYTE(m_Teams[ti].color[1]);
			WRITE_BYTE(m_Teams[ti].color[2]);
			// Calculated on client side via (RGBtoHSV/360)*255		WRITE_BYTE(m_Teams[ti].colormap & 0xFF);// 1 byte
			WRITE_STRING(m_Teams[ti].name);
		}
		MESSAGE_END();
	}

	CLIENT_INDEX clientIndex = pPlayer->entindex();
	// loop through all active players and send their team info to the new client
	for (CLIENT_INDEX pi = 1; pi <= gpGlobals->maxClients; ++pi)
	{
		CBasePlayer *plr = UTIL_ClientByIndex(pi);
		if (plr)
		{
			if (pi != clientIndex)// don't send MY info to me
			{
				// Send everyone's team info to this player
				MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict());// dest: self
					WRITE_BYTE(plr->entindex());
					WRITE_BYTE(plr->pev->team);
				MESSAGE_END();
			}
			// sent MY team info to ALL players, even myself!
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, plr->edict());// dest: all
				WRITE_BYTE(clientIndex);
				WRITE_BYTE(pPlayer->pev->team);
			MESSAGE_END();
		}
	}
	// Important!
	CGameRulesMultiplay::InitHUD(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Game rules may require the player to choose team or something
// Input  : *pPlayer - 
// Output : int - menu index, 0 = none
//-----------------------------------------------------------------------------
int CGameRulesTeamplay::ShouldShowStartMenu(CBasePlayer *pPlayer)// XDM3038a
{
	if ((pPlayer->pev->team == TEAM_NONE) && (mp_teammenu.value > 0) && (m_iNumTeams > 0))
		return MENU_TEAM;

	return CGameRulesMultiplay::ShouldShowStartMenu(pPlayer);// may be MOTD or MENU_CLOSEALL;
}

//-----------------------------------------------------------------------------
// Purpose: Is this spawn spot valid for specified player?
// Input  : *pPlayer - 
//			pSpot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::ValidateSpawnPoint(CBaseEntity *pPlayer, CBaseEntity *pSpot)// XDM3038a
{
	DBG_GR_PRINT("CGameRulesTeamplay::ValidateSpawnPoint(%d %s, %d %s)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"", pSpot?pSpot->entindex():0, pSpot?STRING(pSpot->pev->classname):"");
	if (!pSpot || !pPlayer)
		return false;

	if (pPlayer->IsPlayer())
	{
		if (((CBasePlayer*)pPlayer)->IsObserver())
			return true;// XDM3038a: always valid for spectators
	}
	if (pSpot->pev->team != TEAM_NONE && (pSpot->pev->team != pPlayer->pev->team))
	{
		//conprintf(2, "ValidateSpawnPoint(%d %d %d) failed: wrong team %d (req %d)!\n", pPlayer->entindex(), pSpot->entindex(), clear, pSpot->pev->team, pPlayer->pev->team);
		return false;
	}
	return CGameRulesMultiplay::ValidateSpawnPoint(pPlayer, pSpot);
}

//-----------------------------------------------------------------------------
// Purpose: A player about to spawn (first part).
// Note   : Called for everyone, can prevent player from spawning.
// Input  : *pPlayer -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::PlayerSpawnStart(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::PlayerBeginSpawn(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (CGameRulesMultiplay::PlayerSpawnStart(pPlayer))
	{
		if (pPlayer->pev->team <= TEAM_NONE)// make sure to have set it in CGameRulesMultiplay::ClientConnected()
		{
			AssignPlayer(pPlayer/*, false*/);// assign player just on server
		}
		if ((uint32)(gpGlobals->time - m_fStartTime) >= m_Teams[pPlayer->pev->team].spawndelay)// XDM3038a
		{
			// in case someone implements special game rules like humans vs robots:	ApplyPlayerSpecialProperties(pPlayer);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a
// Input  : *pPlayer - 
// Output : true - allow, false - deny
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::PlayerStopObserver(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::PlayerStopObserver(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (pPlayer->pev->team == TEAM_NONE)// player wants to join the game without selecting a team
	{
		//conprintf(1, "Spectator %d wanted to join the game\n", entindex());
		CLIENT_COMMAND(pPlayer->edict(), "chooseteam\n");
		return false;
	}
	return CGameRulesMultiplay::PlayerStopObserver(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: Should player become observer instead of spawning?
// Input  : *pPlayer - 
// Output : bool
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::FPlayerStartAsObserver(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::FPlayerStartAsObserver(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	if (pPlayer->m_iSpawnState == SPAWNSTATE_CONNECTED)// first time
	{
		if ((m_iNumTeams > 0) && (ShouldShowStartMenu(pPlayer) == MENU_TEAM))// important to check for exactly TEAM menu
			return true;
	}
	return CGameRulesMultiplay::FPlayerStartAsObserver(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Assign player to ANY available team. TEAM JOIN LOGIC IS HERE!
// WARNING! neither pev->team, nor "team" kv gets saved during level change.
// Input  : *pPlayer - a newbie
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::AssignPlayer(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::AssignPlayer(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	const char *plrteam = GetPlayerTeamName(pPlayer);
	TEAM_ID newteam = GetTeamIndex(plrteam);
	if (newteam > TEAM_NONE)// valid team
	{
		if (!PlayerIsInTeam(pPlayer, newteam))// XDM3037: FIX. already there?
		{
			if (mp_teambalance.value > 0.0f)// XDM3037a: ignore player's preference
				newteam = TeamWithFewestPlayers();

			AddPlayerToTeam(pPlayer, newteam);// * joined team %s
		}
		//else return true;
	}
	else
	{
		newteam = CreateNewTeam(plrteam);// try to create a new team
		if (newteam <= TEAM_NONE)// failed
		{
			// TODO: anyone uses this? if (FBitSet(g_pWorld->pev->spawnflags, SF_WORLD_FORCETEAM))// force join first team
			if (mp_teambalance.value <= 0.0f)// server doesn't balance teams, just stick everyone into default team
			{
				if (strlen(mp_defaultteam.string) > 0)
					newteam = GetTeamIndex(mp_defaultteam.string);
			}
			if (newteam <= TEAM_NONE)// defaultteam may fail too!
				newteam = TeamWithFewestPlayers();

			DBG_GR_PRINT(" * AssignPlayer: unable to create team %s, for player %s. Using team: %hd instead\n", plrteam, STRING(pPlayer->pev->netname), newteam);
		}
		else
			UTIL_LogPrintf("Created team \"%s\" for player \"%s<%i><%s>\"\n", plrteam, STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));

		AddPlayerToTeam(pPlayer, newteam);//* assigned to team %s
	}
	//conprintf(1, " * AssignPlayer %s result: team %d\n", STRING(pPlayer->pev->netname), pPlayer->pev->team);
	//done in AddPlayerToTeam() RecountTeams(false);// don't send yet!
}

//-----------------------------------------------------------------------------
// Purpose: Change player's team (by name)
// Input  : *pPlayer -
//			*pTeamName - desired team name, TeamWithFewestPlayers() if invalid
//			bKill - if necessary
//			bGib -
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::ChangePlayerTeam(CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib)
{
	DBG_GR_PRINT("CGameRulesTeamplay::ChangePlayerTeam(%d %s, %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), pTeamName);
	if (pPlayer == NULL)
		return;

	TEAM_ID teamindex = GetTeamIndex(pTeamName);
	if (teamindex == TEAM_NONE)// same as in !IsValidTeam()
	{
		ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#TEAM_NOT_FOUND", pTeamName);
		return;//teamindex = TeamWithFewestPlayers();
	}
	ChangePlayerTeam(pPlayer, teamindex, bKill, bGib);
}

//-----------------------------------------------------------------------------
// Purpose: Change player's team: move data, assign colors, parameters, etc.
// Input  : *pPlayer -
//			teamIndex - desired team ID, TeamWithFewestPlayers() if invalid
//			bKill - if necessary
//			bGib -
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::ChangePlayerTeam(CBasePlayer *pPlayer, TEAM_ID teamindex, bool bKill, bool bGib)
{
	DBG_GR_PRINT("CGameRulesTeamplay::ChangePlayerTeam(%d %s, %hd)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), teamindex);
	if (pPlayer == NULL)
		return;

	if ((mp_teamchangemax.value > 0.0) && (pPlayer->m_iNumTeamChanges >= mp_teamchangemax.value))// XDM3037
		return;

	if (teamindex != TEAM_NONE && teamindex == pPlayer->pev->team)// XDM3037a: TESTME
		return;

	if (teamindex == TEAM_NONE || !IsValidTeam(teamindex))// XDM3035: TEAM_NONE is for spectators, but is technically a usual assignable team
		teamindex = TeamWithFewestPlayers();

	//no! this fails when all teams have equal players!	if (teamindex == TEAM_NONE)
	//	return;

	CLIENT_INDEX clientIndex = pPlayer->entindex();
	TEAM_ID previousTeam = pPlayer->pev->team;

	RemovePlayerFromTeam(pPlayer, pPlayer->pev->team);

	if (bKill)// kill the player, remove a death, and let them start on the new team
	{
		if (!pPlayer->IsObserver() && pPlayer->IsAlive())// XDM3034
		{
			m_DisableDeathMessages = true;
			m_DisableDeathPenalty = true;
			pPlayer->Killed(g_pWorld, g_pWorld, bGib?GIB_ALWAYS:GIB_NEVER);// XDM3034
			m_DisableDeathMessages = false;
			m_DisableDeathPenalty = false;
		}
	}
	else// PlayerKilled() is not called and satchels are not deactivated!
	{
		// XDM3035: team mines and satchels
		if (mp_teammines.value > 0)// if mines "belong to players only", keep them
			DeactivateSatchels(pPlayer);// satchels
	}

	if (mp_teammines.value > 0)// XDM3035
		DeactivateMines(pPlayer);// mines

	AddPlayerToTeam(pPlayer, teamindex);

	// Notify everyone's HUD of the team change
	/* AddPlayerToTeam -> RecountTeams(true);// XDM3038

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
		WRITE_BYTE(clientIndex);
		WRITE_BYTE(teamindex);
	MESSAGE_END();*/
	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(clientIndex);
		WRITE_SHORT((int)pPlayer->pev->frags);
		WRITE_SHORT(pPlayer->m_iDeaths);
	MESSAGE_END();

	if (pPlayer->IsObserver())
		pPlayer->ObserverStop();// WARNING: this calls Spawn() and InitHUD()!

	//if (sv_lognotice.value > 0)
		UTIL_LogPrintf("\"%s<%i><%s>\" joined team \"%s\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), GetTeamByID(teamindex)->name);

	if (pPlayer->m_iSpawnState >= SPAWNSTATE_SPAWNED && previousTeam != TEAM_NONE)// XDM3038a
		pPlayer->m_iNumTeamChanges++;

	// force close team menu
	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pPlayer->edict());
		WRITE_BYTE(MENU_TEAM);
		WRITE_BYTE(MENUFLAG_CLOSE);
	MESSAGE_END();

	RecountTeams(false);// XDM3037a: safety?
}

//-----------------------------------------------------------------------------
// Purpose: Client has been disconnected
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::ClientDisconnected(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::ClientDisconnected(%d %s)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"");
	if (pPlayer)
		RemovePlayerFromTeam(pPlayer, pPlayer->pev->team);
		// XDM3038: clear client information
		/* -> RemovePlayerFromTeam MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
		WRITE_BYTE(pPlayer->entindex());
		WRITE_BYTE(TEAM_NONE);
	MESSAGE_END();*/
	CGameRulesMultiplay::ClientDisconnected(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Check if the game has reached one of its limits and go to intermission
// Output : Returns true if game has ended
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::CheckLimits(void)
{
	DBG_GR_PRINT("CGameRulesTeamplay::CheckLimits()\n");
	uint32 iscorelimit = g_pGameRules->GetScoreLimit();
	if (iscorelimit > 0)//XDM3037a
	{
		int score_remaining = 0;
		TEAM_ID bestteam = g_pGameRules->GetBestTeam();// use with g_pGameRules-> only!
		team_t *pTeam = GetTeamByID(bestteam);
		if (pTeam)
		{
			int teamscore = g_pGameRules->GetTeamScore(bestteam);// can be negative
			if (/*(iscorelimit > 0 && */teamscore >= iscorelimit)// NOTANERROR: score can be negative, limit can't!
			{
				SERVER_PRINTF("GAME: ended by team score limit (team %hd \"%s\" wins with score %d)\n", bestteam, pTeam->name, teamscore);
				IntermissionStart(g_pGameRules->GetBestPlayer(bestteam), g_pGameRules->GetTeamBaseEntity(bestteam));
				return true;
			}

			if (m_LeadingTeam != bestteam && teamscore > 0)
			{
				m_LeadingTeam = bestteam;
				if (m_LeadingTeam != TEAM_NONE)
				{
				MESSAGE_BEGIN(MSG_BROADCAST, gmsgGREvent);
					WRITE_BYTE(GAME_EVENT_TAKES_LEAD);
					WRITE_SHORT(0);// don't send players in teamplay
					WRITE_SHORT(m_LeadingTeam);
				MESSAGE_END();
				}
			}

			CBasePlayer *pBestPlayer = g_pGameRules->GetBestPlayer(bestteam);
			if (pBestPlayer)
				m_iLeader = pBestPlayer->entindex();

			score_remaining = iscorelimit - teamscore;// TODO: type, sanity check!
		}
		else
		{
			DBG_GR_PRINT("DEBUG: score_remaining(%d) = iscorelimit(%u)\n", score_remaining, iscorelimit);
			score_remaining = iscorelimit;
		}
		m_iRemainingScore = score_remaining;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check if it is ok to end the game by force (time limit)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::CheckEndConditions(void)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::CheckEndConditions()\n");
	if (g_pGameRules->GetBestTeam())// CTF/DOM should redirect to GetBestTeamByScore() inside
		return true;// everything is ended according to rules

	return false;//	return CGameRulesMultiplay::CheckEndConditions();// we'd better use overtime (if allowed)
}

//-----------------------------------------------------------------------------
// Purpose: Disallow players to change vital user info (e.g. model color)
// Warning: When player joins game from spectator mode, he's still IsObserver() here!
// Input  : *pPlayer -
//			*infobuffer -
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::ClientUserInfoChanged(%d %s, %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), infobuffer);
	CGameRulesMultiplay::ClientUserInfoChanged(pPlayer, infobuffer);// we may want this even after GameOver

	if (IsGameOver())
		return;

	CLIENT_INDEX playerindex = pPlayer->entindex();
	if (!pPlayer->IsObserver())// XDM3037a: IMPORTANT: prevent auto-joining, but keep colormap updates!
	{
		// Warning: client infobuffer is lowercase!
		char *plrteam = GET_INFO_KEY_VALUE(infobuffer, "team");
		if (IsValidTeam(plrteam))// player entered valid team name
		{
			if (GetTeamIndex(plrteam) != pPlayer->pev->team)// player entered a NEW team name
			{
				//if (sv_lognotice.value > 0.0f)
					UTIL_LogPrintf("\"%s<%i><%s>\" attempts to change team to \"%s\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), plrteam);

				if (mp_teamchange.value > 0.0f)
					ChangePlayerTeam(pPlayer, plrteam, (mp_teamchangekill.value > 0.0f), (mp_teamchangekill.value > 1.0f));// recursion danger!
			}
		}
		else// just fix it
		{
			char *validteam = (char *)GetTeamName(pPlayer->pev->team);
			//if (sv_lognotice.value > 0.0f)
				UTIL_LogPrintf("\"%s<%i><%s>\" has invalid team \"%s\", resetting to \"%s\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), plrteam, validteam);

			SET_CLIENT_KEY_VALUE(playerindex, infobuffer, "team", validteam);
		}
	}// !IsObserver

	// Validate player colors
	char value[4];
	uint16 topcolor = m_Teams[pPlayer->pev->team].colormap & 0x00FF;// ColorMapExtract(m_Teams[pPlayer->pev->team].colormap)?
	uint16 bottomcolor = (m_Teams[pPlayer->pev->team].colormap & 0xFF00) >> CHAR_BIT;
	pPlayer->pev->colormap = (int)(m_Teams[pPlayer->pev->team].colormap) & 0x0000FFFF;// XDM3038: warning! 2 bytes to 4!

	// XDM3037: no traffic impact, comparison is redundant
	_snprintf(value, 4, "%hu", topcolor);
	SET_CLIENT_KEY_VALUE(playerindex, infobuffer, "topcolor", value);
	_snprintf(value, 4, "%hu", bottomcolor);
	SET_CLIENT_KEY_VALUE(playerindex, infobuffer, "bottomcolor", value);
}

//-----------------------------------------------------------------------------
// Purpose: A player got killed, run logic
// Input  : *pVictim - a player that was killed
//			*pKiller - a player, monster or whatever it can be
//			*pInflictor - the actual entity that did the damage (weapon, projectile, etc.)
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	DBG_GR_PRINT("CGameRulesTeamplay::PlayerKilled(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->netname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");
	if (!m_DisableDeathPenalty)
	{
		CGameRulesMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
		RecountTeams(false);// called by AddScore(), but AddScore() itself is not ALWAYS called by PlayerKilled()
	}
}

//-----------------------------------------------------------------------------
// Purpose: A monster got killed, add score to killer's team
// Input  : *pVictim - a monster that was killed
//			*pKiller - a player, monster or whatever it can be
//			*pInflictor - the actual entity that did the damage (weapon, projectile, etc.)
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor)
{
	DBG_GR_PRINT("CGameRulesTeamplay::MonsterKilled(%d %s, %d %s, %d %s)\n", pVictim->entindex(), STRING(pVictim->pev->classname), pKiller?pKiller->entindex():0, pKiller?STRING(pKiller->pev->netname):"", pInflictor?pInflictor->entindex():0, pInflictor?STRING(pInflictor->pev->classname):"");
	if (mp_monsterfrags.value > 0.0f)
	{
		if (pKiller && pKiller->IsPlayer())
			AddScoreToTeam(pKiller->pev->team, IPointsForKill(pKiller, pVictim));// TODO: different score for killing different monsters?
	}
	CGameRulesMultiplay::MonsterKilled(pVictim, pKiller, pInflictor);
}

//-----------------------------------------------------------------------------
// Purpose: Can this player take damage from this attacker?
// Input  : *pPlayer -
//			*pAttacker -
// Output : Returns true if can
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::FPlayerCanTakeDamage(%d %s, %d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), pAttacker?pAttacker->entindex():0, pAttacker?STRING(pAttacker->pev->classname):"");
	if (pAttacker && PlayerRelationship(pPlayer, pAttacker) == GR_TEAMMATE)
	{
		if (pAttacker != pPlayer && mp_friendlyfire.value <= 0.0f)// allow hurting self
			return false;// my teammate hit me.
	}
	return CGameRulesMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}

//-----------------------------------------------------------------------------
// Purpose: Determines relationship between given player and entity
// Warning: DO NOT call ->IRelationship from here!!!
// Input  : *pPlayer -
//			*pTarget -
// Output : int GR_NEUTRAL
//-----------------------------------------------------------------------------
int CGameRulesTeamplay::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::PlayerRelationship(%d %s, %d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->classname), pTarget?pTarget->entindex():0, pTarget?STRING(pTarget->pev->classname):"");
	if (!pPlayer || !pTarget)// || !pTarget->IsPlayer())// TODO: team monsters?
		return GR_NEUTRAL;

	if (pPlayer->pev->team == TEAM_NONE)// player is not in any team, probably a spectator?
		return GR_NEUTRAL;

	if (!IsRealTeam(pTarget->pev->team))// target entity does not belong to any real team
		return GR_NOTTEAMMATE;

	if (pPlayer->pev->team == pTarget->pev->team)// actual teams are valid and equal
		return GR_TEAMMATE;

	return CGameRulesMultiplay::PlayerRelationship(pPlayer, pTarget);
}

//-----------------------------------------------------------------------------
// Purpose: Add score and awards
// Input  : *pWinner -
//			score -
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::AddScore(CBaseEntity *pWinner, int score)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::AddScore(%d %s, %d)\n", pWinner->entindex(), STRING(pWinner->pev->netname), score);
	bool bRet = CGameRulesMultiplay::AddScore(pWinner, score);// CheckLimits is NOT there
	if (bRet)
	{
		RecountTeams(false);
		//no, it's in PlayerKilled	CheckLimits();
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: Allow autoaim, unless target is a teammate
// Input  : *pPlayer -
//			*pTarget -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::FShouldAutoAim(CBasePlayer *pPlayer, CBaseEntity *pTarget)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::FShouldAutoAim(%d %s, %d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->classname), pTarget?pTarget->entindex():0, pTarget?STRING(pTarget->pev->netname):"");
	// we don't check for friendly fire here, because nobody would try to aim at a teammate anyway
	if (pTarget && pTarget->IsPlayer())
	{
		if (PlayerRelationship(pPlayer, pTarget) == GR_TEAMMATE)
			return false;// don't autoaim at teammates
	}

	return CGameRulesMultiplay::FShouldAutoAim(pPlayer, pTarget);
}

//-----------------------------------------------------------------------------
// Purpose: how many points to add after each kill
// Warning: CONCEPT: relationship SHOULD NOT be checked here! Just amount of points
// Input  : *pAttacker -
//			*pKilled -
// Output : int
//-----------------------------------------------------------------------------
/*int CGameRulesTeamplay::IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled)
{
	if (!pKilled)
		return 0;

	if (!pAttacker)
		return CGameRulesMultiplay::IPointsForKill(pAttacker, pKilled);

	if (pAttacker != pKilled && PlayerRelationship(pAttacker, pKilled) == GR_TEAMMATE)
		return -1;

	return SCORE_AWARD_NORMAL;
}*/

//-----------------------------------------------------------------------------
// Purpose: Find team with least players
// Output : TEAM_ID team index
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesTeamplay::TeamWithFewestPlayers(void)
{
	DBG_GR_PRINT("CGameRulesTeamplay::TeamWithFewestPlayers()\n");
	short minPlayers = gpGlobals->maxClients;// INT_MAX or something
	TEAM_ID idx = TEAM_NONE;
	for (TEAM_ID ti = 1; ti < m_iNumTeams; ++ti)// start from 1
	{
		//conprintf(1, " TFP: passed team %d (%d players)\n", i, m_Teams[i].playercount);
		if (m_Teams[ti].playercount < minPlayers)
		{
			minPlayers = m_Teams[ti].playercount;
			idx = ti;
		}
	}
	//conprintf(1, " TFP: returning team %d (%d players)\n", idx, m_Teams[idx].playercount);
	return idx;// should never return TEAM_NONE
}

//-----------------------------------------------------------------------------
// Purpose: Loop through all teams, recounting everything
// Input  : bResendInfo - Someone's info changed, send the team info to everyone
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::RecountTeams(bool bResendInfo)
{
	DBG_GR_PRINT("CGameRulesTeamplay::RecountTeams()\n");
/*#if defined (_DEBUG)
	int m_LastFrags[MAX_TEAMS+1];
#endif*/
	TEAM_ID ti;
	// Clear dynamic data which is acquired elsewhere (e.g. from pev->frags)
	for (ti = 0; ti < m_iNumTeams; ++ti)
	{
/*#if defined (_DEBUG)
		m_LastFrags[ti] = m_Teams[ti].score;
#endif*/
		m_Teams[ti].score = 0;// XDM3035: TODO TESTME REVISIT
		m_Teams[ti].loses = 0;
		//NO!	m_Teams[ti].extrascore = 0;
		m_Teams[ti].playercount = 0;
	}
	// reassign all clients
	for (CLIENT_INDEX pi = 1; pi <= gpGlobals->maxClients; ++pi)
	{
		CBasePlayer *pPlayer = UTIL_ClientByIndex(pi);
		if (pPlayer)
		{
			if (pPlayer->IsObserver())// XDM3038a: since we now keep pev->team for spectators, accumulate data into virtual team
				ti = TEAM_NONE;
			else
				ti = pPlayer->pev->team;

			m_Teams[ti].score += (int)pPlayer->pev->frags;
			m_Teams[ti].loses += pPlayer->m_iDeaths;
			m_Teams[ti].playercount++;
			//pPlayer->pev->colormap = m_Teams[ti].colormap;// XDM3034: TMP: players may change their look using console!
		}
	}
/*#if defined (_DEBUG)
	for (ti = 0; ti < m_iNumTeams; ++ti)
	{
		if (m_LastFrags[ti] != m_Teams[ti].score)
			conprintf(2, "RecountTeams(): updated frags for team %h from %d to %d\n", ti, m_LastFrags[ti], m_Teams[ti].score);
	}
#endif*/
}

//-----------------------------------------------------------------------------
// Purpose: Find team by name
// Warning: IMPORTANT: Must be case-insensitive! Stupid HL engine breaks everything.
// Input  : *pTeamName - ANY case should be valid!
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesTeamplay::GetTeamIndex(const char *pTeamName)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::GetTeamIndex(%s)\n", pTeamName);
	if (pTeamName && *pTeamName != 0 && strlen(pTeamName) > 0)
	{
		for (TEAM_ID tm = 0; tm < MAX_TEAMS+1; ++tm)
		{
			if (!_stricmp(m_Teams[tm].name, pTeamName))// use case-insensitive comparison
				return tm;
		}
	}
	return TEAM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Get team name by index
// Input  : teamIndex -
// Output : const char
//-----------------------------------------------------------------------------
const char *CGameRulesTeamplay::GetTeamName(TEAM_ID teamIndex)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::GetTeamName(%d)\n", teamIndex);
	if (IsRealTeam(teamIndex))// or IsValidTeam?
		return m_Teams[teamIndex].name;

	return "";
}

//-----------------------------------------------------------------------------
// Purpose: Retrieve pointer to a structure, NULL if the index is invalid
// Input  : team - ID
// Output : team_t
//-----------------------------------------------------------------------------
team_t *CGameRulesTeamplay::GetTeamByID(TEAM_ID teamIndex)
{
	DBG_GR_PRINT("CGameRulesTeamplay::GetTeamByID(%d)\n", teamIndex);
	if (IsValidTeam(teamIndex))
		return &m_Teams[teamIndex];

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Validate team name
// Input  : *pTeamName -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::IsValidTeam(const char *pTeamName)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::IsValidTeam(%s)\n", pTeamName);
	//if (!m_bTeamLimit)// Any team is valid if the teamlist isn't set
	//	return true;

	return (GetTeamIndex(pTeamName) != TEAM_NONE) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: validate team index, accepts TEAM_NONE
// Input  : team -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::IsValidTeam(TEAM_ID teamIndex)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::IsValidTeam(%d)\n", teamIndex);
	if (teamIndex < TEAM_NONE || teamIndex >= m_iNumTeams)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Validate team index, ignores TEAM_NONE and all unused teams.
// Input  : team -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::IsRealTeam(TEAM_ID teamIndex)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::IsRealTeam(%d)\n", teamIndex);
	if (teamIndex < TEAM_1 || teamIndex >= m_iNumTeams)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Special team score - flags/points/gold/etc., NOT FRAGS!
// Input  : teamIndex -
//			score -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::AddScoreToTeam(TEAM_ID teamIndex, int score)
{
	DBG_GR_PRINT("CGameRulesTeamplay::AddScoreToTeam(%d, %d)\n", teamIndex, score);
	if (!IsValidTeam(teamIndex))
		return false;
	if (IsGameOver())
		return false;

	m_Teams[teamIndex].extrascore += score;

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamScore, NULL);
		WRITE_BYTE(teamIndex);
		WRITE_SHORT(m_Teams[teamIndex].extrascore);
	MESSAGE_END();

	CheckLimits();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Number of players in a team
// Input  : teamIndex
// Output : int - number
//-----------------------------------------------------------------------------
short CGameRulesTeamplay::NumPlayersInTeam(TEAM_ID teamIndex)
{
	DBG_GR_PRINT("CGameRulesTeamplay::NumPlayersInTeam(%d)\n", teamIndex);
	if (IsValidTeam(teamIndex))
		return m_Teams[teamIndex].playercount;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get team score that counts as a winning basis
// Input  : teamIndex
// Output : int - score MUST BE IN THE SAME UNITS AS GetScoreLimit()!!!
//-----------------------------------------------------------------------------
int CGameRulesTeamplay::GetTeamScore(TEAM_ID teamIndex)
{
	DBG_GR_PRINT("CGameRulesTeamplay::GetTeamScore(%d)\n", teamIndex);
	if (IsValidTeam(teamIndex))
	{
		if (g_pGameRules->FUseExtraScore())
			return m_Teams[teamIndex].extrascore;
		else
			return m_Teams[teamIndex].score;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Overridden for teamplay
// Input  : teamIndex - may be TEAM_NONE
// Output : CBasePlayer *
//-----------------------------------------------------------------------------
CBasePlayer *CGameRulesTeamplay::GetBestPlayer(TEAM_ID teamIndex)
{
	DBG_GR_PRINT("CGameRulesTeamplay::GetBestPlayer(%d)\n", teamIndex);
	if (teamIndex == TEAM_NONE)
		return NULL;// Don't substitute GetBestTeam() here, just fail!

	return CGameRulesMultiplay::GetBestPlayer(teamIndex);
}

//-----------------------------------------------------------------------------
// Purpose: Get team with the best score (FRAG VERSION)
// Note   : GetTeamScore() is not used here on purpose.
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesTeamplay::GetBestTeam(void)
{
	DBG_GR_PRINT("CGameRulesTeamplay::GetBestTeam()\n");
	TEAM_ID bestteam = TEAM_NONE;
	int score;// can be negative!
	int bestscore = 0;// Right now, we get TEAM_NONE if all teams have 0 // -65535;
	uint32 bestloses = UINT_MAX;// XDM3038
	short fewestplayercount = gpGlobals->maxClients;//MAX_CLIENTS;
	short numteams_samescore = 0;// XDM3037a
	for (TEAM_ID ti = TEAM_1/* check this */; ti < m_iNumTeams; ++ti)
	{
		score = m_Teams[ti].score;// nope! GetTeamScore(i)

		if (score > bestscore)
		{
			bestscore = score;
			bestloses = m_Teams[ti].loses;
			fewestplayercount = m_Teams[ti].playercount;
			bestteam = ti;
			numteams_samescore = 1;// reset counter
		}
		else if (score == bestscore)
		{
			if (m_Teams[ti].loses < bestloses)
			{
				//bestscore = score;
				bestloses = m_Teams[ti].loses;
				fewestplayercount = m_Teams[ti].playercount;
				bestteam = ti;
				numteams_samescore = 1;// reset counter
			}
			else if (m_Teams[ti].loses == bestloses)
			{
				if (m_Teams[ti].playercount < fewestplayercount)
				{
					//bestscore = score;
					//bestloses = m_Teams[i].loses;
					fewestplayercount = m_Teams[ti].playercount;
					bestteam = ti;
					numteams_samescore = 1;// reset counter
				}
				else if (m_Teams[ti].playercount == fewestplayercount)
					++numteams_samescore;
			}
		}
	}
	if (bestscore <= 0 || numteams_samescore > 1)// not here || mp_overtime.value > 0)// XDM3037a: draw game
		bestteam = TEAM_NONE;

	//conprintf(1, "CGameRulesTeamplay::GetBestTeam() %d\n", bestteam);
	return bestteam;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: get best team by extrascore only (not frags or deaths)
// Update : XDM3037a: does not accept situation where two teams have same score
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesTeamplay::GetBestTeamByScore(void)
{
	DBG_GR_PRINT("CGameRulesTeamplay::GetBestTeamByScore()\n");
	int bestscore = 0;// can be negative
	int ts = 0;
	TEAM_ID bestteam = TEAM_NONE;
	short numteams_samescore = 0;// XDM3037a
	for (TEAM_ID ti = TEAM_1/* check this */; ti < m_iNumTeams; ++ti)
	{
		ts = m_Teams[ti].extrascore;
		if (ts > bestscore)
		{
			bestscore = ts;
			bestteam = ti;
			numteams_samescore = 1;// reset counter
		}
		else if (ts == bestscore)
			++numteams_samescore;
	}

	if (bestscore <= 0 || numteams_samescore > 1)// not here || mp_overtime.value > 0)// XDM3037a: draw game
		bestteam = TEAM_NONE;

	return bestteam;
}

//-----------------------------------------------------------------------------
// Purpose: Score limit for this game type
// Note   : Any game rules only allow winning by ONE KIND OF SCORE (or time)!
// Output : uint32 - score MUST BE IN THE SAME UNITS AS GetTeamScore()!!!
//-----------------------------------------------------------------------------
uint32 CGameRulesTeamplay::GetScoreLimit(void)
{
//	DBG_GR_PRINT("CGameRulesTeamplay::GetScoreLimit()\n");
	if (g_pGameRules->FUseExtraScore())
	{
		if (mp_scorelimit.value < 0.0f)// XDM3038: TODO: revisit, use better conversion
			mp_scorelimit.value = 0.0f;
		else
			mp_scorelimit.value = (int)mp_scorelimit.value;

		return (uint32)mp_scorelimit.value;// try to use EXTRA SCORE limit
	}
	return CGameRulesMultiplay::GetScoreLimit();// XDM3037a: frags
}

//-----------------------------------------------------------------------------
// Purpose: If a player joined server with a NEW UNIQUE team name specified,
//          server tries to create this new team if possible.
// Input  : *pTeamName -
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesTeamplay::CreateNewTeam(const char *pTeamName)
{
	DBG_GR_PRINT("CGameRulesTeamplay::CreateNewTeam(%s)\n", pTeamName);
	ASSERT(g_pGameRules != NULL);
	ASSERT(g_pGameRules == this);
	TEAM_ID tm = TEAM_NONE;

	if ((m_iNumTeams < /*g_pGameRules->*/MaxTeams()+1)/* 0th == unassigned/spectators */ /*&& !m_bTeamLimit*/ && pTeamName && (*pTeamName != 0))
	{
		tm = m_iNumTeams;
		++m_iNumTeams;
		m_Teams[tm].playercount = 0;
		GetTeamColor(tm, m_Teams[tm].color[0], m_Teams[tm].color[1], m_Teams[tm].color[2]);
		m_Teams[tm].color.a = 255;
		m_Teams[tm].colormap = RGB2colormap(m_Teams[tm].color[0], m_Teams[tm].color[1], m_Teams[tm].color[2]);// XDM3037a
		if (g_pWorld)
			m_Teams[tm].spawndelay = g_pWorld->m_iTeamDelay[tm];// XDM3038a
		else
			m_Teams[tm].spawndelay = 0.0f;

		strncpy(m_Teams[tm].name, pTeamName, MAX_TEAMNAME_LENGTH);
		UTIL_LogPrintf("Created new team: \"%s\" (color: %d %d %d, colormap: %04X)\n", m_Teams[tm].name, m_Teams[tm].color[0], m_Teams[tm].color[1], m_Teams[tm].color[2], m_Teams[tm].colormap);
	}
	return tm;
}

//-----------------------------------------------------------------------------
// Purpose: PlayerIsInTeam
// Input  : *pPlayer -
//			teamIndex -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::PlayerIsInTeam(CBasePlayer *pPlayer, TEAM_ID teamIndex)
{
	DBG_GR_PRINT("CGameRulesTeamplay::PlayerIsInTeam(%d %s, %d)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), teamIndex);
	if (teamIndex > TEAM_NONE && pPlayer->pev->team == teamIndex && !pPlayer->IsObserver())// XDM3038a: keep team for observers
		return true;// TODO: recheck if we should ALLOW TEAM_NONE here!

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate player's and team score, assign proper color map values
// Input  : *pPlayer -
//			teamIndex -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::AddPlayerToTeam(CBasePlayer *pPlayer, TEAM_ID teamIndex)
{
	if (pPlayer == NULL)
		return false;
	DBG_GR_PRINT("CGameRulesTeamplay::AddPlayerToTeam(%d %s, %d)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), teamIndex);
	if (teamIndex <= TEAM_NONE)
		return false;

	if (PlayerIsInTeam(pPlayer, teamIndex))// already in that team
		return false;

//	conprintf(1, " ADDed %s to team %d (%s)\n", STRING(pPlayer->pev->netname), teamIndex, m_Teams[teamIndex].name);
	m_Teams[teamIndex].score += (int)pPlayer->pev->frags;
	m_Teams[teamIndex].playercount ++;

//	if (!pPlayer->IsObserver() || (pPlayer->m_iSpawnState <= SPAWNSTATE_CONNECTED))
		pPlayer->pev->team = teamIndex;
//	else
//BADBAD		pPlayer->pev->playerclass = teamIndex;

	pPlayer->pev->colormap = (int)(m_Teams[teamIndex].colormap) & 0x0000FFFF;// XDM3037a: warning! 2 bytes to 4!
	SetPlayerTeamParams(pPlayer);

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
		WRITE_BYTE(pPlayer->entindex());
		WRITE_BYTE(pPlayer->pev->team);
	MESSAGE_END();
	UTIL_LogPrintf("\"%s<%i><%s>\" added to team %d\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), teamIndex);
	RecountTeams(true);// XDM3038
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: RemovePlayerFromTeam - removes player from specified team if he is there
// Input  : *pPlayer -
//			teamIndex -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::RemovePlayerFromTeam(CBasePlayer *pPlayer, TEAM_ID teamIndex)
{
	DBG_GR_PRINT("CGameRulesTeamplay::RemovePlayerFromTeam(%d %s, %hd)\n", pPlayer?pPlayer->entindex():0, pPlayer?STRING(pPlayer->pev->netname):"", teamIndex);
	if (pPlayer == NULL)
		return false;
	if (teamIndex > MaxTeams())// XDM3038
		return false;
	//else if (teamIndex == TEAM_NONE)

	if (!PlayerIsInTeam(pPlayer, teamIndex))
		return false;

	m_Teams[teamIndex].score -= (int)pPlayer->pev->frags;
	m_Teams[teamIndex].playercount--;

	if (!FBitSet(pPlayer->pev->flags, FL_SPECTATOR))// XDM3038a: this may be called from ObserverStart()
	{
		pPlayer->pev->team = TEAM_NONE;
		pPlayer->pev->colormap = 0;
		// keep player's team name for next level? // SetPlayerTeamParams(pPlayer);
		MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
			WRITE_BYTE(pPlayer->entindex());
			WRITE_BYTE(pPlayer->pev->team);
		MESSAGE_END();
	}
	UTIL_LogPrintf("\"%s<%i><%s>\" removed from team %hd\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), teamIndex);
	RecountTeams(true);// XDM3038
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Client entered a console command
// Input  : *pPlayer - client
//			*pcmd - command line, use CMD_ARGC() and CMD_ARGV()
// Output : Returns true if the command was recognized.
//-----------------------------------------------------------------------------
bool CGameRulesTeamplay::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	DBG_GR_PRINT("CGameRulesTeamplay::ClientCommand(%d %s, %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname), pcmd);
	if (FStrEq(pcmd, "jointeam"))// index TODO: UNDONE: shouldn't be a command..?
	{
		if (CMD_ARGC() > 1)
		{
			if (!IsGameOver())
			{
				if (pPlayer->m_iSpawnState > SPAWNSTATE_CONNECTED && mp_teamchange.value <= 0.0f)// XDM3038a
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#TEAMCHANGENOTALLOWED");
				else// always allow joining a team if just connected
					ChangePlayerTeam(pPlayer, atoi(CMD_ARGV(1)), (mp_teamchangekill.value > 0.0f), (mp_teamchangekill.value > 1.0f));
			}
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Usage: \"<%s> <team number 1-4>\" or \"<team> <team name>\"\n", CMD_ARGV(0));
	}
#if defined (_DEBUG)
	else if (FStrEq(pcmd, "dumpteams"))
	{
		ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, "m_iNumTeams: %hd\n", m_iNumTeams);
		for (TEAM_ID i = 0; i < m_iNumTeams; ++i)
			ClientPrintF(pPlayer->pev, HUD_PRINTCONSOLE, 0, " > %d %s\tscore %d\textra %d\tplayers %d\tcmap %d\n", i, m_Teams[i].name, m_Teams[i].score, m_Teams[i].extrascore, m_Teams[i].playercount, m_Teams[i].colormap);
	}
#endif
	else return CGameRulesMultiplay::ClientCommand(pPlayer, pcmd);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieve the team name entered by this player
// Input  : *pPlayer -
// Output : const char *name
//-----------------------------------------------------------------------------
const char *CGameRulesTeamplay::GetPlayerTeamName(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::GetPlayerTeamName(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	return GET_INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(pPlayer->edict()), "team");
}

//-----------------------------------------------------------------------------
// Purpose: Update client user info buffer
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::SetPlayerTeamParams(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesTeamplay::SetPlayerTeamParams(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	char *infobuffer = GET_INFO_KEY_BUFFER(pPlayer->edict());
	if (!infobuffer)
		return;

	//conprintf(1, " -- SetPlayerTeamParams() %s %s\n", STRING(pPlayer->pev->netname), GetTeamName(pPlayer->pev->team));
	SET_CLIENT_KEY_VALUE(pPlayer->entindex(), infobuffer, "team", (char *)GetTeamName(pPlayer->pev->team));

	ClientUserInfoChanged(pPlayer, infobuffer);
}

//-----------------------------------------------------------------------------
// Purpose: dump debug info to console
//-----------------------------------------------------------------------------
void CGameRulesTeamplay::DumpInfo(void)
{
	CGameRulesMultiplay::DumpInfo();// must be first
	conprintf(1, "Teams (total %d):\nid\tfrg\tdth\tplr\t\tscr\tnam--\n", GetNumberOfTeams());
	for (TEAM_ID i = TEAM_NONE; i < m_iNumTeams; ++i)// m_iNumTeams includes 0th!
	{
		conprintf(1, "%d  %d %u %hd %d (%s)\n", i, 
		m_Teams[i].score,
		m_Teams[i].loses,
		//m_Teams[i].ping,
		//m_Teams[i].packetloss,
		m_Teams[i].playercount,
		m_Teams[i].extrascore,
		m_Teams[i].name);
	}
	TEAM_ID bestteam = g_pGameRules->GetBestTeam();
	conprintf(1, "Best team: %hd (%s)\n", bestteam, GetTeamName(bestteam));
}
