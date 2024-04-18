//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef ROUND_GAMERULES_H
#define ROUND_GAMERULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

// right now this is a stub

enum round_state_e
{
	ROUND_STATE_WAITING = 0,// waiting for players to join
	ROUND_STATE_SPAWNING,// players spawn only at this point
	ROUND_STATE_ACTIVE,// the game is on, respawning is not allowed
	ROUND_STATE_FINISHED// round has ended, announce scores
};

// Round-based game rules
// Rounds can be played without changing map.
// Possible derived game rules: assault, defuse, etc.
class CGameRulesRoundBased : public CGameRulesTeamplay
{
public:
	CGameRulesRoundBased();

	virtual void Initialize(void);
	virtual short GetGameType(void) { return GT_ROUND; }
	//virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
	//virtual void ClientDisconnected(CBasePlayer *pPlayer);
	virtual void InitHUD(CBasePlayer *pPlayer);
	virtual void StartFrame(void);
	virtual bool IsRoundBased(void) { return true; }
	virtual const char *GetGameDescription(void) { return "Round TDM"; }

	virtual bool FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker);

	//virtual bool PlayerSpawnStart(CBasePlayer *pPlayer);// XDM3038a
	virtual bool FPlayerCanRespawn(const CBasePlayer *pPlayer);// XDM3037a
	virtual bool FPlayerStartAsObserver(CBasePlayer *pPlayer);// XDM3037a
	virtual bool FStartImmediately(void) { return false; }// XDM3038
	virtual int ShouldShowStartMenu(CBasePlayer *pPlayer);// XDM3038a

	virtual TEAM_ID GetBestTeam(void);// XDM3038a

	virtual void EndMultiplayerGame(void);
	virtual void ChangeLevel(void);

	virtual void RoundStart(void);
	virtual void RoundEnd(void);

	virtual uint32 GetRoundsLimit(void);// { return m_iNumRounds; }
	virtual uint32 GetRoundsPlayed(void) { return m_iRoundsCompleted; }

	float m_fRoundStartTime;

protected:
	uint32 m_iNumRounds;// how many rounds to play on this map
	uint32 m_iRoundsCompleted;// also serves as current round index
	short m_iRoundState;// round_state_e
	//bool m_bSwitchTeams;
};

#endif // ROUND_GAMERULES_H
