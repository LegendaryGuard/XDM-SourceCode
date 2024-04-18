//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef TEAMPLAY_GAMERULES_H
#define TEAMPLAY_GAMERULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

#include "color.h"
#include "colors.h"

// Colormap is Hue in 0...255 scale
static int teamcolormapdefault[MAX_TEAMS+1] =
{
	COLORMAP_CYAN,
	COLORMAP_GREEN,
	COLORMAP_BLUE,
	COLORMAP_RED,
	COLORMAP_YELLOW, // XDM3035: was COLORMAP_CYAN,
};

typedef struct team_s
{
	int score;// frags: can be negative
	int extrascore;// goals
	uint32 loses;// deaths
	short playercount;
	unsigned short colormap;
	char name[MAX_TEAMNAME_LENGTH];
	Color color;// byte color[3];
	uint32 spawndelay;// XDM3038a: how soon players of this team are allowed to spawn (seconds)
} team_t;

// really should not be used externally
void GetTeamColor(TEAM_ID team, byte &r, byte &g, byte &b);

class CGameRulesTeamplay : public CGameRulesMultiplay
{
public:
	CGameRulesTeamplay();

	virtual short GetGameType(void) { return GT_TEAMPLAY; }// XDM3035

	//virtual void StartFrame(void);
	virtual void Initialize(void);

	virtual bool IsTeamplay(void) { return true; }
	virtual const char *GetGameDescription(void) { return "Teamplay"; }  // this is the game name that gets seen in the server browser

	//virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
	virtual void ClientDisconnected(CBasePlayer *pPlayer);
	virtual bool ClientCommand(CBasePlayer *pPlayer, const char *pcmd);
	virtual void ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer);
	virtual void InitHUD(CBasePlayer *pPlayer);

	virtual bool CheckLimits(void);
	virtual bool CheckEndConditions(void);// XDM3038

	virtual bool FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker);
	virtual bool FShouldAutoAim(CBasePlayer *pPlayer, CBaseEntity *pTarget);

	virtual int ShouldShowStartMenu(CBasePlayer *pPlayer);// XDM3038a
	virtual bool ValidateSpawnPoint(CBaseEntity *pPlayer, CBaseEntity *pSpot);// XDM3038a

	virtual bool PlayerSpawnStart(CBasePlayer *pPlayer);// XDM3038a: called by CBasePlayer::Spawn() at the very beginning
	//virtual bool PlayerStartObserver(CBasePlayer *pPlayer);// XDM3037a
	virtual bool PlayerStopObserver(CBasePlayer *pPlayer);// XDM3037a
	virtual bool FPlayerStartAsObserver(CBasePlayer *pPlayer);// XDM3037a

	//virtual int IPointsForKill(CBaseEntity *pAttacker, CBaseEntity *pKilled);
	virtual void PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);
	virtual void MonsterKilled(CBaseMonster *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);// XDM3035a

	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
	virtual bool AddScore(CBaseEntity *pWinner, int score);// XDM3037a

	virtual TEAM_ID GetTeamIndex(const char *pTeamName);
	virtual const char *GetTeamName(TEAM_ID teamIndex);
	team_t *GetTeamByID(TEAM_ID teamIndex);

	virtual bool IsValidTeam(const char *pTeamName);
	virtual bool IsValidTeam(TEAM_ID teamIndex);
	virtual bool IsRealTeam(TEAM_ID teamIndex);

	virtual void ChangePlayerTeam(CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib);
	virtual void ChangePlayerTeam(CBasePlayer *pPlayer, TEAM_ID teamindex, bool bKill, bool bGib);

	virtual bool AddScoreToTeam(TEAM_ID teamIndex, int score);

	virtual short NumPlayersInTeam(TEAM_ID teamIndex);
	virtual short MaxTeams(void) { return MAX_TEAMS; }// Maximum theoretically possible number of active teams
	virtual short GetNumberOfTeams(void) { return (m_iNumTeams - 1); }// Active joinable teams only!

	virtual bool FUseExtraScore(void) { return false; }// XDM3037a
	virtual int GetTeamScore(TEAM_ID teamIndex);// XDM3037a
	virtual CBasePlayer *GetBestPlayer(TEAM_ID teamIndex);
	virtual TEAM_ID GetBestTeam(void);
	virtual TEAM_ID GetBestTeamByScore(void);// XDM3037a
	virtual uint32 GetScoreLimit(void);

	virtual CBaseEntity *GetTeamBaseEntity(TEAM_ID teamIndex);
	virtual void SetTeamBaseEntity(TEAM_ID teamIndex, CBaseEntity *pEntity);

	virtual void DumpInfo(void);

	TEAM_ID TeamWithFewestPlayers(void);
	bool PlayerIsInTeam(CBasePlayer *pPlayer, TEAM_ID teamIndex);

protected:
	void AssignPlayer(CBasePlayer *pPlayer);
	void RecountTeams(bool bResendInfo);

	TEAM_ID CreateNewTeam(const char *pTeamName);
	bool AddPlayerToTeam(CBasePlayer *pPlayer, TEAM_ID teamIndex);
	bool RemovePlayerFromTeam(CBasePlayer *pPlayer, TEAM_ID teamIndex);

	const char *GetPlayerTeamName(CBasePlayer *pPlayer);
	void SetPlayerTeamParams(CBasePlayer *pPlayer);

	EHANDLE m_hBaseEntities[MAX_TEAMS+1];
	team_t m_Teams[MAX_TEAMS+1];// 0 = unassigned
	TEAM_ID m_LeadingTeam;
	short m_iNumTeams;// including team 0
	bool m_DisableDeathMessages;// little hacks to disable 
	bool m_DisableDeathPenalty;
	//bool m_bTeamLimit;// This means the server set only some teams as valid
};

#endif // TEAMPLAY_GAMERULES_H
