//-----------------------------------------------------------------------------
// X-Half-Life
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#ifndef CTF_GAMERULES_H
#define CTF_GAMERULES_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif // !__MINGW32__
#endif // _WIN32

class CGameRulesCTF : public CGameRulesTeamplay
{
public:
	virtual short GetGameType(void) { return GT_CTF; }// XDM3035
	virtual void Initialize(void);// XDM3037
	virtual void InitHUD(CBasePlayer *pPlayer);
	virtual void ClientDisconnected(CBasePlayer *pPlayer);
	virtual const char *GetGameDescription(void) {return "Capture The Flag";}
	virtual bool FUseExtraScore(void) { return true; }// XDM3037a
	virtual TEAM_ID GetBestTeam(void);
	virtual uint32 GetScoreLimit(void);
	virtual short MaxTeams(void) { return MAX_CTF_TEAMS; }
	uint32 m_iNumCaptureZones;
};


#define CAPTUREOBJECT_SIZE_MINS		Vector(-32,-32,0)
#define CAPTUREOBJECT_SIZE_MAXS		Vector(32,32,96)

#define CAPTUREOBJECT_SCORE			1

// XDM3038a
#define SF_CAPTUREOBJECT_SLOWDOWN			1
#define SF_CAPTUREOBJECT_HOLSTERWEAPON		2

//=================================================================
// CCaptureObject
// WARNING! Team indexes start from 1! (TEAM_1)
//=================================================================
class CCaptureObject : public CBaseAnimating
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	void EXPORT CaptureObjectThink(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int ShouldCollide(CBaseEntity *pOther);
	virtual bool IsPushable(void);
	virtual bool IsGameGoal(void) const { return true; }

	virtual void Captured(CBaseEntity *pPlayer);
	virtual void Taken(CBaseEntity *pPlayer);
	virtual void Drop(CBaseEntity *pPlayer);
	virtual void Return(CBaseEntity *pPlayer);
	virtual void Reset(CBaseEntity *pPlayer);
};


//=================================================================
// CCaptureZone
// A brush entity (trigger) to carry the flag to
//=================================================================
class CCaptureZone : public CBaseEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Touch(CBaseEntity *pOther);
};

#endif // CTF_GAMERULES_H
