//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef LMS_GAMERULES_H
#define LMS_GAMERULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

// Last Man Standing
// Subclass CGameRulesRoundBased to inherit players join mechanism
class CGameRulesLMS : public CGameRulesMultiplay// no CGameRulesRoundBased
{
public:
	virtual const char *GetGameDescription(void) { return "Last Man Standing"; }
	virtual short GetGameType(void) { return GT_LMS; }// XDM3035

	virtual void StartFrame(void);
	//virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

	virtual void InitHUD(CBasePlayer *pPlayer);
	virtual bool CheckLimits(void);
	virtual bool CheckEndConditions(void);// XDM3038

	//virtual void PlayerThink(CBasePlayer *pPlayer);
	virtual void PlayerKilled(CBasePlayer *pVictim, CBaseEntity *pKiller, CBaseEntity *pInflictor);
	virtual bool PlayerStartObserver(CBasePlayer *pPlayer);// XDM3037a
	virtual bool PlayerStopObserver(CBasePlayer *pPlayer);// XDM3037a

	virtual bool FPlayerCanRespawn(const CBasePlayer *pPlayer);
	virtual bool FForceRespawnPlayer(CBasePlayer *pPlayer) { return true; }
	virtual bool FPlayerStartAsObserver(CBasePlayer *pPlayer);// XDM3037a
	virtual bool FAllowSpectatorChange(CBasePlayer *pPlayer)  { return false; }

	virtual bool FStartImmediately(void) { return false; }// XDM3038
	virtual int SpawnSpotUsePolicy(void) { return SPAWNSPOT_FORCE_WAIT; }// XDM3038a

	virtual CBasePlayer *GetBestPlayer(TEAM_ID teamIndex);

	short m_iLastCheckedNumActivePlayers;
	short m_iLastCheckedNumFinishedPlayers;
};

#endif // LMS_GAMERULES_H
