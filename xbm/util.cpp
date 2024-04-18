#include "extdll.h"
#include "util.h"
#include "engine.h"
#include "usercmd.h"
#include "weapondef.h"
#include "bot.h"
#include "bot_cvar.h"
#include "bot_func.h"
#include <assert.h>
#if _MSC_VER > 1200
#include <crtdbg.h>
#endif

int gmsgTextMsg = 0;
int gmsgSayText = 0;
int gmsgShowMenu = 0;

void CmdStart(const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed);
void CmdEnd(const edict_t *player);

void BotPrintf(bot_t *pBot, char *szFmt, ...)
{
	if (pBot == NULL)
		return;
	if (g_bot_debug.value > 0.0f)
	{
		static char	string[1024];
		//int ofs = sprintf(
		va_list argptr;
		va_start(argptr, szFmt);
		vsnprintf(string, 1024, szFmt, argptr);//+ofs
		va_end(argptr);
		conprintf(2, "Bot %s[%d]: %s", pBot->name, ENTINDEX(pBot->pEdict), string);
	}
}

/*Vector UTIL_VecToAngles(const Vector &vec)
{
	float rgflVecOut[3];
	VectorAngles(vec, rgflVecOut);// XDM3038: instead of VEC_TO_ANGLES(vec, rgflVecOut);
	return Vector(rgflVecOut);
}*/

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr)
{
	//TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr);
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRACE_IGNORE_MONSTERS:TRACE_IGNORE_NOTHING) | (ignoreGlass == ignore_glass?TRACE_IGNORE_GLASS:TRACE_IGNORE_NOTHING), pentIgnore, ptr);
}

void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr)
{
	//TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr);
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRACE_IGNORE_MONSTERS:TRACE_IGNORE_NOTHING), pentIgnore, ptr);
}

edict_t *UTIL_FindEntityInSphere(edict_t *pentStart, const Vector &vecCenter, float flRadius)
{
	edict_t  *pentEntity = FIND_ENTITY_IN_SPHERE(pentStart, vecCenter, flRadius);
	if (!FNullEnt(pentEntity))
		return pentEntity;

	return NULL;
}

edict_t *UTIL_FindEntityByString(edict_t *pentStart, const char *szKeyword, const char *szValue)
{
	edict_t *pentEntity = FIND_ENTITY_BY_STRING(pentStart, szKeyword, szValue);
	if (!FNullEnt(pentEntity))
		return pentEntity;

	return NULL;
}

edict_t *UTIL_FindEntityByClassname(edict_t *pentStart, const char *szName)
{
	return UTIL_FindEntityByString(pentStart, "classname", szName);
}

edict_t *UTIL_FindEntityByTargetname(edict_t *pentStart, const char *szName)
{
	return UTIL_FindEntityByString(pentStart, "targetname", szName);
}

void ClientPrint(entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4)
{
	if (gmsgTextMsg == 0)
		gmsgTextMsg = REG_USER_MSG("TextMsg", -1);

	if (client)
		MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, NULL, ENT(client));
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgTextMsg);

		WRITE_BYTE(msg_dest);
		WRITE_STRING(msg_name);
	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);

	MESSAGE_END();
}

/*void UTIL_SayText( const char *pText, edict_t *pEdict )
{
	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG( "SayText", -1 );

	pfnMessageBegin( MSG_ONE, gmsgSayText, NULL, pEdict );
	pfnWriteByte(pEdict?ENTINDEX(pEdict):0);
		pfnWriteString( pText );
	pfnMessageEnd();
}*/

#define MAX_SAYTEXT MAX_USER_MSG_DATA-2
//-----------------------------------------------------------------------------
// Purpose: Saytext for BOTS! UNDONE: Why do we actually use this hack?! Send to game DLL!
// Input  : *pEntity - 
//			*pText - 
//			teamonly - 
//-----------------------------------------------------------------------------
void BOT_Host_Say(edict_t *pEntity, const char *pText, bool teamonly)
{
	if (pEntity == NULL || pText == NULL || *pText == 0)
		return;

	// NEW: this method is universal, BUT it has length limit of FAKE_CMD_ARG_LEN!
	if (teamonly)
		FakeClientCommand(pEntity, "say_team", pText);
	else
		FakeClientCommand(pEntity, "say", pText);

#if 0
	char text[MAX_SAYTEXT];
	// turn on color set 2  (color on,  no sound)
	if (mod_id == GAME_XDM_DLL)// XDM3035: XDM uses different mechanism (sends player ID)
	{
		// XDM3038a: no more of this shit! text[0] = 2;
		// text[1] = 0;
		// MAN: "If count is greater than the length of strSource, the length of strSource is used in place of count."
		// strncat(text, pText, MAX_SAYTEXT);
//works		strncpy(text, pText, MAX_SAYTEXT);
//works		text[MAX_SAYTEXT-1] = '\0';

	}
	else// Old HL method: print player name directly into the chat
	{
		if (teamonly)
			_snprintf(text, MAX_SAYTEXT, "%c(TEAM) %s: %s\0", 2, STRING(pEntity->v.netname), pText);
		else
			_snprintf(text, MAX_SAYTEXT, "%c%s: %s\0", 2, STRING(pEntity->v.netname), pText);

		/*j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
		if ( (int)strlen(pText) > j )
			message[j] = 0;*/
	}

	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	// XDM3037a
	CLIENT_INDEX player_index;
	edict_t *pClient;
	for (player_index = 1; player_index <= gpGlobals->maxClients; ++player_index)
	{
		pClient = INDEXENT(player_index);
		if (FNullEnt(pClient) || pClient->free)//? || (pClient == pEntity))
			continue;
		if (!FBitSet(pClient->v.flags, FL_CLIENT))
			continue;
		if (teamonly && pClient->v.team != pEntity->v.team)
			continue;
		if (g_intermission == 0)
		{
			if (UTIL_IsSpectator(pEntity) && !UTIL_IsSpectator(pClient))
				continue;
		}
		(*g_engfuncs.pfnMessageBegin)(MSG_ONE, gmsgSayText, NULL, pClient);
			(*g_engfuncs.pfnWriteByte)(ENTINDEX(pEntity) | ((teamonly && mod_id==GAME_XDM_DLL)?128:0));
			(*g_engfuncs.pfnWriteString)(text);
		(*g_engfuncs.pfnMessageEnd)();
	}
	// echo to server console
	//if (sv_lognotice.value > 0)
	// NOSPAM	ALERT(at_logged, "\"%s (%d)<%d><%d>\"%s \"%s\"\n", STRING(pEntity->v.netname), pEntity->v.team, GETPLAYERUSERID(pEntity), GETPLAYERAUTHID(pEntity), teamonly?" (team)":"", text);
#endif
}


#if defined (_DEBUG)
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;

	conprintf(1, "XBM: entvars_t pContainingEntity is NULL, calling into engine\n");
	edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);

	if (pent == NULL)
		conprintf(1, "XBM: ERROR! Even the engine couldn't FindEntityByVars!\n");

	((entvars_t *)pev)->pContainingEntity = pent;
	return pent;
}
#endif //_DEBUG


// return team number 0 through 3 based what MOD uses for team numbers
/*int UTIL_GetTeam(edict_t *pEntity)
{
	// must be HL or OpFor deathmatch...
	char *infobuffer;
	char model_name[32];
	
	if (team_names[0][0] == 0)
	{
		char *pName;
		char teamlist[MAX_TEAMS*MAX_TEAMNAME_LENGTH];
		int i;
		
		num_teams = 0;
		strcpy(teamlist, CVAR_GET_STRING("mp_teamlist"));
		pName = teamlist;
		pName = strtok(pName, ";");
		
		while (pName != NULL && *pName)
		{
			// check that team isn't defined twice
			for (i=0; i < num_teams; i++)
				if (_stricmp(pName, team_names[i]) == 0)
					break;
				if (i == num_teams)
				{
					strcpy(team_names[num_teams], pName);
					num_teams++;
				}
				pName = strtok(NULL, ";");
		}
	}

	infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEntity );
	strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

	for (int index=0; index < num_teams; index++)
	{
		if (_stricmp(model_name, team_names[index]) == 0)
			return index;
	}

	return 0;
//	return pEntity->v.team;
}

// return class number 0 through N
int UTIL_GetClass(edict_t *pEntity)
{
	return 0;
}*/

//-----------------------------------------------------------------------------
// Purpose: TODO: this must be more reliable than just MOVETYPE_FLY check!
// Input  : *pEdict - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsOnLadder(const edict_t *pEdict)
{
	if (pEdict->v.movetype == MOVETYPE_FLY)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: centralized
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsActiveBot(int bot_index)
{
	return (bot_index >= 0 && bots[bot_index].is_used && bots[bot_index].pEdict != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Um... when XDM_EntityRelationship() is unavailable?
// Input  : pBot - 
//			pEdict - entity to check (player)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsFriend(const bot_t *pBot, const edict_t *pEdict)
{
	if (pBot && pEdict)
	{
		if (pEdict == pBot->pBotUser)
			return true;

		if (IsTeamplay())
		{
			if (pEdict->v.team == pBot->pEdict->v.team)
				return true;
		}
		else if (g_iGameType == GT_COOP)
		{
			if (pEdict->v.flags & FL_CLIENT)
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Count avtive bots
// Output : short
//-----------------------------------------------------------------------------
short UTIL_CountBots(void)
{
	short count = 0;
	short index;
	for (index=0; index < MAX_PLAYERS; ++index)
	{
		if (IsActiveBot(index))// count the number of bots in use
			++count;
	}
	return count;
}

//-----------------------------------------------------------------------------
// Purpose: Find bot's index in the 'bots' array by edict
// Input  : *pEdict - 
// Output : int - bot_t bots[i] or -1 if failed
//-----------------------------------------------------------------------------
short UTIL_GetBotIndex(const edict_t *pEdict)
{
	short index;
	for (index=0; index < MAX_PLAYERS; ++index)
	{
		if (bots[index].pEdict == pEdict)
			return index;
	}
	return -1;// return -1 if edict is not a bot
}

//-----------------------------------------------------------------------------
// Purpose: GetBotPointer by edict_t
// Input  : *pEdict - 
// Output : bot_t
//-----------------------------------------------------------------------------
bot_t *UTIL_GetBotPointer(const edict_t *pEdict)
{
	short index = UTIL_GetBotIndex(pEdict);
	if (index >= 0)
		return (&bots[index]);

	return NULL;// return NULL if edict is not a bot
}

//-----------------------------------------------------------------------------
// Purpose: From bot's point of view
// Input  : *pEdict - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsAlive(edict_t *pEdict)
{
	return ((pEdict->v.deadflag == DEAD_NO) &&
		(pEdict->v.health > 0) &&
		!(pEdict->v.flags & FL_NOTARGET) &&
		(pEdict->v.takedamage != 0));
}

bool FInViewCone(const Vector &origin, edict_t *pEdict)
{
	UTIL_MakeVectors(pEdict->v.angles);
	Vector2D vec2LOS((origin - pEdict->v.origin).Make2D());
	vec_t flDot = DotProduct(vec2LOS.Normalize(), gpGlobals->v_forward.Make2D());
	if (flDot > 0.5)// 60 degree field of view //m_flFieldOfView)
		return true;

	return false;
}

bool FVisible(const Vector &vecOrigin, edict_t *pEdict)
{
	// look through caller's eyes
	Vector vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	bool bInWater = (POINT_CONTENTS(vecOrigin) == CONTENTS_WATER);
	bool bLookerInWater = (POINT_CONTENTS(vecLookerOrigin) == CONTENTS_WATER);

	// don't look through water
	if (bInWater != bLookerInWater)
		return false;

	TraceResult tr;
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);

	if (tr.flFraction != 1.0)
		return false;  // Line of sight is not established
	else
		return true;  // line of sight is valid.
}

Vector GetGunPosition(edict_t *pEdict)
{
	return (pEdict->v.origin + pEdict->v.view_ofs);
}

void UTIL_SelectItem(edict_t *pEdict, char *item_name)
{
	FakeClientCommand(pEdict, item_name, NULL, NULL);
}

void UTIL_SelectItem(edict_t *pEdict, const int &iID)
{
	static char buf[8];
	_snprintf(buf, 8, "%d", iID);
	FakeClientCommand(pEdict, "_sw", buf, NULL);
}

/* XDM3038: OBSOLETE void UTIL_SelectWeapon(edict_t *pEdict, const int &weapon_index)
{
	static int s_weapon_index = WEAPON_NONE;
	s_weapon_index = weapon_index;
	usercmd_t user;
	user.lerp_msec = 0;
	user.msec = 0;
	user.viewangles = pEdict->v.v_angle;
	user.forwardmove = 0;
	user.sidemove = 0;
	user.upmove = 0;
	user.lightlevel = 127;
	user.buttons = 0;
	user.impulse = 0;
	user.weaponselect = s_weapon_index;
	user.impact_index = 0;
	user.impact_position.Clear();
	CmdStart(pEdict, &user, 0);
	CmdEnd(pEdict);
}*/

Vector VecBModelOrigin(entvars_t *pevBModel)
{
	return (pevBModel->absmin + pevBModel->absmax) * 0.5f;// XDM: proper way
}

bool UpdateSounds(edict_t *pEdict, edict_t *pPlayer)
{
	// update sounds made by this player, alert bots if they are nearby...
	if (g_pmp_footsteps->value > 0.0)
	{
		// check if this player is moving fast enough to make sounds...
		if (pPlayer->v.velocity.Length2D() > 220.0f)
		{
			float sensitivity = 1.0f;
			float volume = 500.0f;  // volume of sound being made (just pick something)
			Vector v_sound = pPlayer->v.origin - pEdict->v.origin;
			vec_t distance = v_sound.Length();
			// is the bot close enough to hear this sound?
			if (distance < (volume * sensitivity))
			{
				Vector bot_angles;// = UTIL_VecToAngles(v_sound);
				VectorAngles(v_sound, bot_angles);
				pEdict->v.ideal_yaw = bot_angles.y;
				BotFixIdealYaw(pEdict);
				return true;
			}
		}
	}
	return false;
}

void UTIL_ShowMenu(edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText)
{
	if (gmsgShowMenu == 0)
		gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);

	pfnMessageBegin(MSG_ONE, gmsgShowMenu, NULL, pEdict);
		pfnWriteShort(slots);
		pfnWriteChar(displaytime);
		pfnWriteByte(needmore);
		pfnWriteString(pText);
	pfnMessageEnd();
}

void UTIL_BuildFileName(char *filename, const char *arg1, const char *arg2)
{
	filename[0] = 0;

	if (mod_id == GAME_XDM_DLL)// XBM
	{
		strcpy(filename, "XDM/");
	}
	else
	{
		if (mod_id != GAME_VALVE_DLL)
			conprintf(1, "Error in UTIL_BuildFileName(): unknown mod ID: %d!\n", mod_id);

		strcpy(filename, "valve/");
	}

	if ((arg1) && (arg2))
	{
		if (*arg1 && *arg2)
		{
			strcat(filename, arg1);
			strcat(filename, "/");
			strcat(filename, arg2);
		}
		return;
	}

	if (arg1)
	{
		if (*arg1)
			strcat(filename, arg1);
	}

	// convert to MS-DOS or Linux format...
	//UTIL_Pathname_Convert(filename);
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf(char *fmt, ...)
{
	va_list argptr;
	static char string[1024];
	va_start(argptr, fmt);
	vsnprintf(string, 1024, fmt, argptr);
	va_end(argptr);
	// Print to server console
	ALERT(at_logged, "%s", string);
}

// This function fixes the erratic behaviour caused by the use of the GET_GAME_DIR engine
// macro, which returns either an absolute directory path, or a relative one, depending on
// whether the game server is run standalone or not. This one always return a RELATIVE path.
void GetGameDir(char *game_dir)
{
	unsigned char length, fieldstart, fieldstop;
	GET_GAME_DIR(game_dir); // call the engine macro and let it mallocate for the char pointer

	length = strlen(game_dir); // get the length of the returned string
	length--; // ignore the trailing string terminator

	// format the returned string to get the last directory name
	fieldstop = length;
	while (((game_dir[fieldstop] == '\\') || (game_dir[fieldstop] == '/')) && (fieldstop > 0))
		fieldstop--; // shift back any trailing separator

	fieldstart = fieldstop;
	while ((game_dir[fieldstart] != '\\') && (game_dir[fieldstart] != '/') && (fieldstart > 0))
		fieldstart--; // shift back to the start of the last subdirectory name

	if ((game_dir[fieldstart] == '\\') || (game_dir[fieldstart] == '/'))
		fieldstart++; // if we reached a separator, step over it

	// now copy the formatted string back onto itself character per character
	for (length = fieldstart; length <= fieldstop; length++)
		game_dir[length - fieldstart] = game_dir[length];

	game_dir[length - fieldstart] = 0; // terminate the string
}

//-----------------------------------------------------------------------------
// Purpose: Is this a valid team ID?
// Input  : &team_id - TEAM_ID
// Output : Returns true if TEAM_NONE to TEAM_4
//-----------------------------------------------------------------------------
bool IsValidTeam(const TEAM_ID &team_id)
{
	return (team_id >= TEAM_NONE && team_id <= MAX_TEAMS);//gViewPort->GetNumberOfTeams());
}

int g_iNumTeams = MAX_TEAMS;// stub
//-----------------------------------------------------------------------------
// Purpose: Is this a real, playable team? // TEAM_NONE must be invalid here!
// Input  : &team_id - TEAM_ID
// Output : Returns true if TEAM_1 to TEAM_4 and active
//-----------------------------------------------------------------------------
bool IsActiveTeam(const TEAM_ID &team_id)
{
	return (team_id > TEAM_NONE && team_id <= g_iNumTeams);//gViewPort->GetNumberOfTeams());
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pent - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsValidEntity(const edict_t *pent)
{
	if (pent == NULL)
		return false;
	if (pent->free)
		return false;
	if (FBitSet(pent->v.flags, FL_KILLME))// XDM3037
		return false;
	if (pent->pvPrivateData == NULL)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pent - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_IsSpectator(const edict_t *pent)
{
	if (pent)
	{
		if ((pent->v.flags & FL_CLIENT) || (pent->v.flags & FL_FAKECLIENT))
		{
			if (pent->v.iuser1 != 0)//OBS_NONE)
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns players and bots as edict_t *pointer
// Input  : playerIndex - player/entity index
//-----------------------------------------------------------------------------
edict_t *UTIL_ClientByIndex(const CLIENT_INDEX playerIndex)
{
	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t *e = INDEXENT(playerIndex);
		if (UTIL_IsValidEntity(e))// && (e->v.flags & FL_CLIENT || e->v.flags & FL_FAKECLIENT))
			return e;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get entity name or at least something useful...
// Input  : *pEntity - name source
//			*output - print here
//			max_len - maximum length of output string
//-----------------------------------------------------------------------------
void GetEntityPrintableName(edict_t *pEntity, char *output, const size_t max_len)
{
	if (pEntity && output)
	{
		if (!FStringNull(pEntity->v.netname))
		{
			strncpy(output, STRING(pEntity->v.netname), max_len);
			output[max_len-1] = 0;
		}
		else if (!FStringNull(pEntity->v.targetname))// XDM3035: monsters?
		{
			strncpy(output, STRING(pEntity->v.targetname), max_len);
			output[max_len-1] = 0;
		}
		else if (!FStringNull(pEntity->v.classname))// XDM3035: desperate
		{
			strncpy(output, STRING(pEntity->v.classname), max_len);
			output[max_len-1] = 0;
		}
		else
			*output = 0;
			//strcpy(output, "anonymous\0");// J4F
	}
}

// use TE_BEAMPOINTS, TE_BEAMTORUS, TE_BEAMDISK or TE_BEAMCYLINDER as types, Parameters are NOT converted (e.g. life*0.1, etc.)
void BeamEffect(int type, const Vector &vecPos, const Vector &vecAxis, int spriteindex, int startframe, int fps, int life, int width, int noise, const Vector &color, int brightness, int speed)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecPos);
		WRITE_BYTE(type);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_COORD(vecAxis.x);
		WRITE_COORD(vecAxis.y);
		WRITE_COORD(vecAxis.z);
		WRITE_SHORT(spriteindex);
		WRITE_BYTE(min(startframe, 255));
		WRITE_BYTE(min(fps, 255));
		WRITE_BYTE(min(life, 255));
		WRITE_BYTE(min(width, 255));
		WRITE_BYTE(min(noise, 255));
		WRITE_BYTE(min(color.x, 255));
		WRITE_BYTE(min(color.y, 255));
		WRITE_BYTE(min(color.z, 255));
		WRITE_BYTE(min(brightness, 255));
		WRITE_BYTE(min(speed, 255));
	MESSAGE_END();
}

void GlowSprite(const Vector &vecPos, int mdl_idx, int life, int scale, int fade)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, vecPos);
		WRITE_BYTE(TE_GLOWSPRITE);
		WRITE_COORD(vecPos.x);
		WRITE_COORD(vecPos.y);
		WRITE_COORD(vecPos.z);
		WRITE_SHORT(mdl_idx);
		WRITE_BYTE(min(life, 255));
		WRITE_BYTE(min(scale, 255));
		WRITE_BYTE(min(fade, 255));
	MESSAGE_END();
}

void UTIL_ShowLine(const Vector &start, const Vector &end, float life, byte r, byte g, byte b)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, start);
		WRITE_BYTE(TE_LINE);
		WRITE_COORD(start.x);
		WRITE_COORD(start.y);
		WRITE_COORD(start.z);
		WRITE_COORD(end.x);
		WRITE_COORD(end.y);
		WRITE_COORD(end.z);
		WRITE_SHORT((int)(life*10.0f));
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
	MESSAGE_END();
}

// how to maket this work properly?
void UTIL_ShowBox(const Vector &origin, const Vector &mins, const Vector &maxs, float life, byte r, byte g, byte b)
{
	MESSAGE_BEGIN(MSG_PVS, svc_temp_entity, origin);
		WRITE_BYTE(TE_BOX);
		WRITE_COORD(origin.x + mins.x);
		WRITE_COORD(origin.y + mins.y);
		WRITE_COORD(origin.z + mins.z);
		WRITE_COORD(origin.x + maxs.x);
		WRITE_COORD(origin.y + maxs.y);
		WRITE_COORD(origin.z + maxs.z);
		WRITE_SHORT((int)(life*10.0f));
		WRITE_BYTE(r);
		WRITE_BYTE(g);
		WRITE_BYTE(b);
	MESSAGE_END();
}

//-----------------------------------------------------------------------------
// Purpose: GameRules->IsTeamplay()
// Input  : &gamerules - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsTeamGame(const int &gamerules)
{
	if (gamerules >= GT_TEAMPLAY)
		return true;

	return false;
}

bool IsTeamplay(void)
{
	//return IsTeamGame(g_iGameType);
	if (g_iGameType >= GT_TEAMPLAY)
		return true;

	return false;
}

bool GameRulesHaveGoal(void)
{
	switch (g_iGameType)
	{
	case GT_COOP:
		{
			if (g_iGameMode == COOP_MODE_LEVEL)
				return true;
		}
		break;
	case GT_CTF:
	case GT_DOMINATION:
	//case GT_ASSAULT:
	case GT_ROUND:// HACK! FAKE!!!!!!
		{
			return true;
		}
		break;
	}
	return false;
}
