#include "hud.h"
#include "cl_util.h"
#include "vgui_Viewport.h"
#include "vgui_ScorePanel.h"
#include "vgui_EntityEntryPanel.h"
#include "RenderManager.h"
#include "in_defs.h"
#include "event_api.h"
#include "cl_fx.h"


void __CmdFunc_MapBriefing(void)// XDM
{
	if (gViewPort)
		gViewPort->ShowMenuPanel(MENU_MAPBRIEFING);
}

/*void __CmdFunc_MapInfo(void)// XDM: UNDONE: show fullscreen map overview with info text
{
	if (gViewPort)
		gViewPort->ShowMenuPanel(MENU_MAPINFO);
}*/

void __CmdFunc_OpenCommandMenu(void)
{
	if (gViewPort)
		gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu, true);
}

void __CmdFunc_CloseCommandMenu(void)
{
	if (gViewPort)
		gViewPort->InputSignalHideCommandMenu();
}

void __CmdFunc_ToggleCommandMenu(void)
{
	if (gViewPort)
	{
		if (gViewPort->IsCommandMenuOpened(gViewPort->m_StandardMenu))
			gViewPort->InputSignalHideCommandMenu();
		else
			gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu, true);
	}
}

void __CmdFunc_CloseMenus(void)
{
	if (gViewPort)
		gViewPort->HideCommandMenu();
}

void __CmdFunc_ForceCloseCommandMenu(void)
{
	if (gViewPort)
		gViewPort->HideCommandMenu();
}

void __CmdFunc_ToggleServerBrowser(void)
{
	if (gViewPort)
		gViewPort->ToggleServerBrowser();
}

void __CmdFunc_ToggleMusicPlayer(void)
{
	if (gViewPort)
		gViewPort->ToggleMusicPlayer();
}

void __CmdFunc_ShowMOTD(void)
{
	if (gViewPort)
		gViewPort->ShowMenuPanel(MENU_INTRO);
}

void __CmdFunc_VoiceTweak(void)
{
	if (gViewPort)
		gViewPort->ShowMenuPanel(MENU_VOICETWEAK);
}

void __CmdFunc_VGUIMenu(void)
{
	if (gViewPort)
		gViewPort->ShowMenuPanel(atoi(CMD_ARGV(1)));
}

void __CmdFunc_ChooseTeam(void)
{
	if (gViewPort)
		gViewPort->ShowMenuPanel(MENU_TEAM);
}

void __CmdFunc_Dir(void)
{
	UTIL_ListFiles(CMD_ARGV(1));
}

void __CmdFunc_ProfileSave(void)
{
	if (CMD_ARGC() >= 2)
	{
		char cfgfile[MAX_PATH];
		_snprintf(cfgfile, MAX_PATH, "%s.cfg", CMD_ARGV(1));
		CONFIG_GenerateFromList("profile.lst", cfgfile);
	}
	else
		conprintf(0, "Usage: %s <config name without extension>\n", CMD_ARGV(0));
}

void __CmdFunc_UI_ReloadScheme(void)
{
	if (gViewPort)
	{
		gViewPort->GetSchemeManager()->LoadScheme();
		gViewPort->LoadScheme();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Mouse Manipulation Mode(tm)
//-----------------------------------------------------------------------------
void __CmdFunc_ToggleSelectionMode(void)
{
	if (g_pCvarDeveloper && g_pCvarDeveloper->value <= 0.0f)
		return;
	if (gHUD.IsSpectator())
		return;

	if (CMD_ARGC() >= 2)
	{
		uint32 newmode = strtoul(CMD_ARGV(1), NULL, 10);
		if (newmode == g_iMouseManipulationMode)// XDM3037: toggle
			g_iMouseManipulationMode = 0;
		else
			g_iMouseManipulationMode = newmode;
	}
	else
	{
		if (g_iMouseManipulationMode == 0)
			g_iMouseManipulationMode = 1;
		else // if (g_iMouseManipulationMode == 1)
			g_iMouseManipulationMode = 0;
	}
	gViewPort->UpdateCursorState();// !
}

//-----------------------------------------------------------------------------
// Purpose: temporary cvar
//-----------------------------------------------------------------------------
void __CmdFunc_SetTmp(void)// XDM3034: will this be useful...ever?
{
	/*cvar_t *pv = NULL:
	pv = CVAR_GET_POINTER(CMD_ARGV(1));
	if (pv)
		pv->string = ALLOCATE BUFFER??? g_pCvarTMP->string;*/
	char cmd[256];
	_snprintf(cmd, 256, "%s \"%s\"\0", CMD_ARGV(1), g_pCvarTmp->string);
	cmd[255] = '\0';
	CLIENT_COMMAND(cmd);
}

//-----------------------------------------------------------------------------
// Purpose: Print a list of all systems in memory
//-----------------------------------------------------------------------------
void __CmdFunc_ListRS(void)
{
	if (g_pRenderManager)
		g_pRenderManager->ListSystems();
	else
		conprintf(0, "RenderManager is offline.\n");
}

//-----------------------------------------------------------------------------
// Purpose: Load RS from a file
//-----------------------------------------------------------------------------
void __CmdFunc_LoadRS(void)
{
	if (g_pRenderManager && !gHUD.IsSpectator())
	{
		if (CMD_ARGC() >= 2)
		{
			int iFollowEnt = 0;
			int iFollowAttachment = 0;
			int iFollowModelIndex = 0;
			int iSystemFlags = 0;
			cl_entity_t *pFollowEnt = NULL;
			float fTTL = 0.0f;
			if (CMD_ARGC() >= 3)// followent specified
			{
				iFollowEnt = atoi(CMD_ARGV(2));
				if (iFollowEnt >= 0)
				{
					pFollowEnt = GetUpdatingEntity(iFollowEnt);
					if (pFollowEnt)
						iFollowModelIndex = pFollowEnt->curstate.modelindex;
					else
						conprintf(0, "%s warning: follow entity not found!\n", CMD_ARGV(0));
				}
				if (CMD_ARGC() >= 4)
				{
					iFollowAttachment = atoi(CMD_ARGV(3));
					if (CMD_ARGC() >= 5)
					{
						iSystemFlags = atoi(CMD_ARGV(4));
						if (CMD_ARGC() >= 6)
							fTTL = atof(CMD_ARGV(5));
					}
				}
			}
			uint32 nLoaded = LoadRenderSystems(CMD_ARGV(1), g_vecViewOrigin/*gHUD.m_vecOrigin*/, 0, iFollowEnt, iFollowModelIndex, iFollowAttachment, 0, iSystemFlags, fTTL);
			conprintf(0, "Loaded %u systems from \"%s\"\n", nLoaded, CMD_ARGV(1));
		}
		else
			conprintf(0, "usage: %s <system.txt> [followentindex] [attachment] [sysflags] [ttl]\n", CMD_ARGV(0));
	}
	else
		conprintf(0, "RenderManager is offline.\n");
}

//-----------------------------------------------------------------------------
// Purpose: RS search engine
//-----------------------------------------------------------------------------
void __CmdFunc_SearchRS(void)
{
	if (g_pRenderManager)
		g_pRenderManager->Cmd_SearchRS();
	else
		conprintf(0, "RenderManager is offline.\n");
}

//-----------------------------------------------------------------------------
// Purpose: Request statistics from the server at any time
//-----------------------------------------------------------------------------
void __CmdFunc_ShowStats(void)
{
	if (gViewPort)
	{
		if (gViewPort->GetScoreBoard()->m_flWaitForStatsTime > gEngfuncs.GetClientTime())
			return;// already waiting

		conprintf(1, "Requesting statistics...\n");
		gViewPort->GetScoreBoard()->m_flWaitForStatsTime = gEngfuncs.GetClientTime() + 2.0f;
		SERVER_COMMAND(CMD_ARGV(0));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy cvar
//-----------------------------------------------------------------------------
void __CmdFunc_CVarCpy(void)
{
	if (CMD_ARGC() == 2)
	{
#if defined (CLDLL_NEWFUNCTIONS)
		gEngfuncs.Cvar_Set(CMD_ARGV(1), CVAR_GET_STRING(CMD_ARGV(2)));
#else
		CVAR_SET_FLOAT(CMD_ARGV(1), CVAR_GET_FLOAT(CMD_ARGV(2)));
#endif
	}
	else
		conprintf(0, "usage: %s <destination> <source>\n", CMD_ARGV(0));
}

//-----------------------------------------------------------------------------
// Purpose: Start creating entity at my position
//-----------------------------------------------------------------------------
void __CmdFunc_CreateAtMe(void)
{
	if (gViewPort && !gHUD.IsSpectator())
	{
		if (gViewPort->AllowedToPrintText())
		{
			CEntityEntryPanel *pPanel = new CEntityEntryPanel(-1,-1, XRES(EEP_WIDTH), YRES(EEP_HEIGHT));
			cl_entity_t *pPlayer = gEngfuncs.GetLocalPlayer();
			if (pPanel)
			{
				pPanel->m_vTargetOrigin = pPlayer->origin;//gHUD.m_vecOrigin;
				pPanel->m_vTargetAngles = g_vecViewAngles;//gHUD.m_vecAngles;
				//pPanel->m_iRenderSystemIndex = invalid;
				gViewPort->ShowMenuPanel(pPanel, false);
			}
		}
	}
}

#if defined (_DEBUG)
void __CmdFunc_DBG_SetFog(void)
{
	if (g_pCvarDeveloper->value > 0.0)
	{
		if (gHUD.m_iGameType == GT_SINGLE)
		{
			if (CMD_ARGC() < 5)
				conprintf(0, "usage: %s r g b StartDist EndDist\n", CMD_ARGV(0));
			else
			{
				if (atoi(CMD_ARGV(5)) == 0)
				{
					ResetFog();
					gHUD.m_iFogMode = 0;
				}
				else
				{
					RenderFog(atoi(CMD_ARGV(1)),atoi(CMD_ARGV(2)),atoi(CMD_ARGV(3)), atof(CMD_ARGV(4)), atof(CMD_ARGV(5)), false);
					gHUD.m_iFogMode = 1;
				}
			}
		}
		else
			conprintf(0, "command not allowed\n");
	}
}

/*void __CmdFunc_DBG_ApplyForce(void)
{
	if (g_pRenderManager)
		g_pRenderManager->ApplyForce(gHUD.m_vecOrigin, Vector(0,128,0), 256.0, atoi(CMD_ARGV(1))>0?true:false);
}*/

void __CmdFunc_DBG_DumpScoreBoard(void)
{
	if (gViewPort && gViewPort->GetScoreBoard())
		gViewPort->GetScoreBoard()->DumpInfo();
}

/*void __CmdFunc_DBG_PlaySound(void)
{
	PlaySound(CMD_ARGV(1), VOL_NORM);
}*/

void __CmdFunc_DBG_SaySound(void)
{
	if (CMD_ARGC() >= 2)
	{
		if (gHUD.m_pLocalPlayer)
			gEngfuncs.pEventAPI->EV_PlaySound(gEngfuncs.GetLocalPlayer()->index, gHUD.m_pLocalPlayer->origin, CHAN_VOICE, CMD_ARGV(1), VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	}
	else
		conprintf(0, "Play sound through CHAN_VOICE on client side without precaching.\n");
}

// dbg_screenfade "127 191 127" 2 1 0
void __CmdFunc_DBG_ScreenFade(void)
{
	if (CMD_ARGC() == 5)
	{
		byte r,g,b,a;
		StringToRGBA(CMD_ARGV(1), r,g,b,a);
		CL_ScreenFade(r,g,b, a, atof(CMD_ARGV(2)), atof(CMD_ARGV(3)), atoi(CMD_ARGV(4)));
	}
	else
		conprintf(0, "Usage: %s <\"r g b a\"> <fadetime> <fadehold> <flags>\n", CMD_ARGV(0));
}

void __CmdFunc_DBG_ClientsData(void)
{
	for (CLIENT_INDEX i=1; i<=gEngfuncs.GetMaxClients(); ++i)
	{
		cl_entity_t *pPlayer = gEngfuncs.GetEntityByIndex(i);
		if (pPlayer)
		{
			conprintf(0, " client %d: origin: sv (%g %g %g) cl (%g %g %g), angles: sv (%g %g %g) cl (%g %g %g)\n", i,
				pPlayer->curstate.origin.x, pPlayer->curstate.origin.y, pPlayer->curstate.origin.z,
				pPlayer->origin.x, pPlayer->origin.y, pPlayer->origin.z,
				pPlayer->curstate.angles.x, pPlayer->curstate.angles.y, pPlayer->curstate.angles.z,
				pPlayer->angles.x, pPlayer->angles.y, pPlayer->angles.z);
		}
	}
}

/*void __CmdFunc_UI_BuildMode(void)
{
	if (gViewPort)
		App::getInstance()->enableBuildMode();
}*/

#endif // _DEBUG

// Register commands into engine so they are available from console
void CL_RegisterCommands(void)
{
	HOOK_COMMAND("+commandmenu", OpenCommandMenu);
	HOOK_COMMAND("-commandmenu", CloseCommandMenu);
	HOOK_COMMAND("commandmenu", ToggleCommandMenu);
	HOOK_COMMAND("closemenus", CloseMenus);// For HL1111+
	HOOK_COMMAND("ForceCloseCommandMenu", ForceCloseCommandMenu);
	HOOK_COMMAND("togglebrowser", ToggleServerBrowser);
	HOOK_COMMAND("togglemplayer", ToggleMusicPlayer);// XDM
	HOOK_COMMAND("showmotd", ShowMOTD);
	HOOK_COMMAND("showmapbriefing", MapBriefing);// XDM3035: autocompletion made me go mad
	// XDM3038: undone	HOOK_COMMAND("showmapinfo", MapInfo);
	HOOK_COMMAND("voicetweak", VoiceTweak);
	HOOK_COMMAND("vguimenu", VGUIMenu);
	HOOK_COMMAND("chooseteam", ChooseTeam);
	HOOK_COMMAND("cvarcpy", CVarCpy);// copy arg2 to arg1
	HOOK_COMMAND("dir", Dir);
	HOOK_COMMAND("ls", Dir);
	HOOK_COMMAND("profilesave", ProfileSave);
	HOOK_COMMAND("ui_reloadscheme", UI_ReloadScheme);
	HOOK_COMMAND("settmp", SetTmp);
	HOOK_COMMAND("listrs", ListRS);
	HOOK_COMMAND("loadrs", LoadRS);
	HOOK_COMMAND("searchrs", SearchRS);
	HOOK_COMMAND("showstats", ShowStats);
	HOOK_COMMAND("toggleselmode", ToggleSelectionMode);
	HOOK_COMMAND("createatme", CreateAtMe);
	// These are developer-only commands, built only in debug mode
#if defined (_DEBUG)
	HOOK_COMMAND("dbg_setfog", DBG_SetFog);
	//HOOK_COMMAND("dbg_appforce", DBG_ApplyForce);
	HOOK_COMMAND("dbg_dumpsb", DBG_DumpScoreBoard);
	//HOOK_COMMAND("dbg_playsnd", DBG_PlaySound);
	HOOK_COMMAND("dbg_saysnd", DBG_SaySound);
	HOOK_COMMAND("dbg_screenfade", DBG_ScreenFade);// XDM3037a
	HOOK_COMMAND("dbg_showclients", DBG_ClientsData);// XDM3038a
	//bad	HOOK_COMMAND("ui_buildmode", UI_BuildMode);
#endif
}
