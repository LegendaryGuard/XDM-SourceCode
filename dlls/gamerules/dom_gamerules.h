//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef DOM_GAMERULES_H
#define DOM_GAMERULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

class CGameRulesDomination : public CGameRulesTeamplay
{
public:
	//CGameRulesDomination(); WARNING!! This causes weird network error on level change!
	virtual short GetGameType(void) { return GT_DOMINATION; }// XDM3035
	virtual void InitHUD(CBasePlayer *pPlayer);
	virtual const char *GetGameDescription(void) { return "Domination"; }
	virtual bool FUseExtraScore(void) { return true; }// XDM3037a
	virtual TEAM_ID GetBestTeam(void);
	//virtual CBaseEntity *GetTeamBaseEntity(TEAM_ID teamIndex);
};

#endif // DOM_GAMERULES_H
