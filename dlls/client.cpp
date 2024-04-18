/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
// Half-Life-compatible S2C interface
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "maprules.h"
#include "gamerules.h"
#include "game.h"
#include "custom.h"
#include "customentity.h"
#include "weapons.h"
#include "globals.h"
#include "movewith.h"
#include "items.h"
#include "entconfig.h"
#include "entity_state.h"
#include "usercmd.h"
#include "pm_shared.h"
#include "pm_materials.h"
#include "voice_gamemgr.h"

extern CVoiceGameMgr g_VoiceGameMgr;

DLL_GLOBAL bool g_ServerActive = false;
unsigned char g_ClientShouldInitialize[MAX_CLIENTS+1];// XDM3035a: HACK to re-init after level change
//static int g_iPlayerSerialNumber = 1;

//-----------------------------------------------------------------------------
// Purpose: XHL depends on serialnumber to detect disconnected players
// Input  : *pEntity -
//-----------------------------------------------------------------------------
void ClientAssignSerialNumber(edict_t *pEntity)
{
	pEntity->serialnumber = GETPLAYERUSERID(pEntity);// XDM3038: if this provides really unique numbers for EVERY connected player, it's really what we want
	//ASSERT(pEntity->serialnumber != 0);
	if (pEntity->serialnumber == 0)
	{
		pEntity->serialnumber = ENTINDEX(pEntity);
		conprintf(1, "Warning: player %d couldn't get User ID!\n", pEntity->serialnumber);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player connects to a server
// Note   : In singleplayer this is called only once, not every map change!
// Input  : *pEntity -
//-----------------------------------------------------------------------------
BOOL ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	DBG_SV_PRINT("ClientConnect(sn %d, %s, %s)\n", pEntity->serialnumber, pszName, pszAddress);// looks bad and distracts, szRejectReason);
	if (!FBitSet(pEntity->v.flags, FL_SPECTATOR))
	{
		if (!FStringNull(pEntity->v.netname))
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" is connecting\n", STRING(pEntity->v.netname), GETPLAYERUSERID(pEntity), GETPLAYERAUTHID(pEntity), GET_INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(pEntity), "team"));
	}
	// WARNING: wa cannot get instance because players do not have private data yet
	if (IsMultiplayer())//if (gpGlobals->maxClients > 1)// causes problems in sp
	{
		SetBits(pEntity->v.effects, EF_NODRAW);// XDM3038
		pEntity->v.origin.z = -4095;// XDM3038c
	}
	if (g_pGameRules)// some buggy engines needs this
	{
		if (g_pGameRules->ClientConnected(pEntity, pszName, pszAddress, szRejectReason))
		{
			// XDM3037: one place is enough!	g_ClientShouldInitialize[ENTINDEX(pEntity)] = 1;// XDM3035c
			// XDM3038a: game rules decide		pEntity->v.frags = 0;
			// DON'T!	pEntity->v.flags = 0;
			ClearBits(pEntity->v.flags, FL_NOTARGET);// XDM3038
			ClearBits(pEntity->v.spawnflags, SF_NOREFERENCE|SF_NORESPAWN);// XDM3038
			ClientAssignSerialNumber(pEntity);
			return TRUE;
		}
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player disconnects from a server
// Input  : *pEntity -
//-----------------------------------------------------------------------------
void ClientDisconnect(edict_t *pEntity)
{
	DBG_SV_PRINT("ClientDisconnect(sn %d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(pEntity));
	if (pSound)// since this client isn't around to think anymore, reset their sound.
		pSound->Reset();

	if (!FBitSet(pEntity->v.flags, FL_SPECTATOR))
	{
		if (!FStringNull(pEntity->v.netname))
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" disconnected\n", STRING(pEntity->v.netname), GETPLAYERUSERID(pEntity), GETPLAYERAUTHID(pEntity), GET_INFO_KEY_VALUE(GET_INFO_KEY_BUFFER(pEntity), "team"));
	}
	// since the edict doesn't get deleted, fix it so it won't interfere.
	if (IsMultiplayer())//if (gpGlobals->maxClients > 1)
	{
		pEntity->v.origin.Set(0,0,-4095);//-MAX_ABS_ORIGIN+1.0f);
		pEntity->v.oldorigin = pEntity->v.origin;
		pEntity->v.velocity.Clear();
		pEntity->v.basevelocity.Clear();
	}
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pEntity);// XDM
	if (pPlayer)
	{
		pPlayer->m_iSpawnState = SPAWNSTATE_UNCONNECTED;
		pPlayer->CheckPointsClear();// XDM3038

		if (g_pGameRules)
			g_pGameRules->ClientDisconnected(pPlayer);
	}
	//pEntity->v.modelindex = 0;
	//pEntity->v.model = iStringNull;// XDM3037a: safe?
	pEntity->v.nextthink = 0;
	pEntity->v.movetype = MOVETYPE_NONE;
	pEntity->v.solid = SOLID_NOT;
	pEntity->v.effects = EF_NOINTERP | EF_NODRAW;// XDM3035
	pEntity->v.health = 0;
	pEntity->v.frags = 0;
	pEntity->v.weapons = 0;
	pEntity->v.takedamage = DAMAGE_NO;// don't attract autoaim
	pEntity->v.deadflag = DEAD_DEAD;
	SetBits(pEntity->v.spawnflags, SF_NOREFERENCE|SF_NORESPAWN);
	pEntity->v.flags = FL_NOTARGET;// XDM3038
	pEntity->v.team = TEAM_NONE;
	pEntity->v.netname = iStringNull;// XDM3037: so IsActivePlayer() will fail
	pEntity->v.groupinfo = 0;// XDM3038c: reset collision group info
	pEntity->v.iuser1 = 0;
	pEntity->v.euser2 = NULL;// XDM3037: this edict is no longer watching through any entity
	SET_ORIGIN(pEntity, pEntity->v.origin);
	pEntity->serialnumber = 0;// XDM3038: SAFE?! EHANDLE MUST FAIL!!!
}

//-----------------------------------------------------------------------------
// Purpose: Player entered the suicide command
// Input  : *pEntity -
//-----------------------------------------------------------------------------
void ClientKill(edict_t *pEntity)
{
	DBG_SV_PRINT("ClientKill(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	if (g_pGameRules == NULL || g_pGameRules->IsGameOver())// XDM3037
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pEntity);
	if (pPlayer)
	{
		if (pPlayer->m_fNextSuicideTime > gpGlobals->time)
			return;// prevent suiciding too ofter

		pPlayer->m_fNextSuicideTime = gpGlobals->time + 5.0f; // don't let them suicide for 5 seconds after suiciding
		// have the player kill themself
		pPlayer->pev->health = 0.0f;
		pPlayer->Killed(pPlayer, pPlayer, (mp_instagib.value > 0.0f)?GIB_ALWAYS:GIB_DISINTEGRATE);// XDM3035b just for fun :)
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after successful Connect(), once per map
// Note   : In singleplayer this is called only once, not every map change!
// Input  : *pEntity -
//-----------------------------------------------------------------------------
void ClientPutInServer(edict_t *pEntity)
{
	DBG_SV_PRINT("ClientPutInServer(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	g_ClientShouldInitialize[ENTINDEX(pEntity)] = 1;// XDM3037
	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)VARS(pEntity), "player");// XDM
	if (pPlayer)
	{
		if (pEntity->serialnumber == 0)
			ClientAssignSerialNumber(pEntity);

		pPlayer->SetCustomDecalFrames(-1);// Assume none;
		if (pPlayer->m_iSpawnState == SPAWNSTATE_UNCONNECTED)// XDM3037a
			pPlayer->m_iSpawnState = SPAWNSTATE_CONNECTED;
		else
			conprintf(0, "ClientPutInServer(): pPlayer->m_iSpawnState = %d!\n", pPlayer->m_iSpawnState);

		// Allocate a CBasePlayer for pev, and call spawn
		pPlayer->Spawn(FALSE);// XDM3035
		// Reset interpolation during first frame
		pPlayer->pev->effects |= EF_NOINTERP;
		pPlayer->m_Stats[STAT_MAPS_COUNT]++;// XDM3038c: once per map!
	}
	else
	{
		DBG_FORCEBREAK
	}
}

#define MAX_SAYTEXT		MAX_USER_MSG_DATA-2
//-----------------------------------------------------------------------------
// Purpose: pEntity says something to the chat
// XDM3035: now that was a huge part of a bullshit
// Input  : *pEntity - the one who talks
//			*pText - "hello world"
//			teamonly - 
//-----------------------------------------------------------------------------
void Host_Say(edict_t *pEntity, const char *pText, bool teamonly)
{
	if (pEntity == NULL || pText == NULL)
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pEntity);//GetClassPtr((CBasePlayer *)VARS(pEntity));
	if (pPlayer->m_flNextChatTime > gpGlobals->time)
		return;

	size_t textlen = strlen(pText);
	if (textlen < 1)
		return;

	const char *p = pText;// NEW
	if (*p == '"')// remove surrounding quotes if present
	{
		++p;
		//p[strlen(p)-1] = 0;
		if (pText[textlen-1] == '"')// closing quote
			--textlen;

		--textlen;
	}

	if (textlen < 1)// re-check after removing quotes, very useful
		return;
	else if (textlen > MAX_SAYTEXT)// XDM3037a: overflow check
		textlen = MAX_SAYTEXT;

	// make sure the text has content
	// HL20130901
	if (!p || !p[0])
		return;  // no character found, so say nothing

	/*if (g_iProtocolVersion <= 46)// XDM3037a: old non-unicode system accepts unicode strings, but doesn't print them
	{
		const char *pc = NULL;
		for (pc = p; pc != NULL && *pc != 0; ++pc)
		{
			if (isprint(*pc) && !isspace(*pc))
			{
				pc = NULL;	// we've found an alphanumeric character,  so text is valid
				break;
			}
		}
		if (pc != NULL)
			return;
	}
	else*/// if (g_iProtocolVersion > 46)
	{
		if (!Q_UnicodeValidate(p))
			return;
	}

	char text[MAX_SAYTEXT];// XDM3037a
	//XDM3038a: no more	text[0] = 2;// flag for client saytext parser
	//XDM3038a	text[1] = 0;// nullterm
	//strncat(text, p, textlen);
	strncpy(text, p, textlen);
	text[textlen] = '\0';

	pPlayer->m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;

	// ignore this rule if not in team game
	if (g_pGameRules && !g_pGameRules->IsTeamplay())
		teamonly = false;

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop
	CBasePlayer *client = NULL;
	for (CLIENT_INDEX i = 1; i <= gpGlobals->maxClients; ++i)
	{
		client = UTIL_ClientByIndex(i);
		if (client == NULL)
			continue;
		if (!client->IsNetClient())// Not a client? (should never be true)
			continue;
		if (g_pGameRules && !g_pGameRules->IsGameOver())// XDM3035c: everyone talks during intermission
		{
			if (pPlayer->IsObserver() && !client->IsObserver())
				continue;
		}
		// can the receiver hear the sender? or has he muted him?
		if (g_VoiceGameMgr.PlayerHasBlockedPlayer(client, pPlayer))
			continue;

		if (teamonly && g_pGameRules && g_pGameRules->IsTeamplay())// skip all non-teammates
		{
			int r = g_pGameRules->PlayerRelationship(client, CBaseEntity::Instance(pEntity));
			if (r != GR_TEAMMATE && r != GR_ALLY)
				continue;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, client->edict());
			WRITE_BYTE(ENTINDEX(pEntity) | (teamonly?SAYTEXT_BIT_TEAMONLY:0));// XDM3035: last bit means team only. Enough for current MAX_PLAYERS
			WRITE_STRING(text);
		MESSAGE_END();
	}
	if (sv_logchat.value > 0.0f)// XDM
		UTIL_LogPrintf("\"%s (%d)<%d><%d>\"%s \"%s\"\n", STRING(pEntity->v.netname), pEntity->v.team, GETPLAYERUSERID(pEntity), GETPLAYERAUTHID(pEntity), teamonly?" (team)":"", p);
}


//-----------------------------------------------------------------------------
// Purpose: When a player enters console command, it is parsed by client DLL,
// and, if not handled, passed down to server here.
// CMD_ARGS - get all arguments, excluding the command itself
// CMD_ARGV - get argument (0 is the command itself)
// CMD_ARGC - number of strings including the command
// Warning: potential stack overflow point!
// Input  : *pEntity - client who issued a command
//-----------------------------------------------------------------------------
void ClientCommand(edict_t *pEntity)
{
	// Is the client spawned yet?
	if (pEntity->pvPrivateData == NULL)
		return;

	const char *pcmd = CMD_ARGV(0);
	const char *arg1 = CMD_ARGV(1);
	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pEntity);//GET_PRIVATE(pEntity);
	if (!pPlayer)
		return;

	//DBG_PRINTF("ClientCommand(%d): ARGC: %d, ARGV(0): %s, ARGS: \"%s\"\n", pPlayer->entindex(), CMD_ARGC(), pcmd, CMD_ARGS());
	if (strbegin(pcmd, "weapon_"))// XDM3035: nobody should use this now
	{
		pPlayer->SelectItem(pcmd);
	}
	else if (FStrEq(pcmd, "select"))// XDM3038: more intuitive
	{
		pPlayer->SelectItem(arg1);
	}
	else if (FStrEq(pcmd, "drop"))
	{
		if (/* XDM3038c g_pGameRules->GetGameType() == GT_CTF && */pPlayer->m_pCarryingObject && CMD_ARGC() <= 1)
			pPlayer->m_pCarryingObject->Use(pPlayer, pPlayer, USE_TOGGLE, 0.0f);
		else// player is dropping an item.
			pPlayer->DropPlayerItem((char *)CMD_ARGV(1));
	}
	else if (FStrEq(pcmd, "holster"))// XDM3038: should never get here. Left for bots.
	{
		if (pPlayer->m_pActiveItem)
		{
			if (pPlayer->m_pActiveItem->IsHolstered())// don't act during initialization/restore
				pPlayer->DeployActiveItem();
			else if (!pPlayer->m_fInitHUD && pPlayer->m_pActiveItem->CanHolster())
				pPlayer->m_pActiveItem->Holster();
		}
	}
	else if (FStrEq(pcmd, ".u") && CMD_ARGC() > 2)// Use entity by mouse click. TODO: integrate with CBasePlayer::PlayerUse()
	{
		if (CMD_ARGC() > CMDARG_USE_VALUE)
		{
			CBaseEntity *pObject = UTIL_EntityByIndex(atoi(CMD_ARGV(CMDARG_USE_ENTINDEX)));
			if (pObject && pObject != g_pWorld)// && pPlayer->FVisible(pObject->Center()))// prevent cheating??
			{
				int caps = pObject->ObjectCaps();
				if (FBitSet(caps, FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
				{
					Vector vecSrc(pPlayer->EyePosition()), vecDst(pObject->Center());
					if ((vecDst - vecSrc).Length() <= PLAYER_USE_SEARCH_RADIUS)// prevent remote usage
					{
						if (FBitSet(caps, FCAP_ONLYDIRECT_USE))
						{
							TraceResult tr;
							UTIL_TraceLine(vecSrc, vecDst, ignore_monsters, dont_ignore_glass, pPlayer->edict(), &tr);// UNDONE: trace to object's closest plane, not center
							if (tr.flFraction < 1.0f && tr.pHit != pObject->edict())
								return;
						}
						pPlayer->PlayerUseObject(atoi(CMD_ARGV(CMDARG_USE_STATE)), pObject, atof(CMD_ARGV(CMDARG_USE_VALUE)));
					}
				}
			}
		}
	}
	else if (FStrEq(pcmd, "say"))
	{
		Host_Say(pEntity, CMD_ARGS(), false);
	}
	else if (FStrEq(pcmd, "say_team"))
	{
		Host_Say(pEntity, CMD_ARGS(), true);
	}
	else if (FStrEq(pcmd, "say_forward"))
	{
		if (CMD_ARGC() > 1)
		{
			if (pPlayer->IsObserver())
			{
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "#MSG_NO_SPECTATORS");
			}
			else
			{
				CBaseEntity *pClient = UTIL_FindEntityForward(pPlayer);
				if (pClient && pClient->IsPlayer())
				{
					if (g_pGameRules && g_pGameRules->IsTeamplay() && g_pGameRules->PlayerRelationship(pPlayer, pClient) != GR_TEAMMATE)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#MSG_NO_ENEMIES");
					}
					else
					{
						MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pClient->edict());
							WRITE_BYTE(ENTINDEX(pEntity));
							WRITE_STRING(CMD_ARGS());// CMD_ARGV(1));
						MESSAGE_END();
						ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#MSG_SENT_TO", STRING(pClient->pev->netname));
					}
				}
				else
					ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "#MSG_NO_CLIENT");
			}
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Usage: %s <\"text\">\n", pcmd);
	}
	else if (FStrEq(pcmd, "sayto"))
	{
		if (CMD_ARGC() > 2)
		{
			CBasePlayer *pClient;
			if (*arg1 == '#')
				pClient = UTIL_ClientByIndex(atoi(arg1+1));
			else
				pClient = UTIL_ClientByName(arg1);

			if (pClient)
			{
				if (pPlayer->IsObserver() && !pClient->IsObserver())
				{
					ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#MSG_NO_SPECTATORS");
				}
				else if (g_pGameRules && g_pGameRules->IsTeamplay() && g_pGameRules->PlayerRelationship(pPlayer, pClient) != GR_TEAMMATE)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#MSG_NO_ENEMIES");
				}
				else
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pClient->edict());
						WRITE_BYTE(ENTINDEX(pEntity));
						WRITE_STRING(CMD_ARGV(2));
					MESSAGE_END();
					ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#MSG_SENT_TO", STRING(pClient->pev->netname));
				}
			}
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "#MSG_NO_CLIENT");
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Usage: %s <#index|name> <\"text\">\n", pcmd);
	}
	else if (FStrEq(pcmd, "fullupdate"))
	{
		pPlayer->ForceClientDllUpdate();
	}
	else if (FStrEq(pcmd, "speaksound"))
	{
		if (arg1)
		{
			if (pPlayer->IsAlive())// dead don't talk )
			{
				int nargs = CMD_ARGC();
				MESSAGE_BEGIN(MSG_BROADCAST, gmsgSpeakSnd, NULL);
					WRITE_SHORT(pPlayer->entindex());
					WRITE_BYTE(nargs>2 ? atoi(CMD_ARGV(2)):255);// vol
					WRITE_BYTE(nargs>3 ? atoi(CMD_ARGV(3)):PITCH_NORM);// pitch
					WRITE_STRING(arg1);
				MESSAGE_END();
			}
		}
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "Usage: %s <\"soundname\"> [volume 1-255] [pitch 1-255]\n", pcmd);
	}
	else if (g_pGameRules && g_pGameRules->ClientCommand(pPlayer, pcmd))
	{
	}
	else if (FStrEq(pcmd, "use"))// Who uses this?
	{
		pPlayer->SelectItem((char *)CMD_ARGV(1));
	}
	else
	{
		char command[128];
		strncpy(command, pcmd, 128);
		command[127] = '\0';
		ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "#CMD_UNKNOWN\n", command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after the player changes userinfo - gives dll a chance to modify it before it gets sent into the rest of the engine.
// Input  : *pEntity -
//			*infobuffer -
//-----------------------------------------------------------------------------
void ClientUserInfoChanged(edict_t *pEntity, char *infobuffer)
{
	// Is the client spawned yet?
	if (!pEntity->pvPrivateData)
		return;

	DBG_SV_PRINT("ClientUserInfoChanged(%d %s, %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname), infobuffer);

	char *pValue = GET_INFO_KEY_VALUE(infobuffer, "name");
	// Notify everyone if someone changes their name, and it isn't the first time (changing no name to current name)
	if (pValue && !FStringNull(pEntity->v.netname) && STRING(pEntity->v.netname)[0] != '\0' && !FStrEq(STRING(pEntity->v.netname), pValue))
	{
		char szName[256];
		strncpy(szName, pValue, sizeof(szName)-1);
		szName[sizeof(szName)-1] = '\0';
		// First parse the name and remove any %'s
		for (char *pApersand = szName; pApersand != NULL && *pApersand != 0; ++pApersand)
		{
			if (*pApersand == '%')
				*pApersand = ' ';// Replace it with a space
		}
		SET_CLIENT_KEY_VALUE(ENTINDEX(pEntity), infobuffer, "name", szName);

		if (IsMultiplayer())
		{
			ClientPrint(NULL, HUD_PRINTTALK, "+ #CL_CHNAME\n", STRING(pEntity->v.netname), szName);
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" changed name to \"%s\"\n", STRING(pEntity->v.netname), GETPLAYERUSERID(pEntity), GETPLAYERAUTHID(pEntity), GET_INFO_KEY_VALUE(infobuffer, "team"), szName);
		}
	}
	pValue = GET_INFO_KEY_VALUE(infobuffer, "model");
	if (pValue)
	{
		char szName[256];
		_snprintf(szName, 255, "models/player/%s/%s.mdl", pValue, pValue);
		szName[255] = '\0';
		if (!UTIL_FileExists(szName))
		{
			ClientPrint(&pEntity->v, HUD_PRINTDEFAULT, "#CL_NOMODEL\n", pValue);
			SET_CLIENT_KEY_VALUE(ENTINDEX(pEntity), infobuffer, "model", "");// this resets to default model in the engine
			CVAR_SET_STRING("model", "");
		}
	}

	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pEntity);//GetClassPtr((CBasePlayer *)VARS(pEntity));
	if (pPlayer)
	{
		//pPlayer->m_iLocalWeapons = atoi(GET_INFO_KEY_VALUE(infobuffer, "cl_lw"));// XDM3038c
		if (g_pGameRules)
			g_pGameRules->ClientUserInfoChanged(pPlayer, infobuffer);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Note: Every call to ServerActivate should be matched by a call to ServerDeactivate
//-----------------------------------------------------------------------------
void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	DBG_SV_PRINT("ServerActivate(%d %d)\n", edictCount, clientMax);
	int i;
	CBaseEntity *pClass;
	if (g_ServerActive == false)
	{
		g_ServerActive = true;
		// Link user messages here to make sure first client can get them...
		LinkUserMessages();
		edict_t *pEdict;
		// Clients have not been initialized yet
		for (i = 0; i < edictCount; ++i)
		{
			pEdict = &pEdictList[i];
			if (pEdict->free)
				continue;

			// Clients aren't necessarily initialized until ClientPutInServer()
			if (i <= clientMax)
			{
				// XDM3037: one place is enough!	g_ClientShouldInitialize[i] = 1;// XDM3035a
				continue;
			}
			if (pEdict->pvPrivateData == NULL)
				continue;

			pClass = CBaseEntity::Instance(pEdict);
			// Activate this entity if it's got a class & isn't dormant
			if (pClass && !FBitSet(pEdict->v.flags, FL_DORMANT))
				pClass->Activate();
			else
				conprintf(1, "ServerActivate(): Can't instance %s\n", STRING(pEdict->v.classname));
		}

		ENTCONFIG_ExecMapConfig();// XDM3035b: execute map cfg file AFTER game rules have been installed AND ALL entitieas have been spawned!

		if (g_pGameRules)
		{
			SpawnPointInitialize();
			g_pGameRules->ServerActivate(pEdictList, edictCount, clientMax);// call this last
		}
		else
			conprintf(0, "ServerActivate(): no GameRules!\n");
	}
	PM_InitMaterials(STRING(gpGlobals->mapname));// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ServerDeactivate(void)
{
	DBG_SV_PRINT("ServerDeactivate()\n");
	SERVER_PRINT("XDM: ServerDeactivate()\n");
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate
	if (!g_ServerActive)
		return;

	g_ServerActive = false;
	// Peform any shutdown operations here...

	CVAR_SET_STRING("sv_skyname", "");// XDM3038: reset sky on level change
	// XDM3037: hopefully, this will help the engine
	if (g_pGameRules == NULL || !IsMultiplayer())
	{
		gpGlobals->deathmatch = 0;
		gpGlobals->coop = 0;
		gpGlobals->teamplay = 0;
		gpGlobals->serverflags = 0;
		gpGlobals->found_secrets = 0;
		gpGlobals->maxClients = 1;
	}
}

// a cached version of gpGlobals->frametime. The engine sets frametime to 0 if the player is frozen... so we just cache it in prethink,
// allowing it to be restored later and used by CheckDesiredList.
float g_flCachedFrametime = 0.0f;

//TEST float g_PrePitch = 0.0f;

//-----------------------------------------------------------------------------
// Purpose: Called every frame before physics are run
// Input  : *pEntity -
//-----------------------------------------------------------------------------
void PlayerPreThink(edict_t *pEntity)
{
	DBG_SV_PRINT("PlayerPreThink(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	//TEST if (ENTINDEX(pEntity) == 1)
	//	g_PrePitch = pEntity->v.angles[PITCH];
	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pEntity);//GET_PRIVATE(pEntity);
	if (pPlayer)
	{
#if !defined (SV_NO_PITCH_CORRECTION)
		pPlayer->pev->angles.x *= -PITCH_CORRECTION_MULTIPLIER;// XDM3038c
#endif
		pPlayer->PreThink();
	}
	g_flCachedFrametime = gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame after physics are run
// Input  : *pEntity -
//-----------------------------------------------------------------------------
void PlayerPostThink(edict_t *pEntity)
{
	DBG_SV_PRINT("PlayerPostThink(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	/*static lasttime;
	conprintf(1, "PlayerPostThink delta %g\n", gpGlobals->time - lasttime);
	lasttime = gpGlobals->time;*/

	//TEST if (ENTINDEX(pEntity) == 1)
	//	conprintf(2, "PlayerPostThink(): angles[PITCH] pre: %f, post: %f\n", g_PrePitch, pEntity->v.angles[PITCH]);

	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pEntity);//GET_PRIVATE(pEntity);
	if (pPlayer)
	{
#if !defined (SV_NO_PITCH_CORRECTION)
		pPlayer->pev->angles.x *= -PITCH_CORRECTION_MULTIPLIER;// XDM3038c
#endif
		pPlayer->PostThink();
		// XDM3038c: prevent ugly spinning
		NormalizeValueF(&pPlayer->pev->punchangle.x, -30.0f, 30.0f);
		NormalizeValueF(&pPlayer->pev->punchangle.y, -30.0f, 30.0f);
		NormalizeValueF(&pPlayer->pev->punchangle.z, -30.0f, 30.0f);
	}

#if defined(MOVEWITH)
	gpGlobals->frametime = g_flCachedFrametime;// restore valid frame time for SHL stuff
	CheckDesiredList();// SHL
#endif
}

//-----------------------------------------------------------------------------
// Purpose: unknown
//-----------------------------------------------------------------------------
void ParmsNewLevel(void)
{
	DBG_SV_PRINT("ParmsNewLevel()\n");
}

//-----------------------------------------------------------------------------
// Purpose: unknown
//-----------------------------------------------------------------------------
void ParmsChangeLevel(void)
{
	DBG_SV_PRINT("ParmsChangeLevel()\n");

	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	if (pSaveData)
		pSaveData->connectionCount = BuildChangeList(pSaveData->levelList, MAX_LEVEL_CONNECTIONS);
}

//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
/*#if defined (_DEBUG)
#include <sys/timeb.h>
	struct _timeb timebuffer;
	double sv_time = 0.0;
	double sv_timeprev = 0.0;
	double sv_frametime = 0.0;
#endif*/
//extern char g_szTestValue[32];
//extern edict_t *g_pTestPointer;


//-----------------------------------------------------------------------------
// Purpose: Called by the engine at the START of every frame
//-----------------------------------------------------------------------------
void StartFrame(void)
{
	DBG_SV_PRINT("StartFrame()\n");

	/* XDM3038c: simple hacky memory change tester	if (g_pTestPointer)
	{
		if (!FStrEq(g_szTestValue, STRING(g_pTestPointer->v.target)))
		{
			conprintf(2, "TEST POINTER VALUE CHANGED TO %s!!!\n", STRING(g_pTestPointer->v.target));
			DBG_FORCEBREAK
		}
	}*/
	// always crashes during loading because all pointers are invalid
	/*if (g_ServerActive && gpGlobals->time > 20 && gpGlobals->maxEntities > 0 && g_pWorld && g_pGameRules)// CRASH PREVENTION TESTING ONLY!
	{
		if (g_pGameRules->GetGameState() == GAME_STATE_ACTIVE)
		{
			int i = gpGlobals->maxClients;// skip players
			edict_t *pEdict = INDEXENT(i);
			for (; i < gpGlobals->maxEntities; ++i, pEdict++)
			{
				if (UTIL_IsValidEntity(pEdict))
				{
					CBaseEntity *pEntity = CBaseEntity::Instance(pEdict);
					if (pEntity)
					{
						if (pEntity->pev->solid != SOLID_NOT && (FStringNull(pEntity->pev->model) || pEntity->pev->modelindex == 0))
							conprintf(0, "ERROR: entity %d %s %s with invalid solid setting!!!\n", pEntity->entindex(), STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname));
					}
					// WARNING: This is VERY, VERY VERY VERY dangerous!! TEST ONLY
					if (pEntity->m_flRemoveTime > 0.0f && pEntity->m_flRemoveTime <= gpGlobals->time)// XDM3038a: we check this AFTER Think()ing so user code may intercept this
					{
						ASSERT(pEntity->GetExistenceState() != ENTITY_EXSTATE_CONTAINER && pEntity->GetExistenceState() != ENTITY_EXSTATE_CARRIED);// what about containers?
						DBG_SV_PRINT("DispatchThink(%d %s): entity removed by m_flRemoveTime\n", pEdict->serialnumber, STRING(pEntity->pev->classname));
						//pEntity->UpdateOnRemove();
					}
				}
			}
		}
	}*/

	if (g_pGameRules)
		g_pGameRules->StartFrame();

	/*if (g_fGameOver)
		return;

#if defined (_DEBUG)
	_ftime(&timebuffer);
	sv_time = (double)timebuffer.time + ((double)timebuffer.millitm/1000);// is this a right thing to do?
	sv_frametime = sv_time - sv_timeprev;
	sv_timeprev = sv_time;

	if (test1.value > 9000 && sv_frametime > 1.000)
	{
		SERVER_PRINTF(">>>>> WHAT THE BUG?!!!!! FRAME TIME %f TOO LONG!!\n", sv_frametime);
 		DBG_FORCEBREAK
	}
#endif*/
	//g_ulFrameCount++;
#if defined(MOVEWITH)
	CheckAssistList();
#endif

	if (pSoundEnt)
		if (pSoundEnt->m_fNextUpdate <= gpGlobals->time)// XDM3038a
			pSoundEnt->Think();
}

// If it's defined in a .C file
// Must be extern "C" in ALL CPP FILES and NOT defined in headers!
/*extern "C" */
extern const char **gStepSounds[];

//-----------------------------------------------------------------------------
// Purpose: Precache common resources used by players
//-----------------------------------------------------------------------------
void ClientPrecache(void)
{
	DBG_SV_PRINT("ClientPrecache()\n");

	PRECACHE_MODEL("models/player.mdl");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, VEC_HULL_MIN, VEC_HULL_MAX, "models/player.mdl");// XDM3035

	PRECACHE_SOUND("player/pl_fallpain1.wav");// XDM
	PRECACHE_SOUND("player/pl_fallpain2.wav");
	PRECACHE_SOUND("player/pl_fallpain3.wav");
	PRECACHE_SOUND("player/pl_jump1.wav");
	PRECACHE_SOUND("player/pl_jump2.wav");
	PRECACHE_SOUND("player/pl_swim1.wav");// breathe bubbles
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");
	PRECACHE_SOUND("player/sprayer.wav");

	for (uint32 m=0; m<STEP_LAST; ++m)// XDM3035a
	{
		for (uint32 i=0; i<NUM_STEP_SOUNDS; ++i)
		{
			if (gStepSounds[m])
				PRECACHE_SOUND((char *)gStepSounds[m][i]);
		}
	}

	PRECACHE_SOUND("plats/train_use1.wav");// use a train
	PRECACHE_SOUND("items/flashlight1.wav");// XDM

	// XDM: these are reused on server side
	// player use sounds
	PRECACHE_SOUND("common/wpn_select.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");

	// XDM3038c: WARNING: NOTE: modern HL seems to work fine enough without these. It allows us to run HLDS with large maps like CO_AI03
	/*PRECACHE_SOUND("player/geiger6.wav");
	PRECACHE_SOUND("player/geiger5.wav");
	PRECACHE_SOUND("player/geiger4.wav");
	PRECACHE_SOUND("player/geiger3.wav");
	PRECACHE_SOUND("player/geiger2.wav");
	PRECACHE_SOUND("player/geiger1.wav");*/

	//if (IsMultiplayer())
	// CL	PRECACHE_SOUND("player/respawn.wav");
}

//-----------------------------------------------------------------------------
// Purpose: Returns the descriptive name of this game visible in server browser
// Output : constant terminated string
//-----------------------------------------------------------------------------
const char *GetGameDescription(void)
{
	DBG_SV_PRINT("GetGameDescription()\n");
	if (g_pGameRules) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "(initializing)";
}

//-----------------------------------------------------------------------------
// Purpose: Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
// Input  : *error_string -
//-----------------------------------------------------------------------------
void Sys_Error(const char *error_string)
{
	//DBG_FORCEBREAK
	//puts(error_string);
	perror(error_string);
	// Default case, do nothing. MOD AUTHORS: Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
	//SERVER_PRINT(error_string);
}
// "Error: lightmap for texture aaatrigger too large (3 x 192 = 576 luxels); cannot exceed 324"
// "Not enough memory to load map images (1)."
// "Texture Overflow: MAX_GLTEXTURES"

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization(edict_t *pEntity, customization_t *pCust)
{
	DBG_SV_PRINT("PlayerCustomization(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pEntity);//GET_PRIVATE(pEntity);
	if (!pPlayer)
	{
		conprintf(0, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		conprintf(0, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2); // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		conprintf(0, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
}

/*
================
SpectatorConnect
A spectator has joined the game
================
*/
void SpectatorConnect(edict_t *pEntity)
{
	DBG_SV_PRINT("SpectatorConnect(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);
	if (pPlayer)
		pPlayer->SpectatorConnect();
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect(edict_t *pEntity)
{
	DBG_SV_PRINT("SpectatorDisconnect(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);
	if (pPlayer)
		pPlayer->SpectatorDisconnect();
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink(edict_t *pEntity)
{
	DBG_SV_PRINT("SpectatorThink(%d %s)\n", pEntity->serialnumber, STRING(pEntity->v.netname));
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);
	if (pPlayer)
		pPlayer->SpectatorThink();
}


////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used. Otherwise, the current
entity's origin is used. Either is offset by the view_ofs to get the eye position.
From the eye position, we set up the PAS and PVS to use for filtering network messages to the client. At this point, we could
override the actual PAS or PVS values, or use a different origin.
NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility(edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas)
{
	DBG_SV_PRINT("SetupVisibility(%d %s)\n", pClient->serialnumber, STRING(pClient->v.netname));

	edict_t *pView = pClient;
	// Find the client's PVS
	if (pViewEntity && pViewEntity != pClient)
	{
		pView = pViewEntity;
		pClient->v.view_ofs = pViewEntity->v.view_ofs;// XDM3037: since only players have view_ofs on client side, we have to do this. Hack?
	}

	if (FBitSet(pClient->v.flags, FL_PROXY))
	{
		*pvs = NULL;// the spectator proxy sees
		*pas = NULL;// and hears everything
		return;
	}

	if (pClient->v.iuser1 != OBS_ROAMING && pClient->v.iuser1 != OBS_MAP_FREE)// XDM3037a
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pClient);
		// Tracking Spectators use the visibility of their target
		if (pPlayer && pPlayer->m_hObserverTarget.IsValid())
			pView = pPlayer->m_hObserverTarget.Get();
		else if (pClient->v.iuser2 > 0)
			pView = UTIL_ClientEdictByIndex(pClient->v.iuser2);

		if (pView == NULL)
			pView = pClient;
	}

	Vector org(pView->v.origin);
	/*if (FBitSet(pView->v.effects, EF_MERGE_VISIBILITY))// XDM: g-cont's strange techinque to use foreign PVS
	{
		org = pView->v.origin;
	}
	else*/
	{
		org += pView->v.view_ofs;
		if (FBitSet(pView->v.flags, FL_DUCKING))
			org += (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
	}
	*pvs = ENGINE_SET_PVS(org);
	*pas = ENGINE_SET_PAS(org);
}


#define HOSTFLAG_LOCALWEAPONS		1

//-----------------------------------------------------------------------------
// Purpose: Should this entity be sent to specified recipient
// Input  : *state - server maintained copy of the state info that is transmitted to the client.
//				We could alter values copied into state to send the "host" a different look for a particular entity update, etc.
//			e - entindex of ent
//			ent - the entity that is being added to the update, if 1 is returned
//			host - edict of the recipient (player)
//			hostflags - 1 if client has cl_lw on
//			player - 1 if the ent/e is a player and 0 otherwise
//			pSet - either the PAS or PVS that we previously set up. We can use it to ask the engine to filter the entity against the PAS or PVS.
//				We could also use the pas/ pvs that we set in SetupVisibility, if we wanted to. Caching the value is valid in that case, but still only for the current frame
// Output : int 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise
//-----------------------------------------------------------------------------
int AddToFullPack(struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet)
{
//	DBG_SV_PRINT("AddToFullPack(%d)\n", e);
	// Ignore ents without valid / visible models
	if (player == 0 && ent->v.modelindex == 0)// XDM3037a: flame clouds use this || FStringNull(ent->v.model)). Allow spectators with no model!
		return 0;

	// Don't send entity to local client if the client says it's predicting the entity itself.
	if (FBitSet(ent->v.flags, FL_SKIPLOCALHOST))
	{
		if ((hostflags & HOSTFLAG_LOCALWEAPONS) && (ent->v.owner == host))
			return 0;

		if (host->v.iuser1 == OBS_IN_EYE)// don't send the local host observers in eye mode either
			return 0;
	}

	CBaseEntity *pEntity = CBaseEntity::Instance(ent);
	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(host);
	if (pEntity == NULL || pPlayer == NULL)
		return 0;

	if (ent == host)// XDM3038c: fastest checks first
		goto accept;
	else// if (ent != host)
	{
		if (FBitSet(ent->v.effects, EF_NODRAW))// don't send if flagged for NODRAW and it's not the host getting the message
			return 0;

		if (FBitSet(ent->v.flags, FL_DORMANT|FL_SPECTATOR))// Don't send dormant entities/spectators to other players
			return 0;
	}

	if (e == 0)// XDM3038a: BUGBUG! HL engine does not update the world!!!
		goto accept;

	// XDM3038a: WARNING: This is VERY, VERY VERY VERY dangerous!!
	if (pEntity->m_flRemoveTime > 0.0f && pEntity->m_flRemoveTime <= gpGlobals->time)// XDM3038a: we check this AFTER Think()ing so user code may intercept this
	{
		if (ASSERT(pEntity->GetExistenceState() == ENTITY_EXSTATE_WORLD))// what about containers?
		{
			DBG_SV_PRINT("AddToFullPack(%d %s): entity removed by m_flRemoveTime\n", ent->serialnumber, STRING(pEntity->pev->classname));
			pEntity->UpdateOnRemove();
			//return?
		}
		return 0;
	}

	if (pEntity->GetExistenceState() != ENTITY_EXSTATE_WORLD)// XDM3038a: lets us live without resetting modelindex
		return 0;

	// ValveHACK
	if (FBitSet(ent->v.flags, FL_CUSTOMENTITY))// entityType == ENTITY_BEAM
	{
		// XDM3037: for egon/plasma beams that have "startpos" too far/in the sky
		int attachentity = (ent->v.skin & 0xFFF);//CBeam::GetEndEntity()
		if (attachentity > 0)
		{
			if (!UTIL_IsValidEntity(INDEXENT(attachentity)))// XDM3038: alywas helps with grunt repels
				return 0;
			if (ENGINE_CHECK_VISIBILITY(INDEXENT(attachentity), pSet))
				goto accept;
		}
		attachentity = (ent->v.sequence & 0xFFF);//CBeam::GetStartEntity()
		if (attachentity > 0)
		{
			if (!UTIL_IsValidEntity(INDEXENT(attachentity)))// XDM3038
				return 0;
			if (ENGINE_CHECK_VISIBILITY(INDEXENT(attachentity), pSet))
				goto accept;
		}
		if (!ent->v.angles.IsZero())// "angles" is actually "endpos" for beams
		{
			TraceResult tr;
			UTIL_TraceLine(ent->v.angles, host->v.origin, ignore_monsters, ignore_glass, host, &tr);
			if (tr.flFraction == 1.0f)// visible
				goto accept;
		}
	}
	/*else if (pEntity->IsGameGoal())// XDM3037a: cheat?
	{
		goto accept;
	}*/

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if (ent != host)
	{
		// XDM: a little trick for 'sky' entities
		if (!FBitSet(ent->v.flags, FL_DRAW_ALWAYS))
		{
			if (!ENGINE_CHECK_VISIBILITY(ent, pSet) ||
				(!FBitSet(ent->v.flags, FL_WORLDBRUSH) && ((VecBModelOrigin(&ent->v) - host->v.origin).Length() > g_psv_zmax->value)))// XDM: PERFORMANCE zmax clip // XDM3034 TESTME
				return 0;
		}
	}

	if (pEntity && pPlayer)
	{
		if (!pEntity->ShouldBeSentTo(pPlayer))// XDM3035c: individual entities
			return 0;
	}

	//if (pEntity->IsPlayer())// XDM3035c: TESTME // XDM3038c: bad. Does not hide spawning players AND hides normal players.
	//	if (pEntity->pev->origin.IsZero())
	//		return 0;

	if (host->v.groupinfo && ent->v.groupinfo)// && ent != host // TEST: 2 && 256
	{
		//if (!(ent->v.groupinfo & host->v.groupinfo))
		//	return 0;

		UTIL_SetGroupTrace(host->v.groupinfo, GROUP_OP_AND);
		// Should always be set, of course
		if (g_groupop == GROUP_OP_AND)
		{
			if (!(ent->v.groupinfo & host->v.groupinfo))
				return 0;
		}
		else if (g_groupop == GROUP_OP_NAND)
		{
			if (ent->v.groupinfo & host->v.groupinfo)
				return 0;
		}
		UTIL_UnsetGroupTrace();
	}

accept:
	memset(state, 0, sizeof(entity_state_s));

	// Flag custom entities.
	/*if (pEntity->IsPlayer())// XDM: this shit is useless because the ugly engine treats any etype as a fukin' beam!
		state->entityType = ENTITY_PLAYER;
	else if (pEntity->IsPlayerItem())
		state->entityType = ENTITY_ITEM;
	else */if (FBitSet(ent->v.flags, FL_CUSTOMENTITY))
		state->entityType = ENTITY_BEAM;
	else
		state->entityType = ENTITY_NORMAL;

	// Assign index so we can track this entity from frame to frame and delta from it.
	state->number = e;
	//state->msg_time = auto
	//state->messagenum = auto

	// Copy state data
	/*if (pEntity->IsBSPModel() && ent->v.origin.IsZero())// XDM3038: nope, bad idea.
		state->origin	= pEntity->Center();
	else*/
		state->origin	= ent->v.origin;

	state->angles		= ent->v.angles;
	//NormalizeAngle360(state->angles);// XDM3038b: since we use unsigned DT_FLOAT in the delta, prepare angles to fit into correct range

	state->modelindex	= ent->v.modelindex;
	state->sequence		= ent->v.sequence;
	state->frame		= ent->v.frame;
	state->colormap		= ent->v.colormap;
	state->skin			= ent->v.skin;
	state->solid		= ent->v.solid;
	state->effects		= ent->v.effects;
	//	if (ent != host && FBitSet(host->v.effects, EF_DIMLIGHT) && pPlayer->IsObserver() && pPlayer->m_hObserverTarget.Get() == ent)
	//clientside	SetBits(state->effects, EF_BRIGHTLIGHT);// XDM3038b: highlight target on demand
	state->scale		= ent->v.scale;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate its position on the client if it moves
	if (!player && ent->v.animtime != 0.0f && ent->v.velocity.IsZero())
		state->eflags |= EFLAG_SLERP;

	if (FBitSet(ent->v.flags, FL_DRAW_ALWAYS))
		state->eflags |= EFLAG_DRAW_ALWAYS;

	if (FBitSet(ent->v.flags, FL_HIGHLIGHT))
		state->eflags |= EFLAG_HIGHLIGHT;

	if (g_pGameRules && g_pGameRules->FShowEntityOnMap(pEntity, pPlayer))// XDM3038
		state->eflags |= EFLAG_ADDTOMAP;

	if (pEntity->IsGameGoal())// XDM3038
		state->eflags |= EFLAG_GAMEGOAL;

	state->rendermode		= ent->v.rendermode;
	state->renderamt		= (int)ent->v.renderamt;
	state->rendercolor.r	= (int)ent->v.rendercolor.x;
	state->rendercolor.g	= (int)ent->v.rendercolor.y;
	state->rendercolor.b	= (int)ent->v.rendercolor.z;
	state->renderfx			= ent->v.renderfx;

	state->movetype			= ent->v.movetype;
	state->animtime			= (int)(1000.0f * ent->v.animtime)/1000.0f;// Round animtime to nearest millisecond
	state->framerate		= ent->v.framerate;
	state->body				= ent->v.body;
	state->controller[0]	= ent->v.controller[0];
	state->controller[1]	= ent->v.controller[1];
	state->controller[2]	= ent->v.controller[2];
	state->controller[3]	= ent->v.controller[3];
	state->blending[0]		= ent->v.blending[0];
	state->blending[1]		= ent->v.blending[1];
	//state->blending[2]	= ?;
	//state->blending[3]	= ?;
	state->velocity			= ent->v.velocity;
	state->mins				= ent->v.mins;
	state->maxs				= ent->v.maxs;

	if (UTIL_IsValidEntity(ent->v.aiment))
	{
		state->aiment		= ENTINDEX(ent->v.aiment);
		//if (state->entityType == ENTITY_NORMAL)
		//	state->origin		= ent->v.aiment->v.origin;// XDM3038c: tried a little hack for capture objects that should emit lights, but useless
	}
	if (UTIL_IsValidEntity(ent->v.owner))// XDM
		state->owner		= ENTINDEX(ent->v.owner);

	state->friction			= ent->v.friction;// XDM3035b
	state->gravity			= ent->v.gravity;
	state->team				= ent->v.team;
	state->playerclass		= ent->v.playerclass;

	// XDM3037a: don't send other players health to prevent cheating
	if (player == 0 || (ent == host) || (ent->v.team != TEAM_NONE && ent->v.team == host->v.team))
		state->health = (int)ent->v.health;// WARNING! potential source of problems!
	else if (pEntity->IsAlive())
		state->health = 1;
	else
		state->health = -1;

	state->spectator		= FBitSet(ent->v.flags, FL_SPECTATOR)?1:0;

	if (!FStringNull(ent->v.weaponmodel))
		state->weaponmodel	= MODEL_INDEX(STRING(ent->v.weaponmodel));

	// Special stuff for players only
	//if (player)
	//{
	state->gaitsequence		= ent->v.gaitsequence;
	state->basevelocity		= ent->v.basevelocity;
	//if (pEntity->IsAlive())
		state->usehull		= FBitSet(ent->v.flags, FL_DUCKING)?HULL_PLAYER_CROUCHING:HULL_PLAYER_STANDING;
	//else
	// XDM3037a: idea		state->usehull		= HULL_DEAD;
	//}
	state->oldbuttons		= ent->v.oldbuttons;
	state->onground			= FBitSet(ent->v.flags, FL_ONGROUND)?0:-1;// XDM3035c: does not cause jump problems
	state->iStepLeft		= ent->v.iStepLeft;
	state->flFallVelocity	= ent->v.flFallVelocity;
	//if (ent->v.fov == 0)
	//	ent->v.fov = DEFAULT_FOV;// XDM3037a: NOTE: some moron did not include FOV into delta description.
	state->fov				= ent->v.fov;
	state->weaponanim		= ent->v.weaponanim;
	state->startpos			= ent->v.startpos;
	state->endpos			= ent->v.endpos;
	state->impacttime		= ent->v.impacttime;
	state->starttime		= ent->v.starttime;

	state->iuser1 = ent->v.iuser1;
	state->iuser2 = ent->v.iuser2;
	state->iuser3 = ent->v.iuser3;
	state->iuser4 = ent->v.iuser4;
	state->fuser1 = ent->v.fuser1;
	state->fuser2 = ent->v.fuser2;
	state->fuser3 = ent->v.fuser3;
	state->fuser4 = ent->v.fuser4;
	state->vuser1 = ent->v.vuser1;
	state->vuser2 = ent->v.vuser2;
	state->vuser3 = ent->v.vuser3;
	state->vuser4 = ent->v.vuser4;
	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
// NOTE   : Tried to keep it in order as close to structure layout as possible
// Input  : player - 1 if the ent/e is a player and 0 otherwise
//			eindex - entindex of entity
//			*baseline - state info that is to be written into baseline
//			entity - the entity that is being processed
//			playermodelindex - special values for player
//			player_mins - 
//			player_maxs - 
//-----------------------------------------------------------------------------
void CreateBaseline(int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs)
{
	DBG_SV_PRINT("CreateBaseline(%d, %d, %d, %d, %d)\n", player, eindex, baseline->number, entity->serialnumber, playermodelindex);

	// XDM3037: added all parameters, so CreateBaseline can be used externally
	if (FBitSet(entity->v.flags, FL_CUSTOMENTITY))
		baseline->entityType	= ENTITY_BEAM;
	else
		baseline->entityType	= ENTITY_NORMAL;

	baseline->number			= eindex;
	baseline->msg_time			= 0;
	baseline->messagenum		= 0;

	baseline->origin			= entity->v.origin;
	baseline->angles			= entity->v.angles;

	//baseline->modelindex
	baseline->sequence			= entity->v.sequence;
	baseline->frame				= entity->v.frame;
	//baseline->colormap			= entity->v.colormap;
	baseline->skin				= entity->v.skin;
	//baseline->solid			= entity->v.solid;
	baseline->effects			= entity->v.effects;
	//baseline->scale			= entity->v.scale;
	baseline->eflags			= 0;

	if (FBitSet(entity->v.flags, FL_DRAW_ALWAYS))
		baseline->eflags |= EFLAG_DRAW_ALWAYS;

	if (FBitSet(entity->v.flags, FL_HIGHLIGHT))
		baseline->eflags |= EFLAG_HIGHLIGHT;

	baseline->rendermode		= entity->v.rendermode;
	baseline->renderamt			= (int)entity->v.renderamt;
	baseline->rendercolor.r		= (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g		= (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b		= (byte)entity->v.rendercolor.z;
	baseline->renderfx			= entity->v.renderfx;

	//baseline->movetype			= entity->v.movetype;
	baseline->animtime			= 0;//(int)(1000.0f * entity->v.animtime)/1000.0f;// Round animtime to nearest millisecond
	//baseline->framerate		= entity->v.framerate;
	baseline->body				= entity->v.body;
	baseline->controller[0]		= entity->v.controller[0];
	baseline->controller[1]		= entity->v.controller[1];
	baseline->controller[2]		= entity->v.controller[2];
	baseline->controller[3]		= entity->v.controller[3];
	baseline->blending[0]		= entity->v.blending[0];
	baseline->blending[1]		= entity->v.blending[1];
	//baseline->blending[2]		= ?;
	//baseline->blending[3]		= ?;
	baseline->velocity			= entity->v.velocity;
	//baseline->mins    			= entity->v.mins;
	//baseline->maxs				= entity->v.maxs;

	if (entity->v.aiment != NULL)
		baseline->aiment		= ENTINDEX(entity->v.aiment);
	else
		baseline->aiment		= 0;

	if (entity->v.owner != NULL)
		baseline->owner			= ENTINDEX(entity->v.owner);
	else
		baseline->owner			= 0;

	//baseline->friction		  	= entity->v.friction;
	//baseline->gravity			= entity->v.gravity;
	baseline->team				= entity->v.team;
	baseline->playerclass		= entity->v.playerclass;
	baseline->health			= (int)entity->v.health;
	baseline->spectator			= FBitSet(entity->v.flags, FL_SPECTATOR)?1:0;
	baseline->weaponmodel		= entity->v.weaponmodel;
	baseline->gaitsequence		= entity->v.gaitsequence;
	baseline->basevelocity	 	= entity->v.basevelocity;
	baseline->usehull			= FBitSet(entity->v.flags, FL_DUCKING)?1:0;
	baseline->oldbuttons		= entity->v.oldbuttons;
	baseline->onground			= FBitSet(entity->v.flags, FL_ONGROUND)?0:-1;
	baseline->iStepLeft			= entity->v.iStepLeft;
	baseline->flFallVelocity	= entity->v.flFallVelocity;
	baseline->fov				= entity->v.fov;
	baseline->weaponanim		= entity->v.weaponanim;
	baseline->startpos			= entity->v.startpos;
	baseline->endpos			= entity->v.endpos;
	baseline->impacttime		= entity->v.impacttime;
	baseline->starttime			= entity->v.starttime;
	baseline->iuser1			= entity->v.iuser1;
	baseline->iuser2			= entity->v.iuser2;
	baseline->iuser3			= entity->v.iuser3;
	baseline->iuser4			= entity->v.iuser4;
	baseline->fuser1			= entity->v.fuser1;
	baseline->fuser2			= entity->v.fuser2;
	baseline->fuser3			= entity->v.fuser3;
	baseline->fuser4			= entity->v.fuser4;
	baseline->vuser1			= entity->v.vuser1;
	baseline->vuser2			= entity->v.vuser2;
	baseline->vuser3			= entity->v.vuser3;
	baseline->vuser4			= entity->v.vuser4;

	if (player)
	{
		baseline->mins			= player_mins;
		baseline->maxs			= player_maxs;
		baseline->colormap		= 0;// XDM3037
		baseline->modelindex	= playermodelindex;
		baseline->friction		= 1.0;
		baseline->movetype		= MOVETYPE_WALK;
		baseline->scale			= 1.0;
		baseline->solid			= SOLID_SLIDEBOX;
		baseline->framerate		= 1.0;
		baseline->gravity		= 1.0;
	}
	else
	{
		baseline->mins			= entity->v.mins;
		baseline->maxs			= entity->v.maxs;
		baseline->colormap		= entity->v.colormap;// XDM
		baseline->modelindex	= entity->v.modelindex;//SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype		= entity->v.movetype;
		baseline->scale			= entity->v.scale;
		baseline->solid			= entity->v.solid;
		baseline->framerate		= entity->v.framerate;
		baseline->gravity		= entity->v.gravity;
	}
	/* old
	baseline->frame			= entity->v.frame;
	baseline->skin			= (short)entity->v.skin;
	baseline->body			= entity->v.body;// XDM
	if (entity->v.aiment)
		baseline->aiment	= ENTINDEX(entity->v.aiment);// XDM
	// render information
	baseline->rendermode	= (byte)entity->v.rendermode;
	baseline->renderamt		= (byte)entity->v.renderamt;
	baseline->rendercolor.r	= (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g	= (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b	= (byte)entity->v.rendercolor.z;
	baseline->renderfx		= (byte)entity->v.renderfx;
	baseline->team			= entity->v.team;// XDM
	baseline->fov			= entity->v.fov;
	baseline->oldbuttons	= 0;// XDM3035c*/
	//baseline->impacttime	= entity->v.impacttime;
	//baseline->starttime	= entity->v.starttime;
}

// XDM3037: since valve claims this will help reducing traffic...
const char *g_InstancedBaselineEntities[] =
{
	"a_grenade",
	"ar_grenade",
	"blackball",
	"bolt",
	"flame_cloud",
	"grenade",
	"hornet",
	"l_grenade",
	"lightp",
	"monster_snark",
	// rare "nucdevice",
	"plasmaball",
	"razordisk",
	"rpg_rocket",
	SATCHEL_CLASSNAME,
	"strtarget",
	"teleporter",
	TRIPMINE_CLASSNAME,
	//"env_projectile", <- cannot be instantiated!
	NULL// !!! terminator
};

//-----------------------------------------------------------------------------
// Purpose: Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
//-----------------------------------------------------------------------------
void CreateInstancedBaselines(void)
{
	DBG_SV_PRINT("CreateInstancedBaselines()\n");
	entity_state_t ibaseline;
	int iret;
	size_t i = 0;
	// Create any additional baselines here for things like grendates, etc.
	while (g_InstancedBaselineEntities[i] != NULL)
	{
		CBaseEntity *pEntity = CBaseEntity::Create(g_InstancedBaselineEntities[i], g_vecZero, g_vecZero, g_vecZero, NULL, 0);
		if (pEntity)
		{
			memset(&ibaseline, 0, sizeof(ibaseline));
			CreateBaseline(0, 0, &ibaseline, pEntity->edict(), 0, g_vecZero, g_vecZero);
			iret = ENGINE_INSTANCE_BASELINE(pEntity->pev->classname, &ibaseline);
			DBG_PRINTF("CreateInstancedBaselines(%s): %d\n", g_InstancedBaselineEntities[i], iret);
			REMOVE_ENTITY(pEntity->edict());
		}
		++i;
	}
	// We could throw in g_ItemInfoArray names here, but they are not filled yet.
}

typedef struct entity_field_alias_s
{
	char name[32];
	int	 field;
} entity_field_alias_t;

enum entity_fields_e
{
	FIELD_ORIGIN0 = 0,
	FIELD_ORIGIN1,
	FIELD_ORIGIN2,
	FIELD_ANGLES0,
	FIELD_ANGLES1,
	FIELD_ANGLES2
};

static entity_field_alias_t entity_field_alias[]=
{
	{ "origin[0]",	0 },
	{ "origin[1]",	0 },
	{ "origin[2]",	0 },
	{ "angles[0]",	0 },
	{ "angles[1]",	0 },
	{ "angles[2]",	0 },
	//{ "fov",		0 },
};

void Entity_FieldInit(struct delta_s *pFields)
{
	size_t sz = ARRAYSIZE(entity_field_alias);
	for (size_t i=0; i<sz; ++i)
	{
		entity_field_alias[i].field = DELTA_FINDFIELD(pFields, entity_field_alias[i].name);
		if (entity_field_alias[i].field < 0)
			conprintf(2, "Entity_FieldInit() error: DeltaFindField(%s) failed!\n", entity_field_alias[i].name);
	}
}

/*
==================
Entity_Encode
Callback for sending entity_state_t info over network.
FIXME:  Move to script
==================
*/
void Entity_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	static bool initialized = 0;
	if (!initialized)
	{
		Entity_FieldInit(pFields);
		initialized = 1;
	}

	entity_state_t *f, *t;
	f = (entity_state_t *)from;
	t = (entity_state_t *)to;
	//DBG_SV_PRINT("Entity_Encode(%d)\n", f->number);

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	bool localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();
	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	// Parametric rockets: origin&angles are not used here
	if ((t->impacttime != 0.0f) && (t->starttime != 0.0f))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) && (t->aiment != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}




static entity_field_alias_t player_field_alias[]=
{
	{ "origin[0]",	0 },
	{ "origin[1]",	0 },
	{ "origin[2]",	0 },
	{ "angles[0]",	0 },
	{ "angles[1]",	0 },
	{ "angles[2]",	0 },
//	{ "fov",		0 },
};

/* Seems like someone forgot to register the "fov" field!

#include "delta.h"

delta_t FieldFOV =
{
	"fov",
	offsetof(entity_state_s, fov),// in bytes
	4,// used for bounds checking in DT_STRING
	DT_FLOAT,// DT_INTEGER, DT_FLOAT etc
	1.0,
	1.0,// for DEFINE_DELTA_POST
	8,// how many bits we send\receive
	0// unsetted by user request
};*/

void Player_FieldInit(struct delta_s *pFields)
{
	//DELTA_SET(&FieldFOV, "fov");
	size_t sz = ARRAYSIZE(player_field_alias);
	for (size_t i=0; i<sz; ++i)
	{
		player_field_alias[i].field = DELTA_FINDFIELD(pFields, player_field_alias[i].name);
		if (player_field_alias[i].field < 0)
			conprintf(2, "Player_FieldInit() error: DeltaFindField(%s) failed!\n", player_field_alias[i].name);
	}
}

/*
==================
Player_Encode
Callback for sending entity_state_t for players info over network.
==================
*/
void Player_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	static bool initialized = 0;
	if (!initialized)
	{
		Player_FieldInit(pFields);
		initialized = 1;
	}

	entity_state_t *f, *t;
	f = (entity_state_t *)from;
	t = (entity_state_t *)to;
//	DBG_SV_PRINT("Player_Encode(%d)\n", f->number);

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	bool localplayer = ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) && ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}
	// XDM3037a: TODO: check. Something's telling me this logic can be optimized
//#if !defined (SV_NO_PITCH_CORRECTION)
//	t->angles.x = -t->angles.x;
//#endif
}




enum custom_entity_fields_e
{
	CUSTOMFIELD_START0 = 0,
	CUSTOMFIELD_START1,
	CUSTOMFIELD_START2,
	CUSTOMFIELD_END0,
	CUSTOMFIELD_END1,
	CUSTOMFIELD_END2,
	CUSTOMFIELD_ENDENT,
	CUSTOMFIELD_STARTENT,
	CUSTOMFIELD_ANIMTIME
	//CUSTOMFIELD_PC
};

entity_field_alias_t custom_entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
	{ "skin",				0 },
	{ "sequence",			0 },
	{ "animtime",			0 },
	//{ "playerclass",		0 },
	//{ "fov",				0 },
};

void Custom_Entity_FieldInit(struct delta_s *pFields)
{
	size_t sz = ARRAYSIZE(custom_entity_field_alias);
	for (size_t i=0; i<sz; ++i)
	{
		custom_entity_field_alias[i].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[i].name);
		if (custom_entity_field_alias[i].field < 0)
			conprintf(2, "Custom_Entity_FieldInit() error: DeltaFindField(%s) failed!\n", custom_entity_field_alias[i].name);
	}
}

/*
==================
Custom_Encode
Callback for sending entity_state_t info ( for custom entities ) over network.
FIXME: Move to script
==================
*/
void Custom_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	DBG_SV_PRINT("Custom_Encode()\n");
	static bool initialized = 0;
	if (!initialized)
	{
		Custom_Entity_FieldInit(pFields);
		initialized = 1;
	}

	entity_state_t *f, *t;
	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	int beamType = t->rendermode & 0x0f;

	if (beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_START0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_START1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_START2 ].field );
	}

	if (beamType != BEAM_POINTS)
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_END0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_END1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_END2 ].field );
	}

	if (beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ENDENT ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_STARTENT ].field );
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ((int)f->animtime == (int)t->animtime)
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field );
	}
}

/*
=================
RegisterEncoders
Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders(void)
{
	DBG_SV_PRINT("RegisterEncoders()\n");
	DELTA_ADDENCODER("Entity_Encode", Entity_Encode);
	DELTA_ADDENCODER("Custom_Encode", Custom_Encode);
	DELTA_ADDENCODER("Player_Encode", Player_Encode);
}


//-----------------------------------------------------------------------------
// Purpose: Pack weapon data structure
// Warning: i != ID!!!
// Warning: Lots of data mangling is involved, do not use this for simple data transmission.
// NOTE: Called only when cl_lw is enabled in this player's Info!
// Input  : player -
//			info - array of weapon_data_s
// Output : Returns 1 on success, 0 on failure.
//-----------------------------------------------------------------------------
int GetWeaponData(struct edict_s *player, struct weapon_data_s *info)
{
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(CBasePlayer::Instance(player));
	if (pPlayer == NULL)
		return 0;

//#ifdef HL1122
	//memset(info, 0, 64 * sizeof(weapon_data_t));
//#else
	memset(info, 0, MAX_WEAPONS * sizeof(weapon_data_t));
//#endif

#if defined(CLIENT_WEAPONS)
	int iID;// item ID
	size_t i = 0;// index in info[]
	CBasePlayerWeapon *pWeapon;
	CBasePlayerItem *pPlayerItem;
	int nWeapons = 0;
#if defined(_DEBUG)
	int sv_wpnbits = 0;
#endif
	for (iID = WEAPON_NONE+1; iID < MAX_WEAPONS; ++iID)
	{
		pPlayerItem = pPlayer->GetInventoryItem(iID);// XDM3038c
		if (pPlayerItem)
		{
			pWeapon = pPlayerItem->GetWeaponPtr();
			if (pWeapon)
			{
				pWeapon->ClientPackData(player, &info[i]);
#if defined(_DEBUG)
				SetBits(sv_wpnbits, (1<<iID));
#endif
				++nWeapons;
				++i;// we now use gapless array
			}
		}
		//else
			// already	info[i].m_iId = WEAPON_NONE;
		//^ ++i;
	}

	//if (nWeapons == 0)
	//	conprintf(2, "SV: GetWeaponData(): 0 weapons to send!\n");

	info[MAX_WEAPONS-1].iuser1 = nWeapons;
#if defined(_DEBUG)
//	info[MAX_WEAPONS-1].iuser2 = sv_wpnbits;
	ASSERTD((player->v.weapons & ~(1<<WEAPON_SUIT)) == sv_wpnbits);
#endif

#endif // CLIENT_WEAPONS
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Data sent to current client only
// Note   : Engine sets cd to 0 before calling
// Input  : ent - player
//			sendweapons - player's cl_lw
//			cd - clientdata_s destination
//-----------------------------------------------------------------------------
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd)
{
	DBG_SV_PRINT("UpdateClientData(%d, %d, %d)\n", ent->serialnumber, sendweapons, cd->weapons);

	if (!ent || !ent->pvPrivateData)
		return;

	entvars_t *pev	= (entvars_t *)&ent->v;
	// XDM3037: this method requires RTTI
#if defined(_CPPRTTI)
	CBasePlayer *pl	= dynamic_cast<CBasePlayer *>(CBasePlayer::Instance((edict_t *)ent));
#else
	CBasePlayer *pl = (CBasePlayer *)CBasePlayer::Instance((edict_t *)ent);
#endif
	entvars_t *pevOrg = NULL;

	// if user is spectating different player in First person, override some vars
	if (pl && pl->pev->iuser1 == OBS_IN_EYE)
	{
		if (pl->m_hObserverTarget)
		{
			pevOrg = pev;
			pev = pl->m_hObserverTarget->pev;
#ifdef _CPPRTTI
			pl = dynamic_cast<CBasePlayer *>(CBasePlayer::Instance(pl->m_hObserverTarget.Get()));
#else
			pl = (CBasePlayer *)CBasePlayer::Instance(pl->m_hObserverTarget.Get());
#endif
		}
	}

	cd->origin			= pev->origin;
	cd->velocity		= pev->velocity;
//#ifdef SERVER_VIEWMODEL
	if (FStringNull(pev->viewmodel))// XDM3037a
		cd->viewmodel	= 0;
	else
		cd->viewmodel	= MODEL_INDEX(STRING(pev->viewmodel));
//#endif
	cd->punchangle		= pev->punchangle;
	cd->flags			= pev->flags;
	cd->waterlevel		= pev->waterlevel;
	cd->watertype		= pev->watertype;
	cd->view_ofs		= pev->view_ofs;
	cd->health			= pev->health;
	cd->bInDuck			= pev->bInDuck;
	cd->weapons			= pev->weapons;
	cd->flTimeStepSound = pev->flTimeStepSound;
	cd->flDuckTime		= pev->flDuckTime;
	cd->flSwimTime		= pev->flSwimTime;
	cd->waterjumptime	= (int)pev->teleport_time;
	cd->maxspeed		= pev->maxspeed;
	//	if (pev->fov == 0)
	//nope		pev->fov = DEFAULT_FOV;

	cd->fov				= pev->fov;
	cd->weaponanim		= pev->weaponanim;
	/* look down if (pl && pl->m_pActiveItem)
		cd->m_iId		= pl->m_pActiveItem->GetID();
	else
		cd->m_iId		= WEAPON_NONE;

	if (pl)
	{
		cd->m_flNextAttack	= pl->m_flNextAttack;// XDM3037a: default
		cd->tfstate			= pl->m_iDeaths;// XDM3038a: pev->team;
	}*/
	cd->pushmsec		= pev->pushmsec;
	cd->deadflag		= pev->deadflag;
	strcpy(cd->physinfo, ENGINE_GETPHYSINFO(ent));
	//Spectator mode
	if (pevOrg != NULL)
	{
		// don't use spec vars from chased player
		cd->iuser1		= pevOrg->iuser1;
		cd->iuser2		= pevOrg->iuser2;
	}
	else
	{
		cd->iuser1		= pev->iuser1;
		cd->iuser2		= pev->iuser2;
	}
	cd->iuser3			= ent->v.iuser3;
	cd->iuser4			= ent->v.iuser4;
	cd->fuser1			= ent->v.fuser1;
	cd->fuser2			= ent->v.fuser2;
	cd->fuser3			= ent->v.fuser3;
	cd->fuser4			= ent->v.fuser4;
	cd->vuser1			= ent->v.vuser1;
	cd->vuser2			= ent->v.vuser2;
	cd->vuser3			= ent->v.vuser3;
	cd->vuser4			= ent->v.vuser4;

	// always!!	if (sendweapons)
	//{
		CBaseEntity *pEntity = CBaseEntity::Instance((edict_t *)ent);
		if (pEntity && pEntity->IsAlive())// XDM3035b: MAY BE NULL!
		{
			if (pEntity->IsPlayer())
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
				if (pPlayer->m_pActiveItem)
					cd->m_iId = pPlayer->m_pActiveItem->GetID();
				else
					cd->m_iId = WEAPON_NONE;

				cd->m_flNextAttack = pPlayer->m_flNextAttack;
				cd->tfstate = pPlayer->m_iDeaths;// XDM3038a: pev->team;
/*#if defined(CLIENT_WEAPONS)// XDM: I HATE THIS HACK!!!!!!!!!!!
				cd->ammo_shells		= pPlayer->AmmoInventory(GetAmmoIndexFromRegistry("buckshot"));
				cd->ammo_nails		= pPlayer->AmmoInventory(GetAmmoIndexFromRegistry("bolts"));
				cd->ammo_cells		= pPlayer->AmmoInventory(GetAmmoIndexFromRegistry("uranium"));
				cd->ammo_rockets	= pPlayer->AmmoInventory(GetAmmoIndexFromRegistry("rockets"));
				CBasePlayerWeapon *gun = pPlayer->m_pActiveItem->GetWeaponPtr();
				if (gun && gun->UseDecrement())
				{
					//cd->m_iId = gun->GetID();
					cd->vuser3.z	= gun->m_iSecondaryAmmoType;
					cd->vuser4.x	= gun->m_iPrimaryAmmoType;
					cd->vuser4.y	= pl->m_rgAmmo[gun->m_iPrimaryAmmoType];
					cd->vuser4.z	= pl->m_rgAmmo[gun->m_iSecondaryAmmoType];
					if (pl->m_pActiveItem->m_iId == WEAPON_RPG)
					{
						cd->vuser2.y = ((CWeaponRPG *)pl->m_pActiveItem)->m_fSpotActive;
						cd->vuser2.z = ((CWeaponRPG *)pl->m_pActiveItem)->m_cActiveRockets;
					}
				}
#endif*/
			}
			else// should never happen
			{
				CBaseMonster *pMonster = pEntity->MyMonsterPointer();
				if (pMonster)
					cd->m_flNextAttack = pMonster->m_flNextAttack;
			}
		}
	//}
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart(const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed)
{
	DBG_SV_PRINT("CmdStart(%d)\n", player->serialnumber);
#ifdef _CPPRTTI
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(CBasePlayer::Instance((edict_t *)player));
#else
	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance((edict_t *)player);
#endif

	if (pPlayer == NULL)
		return;

	if (pPlayer->pev->groupinfo != 0)
		UTIL_SetGroupTrace(pPlayer->pev->groupinfo, GROUP_OP_AND);

	pPlayer->random_seed = random_seed;
	pPlayer->pev->light_level = cmd->lightlevel;// XDM3038c: TESME FIX?

	if (!g_ServerActive)
		return;

	if (cmd->weaponselect > WEAPON_NONE)// 0 is normal, non-zero means select something
	{
		//conprintf(1, "cmd->weaponselect = %d\n", cmd->weaponselect);
		if (pPlayer->m_pActiveItem == NULL || (pPlayer->m_pActiveItem->GetID() != cmd->weaponselect))
		{
			if (pPlayer->SelectItem(cmd->weaponselect) == false)// XDM3037: server reports failure, try auto-selecting another one
			{
				// DANGER! Warning! Potential stack/net overflow!
				/* stalls when player has two best unusable weapons				CBasePlayerItem *pItem = pPlayer->GetInventoryItem(cmd->weaponselect);
				if (pItem)
					pPlayer->SelectNextBestItem(pItem);// this will create a client message!
				*/
				pPlayer->SelectNextBestItem(NULL);
			}
		}
		else if (pPlayer->m_pActiveItem)// XDM3038a: sent from client 0->last active item
		{
			if (pPlayer->m_pActiveItem->IsHolstered())
				pPlayer->DeployActiveItem();// XDM3038
		}
	}
	else if (pPlayer->m_pActiveItem)// XDM3038: holster
	{
		if (!pPlayer->m_fInitHUD && pPlayer->m_pActiveItem->CanHolster() && !pPlayer->m_pActiveItem->IsHolstered())// don't act during initialization/restore
			pPlayer->m_pActiveItem->Holster();
	}
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd(const edict_t *player)
{
	DBG_SV_PRINT("CmdEnd(%d)\n", player->serialnumber);
#ifdef _CPPRTTI
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(CBasePlayer::Instance((edict_t *)player));
#else
	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance((edict_t *)player);
#endif

	if (pPlayer == NULL)
		return;

	if (pPlayer->pev->groupinfo != 0)
		UTIL_UnsetGroupTrace();
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	ConnectionlessPacket(const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size)
{
	DBG_SV_PRINT("ConnectionlessPacket()\n");
	// Parse stuff from args
	//int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds(int hullnumber, float *mins, float *maxs)
{
	DBG_SV_PRINT("GetHullBounds(%d)\n", hullnumber);
	int iret = 0;
	switch (hullnumber)
	{
	case HULL_PLAYER_STANDING:				// Normal player
		VectorCopy(VEC_HULL_MIN, mins);
		VectorCopy(VEC_HULL_MAX, maxs);
		iret = 1;
		break;
	case HULL_PLAYER_CROUCHING:				// Crouched player
	//case HULL_DEAD:					// XDM3037a: indicates the player is dead
		VectorCopy(VEC_DUCK_HULL_MIN, mins);
		VectorCopy(VEC_DUCK_HULL_MAX, maxs);
		iret = 1;
		break;
	case HULL_POINT:				// Point based hull
		VectorCopy(g_vecZero, mins);
		VectorCopy(g_vecZero, maxs);
		iret = 1;
		break;
	/*case HULL_3:// XDM3038: possible, but unnecessary. Let the engine control this hull.
		VectorCopy(VEC_HULL_MIN_3, mins);
		VectorCopy(VEC_HULL_MAX_3, maxs);
		iret = 1;
		break;*/
	}
	return iret;
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int	InconsistentFile(const edict_t *player, const char *filename, char *disconnect_message)
{
	DBG_SV_PRINT("InconsistentFile(%s, %s)\n", player->serialnumber, filename);
	// Default behavior is to kick the player
	_snprintf(disconnect_message, 256, "WARNING: client %d file \"%s\" differs from server!\n", ENTINDEX(player), filename);// XDM3035

	// XDM3038b: force orginal file
	if (strcmp(filename, "models/player.mdl") == 0)// reference player model
		return 1;

	// Server doesn't care?
	if (CVAR_GET_FLOAT("mp_consistency") <= 0.0f)
	{
		SERVER_PRINT(disconnect_message);// XDM3035b: we don't care, but aware
		return 0;
	}
	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too,
  if you want.
================================
*/
int AllowLagCompensation(void)
{
	DBG_SV_PRINT("AllowLagCompensation()\n");
#if defined(CLIENT_WEAPONS)
	return 1;
#else
	return 0;
#endif
}
