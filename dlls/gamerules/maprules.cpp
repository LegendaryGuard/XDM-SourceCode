// Keeps compatibility with valve's hacks
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "color.h"// XDM3037a
#include "cbase.h"
#include "player.h"
#include "maprules.h"
#include "gamerules.h"
#include "game.h"
#include "globals.h"
#include "shake.h"
#include "pm_shared.h"
#include "triggers.h"


//=========================================================
// CRuleEntity -- base class for all rule entities
//=========================================================
void CRuleEntity::Spawn(void)
{
	CBaseDelay::Spawn();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

bool CRuleEntity::TeamMatch(const CBaseEntity *pActivator)
{
	if (pActivator)// XDM3037a
	{
		if (g_pGameRules == NULL)// XDM3038a: ???
			return false;

		if (!g_pGameRules->IsTeamplay())
			return true;

		if (pev->team == TEAM_NONE || pActivator->pev->team == pev->team)//m_teamIndex+1));// XDM3038a: allow a little different approach
			return true;
	}
	return false;
}

bool CRuleEntity::CanFireForActivator(CBaseEntity *pActivator, bool bCheckTeam)
{
	if (!FStringNull(m_iszMaster))
		return UTIL_IsMasterTriggered(STRING(m_iszMaster), pActivator);

	if (bCheckTeam && g_pGameRules && g_pGameRules->IsTeamplay() && pev->team != TEAM_NONE && pActivator->pev->team != pev->team)
		return false;// XDM3038a: TESTME

	return true;
}

//=========================================================
// CRulePointEntity -- base class for all rule "point" entities (not brushes)
//=========================================================
void CRulePointEntity::Spawn(void)
{
	CRuleEntity::Spawn();
	pev->frame = 0;
	pev->model = 0;
	pev->modelindex = 0;
}

//=========================================================
// CRuleBrushEntity -- base class for all rule "brush" entities (not brushes)
// Default behavior is to set up like a trigger, invisible, but keep the model for volume testing
//=========================================================
void CRuleBrushEntity::Spawn(void)
{
	SET_MODEL(edict(), STRING(pev->model));
	CRuleEntity::Spawn();
}




//=========================================================
// Award points to player or a team
//	Points +/- total
//	Flag: Allow negative scores					SF_SCORE_NEGATIVE
//	Flag: Award points to team in teamplay		SF_SCORE_TEAM
//=========================================================
LINK_ENTITY_TO_CLASS(game_score, CGameScore);

void CGameScore::Spawn(void)
{
	CRulePointEntity::Spawn();
}

void CGameScore::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "points"))
	{
		pev->frags = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue(pkvd);
}

void CGameScore::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (g_pGameRules == NULL)
		return;

	if (pev->frags < 0 && !FBitSet(pev->spawnflags, SF_GAMESCORE_NEGATIVE))
	{
		conprintf(2, "CGameScore(%s)::Use(%d, %d): negative score not allowed!\n", STRING(pev->targetname), pActivator?pActivator->entindex():0, pCaller?pCaller->entindex():0);
		return;
	}

	if (!CanFireForActivator(pActivator, true))
		return;

	// Only players can use this
	if (FBitSet(pev->spawnflags, SF_GAMESCORE_EVERYONE))
	{
		g_pGameRules->AddScore(NULL, (int)pev->frags);
	}
	else if (pActivator->IsPlayer())
	{
		if (FBitSet(pev->spawnflags, SF_GAMESCORE_TEAM))
			g_pGameRules->AddScoreToTeam(pActivator->pev->team, (int)pev->frags);
		else
			g_pGameRules->AddScore(pActivator, (int)pev->frags);
	}
}


//=========================================================
// Instantly ends the game in multiplayer
//=========================================================
LINK_ENTITY_TO_CLASS(game_end, CGameEnd);

void CGameEnd::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (g_pGameRules)
	{
		if (!CanFireForActivator(pActivator, true))
			return;

		g_pGameRules->EndMultiplayerGame();
	}
}


//=========================================================
// NON-localized HUD message (use env_message to display a titles.txt message)
//=========================================================
LINK_ENTITY_TO_CLASS(game_text, CGameText);

// Save parms as a block.  Will break save/restore if the structure changes, but this entity didn't ship with Half-Life, so it can't impact saved Half-Life games.
TYPEDESCRIPTION	CGameText::m_SaveData[] =
{
	DEFINE_ARRAY(CGameText, m_textParms, FIELD_CHARACTER, sizeof(hudtextparms_t)),
};

IMPLEMENT_SAVERESTORE(CGameText, CRulePointEntity);

void CGameText::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "channel"))
	{
		m_textParms.channel = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "x"))
	{
		m_textParms.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "y"))
	{
		m_textParms.y = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "effect"))
	{
		m_textParms.effect = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "color"))
	{
		if (StringToRGBA(pkvd->szValue, m_textParms.r1, m_textParms.g1, m_textParms.b1, m_textParms.a1))
			pkvd->fHandled = TRUE;
		else
		{
			pkvd->fHandled = 2;// XDM3037
			//conprintf(1, "Error: %s has bad value %s == \"%s\"!\n", pkvd->szClassName, pkvd->szKeyName, pkvd->szValue);
			m_textParms.r1 = 0;
			m_textParms.g1 = 0;
			m_textParms.b1 = 0;
			m_textParms.a1 = 0;
		}
	}
	else if (FStrEq(pkvd->szKeyName, "color2"))
	{
		if (StringToRGBA(pkvd->szValue, m_textParms.r2, m_textParms.g2, m_textParms.b2, m_textParms.a2))
			pkvd->fHandled = TRUE;
		else
		{
			pkvd->fHandled = 2;// XDM3037
			//conprintf(1, "Error: %s has bad value %s == \"%s\"!\n", pkvd->szClassName, pkvd->szKeyName, pkvd->szValue);
			m_textParms.r2 = 0;
			m_textParms.g2 = 0;
			m_textParms.b2 = 0;
			m_textParms.a2 = 0;
		}
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_textParms.fadeinTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_textParms.fadeoutTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		m_textParms.holdTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fxtime"))
	{
		m_textParms.fxTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue(pkvd);
}

void CGameText::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, false))
		return;

	if (FBitSet(pev->spawnflags, SF_ENVTEXT_ALLPLAYERS))
		UTIL_HudMessageAll(m_textParms, STRING(pev->message));// TODO: team mask
	else if (pActivator->IsNetClient())
		UTIL_HudMessage(pActivator, m_textParms, STRING(pev->message));
}


//=========================================================
// "Masters" like multisource, but based on the team of the activator
// Only allows mastered entity to fire if the team matches my team
// XDM3035a: FIXME: these team indexes are wrong!
//=========================================================
LINK_ENTITY_TO_CLASS(game_team_master, CGameTeamMaster);

void CGameTeamMaster::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "teamindex"))
	{
		pev->team = atoi(pkvd->szValue) + 1;// XDM3038a: this was 0-based index!//m_teamIndex = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0: triggerType = USE_OFF; break;
		case 2: triggerType = USE_TOGGLE; break;
		default: triggerType = USE_ON; break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue(pkvd);
}

void CGameTeamMaster::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, false))// revisit
		return;

	if (useType == USE_SET)
	{
		if (value < 0)
			pev->team = TEAM_NONE;// XDM3037a
		else
			pev->team = pActivator->pev->team;

		return;
	}

	if (FBitSet(pev->spawnflags, SF_TEAMMASTER_ANYTEAM) && g_pGameRules && g_pGameRules->IsValidTeam(pActivator->pev->team) || TeamMatch(pActivator))
	{
		SUB_UseTargets(pActivator, triggerType, value);

		if (FBitSet(pev->spawnflags, SF_TEAMMASTER_FIREONCE))
			Destroy();
	}
}

bool CGameTeamMaster::IsTriggered(const CBaseEntity *pActivator)
{
	return TeamMatch(pActivator);
}


//=========================================================
// Changes the team of the entity it targets to the activator's team
// Flag: Fire once
// Flag: Clear team -- Sets the team to "NONE" instead of activator
//=========================================================
LINK_ENTITY_TO_CLASS(game_team_set, CGameTeamSet);

void CGameTeamSet::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (g_pGameRules == NULL || !g_pGameRules->IsTeamplay())// XDM3038c
	{
		Destroy();
		return;
	}
	if (!CanFireForActivator(pActivator, true))
		return;

	/* XDM3038a: WTF is this?!
	if (ShouldClearTeam())
		SUB_UseTargets(pActivator, USE_SET, -1);
	else
		SUB_UseTargets(pActivator, USE_SET, 0);*/

	if (FBitSet(pev->spawnflags, SF_TEAMSET_CLEARTEAM))
	{
		pev->team = TEAM_NONE;
	}
	else if (!IsActiveTeam(pev->team))
	{
		conprintf(2, "%s[%d] %s has bad team %d! Aborting.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->team);
		return;
	}

	// XDM3038a: TESTME: completely new code
	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->target))) != NULL)
	{
		if (pEntity == this)
			continue;

		if (pEntity->IsPlayer())
			g_pGameRules->ChangePlayerTeam((CBasePlayer *)pEntity, pev->team, true, false);
		else
		{
			pEntity->pev->team = pev->team;// UNDONE: update it somehow?
			if (FBitSet(pev->spawnflags, SF_TEAMSET_SETCOLOR))
				FSetTeamColor(pEntity, pActivator, this);// USE_ON is a must, because it doesn't turn off sprites, beams, etc.
		}
	}

	if (FBitSet(pev->spawnflags, SF_TEAMSET_FIREONCE))
		Destroy();
}



//=========================================================
// Changes the team of the player who fired it
//=========================================================
LINK_ENTITY_TO_CLASS(game_player_team, CGamePlayerTeam);

TEAM_ID CGamePlayerTeam::GetTargetTeam(const char *pszTargetName)
{
	CBaseEntity *pTeamEntity = NULL;
	while ((pTeamEntity = UTIL_FindEntityByTargetname(pTeamEntity, pszTargetName)) != NULL)
	{
		if (FClassnameIs(pTeamEntity->pev, "game_team_master"))
			return pTeamEntity->pev->team;
	}
	return NULL;
}

void CGamePlayerTeam::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, false))// false because pev->team means the desired value
		return;

	if (pActivator->IsPlayer() && g_pGameRules)
		g_pGameRules->ChangePlayerTeam((CBasePlayer *)pActivator, GetTargetTeam(STRING(pev->target)), FBitSet(pev->spawnflags, SF_PTEAM_KILL), FBitSet(pev->spawnflags, SF_PTEAM_GIB));

	if (FBitSet(pev->spawnflags, SF_PTEAM_FIREONCE))
		Destroy();
}



//=========================================================
// Players in the zone fire my target when I'm fired
//=========================================================
LINK_ENTITY_TO_CLASS(game_zone_player, CGamePlayerZone);

TYPEDESCRIPTION	CGamePlayerZone::m_SaveData[] =
{
	DEFINE_FIELD(CGamePlayerZone, m_iszInTarget, FIELD_STRING),
	DEFINE_FIELD(CGamePlayerZone, m_iszOutTarget, FIELD_STRING),
	DEFINE_FIELD(CGamePlayerZone, m_iszInCount, FIELD_STRING),
	DEFINE_FIELD(CGamePlayerZone, m_iszOutCount, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CGamePlayerZone, CRuleBrushEntity);

void CGamePlayerZone::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "intarget"))
	{
		m_iszInTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "outtarget"))
	{
		m_iszOutTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "incount"))
	{
		m_iszInCount = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "outcount"))
	{
		m_iszOutCount = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CRuleBrushEntity::KeyValue(pkvd);
}

void CGamePlayerZone::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, true))
		return;

	int playersInCount = 0;
	int playersOutCount = 0;
	int hullNumber;
	CBasePlayer *pPlayer = NULL;
	TraceResult trace;

	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		pPlayer = UTIL_ClientByIndex(i);
		if (pPlayer)
		{
			if (FBitSet(pPlayer->pev->flags, FL_DUCKING))
				hullNumber = head_hull;
			else
				hullNumber = human_hull;

			UTIL_TraceModel(pPlayer->pev->origin, pPlayer->pev->origin, hullNumber, edict(), &trace);

			if (trace.fStartSolid)
			{
				++playersInCount;
				if (!FStringNull(m_iszInTarget))
					FireTargets(STRING(m_iszInTarget), pPlayer, pActivator, useType, value);
			}
			else
			{
				++playersOutCount;
				if (!FStringNull(m_iszOutTarget))
					FireTargets(STRING(m_iszOutTarget), pPlayer, pActivator, useType, value);
			}
		}
	}

	if (!FStringNull(m_iszInCount))
		FireTargets(STRING(m_iszInCount), pActivator, this, USE_SET, playersInCount);

	if (!FStringNull(m_iszOutCount))
		FireTargets(STRING(m_iszOutCount), pActivator, this, USE_SET, playersOutCount);
}



//=========================================================
// Damages the player who fires it
//=========================================================
LINK_ENTITY_TO_CLASS(game_player_hurt, CGamePlayerHurt);

void CGamePlayerHurt::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, true))
		return;

	if (pActivator->IsPlayer())
	{
		if (pev->dmg < 0)
			pActivator->TakeHealth(-pev->dmg, DMG_GENERIC);
		else
			pActivator->TakeDamage(this, pActivator, pev->dmg, DMG_GENERIC);// XDM3038: pActivator
	}

	SUB_UseTargets(pActivator, useType, value);

	if (FBitSet(pev->spawnflags, SF_PKILL_FIREONCE))
		Destroy();
}



//=========================================================
// Counts events and fires target
//=========================================================
LINK_ENTITY_TO_CLASS(game_counter, CGameCounter);

void CGameCounter::Spawn(void)
{
	// Save off the initial count
	SetInitialValue(CountValue());
	CRulePointEntity::Spawn();
}

void CGameCounter::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, true))
		return;

	switch (useType)
	{
	case USE_ON:
		CountUp();
		break;

	case USE_OFF:
		CountDown();
		break;

	case USE_TOGGLE:
		if (CountValue() == 0)// XDM3038a: toggle between 0 and maximum, "value" must be equal to any of them
			SetCountValue((int)value);
		else if (LimitValue() == (int)value)
			SetCountValue(0);
		else// act as old HL counter
			CountUp();
		break;

	case USE_SET:
		SetCountValue((int)value);
		break;
	}

	if (CountValue() >= LimitValue())// XDM3038c: >=
	{
		SUB_UseTargets(pActivator, USE_TOGGLE, value);// XDM3038: value

		if (FBitSet(pev->spawnflags, SF_GAMECOUNT_FIREONCE))
			Destroy();
		else if (FBitSet(pev->spawnflags, SF_GAMECOUNT_RESET))
			ResetCount();
	}
}



//=========================================================
// Sets the counter's value
//=========================================================
LINK_ENTITY_TO_CLASS(game_counter_set, CGameCounterSet);

void CGameCounterSet::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, true))
		return;

	SUB_UseTargets(pActivator, USE_SET, pev->frags);

	if (FBitSet(pev->spawnflags, SF_GAMECOUNTSET_FIREONCE))
		Destroy();
}


//=========================================================
// Sets the default player equipment on this map
//=========================================================
LINK_ENTITY_TO_CLASS(game_player_equip, CGamePlayerEquip);

void CGamePlayerEquip::KeyValue(KeyValueData *pkvd)
{
	CRulePointEntity::KeyValue(pkvd);// XDM3036
	if (pkvd->fHandled)
		return;

	/*in CRuleEntity	if (FStrEq(pkvd->szKeyName, "master"))
	{
		SetMaster(ALLOC_STRING(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
	{*/
	/*if (pkvd->szKeyName[0] == '%')// XDM3038a: flag is easier to use right now
	{
		if (strcmp(pkvd->szKeyName, "%default%") == 0)
			SetBits(pev->spawnflags, SF_PLAYEREQUIP_ADDTODEFAULT);
		else
			conprintf(1, "Error: %s has bad special value %s == \"%s\"!\n", pkvd->szClassName, pkvd->szKeyName, pkvd->szValue);
	}
	else*/
	{
		for (size_t i = 0; i < MAX_EQUIP; ++i)
		{
			if (m_weaponNames[i] == NULL)
			{
				char tmp[128];
				UTIL_StripToken(pkvd->szKeyName, tmp);
				m_weaponNames[i] = ALLOC_STRING(tmp);
				m_weaponCount[i] = max(1, strtoul(pkvd->szValue, NULL, 10));
				m_weaponCount[i] = max(1, m_weaponCount[i]);
				pkvd->fHandled = TRUE;
				break;
			}
		}
	}
}

// XDM3038a: precache listed items!
void CGamePlayerEquip::Precache(void)
{
	for (size_t i = 0; i < MAX_EQUIP; ++i)
	{
		if (!FStringNull(m_weaponNames[i]))
			UTIL_PrecacheOther(STRING(m_weaponNames[i]));
	}
	CRulePointEntity::Precache();
}

void CGamePlayerEquip::Touch(CBaseEntity *pOther)
{
	if (FBitSet(pev->spawnflags, SF_PLAYEREQUIP_USEONLY))// XDM3037: check first
		return;

	if (!CanFireForActivator(pOther, true))
		return;

	EquipPlayer(pOther);
}

void CGamePlayerEquip::EquipPlayer(CBaseEntity *pEntity)
{
	if (!pEntity->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
	g_SilentItemPickup = true;// XDM3037: make unaccepted items disappear (otherwise item_longjump will stay in world if added to player without suit)
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		for (size_t i = 0; i < MAX_EQUIP; ++i)
		{
			if (FStringNull(m_weaponNames[i]))// XDM3038a
				break;

			for (size_t j = 0; j < m_weaponCount[i]; ++j)
				pPlayer->GiveNamedItem(STRING(m_weaponNames[i]));
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		conprintf(1, "EquipPlayer(%d) exception!\n", pEntity->entindex());
		g_SilentItemPickup = false;
	}
#endif
	g_SilentItemPickup = false;
}

void CGamePlayerEquip::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	EquipPlayer(pActivator);
}



LINK_ENTITY_TO_CLASS(game_restart, CGameRestart);

void CGameRestart::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, true))
		return;

	Destroy();// XDM3037: remove beforehand
	SERVER_COMMAND("restart\n");
}



LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator, true))
		return;

	// UNDONE CBaseContainer *pContainer = NULL;
	//if (!FStringNull(pev->target))
	//	pContainer = UTIL_FindEntityByTargetname(NULL, STRING(pev->target), pActivator);

	CBasePlayer *pPlayer = NULL;
	if (FBitSet(pev->spawnflags, SF_STRIPWEAPONS_EVERYONE))// XDM3038a
	{
		for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
		{
			pPlayer = UTIL_ClientByIndex(i);
			if (pPlayer)// UNDONE if (pContainer) pPlayer->TransferInventory(pContainer);
				pPlayer->RemoveAllItems(FBitSet(pev->spawnflags, SF_STRIPWEAPONS_SUIT), FBitSet(pev->spawnflags, SF_STRIPWEAPONS_QUESTITEMS));// XDM3038c
		}
	}
	else
	{
		if (pActivator && pActivator->IsPlayer())
			pPlayer = (CBasePlayer *)pActivator;
		else if (!IsMultiplayer())
			pPlayer = UTIL_ClientByIndex(1);

		if (pPlayer)// UNDONE if (pContainer) pPlayer->TransferInventory(pContainer);
			pPlayer->RemoveAllItems(FBitSet(pev->spawnflags, SF_STRIPWEAPONS_SUIT), FBitSet(pev->spawnflags, SF_STRIPWEAPONS_QUESTITEMS));// XDM3038c
	}
}


LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION	CRevertSaved::m_SaveData[] =
{
	DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT),// These are not actual times, but durations, so save as floats
	DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

void CRevertSaved::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CRevertSaved::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pActivator)
	{
		//DontThink();// XDM3038: TESTME UNDONE: there was a typo here XDM3037: CoOp support
		pev->enemy = pActivator->edict();
		if (pActivator->IsPlayer())
			UTIL_ScreenFade(pActivator, pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	}
	SetNextThink(MessageTime());// XDM3038a
	SetThink(&CRevertSaved::MessageThink);
}

void CRevertSaved::MessageThink(void)
{
	if (pev->enemy && IsMultiplayer())// XDM3037: CoOp support
		ClientPrint(&pev->enemy->v, HUD_PRINTHUD, STRING(pev->message));
	else
		ClientPrint(NULL, HUD_PRINTHUD, STRING(pev->message));

	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		SetNextThink(nextThink);// XDM3038a
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}

void CRevertSaved::LoadThink(void)
{
	DontThink();// XDM3037
	if (gpGlobals->deathmatch)
	{
		if (!FNullEnt(pev->enemy))// XDM3038c: in CoOp move player to his latest checkpoint
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(pev->enemy);
			if (pEntity)
				pEntity->TakeDamage(g_pWorld, g_pWorld, pEntity->pev->max_health*2.0f/* + pEntity->pev->armorvalue*/, DMG_IGNOREARMOR|DMG_DISINTEGRATING);// XDM3038c: the simplest way to force respawn a player
		}
	}
	else
		SERVER_COMMAND("reload\n");
}


//=========================================================
// Intermission spots, origin and angles used by camera.
// If target is specified, faces the target.
//=========================================================
LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);

// pev->watertype is: 1-only when joining, 2-only when game finished
void CInfoIntermission::Spawn(void)
{
	CPointEntity::Spawn();
	UTIL_SetOrigin(this, pev->origin);
	pev->v_angle.Clear();
	pev->impulse = 0;
	SetNextThink(2);// XDM3038a // let targets spawn!
}

void CInfoIntermission::Think(void)
{
	Activate();

	if (pev->impulse < INFOINTERMISSION_MAX_SEARCH_ATTEMPTS)
		SetNextThink(1.0);
	else
		DontThink();
}

// Whoever calls this first: server or think
void CInfoIntermission::Activate(void)
{
	if (pev->impulse >= INFOINTERMISSION_MAX_SEARCH_ATTEMPTS)
		return;

	edict_t *pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));
	if (!FNullEnt(pTarget))
	{
		VectorAngles((pTarget->v.origin - pev->origin).Normalize(), pev->v_angle);
		//pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
#if !defined (NOSQB)
		pev->v_angle.x = -pev->v_angle.x;
#endif
		pev->impulse = INFOINTERMISSION_MAX_SEARCH_ATTEMPTS;// XDM3038a: stop looking
	}
	else
		++pev->impulse;
}



//-----------------------------------------------------------------------------
// Purpose: 
// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should be moved across.
// Input  : *pLevelList - 
//			maxList - 
// Output : int
//-----------------------------------------------------------------------------
uint32 BuildChangeList(LEVELLIST *pLevelList, uint32 maxList)
{
	DBG_PRINTF("BuildChangeList(%s %s, maxList %d)\n", pLevelList->mapName, pLevelList->landmarkName, maxList);
	edict_t	*pentLandmark;
	uint32 i = 0;
	uint32 count = 0;
	// Find all of the possible level changes on this BSP
	CChangeLevel *pTrigger = NULL;
	while ((pTrigger = (CChangeLevel *)UTIL_FindEntityByClassname(pTrigger, "trigger_changelevel")) != NULL)
	{
		pentLandmark = FindLandmark(pTrigger->m_szLandmarkName);
		if (pentLandmark)
		{
			// Build a list of unique transitions
			if (AddTransitionToList(pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark))
			{
				++count;
				if (count >= maxList)// FULL!!
					break;
			}
		}
	}

	if (gpGlobals->pSaveData && ((SAVERESTOREDATA *)gpGlobals->pSaveData)->pTable)
	{
		CSave saveHelper((SAVERESTOREDATA *)gpGlobals->pSaveData);
		for (i = 0; i < count; ++i)
		{
			int j, entityCount = 0;
			CBaseEntity *pEntList[TRANSITION_MAX_ENTS];
			int entityFlags[TRANSITION_MAX_ENTS];
			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t *pent = ENTITIES_IN_PVS(pLevelList[i].pentLandmark);
			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while (!FNullEnt(pent))
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pent);
				if (pEntity)
				{
					//conprintf(1, "Trying %s\n", STRING(pEntity->pev->classname) );
					int caps = pEntity->ObjectCaps();
					if (!(caps & FCAP_DONT_SAVE))
					{
						int flags = 0;
						// If this entity can be moved or is global, mark it
						if (caps & FCAP_ACROSS_TRANSITION)
							flags |= FENTTABLE_MOVEABLE;
						if (!FStringNull(pEntity->pev->globalname) && !pEntity->IsDormant())
							flags |= FENTTABLE_GLOBAL;
						if (flags)
						{
							pEntList[entityCount] = pEntity;
							entityFlags[entityCount] = flags;
							++entityCount;
							if (entityCount > TRANSITION_MAX_ENTS)
							{
								ALERT(at_error, "Warning: Too many entities across a transition!\n");
								break;
							}
						}
						//else
						//	conprintf(1, "Failed %s\n", STRING(pEntity->pev->classname));
					}
					//else
					//	conprintf(1, "DON'T SAVE %s\n", STRING(pEntity->pev->classname));
				}
				pent = pent->v.chain;
			}

			for (j = 0; j < entityCount; ++j)
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if (entityFlags[j] && InTransitionVolume(pEntList[j], pLevelList[i].landmarkName))
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex(pEntList[j]);
					// Flag it with the level number
					saveHelper.EntityFlagsSet(index, entityFlags[j] | (1<<i));
				}
				//else
				//	conprintf(2, "Screened out %s\n", STRING(pEntList[j]->pev->classname));
			}
		}
	}
	return count;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Add a transition to the list, but ignore duplicates
// (a designer may have placed multiple trigger_changelevels with the same landmark)
// Input  : *pLevelList - 
//			listCount - 
//			*pMapName - 
//			*pLandmarkName - 
//			*pentLandmark - 
// Output : int
//-----------------------------------------------------------------------------
bool AddTransitionToList(LEVELLIST *pLevelList, uint32 listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark)
{
	if (!pLevelList || !pMapName || !pLandmarkName || !pentLandmark)
		return false;

	DBG_PRINTF("AddTransitionToList(%d, %s, %s)\n", listCount, pMapName, pLandmarkName);
	uint32 i;
	for (i = 0; i < listCount; ++i)
	{
		if (pLevelList[i].pentLandmark == pentLandmark && strcmp(pLevelList[i].mapName, pMapName) == 0)
			return false;
	}
	strcpy(pLevelList[listCount].mapName, pMapName);
	strcpy(pLevelList[listCount].landmarkName, pLandmarkName);
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS(pentLandmark)->origin;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			*pVolumeName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool InTransitionVolume(CBaseEntity *pEntity, const char *pVolumeName)
{
	if (pEntity == NULL || pVolumeName == NULL)// XDM3035a
		return false;// true?

	if (pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION)
		return true;// TODO: TESTME: return pEntity->IsInWorld();

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if (pEntity->pev->movetype == MOVETYPE_FOLLOW)
	{
		if (pEntity->pev->aiment != NULL)
			pEntity = CBaseEntity::Instance(pEntity->pev->aiment);
	}

	CBaseEntity *pTarget = NULL;
	bool inVolume = true;// Unless we find a trigger_transition, everything is in the volume
	while ((pTarget = UTIL_FindEntityByClassname(pTarget, "trigger_transition")) != NULL)//while ((pTarget = UTIL_FindEntityByTargetname(pTarget, pVolumeName)) != NULL) nope
	{
		//if (pTarget && FClassnameIs(pTarget->pev, "trigger_transition"))
		if (pTarget && FStrEq(STRING(pTarget->pev->targetname), pVolumeName))
		{
			if (pTarget->Intersects(pEntity))// It touches one, it's in the volume
				return true;
			else
				inVolume = false;// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
	}
	return inVolume;
}

//-----------------------------------------------------------------------------
// Purpose: FindLandmark
// Input  : *pLandmarkName - 
// Output : edict_t
//-----------------------------------------------------------------------------
edict_t *FindLandmark(const char *pLandmarkName)
{
	edict_t *pentLandmark = NULL;
	while ((pentLandmark = FIND_ENTITY_BY_TARGETNAME(pentLandmark, pLandmarkName)) != NULL)// check targetname first because it's rare
	{
		if (FNullEnt(pentLandmark))
			break;
		if (FClassnameIs(pentLandmark, "info_landmark"))// is this really necessary?
			return pentLandmark;
	}
	conprintf(0, "ERROR: Can't find landmark \"%s\"!\n", pLandmarkName);
	return NULL;
}

FILE_GLOBAL char g_szNextMap[MAX_MAPNAME];
FILE_GLOBAL char g_szNextSpot[MAX_MAPNAME];

//-----------------------------------------------------------------------------
// Purpose: 
// Add a transition to the list, but ignore duplicates
// (a designer may have placed multiple trigger_changelevels with the same landmark)
// Input  : *pLevelList - 
//			listCount - 
//			*pMapName - 
//			*pLandmarkName - 
//			*pentLandmark - 
// Output : int
//-----------------------------------------------------------------------------
bool ChangeLevel(const char *pMapName, const char *pLandmarkName, CBaseEntity *pActivator, string_t iszChangeTarget, float fChangeTargetDelay)
{
	CBasePlayer *pPlayer = NULL;
	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else
	{
		SERVER_PRINTF("ChangeLevelNow(%s %s) ERROR: called by non-player %s[%d]!\n", pMapName, pLandmarkName, pActivator?STRING(pActivator->pev->classname):"null", pActivator?pActivator->entindex():0);
		if (!IsMultiplayer())
			pPlayer = UTIL_ClientByIndex(1);// XDM3037
	}

	if (pPlayer == NULL)
		return false;

	if (!InTransitionVolume(pPlayer, pLandmarkName))
	{
		conprintf(2, "ChangeLevelNow(): player %s[%d] is not in the transition volume %s, aborting.\n", STRING(pPlayer->pev->netname), pPlayer->entindex(), pLandmarkName);
		return false;
	}

	edict_t	*pentLandmark = FindLandmark(pLandmarkName);

	if (g_pGameRules && !g_pGameRules->FAllowLevelChange(pPlayer, pMapName, pentLandmark))// XDM3035
		return false;

	// Create an entity to fire the changetarget
	if (!FStringNull(iszChangeTarget))
	{
		CFireAndDie *pFireAndDie = (CFireAndDie *)CBaseEntity::Create("fireanddie", pPlayer->pev->origin, g_vecZero);// XDM3038c
		if (pFireAndDie)
		{
			//pFireAndDie->pev->origin = pPlayer->pev->origin;
			pFireAndDie->pev->target = iszChangeTarget;
			pFireAndDie->m_flDelay = fChangeTargetDelay;
			DispatchSpawn(pFireAndDie->edict());
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy(g_szNextMap, pMapName);
	g_szNextMap[MAX_MAPNAME-1] = '\0';
	// TEST strlwr(g_szNextMap);// XDM3038c: WTF: HACK: FIX: Half-Life requires all map names in lowercase here!

	//m_hActivator = pActivator;
	//SUB_UseTargets(pActivator, USE_TOGGLE, 0.0f);
	g_szNextSpot[0] = 0;// Init landmark to NULL

	// TODO: UNDONE: this should be done through g_pGameRules && g_pGameRules->ChangeLevel(st_szNextMap, st_szNextSpot);!!

	// look for a landmark entity
	if (!FNullEnt(pentLandmark))
	{
		strcpy(g_szNextSpot, pLandmarkName);
		gpGlobals->vecLandmarkOffset = pentLandmark->v.origin;
	}

	//conprintf(1, "Level touches %d levels\n", ChangeList(levels, 16));
	SERVER_PRINTF("CHANGE LEVEL: %s %s\n", g_szNextMap, g_szNextSpot);

	// UNDONE: TESTME: g_pWorld = NULL;// XDM LAST ZOMFD!!!!!!!!!! crash prevention?
	CHANGE_LEVEL(g_szNextMap, g_szNextSpot);
	return true;
}






//-----------------------------------------------------------------------------
// XDM: this whole system is still not prefect!!
// It only supports ONE spot type for the game (map), but some shitty mods
// like AGCTF use different entities for different teams (which is stupid,
// but nobody cares). I don't want to spend my time digging into that shit.
//-----------------------------------------------------------------------------

// WARNING! Look carefully where these are used before changing!
spawnspot_t gSpawnSpotsSpectator[] =
{
	{"info_intermission", 0},
	{"light_environment", 0},
	{SUPERDEFAULT_SP_START, 0},
	{SUPERDEFAULT_MP_START, 0},
	{"info_landmark", 0},
	{"", 0}// MUST BE TERMINATED
};

spawnspot_t gSpawnSpotsSingleplayer[] =
{
	{SUPERDEFAULT_SP_START, 0},
	//{"info_landmark", 0}, XDM3038c: the quantity-based selection system will choose landmarks because there are two of them
	{"", 0}// MUST BE TERMINATED
};

/*spawnspot_t gSpawnSpotsChangeLevel[] =
{
	{"info_landmark", SPAWNSPOT_CLEARAREA},
	{"", 0}// MUST BE TERMINATED
};*/

// the rarest and best go first!
spawnspot_t gSpawnSpotsMultiplayer[] =
{
	{"info_ctfspawn",			SPAWNSPOT_RANDOMIZE|SPAWNSPOT_CLEARAREA},
	{SUPERDEFAULT_MP_START,		SPAWNSPOT_RANDOMIZE|SPAWNSPOT_CLEARAREA},
	{SUPERDEFAULT_SP_START,		SPAWNSPOT_DONTSAVE|SPAWNSPOT_CLEARAREA},
	{"", 0}// MUST BE TERMINATED
	// DO NOT add non-player-start entities here!
};

// experimental
spawnspot_t gSpawnSpotsCoOp[] =
{
	// XDM3038b	{SUPERDEFAULT_MP_START,		SPAWNSPOT_RANDOMIZE|SPAWNSPOT_CLEARAREA},
	// XDM3038b	{SUPERDEFAULT_SP_START,		SPAWNSPOT_DONTSAVE|SPAWNSPOT_CLEARAREA},
	{SUPERDEFAULT_SP_START,		SPAWNSPOT_RANDOMIZE|SPAWNSPOT_CLEARAREA},
	{SUPERDEFAULT_MP_START,		SPAWNSPOT_RANDOMIZE|SPAWNSPOT_CLEARAREA},
	{"", 0}// MUST BE TERMINATED
};

LINK_ENTITY_TO_CLASS(info_landmark, CPointEntity);// special case




//-----------------------------------------------------------------------------
//			CBasePlayerStart - base class for all start points
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(info_player_start, CBasePlayerStart);
LINK_ENTITY_TO_CLASS(info_player_deathmatch, CBasePlayerStart);
//LINK_ENTITY_TO_CLASS(info_player_teamspawn, CBasePlayerStart);// TFC compatibility
// TFC: info_tf_teamset, info_tfgoalm i_t_g
LINK_ENTITY_TO_CLASS(info_ctfspawn, CBasePlayerStart);// OP4CTF compatibility
LINK_ENTITY_TO_CLASS(info_player_team1, CBasePlayerStart);// stupid AGCTF compatibility
LINK_ENTITY_TO_CLASS(info_player_team2, CBasePlayerStart);

// pev->watertype = start type: PLAYERSTART_TYPE_RANDOM
void CBasePlayerStart::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "team_no"))// OP4CTF compatibility
	{
		pev->team = atoi(pkvd->szValue);// XDM3038a: typo?
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "starttype"))// all maps are using just "watertype" already :(
	{
		pev->watertype = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "checkpoint"))// XDM3035c: works like master
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);// XDM3038c: don't use "noise" because it is FIELD_SOUNDNAME
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))// XDM3038a: (alias for pev->message) works in CoOp
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: Reconfigure, revalidate self and let everyone know it via globals
//-----------------------------------------------------------------------------
void CBasePlayerStart::Spawn(void)
{
	// Convert to normal XDM player start
	if (strbegin(STRING(pev->classname), "info_player_team"))
	{
		if ((STRING(pev->classname))[16] == '1')
			pev->team = TEAM_1;
		else if ((STRING(pev->classname))[16] == '2')
			pev->team = TEAM_2;

		SetClassName(SUPERDEFAULT_MP_START);//pev->classname = MAKE_STRING("info_player_deathmatch");
	}
	if (g_pGameRules)
	{
		if (!g_pGameRules->IsTeamplay() || !g_pGameRules->IsValidTeam(pev->team))// XDM3038: kv's shouldn't confuse game logic!
			pev->team = TEAM_NONE;// because this may act as an activator for something and pass its team to that object!
	}
	if (!FStringNull(pev->message))// XDM3038b: for CoOp
		g_iMapHasLandmarkedSpawnSpots++;

	CBaseDelay::Spawn();//Precache();
	pev->movetype = MOVETYPE_NONE;// this ensures entity won't move even with non-zero velocity (which is possible)
	pev->solid = SOLID_NOT;
	pev->modelindex = 0;
	pev->model = 0;
	pev->effects = EF_NODRAW;
	pev->frame = 0;
	pev->takedamage = DAMAGE_NO;
	UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);// XDM3035b: should (only) be useful for realtime placement
}

//-----------------------------------------------------------------------------
// Purpose: Check spawn spot validity
// Input  : *pActivator - a player
// Output : Returns true if SPOT CHECKPOINT was passed by this player AND SPOT is NOT LOCKED
//-----------------------------------------------------------------------------
bool CBasePlayerStart::IsTriggered(const CBaseEntity *pActivator)
{
	if (!FStringNull(pev->netname))// XDM3035c: checkpoint must be activated for this to pass
	{
		if (pActivator->IsPlayer())// !!!
		{
			CBaseEntity *pCheckPoint = UTIL_FindEntityByTargetname(NULL, STRING(pev->netname));
			if (pCheckPoint)
			{
				if (((CBasePlayer *)pActivator)->PassedCheckPoint(pCheckPoint) == false)
					return false;
			}
		}
	}
	return !IsLockedByMaster(pActivator);// XDM3035c
}


//=========================================================
// XDM: spawn point selection
//=========================================================

//DLL_GLOBAL CBaseEntity	*g_pLastSpawnCleared = NULL;


//-----------------------------------------------------------------------------
// Purpose: Clear everything for player to spawn
// Input  : *pSpawnEntity - is going to be spawned here XDM3038b: not only players!
//			vecSpawnOrigin -
//			bClear - destroy everything in radius/box, or the function will fail
//			bKill - don't try to push, just kill
// Output : Returns true if spot is clear[ed]
//-----------------------------------------------------------------------------
bool SpawnPointCheckObstacles(CBaseEntity *pSpawnEntity, const Vector &vecSpawnOrigin, bool bClear, bool bKill)
{
	if (pSpawnEntity->IsPlayer())
	{
		if (((CBasePlayer*)pSpawnEntity)->IsObserver())
			return true;// XDM3038a: always valid for spectators
	}
	DBG_GR_PRINT("SpawnPointCheckObstacles(%d %s, %g %g %g, %s, %s)\n", pSpawnEntity->entindex(), STRING(pSpawnEntity->pev->netname), vecSpawnOrigin.x, vecSpawnOrigin.y, vecSpawnOrigin.z, bClear?"true":"false", bKill?"true":"false");
	CBaseEntity *pEntity = NULL;
	static CBaseEntity *pList[MAX_CLIENTS+4];// should be enough even for maps with just one start spot

	size_t count = UTIL_EntitiesInBox(pList, MAX_CLIENTS+4, vecSpawnOrigin + pSpawnEntity->pev->mins, vecSpawnOrigin + pSpawnEntity->pev->maxs, 0);
	//while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 64.0f)) != NULL)
	for (size_t i = 0; i < count; ++i)
	{
		pEntity = pList[i];
		if (pEntity->pev->solid == SOLID_NOT || pEntity->pev->solid == SOLID_TRIGGER)// something non-solid
			continue;
		if (pEntity == pSpawnEntity)// found self
			continue;
		if (pEntity->ShouldCollide(pSpawnEntity) == 0)// not an obstacle WARNING: normally we need to call top-level engine collision checking function, which will eventually call into these
			continue;
		if (pEntity->IsBreakable())// XDM3038a: in case someone pushes a breakable box to the spawn point
		{
			pEntity->TakeDamage(g_pWorld, g_pWorld, pEntity->pev->max_health*2.0f, DMG_CRUSH|DMG_IGNOREARMOR);
			continue;
		}
		/* FAIL! This destroys lifts and trains!
		if (pEntity->IsMovingBSP())
		{
			if (g_pGameRules && g_pGameRules->IsCoOp() && g_pGameRules->GetGameMode() == COOP_MODE_LEVEL)// XDM3038b: HACK! Helps on c1a0e where trigger is inside doors
			{
				DBG_PRINTF("SpawnPointCheckObstacles(%d %s, %g %g %g, %s): made %d %s non-solid!\n", pSpawnEntity->entindex(), STRING(pSpawnEntity->pev->netname), vecSpawnOrigin.x, vecSpawnOrigin.y, vecSpawnOrigin.z, bClear?"true":"false", pEntity->entindex(), STRING(pEntity->pev->classname));
				pEntity->pev->solid = SOLID_NOT;
				pEntity->pev->rendermode = kRenderTransTexture;//Color;
				pEntity->pev->renderamt = 127;
				pEntity->pev->rendercolor.Set(255,127,0);
				pEntity->pev->renderfx = kRenderFxPulseFast;
				continue;
			}
		}*/
		if (pEntity->IsPlayer() || pEntity->IsMonster())
		{
			if (bClear)
			{
				bool pushed = false;
				if (!bKill)// push
				{
					Vector vecAngle(0,0,0);
					Vector vecDir;
					Vector vecDest;
					int cnt;
					for (vecAngle.z=0; vecAngle.z <360; vecAngle.z += 45.0f)// search around where to push
					{
						AngleVectors(vecAngle, vecDir, NULL, NULL);
						vecDest = vecDir; vecDest *= (HULL_RADIUS+2); vecDest += vecSpawnOrigin;//vecDest = vecSpawnOrigin + vecDir * (HULL_RADIUS+2);
						//UTIL_DebugBeam(pSpot->pev->origin, vecDest, 2.0f);
						cnt = POINT_CONTENTS(vecDest);// should be a lot faster than trace
						if (cnt == CONTENTS_EMPTY || cnt == CONTENTS_WATER)
						{
							pushed = true;
							break;
						}
						else
							continue;
					}
					if (pushed)
					{
						pEntity->pev->origin = vecDest;
						pEntity->pev->velocity = vecDir;
						pEntity->pev->velocity *= 128.0f;
						UTIL_SetOrigin(pEntity, vecDest);
					}
				}
				if (pushed == false || bKill)
				{
					pEntity->TakeDamage(g_pWorld, g_pWorld, pEntity->pev->max_health*2.0f/* + pEntity->pev->armorvalue*/, (mp_instagib.value>0.0f)?(DMG_CRUSH|DMG_IGNOREARMOR):(DMG_IGNOREARMOR));// XDM3034: was VARS(INDEXENT(0)) | DMG_VAPOURIZING ? // XDM3035: gibbage // XDM3038c: DMG_DISINTEGRATING leaves no weapons and causes lots of frustration
					// delay respawn of it a little bit
					if (pEntity->IsPlayer())
						((CBasePlayer *)pEntity)->m_fDiedTime += 0.5f * RANDOM_LONG(0,6);// XDM3038c: randomize ot prevent synchro-death on LMS start, quantize time by 0.5s
					else if (pEntity->pev->nextthink > 0)
						pEntity->pev->nextthink += 2.0f;
				}
			}
			else
			{
				//conprintf(2, "ValidateSpawnPoint(%d %d %d) failed: occupied by %d!\n", pSpawnEntity?pSpawnEntity->entindex():0, pSpot->entindex(), clear, pEntity->entindex());
				return false;// if ent is a client, don't spawn on 'em
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if the spot is ready for use
// Input  : *pSpawnEntity - is going to be spawned here XDM3038b: not only players!
//			*pSpot - info_player_wtf
//			clear - destroy everything in radius/box, or the function will fail
// Output : Returns true if spot is clear[ed]
//-----------------------------------------------------------------------------
bool SpawnPointValidate(CBaseEntity *pSpawnEntity, CBaseEntity *pSpot, bool clear)
{
	if (pSpot == NULL)
		return false;

	// WTF!! Some mappers don't care about leaving untriggered spawn spots! So ignore this rule if desperate!
	if (!pSpot->IsTriggered(pSpawnEntity) && !clear)
	{
		//conprintf(2, "SpawnPointValidate(%d %d %d) rejected: not triggered\n", pSpawnEntity?pSpawnEntity->entindex():0, pSpot->entindex(), clear);
		return false;
	}

	// XDM3038a: validate in terms of game rules
	if (g_pGameRules && !g_pGameRules->ValidateSpawnPoint(pSpawnEntity, pSpot))
	{
		//conprintf(2, "SpawnPointValidate(%d %d %d) rejected: by game rules\n", pSpawnEntity?pSpawnEntity->entindex():0, pSpot->entindex(), clear);
		return false;
	}

	return SpawnPointCheckObstacles(pSpawnEntity, pSpot->pev->origin, clear, (mp_spawnkill.value > 0.0f));
}


DLL_GLOBAL spawnspot_t		*g_pSpotList = NULL;
DLL_GLOBAL CBaseEntity		*g_pLastSpawn[MAX_TEAMS+1] = {NULL,NULL,NULL,NULL,NULL};
DLL_GLOBAL size_t			g_iSpawnPointTypePlayers = 0;
DLL_GLOBAL size_t			g_iSpawnPointTypeSpectators = 0;
DLL_GLOBAL size_t			g_iMapHasLandmarkedSpawnSpots = 0;

//-----------------------------------------------------------------------------
// Purpose: Determine which spawn spots will be used in this map
//-----------------------------------------------------------------------------
void SpawnPointInitialize(void)
{
	if (g_pGameRules && g_pGameRules->GetGameType() == GT_COOP)// XDM3035c: experimental
		g_pSpotList = gSpawnSpotsCoOp;
	else if (IsMultiplayer())
		g_pSpotList = gSpawnSpotsMultiplayer;
	//else if (gpGlobals->startspot)// has targetname // This function is not used during level change anyway
	//	g_pSpotList = gSpawnSpotsChangeLevel;
	else
		g_pSpotList = gSpawnSpotsSingleplayer;

	// NEW system: now we detect which spot type to use on this map. Other possible types will be ignored.
	size_t numtypes = 0;
	while (g_pSpotList[numtypes].classname[0] != '\0')// count number of spot items in the list
		++numtypes;

	size_t iMostSpotsIndex = 0;
	size_t *szSPTCounts = new size_t[numtypes];
	CBaseEntity *pEntity;
	// Count each type, use the type present in the largest amount
	for (g_iSpawnPointTypePlayers = 0; g_iSpawnPointTypePlayers < numtypes; ++g_iSpawnPointTypePlayers)// try all available entity types sequentially
	{
		szSPTCounts[g_iSpawnPointTypePlayers] = 0;
		pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, g_pSpotList[g_iSpawnPointTypePlayers].classname)) != NULL)
			szSPTCounts[g_iSpawnPointTypePlayers]++;//break;// found prioritized type

		DBG_PRINTF("SpawnPointInitialize(): found %u \"%s\"\n", szSPTCounts[g_iSpawnPointTypePlayers], g_pSpotList[g_iSpawnPointTypePlayers].classname);
		if (szSPTCounts[g_iSpawnPointTypePlayers] > szSPTCounts[iMostSpotsIndex])
			iMostSpotsIndex = g_iSpawnPointTypePlayers;
	}
	if (szSPTCounts[iMostSpotsIndex] == 0)//if (g_iSpawnPointTypePlayers == numtypes)// nothing found, invalid index
	{
		SERVER_PRINT("Warning: no spawn spots suitable for players found on this map!\n");
		g_iSpawnPointTypePlayers = 0;
	}
	else
		g_iSpawnPointTypePlayers = iMostSpotsIndex;

	// Now find a spot spectators
	numtypes = 0;
	while (gSpawnSpotsSpectator[numtypes].classname[0] != '\0')// count number of spot items in the list
		++numtypes;
	// Here we don't count. But should we?
	for (g_iSpawnPointTypeSpectators = 0; g_iSpawnPointTypeSpectators < numtypes; ++g_iSpawnPointTypeSpectators)// try all available entity types sequentially
		if (UTIL_FindEntityByClassname(NULL, gSpawnSpotsSpectator[g_iSpawnPointTypeSpectators].classname))
			break;// found prioritized type

	if (g_iSpawnPointTypeSpectators == numtypes)// nothing found, invalid index
	{
		SERVER_PRINT("Warning: no spawn spots suitable for spectators found on this map!\n");
		g_iSpawnPointTypeSpectators = 0;
	}

	SERVER_PRINTF("SpawnPointInitialize(): \"%s\" (%u) will be used for map \"%s\".\n", g_pSpotList[g_iSpawnPointTypePlayers].classname, szSPTCounts[iMostSpotsIndex], STRING(gpGlobals->mapname));
	delete [] szSPTCounts;
	szSPTCounts = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity to spawn at
// PROBLEM: If spots are arranged like all for team1 then all for team2, then we're in serious trouble: all spots of opposing team will be invalidated and cycle will always restart from THE FIRST SPOT!!!
// USES g_pSpotList, g_usSpawnPointType AND SETS GLOBAL g_pLastSpawn
// Input  : *pPlayer - for whom
//			bSpectator - force select spectator spot
// Output : CBaseEntity *pSpot
//-----------------------------------------------------------------------------
CBaseEntity *SpawnPointEntSelect(CBaseEntity *pPlayer, bool bSpectator)
{
	CBaseEntity *pSpot = NULL;
	CBaseEntity *pStartSpot = NULL;
	//CBaseEntity **pLastSpotPtr = NULL;
	spawnspot_t *pSpotList = NULL;// pointer to a selected list (array)
	size_t etype;// g_iSpawnPointTypeXXX
	unsigned short searchpass = 0;// 0 - allowed to skip, 1 - force clear the way
	unsigned short retries = 2;
	bool bSomethingFound = 0;// XDM3037a
	bool save;// save this spot as "last used spot" so next player won't use it

 	if (bSpectator || g_pGameRules && g_pGameRules->IsTeamplay() && pPlayer->pev->team == TEAM_NONE)// XDM: unassigned players (spectators)
	{
		pSpotList = gSpawnSpotsSpectator;
		etype = g_iSpawnPointTypeSpectators;
		save = false;// don't save last spot for spectators
		pSpot = NULL;
	}
	else
	{
		pSpotList = g_pSpotList;
		etype = g_iSpawnPointTypePlayers;
		save = true;
		if (g_pLastSpawn[TEAM_NONE])// last time teamless spawn point was used
			pSpot = g_pLastSpawn[TEAM_NONE];
		else
			pSpot = g_pLastSpawn[pPlayer->pev->team];
	}
	ASSERT(pSpotList != NULL);

	if (g_pGameRules && g_pGameRules->SpawnSpotUsePolicy() == SPAWNSPOT_FORCE_CLEAR)// XDM3037: like in other games: don't skip occupied spots
		searchpass = retries-1;

	pStartSpot = pSpot;
	for (; searchpass<retries; ++searchpass)// start wiping out only after searching ALL of these spots
	{
		// No! Otherwise entities from NULL to LastSpawn will be skipped!	pSpot = pStartSpot;// restart
		bSomethingFound = false;
		while ((pSpot = UTIL_FindEntityByClassname(pSpot, pSpotList[etype].classname)) != pStartSpot)//*pLastSpotPtr)// search all spots of this type
		{
			if (pSpot == NULL)
			{
				if (bSomethingFound)
					continue;// just restart
				else if (pStartSpot == NULL)// we searched from scratch and
					break;// nothing was found, prevent endless loop!
			}
			bSomethingFound = true;
			if (searchpass == 0 && (pSpotList[etype].flags & SPAWNSPOT_RANDOMIZE) && RANDOM_LONG(0,1) == 0)// try randomizing for the first time
				continue;// bad luck, skip

			if (SpawnPointValidate(pPlayer, pSpot, ((pSpotList[etype].flags & SPAWNSPOT_CLEARAREA) && (searchpass > 0))))// clear out the area only on the second pass
			{
				//if (pSpotList[etype].flags & SPAWNSPOT_DONTSAVE)// TESTME!
				//	save = FALSE;

				goto sps_end;// break all cycles, save and exit
				break;
			}
		}
		/*if (g_pGameRules && g_pGameRules->SpawnSpotUsePolicy() == SPAWNSPOT_CLEARAREA_WAIT)// XDM3038a: LMS uses this
		{
			return NULL;// XDM3038a: TODO UNDONE: players who connected first will be telekilled if there's no enough spots (usual case)
			break;
		}*/
	}

sps_end:
	ASSERTSZ(pSpot != NULL, "Error: SpawnPointEntSelect: no spawn spot!\n");
	if (pSpot)
	{
		if (save)
		{
			if (g_pGameRules && g_pGameRules->IsTeamplay())// XDM3038: kv's shouldn't confuse game logic!
			{
				g_pLastSpawn[pSpot->pev->team] = pSpot;
				if (pSpot->pev->team != TEAM_NONE)// we used team-specific point, clear teamless pointer
					g_pLastSpawn[TEAM_NONE] = NULL;
			}
			else
				g_pLastSpawn[TEAM_NONE] = pSpot;
		}
	}
	return pSpot;
}


//-----------------------------------------------------------------------------
// Purpose: Checks current game rules to match entity's m_iszGameRulesPolicy
// Input  : *pEntity - allowed string is "!* !1 m !6", everything is optional
// ! - NOT operator (disallow following value).
// * - mask for all. Example: "* !5" - allow everywhere except GT_CTF
// m - mask for multiplayer. Example: "!* m" or "m !0" - only in multiplayer
// All numbers (IDs) must be separated by space (acceptable by isspace())
//
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VerifyGameRulesPolicy(const char *pPolicyString)
{
	if (!pPolicyString || *pPolicyString == '\0')//if (FStringNull(pEntity->m_iszGameRulesPolicy))
		return true;

	//const char *pPolicyString = STRING(pEntity->m_iszGameRulesPolicy);
	const char *pMask = strchr(pPolicyString, '*');//strstr(pPolicyString, "*");

	char PolicyDefault = POLICY_UNDEFINED;// global
	if (pMask)
	{
		// Make sure this is not the first character in policy string
		if ((pMask > pPolicyString) && (*(pMask - 1) == '!'))//char *pNegative = strchr(pPolicyString, '!');// reversed rule
			PolicyDefault = POLICY_DENY;
		else// mask is found and untouched
			PolicyDefault = POLICY_ALLOW;
	}

	char PolicyMP = POLICY_UNDEFINED;// multiplayer
	pMask = strchr(pPolicyString, 'm');
	if (pMask)
	{
		// Make sure this is not the first character in policy string
		if ((pMask > pPolicyString) && (*(pMask - 1) == '!'))// reversed rule
			PolicyMP = POLICY_DENY;
		else
			PolicyMP = POLICY_ALLOW;
	}

	if (g_pGameRules)// may be NULL during transition!
	{
		char sGT[8];
		_snprintf(sGT, 8, "%d", g_pGameRules->GetGameType());
		sGT[7] = '\0';
		const char *pFound = NULL;
		const char *pSearchStart = pPolicyString;
		do// make sure this is not a part of other decimal number (3 is not part of 13)
		{
			pFound = strstr(pSearchStart, sGT);
			if (pFound)
				pSearchStart = pFound + 1;// can this exceed the string length?
		}
		while (pFound && (pFound > pPolicyString) && isdigit(*(pFound - 1))/*!isspace(*(pFound - 1))*/);// && pSearchStart < end)

		if (pFound)// Highest priority: found current game rules ID
		{
			if ((pFound > pPolicyString) && (*(pFound - 1) == '!'))// reversed rule
				return false;// DENY
			else
				return true;// ALLOW
		}
	}

	// Next priority: multiplayer policy
	if (IsMultiplayer())//gpGlobals->deathmatch)// NOT g_pGameRules && g_pGameRules->IsMultiplayer()
	{
		if (PolicyMP == POLICY_ALLOW)
			return true;
		else if (PolicyMP == POLICY_DENY)
			return false;
		// else undefined
	}

	// Lowest priority: global policy
	/*if (PolicyDefault == POLICY_ALLOW)
		return true;
	else */if (PolicyDefault == POLICY_DENY)
		return false;

	return true;// there is no "completely undefined" case
}
