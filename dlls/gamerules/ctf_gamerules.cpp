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
#include "ctf_gamerules.h"
#include "globals.h"
#include "color.h"// XDM3037a
#include "hltv.h"

#define CTF_OBJECT_CLASSNAME		"info_capture_obj"
#define CTF_BASE_CLASSNAME			"trigger_cap_point"


//-----------------------------------------------------------------------------
// Purpose: First thing called after constructor. Initialize all data, cvars, etc.
// Warning: Called from CWorld::Precache(), so all other entities are not spawned yet!
//-----------------------------------------------------------------------------
void CGameRulesCTF::Initialize(void)
{
	DBG_GR_PRINT("CGameRulesCTF::Initialize()\n");
	m_iNumCaptureZones = 0;// XDM3037: fasterizer
	CGameRulesTeamplay::Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize client HUD
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesCTF::InitHUD(CBasePlayer *pPlayer)
{
	DBG_GR_PRINT("CGameRulesCTF::InitHUD(%d %s)\n", pPlayer->entindex(), STRING(pPlayer->pev->netname));
	CGameRulesTeamplay::InitHUD(pPlayer);
	if (!pPlayer->IsBot())
	{
		// send flags info
		short c = 0;
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, CTF_OBJECT_CLASSNAME)) != NULL)
		{
			//PLAYBACK_EVENT_FULL(FEV_RELIABLE|FEV_HOSTONLY, pPlayer->edict(), g_usCaptureObject, 0.0, pEntity->pev->origin, pEntity->pev->angles, 0.0, 0.0, pEntity->entindex(), CTF_EV_INIT, pEntity->pev->team, 0);
			MESSAGE_BEGIN(MSG_ONE, gmsgFlagInfo, NULL, pPlayer->edict());
				WRITE_SHORT(pEntity->entindex());
				WRITE_BYTE(pEntity->pev->team);
			MESSAGE_END();
			// not here: called manny times! SetTeamBaseEntity(pEntity->pev->team, pEntity);// XDM3035: 20091122
			++c;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: client has been disconnected
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CGameRulesCTF::ClientDisconnected(CBasePlayer *pPlayer)
{
	if (pPlayer && pPlayer->m_pCarryingObject)
		pPlayer->m_pCarryingObject->Use(pPlayer, pPlayer, USE_TOGGLE/*COU_DROP*/, 0.0);

	CGameRulesTeamplay::ClientDisconnected(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: Get team with the best score (extrascore version)
// Output : TEAM_ID
//-----------------------------------------------------------------------------
TEAM_ID CGameRulesCTF::GetBestTeam(void)
{
	return GetBestTeamByScore();// redirect to proper counter
}

//-----------------------------------------------------------------------------
// Purpose: Score limit for this game type
//-----------------------------------------------------------------------------
uint32 CGameRulesCTF::GetScoreLimit(void)
{
	//DBG_GR_PRINT("CGameRulesCTF::GetScoreLimit()\n");
	if (mp_capturelimit.value < 0.0f)// XDM3038: TODO: revisit, use better conversion
		mp_capturelimit.value = 0.0f;
	else
		mp_capturelimit.value = (int)mp_capturelimit.value;

	return (uint32)mp_capturelimit.value;
}




//=================================================================
// CCaptureObject
// WARNING! Team indexes start from 1! (TEAM_1)
//=================================================================

LINK_ENTITY_TO_CLASS(info_capture_obj, CCaptureObject);
// that's just great! more classnames to dump into the export table!
LINK_ENTITY_TO_CLASS(item_ctfflag, CCaptureObject);// Opposing Force CTF compatibility
LINK_ENTITY_TO_CLASS(item_flag_team1, CCaptureObject);// stupid AGCTF compatibility
LINK_ENTITY_TO_CLASS(item_flag_team2, CCaptureObject);// stupid AGCTF compatibility

void CCaptureObject::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "goal_no"))// OP4CTF compatibility
	{
		pev->team = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

void CCaptureObject::Spawn(void)
{
	/* XDM3038c: TESTME experiment if (g_pGameRules->GetGameType() != GT_CTF)// important check, because of pointer casts later
	{
		conprintf(2, "CCaptureObject: removed because of game rules mismatch\n");
		pev->flags = FL_KILLME;//Destroy();
		return;
	}*/
	if (!FClassnameIs(pev, CTF_OBJECT_CLASSNAME))// XDM3038: does not really matter how did we get here
	{
		if (strbegin(STRING(pev->classname), "item_flag_team"))// stupid AGCTF compatibility
		{
			if ((STRING(pev->classname))[14] == '1')
				pev->team = TEAM_1;
			else if ((STRING(pev->classname))[14] == '2')
				pev->team = TEAM_2;
		}
		SetClassName(CTF_OBJECT_CLASSNAME);//pev->classname = MAKE_STRING(CTF_OBJECT_CLASSNAME);
	}
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	pev->takedamage = DAMAGE_NO;

	ASSERT(!FStringNull(pev->model));// XDM3038
	SET_MODEL(edict(), STRING(pev->model));

	// let the model define size?
	UTIL_SetSize(this, CAPTUREOBJECT_SIZE_MINS, CAPTUREOBJECT_SIZE_MAXS);
	UTIL_SetOrigin(this, pev->origin);

	//SetBits(pev->flags, FL_DRAW_ALWAYS);// XDM3034: TESTME: always send position to clients!
	//pev->renderfx = kRenderFxGlowShell;
	pev->renderamt = 255;//4;

	if (pev->team == TEAM_1)
	{
		pev->targetname = MAKE_STRING(CTF_OBJ_TARGETNAME1);
	}
	else if (pev->team == TEAM_2)
	{
		pev->targetname = MAKE_STRING(CTF_OBJ_TARGETNAME2);
	}
	else // if (!g_pGameRules->IsValidTeam(pev->team))
	{
		pev->team = TEAM_NONE;
	}
	pev->skin = pev->team;
	pev->body = pev->team;
	if (g_pGameRules)
	{
		if (g_pGameRules->GetTeamBaseEntity(pev->team) == NULL)// capture OBJECT can NOT override "base" in rules, but capture ZONE CAN
			g_pGameRules->SetTeamBaseEntity(pev->team, this);
	}
	byte r = 255, g = 255, b = 255;
	if (pev->team == TEAM_NONE)
	{
		pev->colormap = COLORMAP_XHL_LOGO & 0x0000FFFF;
	}
	else
	{
		GetTeamColor(pev->team, r,g,b);
		pev->rendercolor.Set(r,g,b);
		pev->colormap = RGB2colormap(r,g,b) & 0x0000FFFF;// XDM3037a: if flag uses special textures (DM_Base/RemapX) Warning! 2 bytes to 4!
	}

	if (g_pGameRules && g_pGameRules->GetGameType() == GT_CTF)//g_pGameRules->IsMultiplayer())
		pev->effects |= EF_DIMLIGHT;// XDM3038a

	m_vecSpawnSpot = pev->origin;
	// XDM3035c: avoid using these!	pev->startpos = pev->origin;
	pev->v_angle = pev->angles;
	pev->sequence = FANIM_POSITION;
	pev->animtime = gpGlobals->time + 0.5f;
	Reset(NULL);
	SetThink(&CCaptureObject::CaptureObjectThink);
	pev->impulse = CO_STAY;
	SetNextThink(0.5);// XDM3038a
}

void CCaptureObject::Precache(void)
{
	if (FStringNull(pev->model))// XDM3038
		pev->model = MAKE_STRING("models/flag.mdl");

	//pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	CBaseAnimating::Precache();

	PRECACHE_SOUND("game/ctf_alarm.wav");
	//PRECACHE_SOUND("game/ctf_captured.wav");
}

void CCaptureObject::Touch(CBaseEntity *pOther)
{
	if (g_pGameRules->IsGameOver())// XDM3035a: don't react after the game has ended
		return;

	//conprintf(1, "CCaptureObject::Touch(%d %s %d)\n", pOther->entindex(), STRING(pOther->pev->netname), pOther->pev->team);
	if (pev->impulse == CO_CARRIED)
		return;

	if (!pOther->IsPlayer())
		return;

	if (!pOther->IsAlive())
		return;

	if (pOther->pev->team == pev->team)
	{
		if (pev->impulse == CO_DROPPED)// pick up our dropped flag
		{
			Return(pOther);
		}
		else if (pev->impulse == CO_STAY)// teammate touched home flag
		{
			// consider this as touchnig the trigger
			CBasePlayer *pPlayer = (CBasePlayer *)pOther;
			if (pPlayer->m_pCarryingObject)
			{
				//if (UTIL_FindEntityByClassname(NULL, CTF_BASE_CLASSNAME) == NULL)// SLOW!!! only if there's no real trigger! This may ruin mapper's intentions.
				if (g_pGameRules->GetGameType() == GT_CTF)// extra check
				{
					if (((CGameRulesCTF *)g_pGameRules)->m_iNumCaptureZones == 0)
						pPlayer->m_pCarryingObject->Use(pOther, this, (USE_TYPE)COU_CAPTURE, 0.0f);
				}
			}
		}
	}
	else
	{
		if (pev->dmgtime <= gpGlobals->time)// prevent self-touching
			Taken(pOther);
	}
}

void CCaptureObject::CaptureObjectThink(void)
{
	if (pev->impulse == CO_DROPPED)
	{
		if (pev->teleport_time > 0.0f && pev->teleport_time <= gpGlobals->time)
			Return(NULL);
	}
	else if (pev->impulse == CO_CARRIED)
	{
		if (pev->aiment)// player
		{
			pev->origin = pev->aiment->v.origin;

			if (pev->aiment->v.velocity.Length() > PLAYER_MAX_WALK_SPEED)// value from CBasePlayer::SetAnimation
				pev->sequence = FANIM_CARRIED;
			else
				pev->sequence = FANIM_CARRIED_IDLE;
		}
		else// ?!!
			Return(NULL);
	}
	SetNextThink(0.1);// XDM3038a
}

void CCaptureObject::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (useType == COU_CAPTURE)
	{
		Captured(pActivator);
	}
	else if (useType == COU_TAKEN)
	{
		Taken(pActivator);
	}
	else if (useType == COU_DROP)
	{
		Drop(pActivator);
		//if (value > 0.0)
		//	pev->velocity *= value;
	}
	else if (useType == COU_RETURN)
	{
		Return(pActivator);
	}
	else
	{
		conprintf(2, "CCaptureObject: unknown use type %d\n", useType);
	}
}

int CCaptureObject::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	if (pev->impulse == CO_DROPPED)
	{
		if (pInflictor && pInflictor->IsBSPModel())
		{
			if (!pAttacker || !pAttacker->IsPlayer())
			{
				pev->health -= flDamage;
				if (pev->health <= 0.0f)
					Return(NULL);// this will call Reset() and restore pev->health

				return 1;
			}
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: never let this object be destroyed!
//-----------------------------------------------------------------------------
void CCaptureObject::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	//conprintf(2, "CCaptureObject: Killed()!\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
// Output : int
//-----------------------------------------------------------------------------
int CCaptureObject::ShouldCollide(CBaseEntity *pOther)// XDM3036
{
	if (pOther->IsPlayer() && !pOther->IsAlive())// ignore/fall through dead players
		return 0;

	return CBaseAnimating::ShouldCollide(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: apply physics on dropped flags
//-----------------------------------------------------------------------------
bool CCaptureObject::IsPushable(void)
{
	//if (pev->impulse == CO_DROPPED)// XDM3037: really bad idea
	//	return TRUE;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: detach flag from a player (if any) and reset state to 'not carried'
// Input  : *pPlayer - a player the flag will be detached from (can be NULL)
//-----------------------------------------------------------------------------
void CCaptureObject::Reset(CBaseEntity *pPlayer)
{
	if (pPlayer && pPlayer->IsPlayer())
	{
		CBasePlayer *pClient = (CBasePlayer *)pPlayer;
		if (pClient)
		{
			if (pClient->m_pCarryingObject == this)// WARNING: Reset() may be called for returning ANOTHER, NON-CARRIED player team's flag!
				pClient->m_pCarryingObject = NULL;
		}
	}

	pev->health = 100;
	pev->movetype = MOVETYPE_NONE;
	pev->aiment = NULL;
	pev->solid = SOLID_TRIGGER;
	pev->sequence = FANIM_NOT_CARRIED;
	pev->owner = NULL;
	//m_hOwner = NULL;
	m_hActivator = NULL;// XDM3037: all other functions call this
	ResetSequenceInfo();
	SUB_UseTargets(pPlayer, USE_OFF, 1.0f);// XDM3035a: in case mapper wants to use this
}

//-----------------------------------------------------------------------------
// Purpose: Drop and detach flag
// Input  : *pPlayer - a player that dropped the flag (can be NULL)
//-----------------------------------------------------------------------------
void CCaptureObject::Drop(CBaseEntity *pPlayer)
{
	edict_t *e = NULL;
	if (pPlayer)
	{
		pev->origin = pPlayer->pev->origin;
		UTIL_SetOrigin(this, pev->origin);
		if (pPlayer->IsAlive())// player used 'drop' command
		{
			//bugbug	pev->origin = pPlayer->pev->origin + gpGlobals->v_forward * 32;
			Vector fwd;
			AngleVectors(pPlayer->pev->angles, fwd, NULL, NULL);
			pev->velocity = pPlayer->pev->velocity + fwd * 160.0f;
		}
		else
		{
			pev->velocity = pPlayer->pev->velocity * 0.85f;
		}
		SetBits(pev->flags, FL_DRAW_ALWAYS);// send to client, light need to follow this entity!
		pev->dmgtime = gpGlobals->time + 0.6f;// prevent self-touching

		//test	UTIL_ClientPrintAll(HUD_PRINTTALK, "* %s dropped %s flag\n", STRING(pPlayer->pev->netname), g_pGameRules->GetTeamName(pev->team));
		if (sv_lognotice.value > 0)
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" dropped \"%s\" flag\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), g_pGameRules->GetTeamName(pPlayer->pev->team), g_pGameRules->GetTeamName(pev->team));

		e = pPlayer->edict();
	}
	else
	{
		//test	UTIL_ClientPrintAll(HUD_PRINTTALK, "* %s flag dropped\n", g_pGameRules->GetTeamName(pev->team));
		if (sv_lognotice.value > 0)
			UTIL_LogPrintf("\"%s\" flag dropped\n", g_pGameRules->GetTeamName(pev->team));
	}

	Reset(pPlayer);
	pev->impulse = CO_DROPPED;
	pev->movetype = MOVETYPE_TOSS;
	pev->sequence = FANIM_ON_GROUND;
	pev->framerate = 1;
	pev->solid = SOLID_TRIGGER;// XDM3035a
	pev->teleport_time = gpGlobals->time + mp_flagstay.value;
	//SET_MODEL(edict(), STRING(pev->model));// XDM3037: this will update size
	UTIL_SetSize(this, CAPTUREOBJECT_SIZE_MINS, CAPTUREOBJECT_SIZE_MAXS);
	PLAYBACK_EVENT_FULL(FEV_RELIABLE|FEV_GLOBAL|FEV_UPDATE, e, g_usCaptureObject, 0.0, pev->origin, pev->angles, mp_flagstay.value, 0.0, entindex(), CTF_EV_DROP, pev->team, 0);
	ResetSequenceInfo();
}

//-----------------------------------------------------------------------------
// Purpose: Return flag to its base
// Input  : *pPlayer - the one who returns this flag (can be NULL)
//-----------------------------------------------------------------------------
void CCaptureObject::Return(CBaseEntity *pPlayer)
{
	Reset(pPlayer);// <-- pev->framerate = 1;
	pev->impulse = CO_STAY;
	pev->origin = m_vecSpawnSpot;
	pev->angles = pev->v_angle;
	pev->sequence = FANIM_POSITION;
	pev->frame = 0;
	UTIL_SetOrigin(this, pev->origin);
	//SET_MODEL(edict(), STRING(pev->model));// XDM3037: this will update size
	UTIL_SetSize(this, CAPTUREOBJECT_SIZE_MINS, CAPTUREOBJECT_SIZE_MAXS);

	edict_t *e = NULL;
	if (pPlayer == NULL)
	{
		if (sv_lognotice.value > 0)
			UTIL_LogPrintf("\"%s\" flag returned\n", g_pGameRules->GetTeamName(pev->team));
	}
	else
	{
		if (pPlayer->IsPlayer())
			((CBasePlayer *)pPlayer)->m_Stats[STAT_FLAG_RETURN_COUNT]++;// XDM3037

		e = pPlayer->edict();
		//test	UTIL_ClientPrintAll(HUD_PRINTTALK, "* %s returned %s flag\n", STRING(pPlayer->pev->netname), g_pGameRules->GetTeamName(pev->team));
		if (sv_lognotice.value > 0)
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" returned \"%s\" flag\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), g_pGameRules->GetTeamName(pPlayer->pev->team), g_pGameRules->GetTeamName(pev->team));
	}

	PLAYBACK_EVENT_FULL(FEV_RELIABLE|FEV_GLOBAL|FEV_UPDATE, e, g_usCaptureObject, 0.0, pev->origin, pev->angles, 0.0, 0.0, entindex(), CTF_EV_RETURN, pev->team, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Flag is being captured, add team score and reset the flag
// Input  : *pPlayer - the one who captured this flag
//-----------------------------------------------------------------------------
void CCaptureObject::Captured(CBaseEntity *pPlayer)
{
	if (pPlayer == NULL)
		return;

	PLAYBACK_EVENT_FULL(FEV_RELIABLE|FEV_GLOBAL|FEV_UPDATE, pPlayer->edict(), g_usCaptureObject, 0.0, pev->origin, pev->angles, 0.0, 0.0, entindex(), CTF_EV_CAPTURED, pev->team, 0);

	if (pPlayer->IsPlayer())// Do this before adding actual score or the game may end
	{
		((CBasePlayer *)pPlayer)->m_Stats[STAT_FLAG_CAPTURE_COUNT]++;// XDM3037a: add before finishing
		SetBits(((CBasePlayer *)pPlayer)->m_iGameFlags, EGF_REACHEDGOAL);// XDM3038c
	}
	g_pGameRules->AddScoreToTeam(pPlayer->pev->team, CAPTUREOBJECT_SCORE);// TODO: variable score?

	if (g_pGameRules->IsGameOver())// XDM3037a: don't reset flag, let the camera show the winner
		return;

	// We don't need this message if game is over. There'll be DRC_FLAG_FINAL
	MESSAGE_BEGIN(MSG_SPEC, svc_director);
		WRITE_BYTE(9);// command length in bytes
		WRITE_BYTE(DRC_CMD_EVENT);
		WRITE_SHORT(pPlayer->entindex());// index number of primary entity
		WRITE_SHORT(entindex());// index number of secondary entity
		WRITE_LONG(14 | DRC_FLAG_DRAMATIC | DRC_FLAG_FACEPLAYER);// eventflags (priority and flags)
	MESSAGE_END();

	Reset(pPlayer);
	if (sv_lognotice.value > 0)
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" captured \"%s\" flag\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), g_pGameRules->GetTeamName(pPlayer->pev->team), g_pGameRules->GetTeamName(pev->team));

	pev->origin = m_vecSpawnSpot;
	pev->angles = pev->v_angle;
	pev->impulse = CO_STAY;
	UTIL_SetOrigin(this, pev->origin);
}

//-----------------------------------------------------------------------------
// Purpose: Flag was picked up by someone
// Input  : *pPlayer - must be somebody!
//-----------------------------------------------------------------------------
void CCaptureObject::Taken(CBaseEntity *pPlayer)
{
	if (pPlayer == NULL)
		return;
	if (!pPlayer->IsPlayer())
		return;

	CBasePlayer *pClient = (CBasePlayer *)pPlayer;
	if (pClient)
	{
		if (pClient->m_pCarryingObject == NULL)
		{
			pClient->m_pCarryingObject = this;
			m_hActivator = pClient;// XDM3037
			//m_hOwner = pClient;
			m_flFrameRate = pClient->m_flFrameRate;
		}
		else
		{
			DBG_PRINTF("%s[%d] failed to attach: player %d already has CarryingObject!\n", STRING(pev->classname), entindex(), pPlayer->entindex());
			return;
		}
	}

	if (sv_lognotice.value > 0)
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" takes \"%s\" flag\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), g_pGameRules->GetTeamName(pPlayer->pev->team), g_pGameRules->GetTeamName(pev->team));

	pev->impulse = CO_CARRIED;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	if (pPlayer->pev->framerate > 0)// ???
		pev->framerate = pPlayer->pev->framerate;
	else
		pev->framerate = 1.0f;

	pev->movetype = MOVETYPE_FOLLOW;
	pev->sequence = FANIM_CARRIED;
	pev->takedamage = DAMAGE_NO;
	UTIL_SetSize(this, g_vecZero, g_vecZero);// XDM3037: ?
	PLAYBACK_EVENT_FULL(FEV_RELIABLE|FEV_GLOBAL|FEV_UPDATE, pPlayer->edict(), g_usCaptureObject, 0.0, pev->origin, pev->angles, 0.0, 0.0, entindex(), CTF_EV_TAKEN, pev->team, 0);

	SUB_UseTargets(pPlayer, USE_ON, 1.0f);// XDM3035a: in case mapper wants to use this
}





//=================================================================
// CCaptureZone
// A brush entity (trigger) to carry the flag to
// WARNING! Team indexes start from 1!!!
//=================================================================

LINK_ENTITY_TO_CLASS(trigger_cap_point, CCaptureZone);
LINK_ENTITY_TO_CLASS(item_ctfbase, CCaptureZone);// OP4CTF compatibility

void CCaptureZone::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "goal_no"))// OP4CTF compatibility
	{
		pev->team = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CCaptureZone::Spawn(void)
{
	if (g_pGameRules->GetGameType() == GT_CTF)
	{
		((CGameRulesCTF *)g_pGameRules)->m_iNumCaptureZones++;// let gamerules know about capture zone existence
	}
	else
	{
		conprintf(2, "CCaptureZone: removed because of game rules mismatch\n");
		pev->flags = FL_KILLME;//Destroy();
		return;
	}
	//SetClassName(CTF_BASE_CLASSNAME);//	pev->classname = ALLOC_STRING(CTF_BASE_CLASSNAME);
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	pev->takedamage = DAMAGE_NO;
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetOrigin(this, pev->origin);
	pev->angles = pev->v_angle;
	DontThink();// XDM3037

	if (pev->team == TEAM_1)
		pev->target = MAKE_STRING(CTF_OBJ_TARGETNAME1);
	else if (pev->team == TEAM_2)
		pev->target = MAKE_STRING(CTF_OBJ_TARGETNAME2);

	if (g_pGameRules)
		g_pGameRules->SetTeamBaseEntity(pev->team, this);

	if ((STRING(pev->model))[0] == '*')// is this a trigger?
	{
		if (showtriggers.value <= 0.0f)
			SetBits(pev->effects, EF_NODRAW);
	}
}

// OP4CTF compatibility: this entity may be both a trigger and a studio model
void CCaptureZone::Precache(void)
{
	pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
}

void CCaptureZone::Touch(CBaseEntity *pOther)
{
	if (g_pGameRules->IsGameOver())// XDM3035a: don't react after the game has ended
		return;

	if (!pOther->IsPlayer())
		return;

	if (!pOther->IsAlive())// pev->deadflag != DEAD_NO)
		return;

	if (pev->team > TEAM_NONE && pOther->pev->team != pev->team)// allow pev->team == 0 - universal capture point! XDM3034
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (pPlayer->m_pCarryingObject)
	{
		if (pPlayer->m_pCarryingObject->pev->team != pev->team)// both indexes start from 1
		{
			edict_t *pEnt = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));
			if (!FNullEnt(pEnt) && pEnt->v.impulse == CO_STAY)// local flag found and is at base
				pPlayer->m_pCarryingObject->Use(pOther, this, (USE_TYPE)COU_CAPTURE, 0.0f);
		}
	}
}

// OP4CTF compatibility
class CBaseCTFDetect : public CPointEntity
{
public:
	virtual void KeyValue(KeyValueData *pkvd);
};

LINK_ENTITY_TO_CLASS(info_ctfdetect, CBaseCTFDetect);

/* OP4CTF compatibility keys:
	score_icon_namebm
	score_icon_nameof
	basedefenddist
	defendcarriertime
	captureassisttime
	poweruprespawntime
	map_score_max*/
void CBaseCTFDetect::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "flagreturntime"))
	{
		if (atof(pkvd->szValue) > 0.0f)
			CVAR_DIRECT_SET(&mp_flagstay, pkvd->szValue);

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "poweruprespawntime"))
	{
		if (atof(pkvd->szValue) > 0.0f)
			CVAR_DIRECT_SET(&mp_itemrespawntime, pkvd->szValue);

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "basedefenddist"))
	{
		pev->speed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}
