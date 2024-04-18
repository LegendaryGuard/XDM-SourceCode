#include "hud.h"
#include "cl_util.h"
#include "cl_fx.h"
#include "vgui_Viewport.h"
#include "vgui_ScorePanel.h"
#include "parsemsg.h"
#include "event_api.h"
#include "in_defs.h"
#include <time.h>
//#include "con_nprint.h"

// XDM: only forward calls if data itself is HUD- or GUI-related!

int __MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_InitHUD(pszName, iSize, pbuf);
}

int __MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf);
}

/* XDM3037a int __MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SetFOV(pszName, iSize, pbuf);
}*/

int __MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_ViewMode(pszName, iSize, pbuf);
}

int __MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_GameMode(pszName, iSize, pbuf);
}

int __MsgFunc_GRInfo(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_GRInfo(pszName, iSize, pbuf);
}

int __MsgFunc_GREvent(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_GREvent(pszName, iSize, pbuf);
}

int __MsgFunc_Logo(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_Logo(pszName, iSize, pbuf);
}

int __MsgFunc_HUDEffects(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_HUDEffects(pszName, iSize, pbuf);
}

int __MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	size_t n = READ_BYTE();// Number of entries transmitted WARNING! +1 for 0th team!

	float h=1.0f,s=0.0f,l=0.0f;// XDM3035
	//for (int i = 0; i < n; ++i)
	for (size_t i = 0; i < MAX_TEAMS+1; ++i)
	{
		if (i < n)// recv
		{
			// XDM3035	g_TeamInfo[i].id = i;// XDM3035
			g_TeamInfo[i].color[0] = READ_BYTE();
			g_TeamInfo[i].color[1] = READ_BYTE();
			g_TeamInfo[i].color[2] = READ_BYTE();

			RGB2HSL(g_TeamInfo[i].color[0],
					g_TeamInfo[i].color[1],
					g_TeamInfo[i].color[2],
					h,s,l);

			strncpy(g_TeamInfo[i].name, READ_STRING(), MAX_TEAMNAME_LENGTH);// XDM
		}
		else// reset to default
		{
			// NO! memset(&g_TeamInfo[i], 0, sizeof(team_info_t));
			g_TeamInfo[i].color[0] = g_iTeamColors[i][0];
			g_TeamInfo[i].color[1] = g_iTeamColors[i][1];
			g_TeamInfo[i].color[2] = g_iTeamColors[i][2];
			g_TeamInfo[i].name[0] = '\0';
		}
		g_TeamInfo[i].color.a = 255;
		// Half-Life colormap is hue in range of 0...255
		g_TeamInfo[i].colormap = (int)(255.0f*(h/360.0f));//RGBtoHSV(g_TeamInfo[i].color);// no READ_BYTE();
		g_TeamInfo[i].colormap |= g_TeamInfo[i].colormap << 8;
	}
	END_READ();

	if (n > 0)
		g_iNumberOfTeams = n-1;// this must represent the number of REAL TEAMS
	else
		g_iNumberOfTeams = 0;

	ASSERT(g_iNumberOfTeams <= MAX_TEAMS);

	//if (gViewPort)
	//	return gViewPort->MsgFunc_TeamNames(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_MOTD(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (gViewPort)
		return gViewPort->MsgFunc_MOTD(pszName, iSize, pbuf);

	return 0;
}

int __MsgFunc_ShowMenu(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (gViewPort)
		return gViewPort->MsgFunc_ShowMenu(pszName, iSize, pbuf);

	return 0;
}

int __MsgFunc_ServerName(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (gViewPort)
		return gViewPort->MsgFunc_ServerName(pszName, iSize, pbuf);

	return 0;
}

int __MsgFunc_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
//	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	CLIENT_INDEX cl = READ_BYTE();
	if (IsValidPlayerIndex(cl))
	{
		/*TEST	int newscore = READ_SHORT();
		if (g_PlayerExtraInfo[cl].score != newscore)
		{
			//conprintf(0, "New score: %+d\n", newscore-g_PlayerExtraInfo[cl].score);
			con_nprint_t info;
			info.index = 10;
			info.time_to_live = 3.0f;
			info.color[0] = (newscore > g_PlayerExtraInfo[cl].score)?0:1;
			info.color[1] = (newscore > g_PlayerExtraInfo[cl].score)?1:0;
			info.color[2] = 0;
			CON_NXPRINTF(&info, "%+d\n", newscore-g_PlayerExtraInfo[cl].score);
			g_PlayerExtraInfo[cl].score = newscore;
		}*/
		g_PlayerExtraInfo[cl].score = READ_SHORT();// In case someone needs score more than +-32768, change this to LONG
		g_PlayerExtraInfo[cl].deaths = READ_SHORT();
	}
	else
		conprintf(1, "MsgFunc_%s: invalid player index: %d (%hd %hd)!\n", pszName, cl, READ_SHORT(), READ_SHORT());// use %hd for short!

	END_READ();

	if (gViewPort)
		gViewPort->MsgFunc_ScoreInfo(pszName, iSize, pbuf);

	return 1;
}

int __MsgFunc_TeamScore(const char *pszName, int iSize, void *pbuf)
{
//	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	TEAM_ID team = READ_BYTE();
	if (IsValidTeam(team))
		g_TeamInfo[team].scores_overriden = READ_SHORT();

	END_READ();
	return 1;
}

// NOTE: teamNum may be TEAM_NONE if a player is leaving the server, but at the same time cl will be a VALID ACTIVE player index!
int __MsgFunc_TeamInfo(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	byte cl = READ_BYTE();
	short teamNum = READ_BYTE();
	END_READ();

	if (IsValidPlayerIndex(cl))// set the players team
	{
		if (IsActivePlayer(cl))// XDM3038: print only for active players
		{
			//GetPlayerInfo(cl, &g_PlayerInfoList[cl]);
			if (g_PlayerExtraInfo[cl].teamnumber > TEAM_NONE && teamNum > TEAM_NONE && g_PlayerExtraInfo[cl].teamnumber != teamNum)
			{
				char str[MAX_CHARS_PER_LINE];// XDM: player has changed team
				_snprintf(str, MAX_CHARS_PER_LINE, BufferedLocaliseTextString("* #CL_CHANGETEAM\n"), g_PlayerInfoList[cl].name, GetTeamName(teamNum));
				str[63] = 0;
				gHUD.m_SayText.SayTextPrint(str, 0, 0);
			}
		}

		g_PlayerExtraInfo[cl].teamnumber = teamNum;// XDM

		if (g_PlayerInfoList[cl].thisplayer)
		{
			//DBG_FORCEBREAK// XDM3037a: TEST
			gHUD.m_iTeamNumber = teamNum;
		}

		RebuildTeams();
	}

	if (gViewPort)
		return gViewPort->MsgFunc_TeamInfo(pszName, iSize, pbuf);

	return 1;
}

int __MsgFunc_Spectator(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	byte cl = READ_BYTE();
	byte newstatus = READ_BYTE();
	END_READ();

	if (IsValidPlayerIndex(cl))
	{
		GetPlayerInfo(cl, &g_PlayerInfoList[cl]);// XDM3037a
		//old	g_IsSpectator[cl] = READ_BYTE();
		if (g_PlayerExtraInfo[cl].observer != newstatus)
		{
			if (g_PlayerInfoList[cl].thisplayer)
			{
				//DBG_FORCEBREAK// XDM3037a: TEST
				CL_ScreenFade(255,255,255,191, 1.0, 0.0, 0);// XDM3037
				if (gViewPort)
				{
					if (gViewPort->GetScoreBoard())
						gViewPort->GetScoreBoard()->m_iLastKilledBy = 0;// XDM3037a: forget last killer as the server forgets it
				}
			}
			g_PlayerExtraInfo[cl].observer = newstatus;
			//g_PlayerInfoList[cl].spectator = g_IsSpectator[cl];// XDM: we can't store data g_PlayerInfoList[cl].spectator because the engine will overwrite it :(

			//if (g_IsSpectator[cl] > 0)// player has become a spectator
			if (g_PlayerExtraInfo[cl].observer > 0)
			{
				// XDM3038a: OBSOLETE g_PlayerExtraInfo[cl].teamnumber = TEAM_NONE;
				g_PlayerExtraInfo[cl].ready = 0;
			}
			if (g_PlayerInfoList[cl].name && *g_PlayerInfoList[cl].name)
			{
				char str[MAX_CHARS_PER_LINE];// XDM3037: notification
				char entstring[MAX_PLAYER_NAME_LENGTH];
				GetEntityPrintableName(cl, entstring, MAX_PLAYER_NAME_LENGTH);
				if (newstatus)
					_snprintf(str, MAX_CHARS_PER_LINE, BufferedLocaliseTextString("* #CL_SPEC_START\n"), entstring);
				else
					_snprintf(str, MAX_CHARS_PER_LINE, BufferedLocaliseTextString("* #CL_SPEC_STOP\n"), entstring);// XDM3038: indicate someone exited spectator mode

				str[63] = 0;
				gHUD.m_SayText.SayTextPrint(str, 0, 0);
			}
			if (gViewPort)
				gViewPort->UpdateOnPlayerInfo();// XDM3038a
		}
		// XDM3033: player will have to select team again on rejoin
	}

	//if (gViewPort)
	//	return gViewPort->MsgFunc_Spectator(pszName, iSize, pbuf);

	return 1;
}

int __MsgFunc_SpecFade(const char *pszName, int iSize, void *pbuf)// HL20130901
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (gViewPort)
		return gViewPort->MsgFunc_SpecFade(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_ResetFade(const char *pszName, int iSize, void *pbuf)// HL20130901
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	if (gViewPort)
		return gViewPort->MsgFunc_ResetFade(pszName, iSize, pbuf);
	return 0;
}

/*OBSOLETE int __MsgFunc_AllowSpec(const char *pszName, int iSize, void *pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_AllowSpec(pszName, iSize, pbuf);
	return 0;
}*/

int __MsgFunc_PlayerStats(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	int cl_index, nstats, index;
	BEGIN_READ(pbuf, iSize);
	cl_index = READ_BYTE();
	nstats = READ_BYTE();
	for (index=0; index < nstats; ++index)
	{
		//g_PlayerStats[cl_index][index] = READ_LONG();
		READ_LONG(g_PlayerStats[cl_index][index]);
	}
	END_READ();

	if (gViewPort && gViewPort->GetScoreBoard())
		gViewPort->GetScoreBoard()->RecievePlayerStats(cl_index);

	if (g_pCvarLogStats->value > 0.0f)
	{
		time_t aclock;
		time(&aclock);
		struct tm *newtime = localtime(&aclock);
		char datetimestr[32];
		strftime(datetimestr, 32, "%Y%m%d_%H%M\0", newtime);
		char statname[MAX_PATH];
		_snprintf(statname, MAX_PATH, "%s/logs/%s_%s_%s.txt\0", GET_GAME_DIR(), datetimestr, GetMapName(), g_PlayerInfoList[cl_index].name);
		statname[MAX_PATH-1] = '\0';
		FILE *pFile = fopen(statname, "w+t");
		if (pFile)
		{
			fprintf(pFile, "// Match statistics for %s on map %s, date: %s\n", g_PlayerInfoList[cl_index].name, GetMapName(), datetimestr);
			for (index=0; index<nstats; ++index)
			{
				_snprintf(statname, MAX_PATH, "#StatParamName%d\0", index);
				fprintf(pFile, "%d\t// %s\n", g_PlayerStats[cl_index][index], BufferedLocaliseTextString(statname));
			}
			fclose(pFile);
			conprintf(0, "Stats written to file.\n");
		}
		else
			conprintf(0, "Error writing stats file \"%s\"!\n", statname);
	}
	return 1;
}

int __MsgFunc_SpeakSnd(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	short entindex	= READ_SHORT();
	float volume	= (float)READ_BYTE()/255.0f;
	int pitch		= READ_BYTE();
	const char *sample	= READ_STRING();// probably safe to keep just a pointer
	END_READ();
	cl_entity_t *ent = gEngfuncs.GetEntityByIndex(entindex);
	if (ent)
		gEngfuncs.pEventAPI->EV_PlaySound(entindex, ent->origin, CHAN_VOICE, sample, volume, ATTN_NORM, 0, pitch);
	return 1;
}

int __MsgFunc_PickedEnt(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("__MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	Vector end;
	int ei = READ_SHORT();
	end.x = READ_COORD();
	end.y = READ_COORD();
	end.z = READ_COORD();
	END_READ();
	CL_EntitySelected(ei, end);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Register server->to->client message handlers
//-----------------------------------------------------------------------------
void CL_RegisterMessages(void)
{
	HOOK_MESSAGE(InitHUD);
	HOOK_MESSAGE(ResetHUD);
// XDM3037a	HOOK_MESSAGE(SetFOV);
	HOOK_MESSAGE(ViewMode);
	HOOK_MESSAGE(GameMode);
	HOOK_MESSAGE(GRInfo);// XDM3035
	HOOK_MESSAGE(GREvent);// XDM3035
	HOOK_MESSAGE(Logo);
	HOOK_MESSAGE(HUDEffects);// XDM3038
	HOOK_MESSAGE(TeamNames);
	HOOK_MESSAGE(MOTD);
	HOOK_MESSAGE(ShowMenu);
	HOOK_MESSAGE(ServerName);
	HOOK_MESSAGE(ScoreInfo);
	HOOK_MESSAGE(TeamScore);
	HOOK_MESSAGE(TeamInfo);
	HOOK_MESSAGE(Spectator);
	HOOK_MESSAGE(SpecFade);// HL20130901
	HOOK_MESSAGE(ResetFade);// HL20130901
//OBSOLETE	HOOK_MESSAGE(AllowSpec);
	HOOK_MESSAGE(PlayerStats);
	HOOK_MESSAGE(SpeakSnd);
	HOOK_MESSAGE(PickedEnt);

	HookFXMessages();
}
