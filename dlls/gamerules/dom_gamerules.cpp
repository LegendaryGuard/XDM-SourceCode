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
#include "dom_gamerules.h"
#include "globals.h"
#include "hltv.h"

// TODO: FireTargets("game_dom_allpoints", g_pWorld, g_pWorld, USE_TOGGLE, 0); when all control points are taken by one team

//-----------------------------------------------------------------------------
// Purpose: Sent initialization messages to client
// Input  : *pPlayer - client
//-----------------------------------------------------------------------------
void CGameRulesDomination::InitHUD(CBasePlayer *pPlayer)
{
	CGameRulesTeamplay::InitHUD(pPlayer);

	if (!pPlayer->IsBot())
	{
		// send checkpoints info
		int c = 0;
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_dom_target")) != NULL)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgDomInfo, NULL, pPlayer->edict());
				WRITE_SHORT(pEntity->entindex());
				WRITE_BYTE(pEntity->pev->team);// XDM3033: overriddes CInfoDomTarget::SendClientData
				WRITE_STRING(STRING(pEntity->pev->message));
			MESSAGE_END();
			SetTeamBaseEntity(pEntity->pev->team, pEntity);// XDM3035: 20091122
			++c;
		}
		ALERT(at_aiconsole, "CGameRulesDomination: initialized %d checkpoints\n", c);
	}
	//FireTargets("game_playerjoin", pPlayer, pPlayer, USE_TOGGLE, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Get team with the best score (extrascore version)
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesDomination::GetBestTeam(void)
{
	return GetBestTeamByScore();// redirect to proper counter
}

//-----------------------------------------------------------------------------
// Purpose: Locate team base. Literally. Home capture point.
// Input  : team - TEAM_ID
// Output : CBaseEntity *can be anything, but now it is info_dom_target
//-----------------------------------------------------------------------------
/*20091121 CBaseEntity *CGameRulesDomination::GetTeamBaseEntity(TEAM_ID teamIndex)
{
	if (IsValidTeam(team))
	{
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_dom_target")) != NULL)
		{
			if (pEntity->pev->team == teamIndex)
				return pEntity;
		}
	}
	return NULL;// g_pWorld?
}*/


//=========================================================
// XDM3037: logic entity itself
//=========================================================
class CInfoDomTarget : public CBaseAnimating
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);
// not needed	virtual void SendClientData(CBasePlayer *pClient, int msgtype, short sendcase);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool IsGameGoal(void) const { return true; }
};

LINK_ENTITY_TO_CLASS(info_dom_target, CInfoDomTarget);

void CInfoDomTarget::Spawn(void)
{
	if (!g_pGameRules || g_pGameRules->GetGameType() != GT_DOMINATION)
	{
		conprintf(2, "CInfoDomTarget: removed because of game rules mismatch\n");
		pev->flags = FL_KILLME;//Destroy();
		return;
	}
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	//ASSERT(!FStringNull(pev->model));// XDM3038
	//SET_MODEL(edict(), STRING(pev->model));
	//UTIL_SetOrigin(this, pev->origin);
	//pev->framerate = 1.0f;
	CBaseAnimating::Spawn();// starts animation
	UTIL_SetSize(this, g_vecZero, g_vecZero);
	pev->team = TEAM_NONE;
	pev->rendercolor.Set(255,255,255);
	pev->dmgtime = gpGlobals->time + 0.1f;

	if (g_pGameRules->FAllowEffects())
		pev->renderfx = kRenderFxGlowShell;
}

void CInfoDomTarget::Precache(void)
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/dom_point.mdl");

	//pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	CBaseAnimating::Precache();
	PRECACHE_SOUND("game/dom_touch.wav");
}

void CInfoDomTarget::Think(void)
{
	if (pev->team <= TEAM_NONE)
		return;

	//ALERT(at_console, "*** CInfoDomTarget -> AddScoreToTeam %d\n", pev->team);
	if (g_pGameRules)
		g_pGameRules->AddScoreToTeam(pev->team, 1);

	SetNextThink(mp_domscoreperiod.value);// XDM3038a
}

// a player has connected and requests update
////// UPDATE: done in CGameRulesDomination::InitHUD
/*void CInfoDomTarget::SendClientData(CBasePlayer *pClient, int msgtype, short sendcase)
{
	PLAYBACK_EVENT_FULL(FEV_HOSTONLY, pClient->edict(), g_usDomPoint, 0.0, g_vecZero, g_vecZero, DOM_LIGHT_RADIUS, 0.0, pev->team, entindex(), 0, 0);
}*/

// used by trigger, activator must be player
void CInfoDomTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (g_pGameRules->IsGameOver())// XDM3035a: don't react after the game has ended
		return;
	if (pActivator == NULL)
		return;
	if (pActivator->pev->team == pev->team)// don't flood
		return;
	if (pev->dmgtime > gpGlobals->time)
		return;
	if (!pActivator->IsPlayer() || !pActivator->IsAlive())
		return;

	if (value >= 0.0f)// != TEAM_NONE?
	{
		//pev->team = (int)value;
		pev->team = pActivator->pev->team;
		byte r = 127, g = 127, b = 127;
		GetTeamColor(pev->team, r,g,b);
		pev->rendercolor.Set(r, g, b);
		pev->body = pev->team;
		pev->skin = pev->team;
		SetNextThink(0);// XDM3038a: now
	}
	PLAYBACK_EVENT_FULL(FEV_GLOBAL|FEV_UPDATE, edict(), g_usDomPoint, 0, pev->origin, pev->angles, DOM_LIGHT_RADIUS, 0, pev->team, pActivator->entindex(), 0, 0);
	if (g_pGameRules->FAllowSpectators())
	{
	MESSAGE_BEGIN(MSG_SPEC, svc_director);
		WRITE_BYTE(9);// command length in bytes
		WRITE_BYTE(DRC_CMD_EVENT);
		WRITE_SHORT(pActivator->entindex());// index number of primary entity
		WRITE_SHORT(entindex());// index number of secondary entity
		WRITE_LONG(14 | DRC_FLAG_DRAMATIC | DRC_FLAG_FACEPLAYER);// eventflags (priority and flags)
	MESSAGE_END();
	}
	pev->dmgtime = gpGlobals->time + 0.1f;
	((CBasePlayer *)pActivator)->m_Stats[STAT_CONTROL_POINT_COUNT]++;// XDM3037
	SetBits(((CBasePlayer *)pActivator)->m_iGameFlags, EGF_REACHEDGOAL);// XDM3038c

	if (sv_lognotice.value > 0)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" takes control point \"%s\"\n",  
			STRING(pActivator->pev->netname),
			GETPLAYERUSERID(pActivator->edict()),
			GETPLAYERAUTHID(pActivator->edict()),
			g_pGameRules->GetTeamName(pActivator->pev->team),
			STRING(pev->targetname));
	}
	if (!FStringNull(pev->target))// XDM3037: set target's color and use it
		FireTargets(STRING(pev->target), pActivator, this, USE_ON, pev->team, FSetTeamColor);// USE_ON is a must, because it doesn't turn off sprites, beams, etc.
}


//=========================================================
// Trigger
// OBSOLETE: use trigger_multiple instead
//=========================================================
class CTriggerDomPoint : public CBaseToggle
{
public:
	virtual void Spawn(void);
	virtual void Touch(CBaseEntity *pOther);
	virtual int	ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(trigger_dom_point, CTriggerDomPoint);

void CTriggerDomPoint::Spawn(void)
{
	if (g_pGameRules->GetGameType() != GT_DOMINATION)
	{
		ALERT(at_aiconsole, "CTriggerDomPoint: removed because of game rules mismatch\n");
		pev->flags = FL_KILLME;//Destroy();
		return;
	}
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	pev->takedamage = DAMAGE_NO;
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetOrigin(this, pev->origin);
	pev->team = TEAM_NONE;

	if (m_flWait < 0.1f)
		m_flWait = 0.5f;

	if (showtriggers.value <= 0.0f)
		SetBits(pev->effects, EF_NODRAW);
}

void CTriggerDomPoint::Touch(CBaseEntity *pOther)
{
	if (g_pGameRules->IsGameOver())// XDM3035a: don't react after the game has ended
		return;

	if (pev->dmgtime > gpGlobals->time)
		return;

	if (!pOther->IsPlayer())
		return;

	if (!pOther->IsAlive())// pev->deadflag != DEAD_NO)
		return;

	if (pOther->pev->team == pev->team)
		return;

	pev->team = pOther->pev->team;
	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), pOther, this, USE_SET, pev->team);

	pev->dmgtime = gpGlobals->time + m_flWait;
}
