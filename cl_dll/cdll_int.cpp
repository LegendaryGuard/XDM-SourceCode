/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
//#undef INTERFACE_H
#include "../public/interface.h"
#include "hud_servers.h"
//#include "interface.h"
#include "in_defs.h"
#include "musicplayer.h"
#include "RenderManager.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "r_studioint.h"
#include "bsputil.h"
#include "studio_util.h"
#include "vgui_Int.h"
#include "vgui_Viewport.h"
#include "vgui_ScorePanel.h"
//#include "studio.h"
//#include "StudioModelRenderer.h"
//#include "GameStudioModelRenderer.h"
#include "voice_status.h"
#include "port.h"
//lin #include <dlfcn.h>
#include "Exports.h"
#include "tri.h"
#include "gl_dynamic.h"

cl_enginefunc_t gEngfuncs;
CHud gHUD;

// HL20130901
#include "particleman.h"
CSysModule *g_hParticleManModule = NULL;
IParticleMan *g_pParticleMan = NULL;

// HL20130901
void CL_LoadParticleMan( void );
void CL_UnloadParticleMan( void );

cvar_t *g_pCvarLW = NULL;
cvar_t *g_pCvarSuitVolume = NULL;
cvar_t *g_pCvarDeveloper = NULL;
cvar_t *g_pCvarServerZMax = NULL;
//cvar_t *g_pCvarTimeLeft = NULL;
//cvar_t *g_pCvarScoreLeft = NULL;
cvar_t *g_pCvarEffects = NULL;
cvar_t *g_pCvarEffectsDLight = NULL;// XDM3038
cvar_t *g_pCvarParticles = NULL;
cvar_t *g_pCvarScheme = NULL;
cvar_t *g_pCvarTFX = NULL;
cvar_t *g_pCvarDeathView = NULL;
cvar_t *g_pCvarPickupVoice = NULL;
cvar_t *g_pCvarAmmoVoice = NULL;// XDM3038a
cvar_t *g_pCvarAnnouncer = NULL;
cvar_t *g_pCvarAnnouncerEvents = NULL;// XDM3037a
cvar_t *g_pCvarAnnouncerLCombo = NULL;// XDM3037a
cvar_t *g_pCvarHLPlayers = NULL;
cvar_t *g_pCvarViewDistance = NULL;
cvar_t *g_pCvarFlashLightMode = NULL;
cvar_t *g_pCvarDefaultFOV = NULL;// at this value mouse sensitivity will be at 1:1
cvar_t *g_pCvarZSR = NULL;
cvar_t *g_pCvarCameraAngles = NULL;
cvar_t *g_pCvarLogStats = NULL;
cvar_t *g_pCvarLODDist = NULL;// XDM3038b
cvar_t *g_pCvarRenderSystem = NULL;// XDM3038: potentially dangerous!
cvar_t *g_pCvarPSAutoRate = NULL;// XDM3038c: automatically decrease rate of particle systems when overflowed
cvar_t *g_pCvarVoiceIconOffset = NULL;// XDM3038a
cvar_t *g_pCvarMasterServerFile = NULL;// XDM3038c
cvar_t *g_pCvarTmp = NULL;
#if defined (_DEBUG_ANGLES)
cvar_t *g_pCvarDbgAngles = NULL;
cvar_t *g_pCvarDbgAnglesClient = NULL;
cvar_t *g_pCvarDbgAnglesMult = NULL;
#endif
#if defined (_DEBUG)
cvar_t *cl_test1 = NULL;
cvar_t *cl_test2 = NULL;
cvar_t *cl_test3 = NULL;
#endif

int g_iWaterLevel = 0;// XDM: FOG

#if !defined (CLDLL_NOFOG)
// Dynamic loading of OpenGL library
#if defined (_WIN32)

HMODULE hOpenGLDLL = NULL;

HMODULE LibLoadOpenGL(void)
{
	return GetModuleHandle("opengl32.dll");
}

FARPROC LibLoadExport(HMODULE pHandle, const char *pName)
{
	return GetProcAddress(pHandle, pName);
}

#else // _WIN32

#include <dlfcn.h>

void *hOpenGLDLL = NULL;

void *LibLoadOpenGL(void)
{
	return dlopen("libGL.so", RTLD_LAZY);
}

void *LibLoadExport(void *pHandle, const char *pName)
{
	return dlsym(pHandle, pName);
}

#endif // _WIN32

GLAPI_glEnable GL_glEnable = NULL;
GLAPI_glDisable GL_glDisable;
GLAPI_glFogi GL_glFogi;
GLAPI_glFogf GL_glFogf;
GLAPI_glFogfv GL_glFogfv;
GLAPI_glHint GL_glHint;
#endif // !CLDLL_NOFOG


unsigned short g_usPM_Fall;// XDM3035a


class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		byte r,g,b;
		if (GetPlayerColor(entindex, r,g,b))
		{
			color[0] = r; color[1] = g; color[2] = b;
		}
		else
		{
			color[0] = color[1] = color[2] = 255;
		}
	}

	virtual void UpdateCursorState()
	{
		gViewPort->UpdateCursorState();
	}

	virtual int	GetAckIconHeight()
	{
		return ScreenHeight - gHUD.m_iFontHeight*3 - 6;
	}

	virtual bool CanShowSpeakerLabels()
	{
		if (gViewPort && gViewPort->GetScoreBoard())
			return !gViewPort->GetScoreBoard()->isVisible();
		else
			return false;
	}
};

CHLVoiceStatusHelper g_VoiceStatusHelper;


void CL_RegisterVariables(void)
{
	// search by name is faster when there are less items so search now, then register new cvars
	g_pCvarLW				= CVAR_GET_POINTER("cl_lw");
	g_pCvarSuitVolume		= CVAR_GET_POINTER("suitvolume");
	g_pCvarDeveloper		= CVAR_GET_POINTER("developer");
	g_pCvarServerZMax		= CVAR_GET_POINTER("sv_zmax");// somehow this exists on client
	// tst	if (g_pCvarServerZMax == NULL) g_pCvarServerZMax = CVAR_CREATE("sv_zmax", "4096", FCVAR_CLIENTDLL);
	//g_pCvarTimeLeft			= NULL;// XDM: it does not extst yet
	//g_pCvarScoreLeft		= NULL;

	g_pCvarEffects			= CVAR_CREATE("cl_effects",				"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarEffectsDLight	= CVAR_CREATE("cl_effects_dlight",		"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarParticles		= CVAR_CREATE("cl_particles",			"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarScheme			= CVAR_CREATE("cl_scheme",				"",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarTFX				= CVAR_CREATE("cl_tournamentfx",		"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarDeathView		= CVAR_CREATE("cl_death_view",			"0",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarPickupVoice		= CVAR_CREATE("cl_pickup_voice",		"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarAmmoVoice		= CVAR_CREATE("cl_ammo_voice",			"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3038a
	g_pCvarAnnouncer		= CVAR_CREATE("cl_announcer_voice",		"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarAnnouncerEvents	= CVAR_CREATE("cl_announcer_events",	"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3037a
	g_pCvarAnnouncerLCombo	= CVAR_CREATE("cl_announcer_losecombo",	"0",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3037a
	g_pCvarHLPlayers		= CVAR_CREATE("cl_highlight_players",	"0",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarViewDistance		= CVAR_CREATE("cl_viewdist",			"2048",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3035c
	g_pCvarFlashLightMode	= CVAR_CREATE("cl_flashlightmode",		"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3037
	g_pCvarDefaultFOV		= CVAR_CREATE("default_fov",			"90",		FCVAR_CLIENTDLL);
	g_pCvarZSR				= CVAR_CREATE("zoom_sensitivity_ratio",	"1.0",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	//g_pCvarCameraAngles	= CVAR_CREATE("cam_angles",				"0 0 0",	FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	g_pCvarLogStats			= CVAR_CREATE("cl_log_stats",			"0",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3037: log player stats to files
	g_pCvarLODDist			= CVAR_CREATE("r_lod_dist",				"1024",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3038b: distance after which next LOD is switched
	g_pCvarRenderSystem		= CVAR_CREATE("r_rendersystem",			"1",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_CHEAT);// XDM3038: should never be changed in game!!
	g_pCvarPSAutoRate		= CVAR_CREATE("r_ps_autorate",			"0",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3038c
	g_pCvarVoiceIconOffset	= CVAR_CREATE("cl_voiceiconoffset",		"20",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3038a
	g_pCvarMasterServerFile	= CVAR_CREATE("cl_masterserverfile",	"valvecomm.lst",	FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3038c
	g_pCvarTmp				= CVAR_CREATE("tmpcvar",				"1",		FCVAR_UNLOGGED | FCVAR_CLIENTDLL);
#if defined (_DEBUG_ANGLES)
	g_pCvarDbgAngles		= CVAR_CREATE("dbg_angles",				"0",		FCVAR_UNLOGGED | FCVAR_CLIENTDLL);
	g_pCvarDbgAnglesClient	= CVAR_CREATE("dbg_angles_client",		"1",		FCVAR_UNLOGGED | FCVAR_CLIENTDLL);
	g_pCvarDbgAnglesMult	= CVAR_CREATE("dbg_angles_mult",		"1",		FCVAR_UNLOGGED | FCVAR_CLIENTDLL);
#endif
#if defined (_DEBUG)
	cl_test1 = CVAR_CREATE("cl_test1", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	cl_test2 = CVAR_CREATE("cl_test2", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	cl_test3 = CVAR_CREATE("cl_test3", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
#endif
}


/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int CL_DLLEXPORT HUD_GetHullBounds(int hullnumber, float *mins, float *maxs)
{
	DBG_CL_PRINT("HUD_GetHullBounds(%d)\n", hullnumber);
//	RecClGetHullBounds(hullnumber, mins, maxs);
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
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	CL_DLLEXPORT HUD_ConnectionlessPacket(const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size)
{
	DBG_CL_PRINT("HUD_ConnectionlessPacket()\n");
//	RecClConnectionlessPacket(net_from, args, response_buffer, response_buffer_size);
	// Parse stuff from args
//	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	CON_DPRINTF("Connectionless packet from %d.%d.%d.%d: \"%s\"\n", net_from->ip[0], net_from->ip[1], net_from->ip[2], net_from->ip[3], args);

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

// XDM: NOTE: called once by the engine (at game startup)
void CL_DLLEXPORT HUD_PlayerMoveInit(struct playermove_s *ppmove)
{
	DBG_CL_PRINT("HUD_PlayerMoveInit(%d)\n", ppmove->player_index);
//	RecClClientMoveInit(ppmove);
	PM_Init(ppmove);
}

char CL_DLLEXPORT HUD_PlayerMoveTexture(char *name)
{
	DBG_CL_PRINT("HUD_PlayerMoveTexture(%s)\n", name);
//	RecClClientTextureType(name);
	return PM_FindTextureType(name);
}

void CL_DLLEXPORT HUD_PlayerMove(struct playermove_s *ppmove, int server)
{
	DBG_CL_PRINT("HUD_PlayerMove(%d, %d)\n", ppmove->player_index, server);
//	RecClClientMove(ppmove, server);
#if defined (CORRECT_PITCH)
	if (ppmove)
		ppmove->cmd.viewangles[PITCH] *= -1.0f;
#endif
	PM_Move(ppmove, server);
	if (ppmove)
	{
		g_iWaterLevel = ppmove->waterlevel;// XDM
		DBG_ANGLES_DRAW(11, ppmove->origin, ppmove->angles, pmove->player_index+1, "PlayerMove() ppmove");
		//DBG_ANGLES_NPRINT(11, "PlayerMove() pitch %f, cmd %f", ppmove->angles[PITCH], ppmove->cmd.viewangles[PITCH]);
	}
}

int CL_DLLEXPORT Initialize(cl_enginefunc_t *pEnginefuncs, int iVersion)
{
	gEngfuncs = *pEnginefuncs;
//	RecClx(pEnginefuncs, iVersion);
	// XDM: TODO: find a reliable way to get game/interface/protocol version
	//	(*pEnginefuncs->pfnGetCvarString)("sv_version");

	if (iVersion != CLDLL_INTERFACE_VERSION)
		return 0;

	memcpy(&gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	DBG_CL_PRINT("CL Initialize(%d)\n", iVersion);

	//if (CVAR_GET_POINTER("host_clientloaded") != NULL)
	//	g_fXashEngine = TRUE;

	conprintf(0, "Initializing X-Half-Life client DLL (%s build %s) interface version %d\n", BUILD_DESC, __DATE__, iVersion);
	EV_HookEvents();
	// HL20130901
	// XDM3037	CL_LoadParticleMan();
	// get tracker interface, if any
	return 1;
}


extern char g_sMapName[MAX_MAPPATH];

/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/
int CL_DLLEXPORT HUD_VidInit(void)
{
	DBG_CL_PRINT("HUD_VidInit()\n");
//	RecClHudVidInit();
	if (g_pRenderManager != NULL)// XDM
		g_pRenderManager->DeleteAllSystems();

	g_sMapName[0] = '\0';// XDM3038a
	memset(g_PlayerExtraInfo, 0, sizeof(extra_player_info_t)*(MAX_PLAYERS+1));// XDM3038c
	memset(g_TeamInfo, 0, sizeof(team_info_t)*(MAX_TEAMS+1));// XDM3038c

	gHUD.VidInit();

	VGui_Startup();

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		GetClientVoiceMgr()->VidInit();// XDM3035c: this will (probably) work better after VGui_Startup
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		conprintf(0, "GetClientVoiceMgr()->VidInit() exception!\n");
	}
#endif

#if !defined (CLDLL_NOFOG)
	gHUD.m_iHardwareMode = IEngineStudio.IsHardware();
	if (gHUD.m_iHardwareMode == 1)// XDM3035: OpenGL
	{
		if (hOpenGLDLL == NULL)
		{
			hOpenGLDLL = LibLoadOpenGL();// XDM3035c: forcibly reuse loaded module, if present (not OpenGL otherwise).
			if (hOpenGLDLL)
			{
				conprintf(1, "CL: OpenGL library loaded.\n");
				GL_glEnable = (GLAPI_glEnable)LibLoadExport(hOpenGLDLL, "glEnable");
				GL_glDisable = (GLAPI_glDisable)LibLoadExport(hOpenGLDLL, "glDisable");
				GL_glFogi = (GLAPI_glFogi)LibLoadExport(hOpenGLDLL, "glFogi");
				GL_glFogf = (GLAPI_glFogf)LibLoadExport(hOpenGLDLL, "glFogf");
				GL_glFogfv = (GLAPI_glFogfv)LibLoadExport(hOpenGLDLL, "glFogfv");
				GL_glHint = (GLAPI_glHint)LibLoadExport(hOpenGLDLL, "glHint");

				if (GL_glEnable)
					conprintf(1, "CL: OpenGL mode initialized.\n");
			}
			else
				conprintf(0, "CL: OpenGL mode failed to initialize!\n");
		}
	}
#endif // !CLDLL_NOFOG

	return 1;
}


/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/
void CL_DLLEXPORT HUD_Init(void)
{
	DBG_CL_PRINT("CL HUD_Init(%d)\n");
//	RecClHudInit();
	InitInput();

	CL_RegisterVariables();// XDM
	CL_RegisterCommands();// XDM
	CL_RegisterMessages();// XDM

	UTIL_LoadRawPalette("gfx/palette.lmp");// XDM3038c

	//CL_Precache();// right now most resources are not ready

	g_usPM_Fall = gEngfuncs.pfnPrecacheEvent(1, "events/pm/fall.sc");

	memset(&g_PlayerStats[0][0], 0, sizeof(g_PlayerStats[0][0])*(MAX_PLAYERS+1)*STAT_NUMELEMENTS);// TODO: 2 elements: local and winner

	gHUD.Init();

	Scheme_Init();

	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);// XDM3035c

	BGM_Init();// XDM

#ifndef NORENDERSYSTEM
	if (g_pRenderManager == NULL && g_pCvarRenderSystem->value > 0 && !gEngfuncs.CheckParm("-norendersystem", NULL))
		g_pRenderManager = new CRenderManager();// XDM
#endif
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/
//-----------------------------------------------------------------------------
// Purpose: called every screen frame to redraw the HUD.
// Input  : flTime - client time in seconds
//			intermission - 1 if intermission, 0 if normal game
// Output : return 1 if something was drawn
//-----------------------------------------------------------------------------
int CL_DLLEXPORT HUD_Redraw(float time, int intermission)
{
//	DBG_CL_PRINT("HUD_Redraw(%d)\n", intermission);
//	RecClHudRedraw(time, intermission);
	VIS_Frame(time);// XDM3035c: HUD_Frame() could be a proper place for this, but engine keeps calling it after server is stopped (critical/precache/network error) which causes crashes (g_pWorld cannot be set to NULL beforehand).
#if defined (_DEBUG)
		ASSERT(g_vecZero.IsZero());
#endif
	//conprintf(1, "HUD_Redraw(%f, %d)\n", time, intermission);
	return gHUD.Redraw(time, intermission);// XDM3038a
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/
int CL_DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime)
{
	DBG_CL_PRINT("HUD_UpdateClientData(iWeaponBits %d)\n", pcldata->iWeaponBits);
//	RecClHudUpdateClientData(pcldata, flTime);
	IN_Commands();

	return gHUD.UpdateClientData(pcldata, flTime);
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/
void CL_DLLEXPORT HUD_Reset(void)
{
	DBG_CL_PRINT("HUD_Reset()\n");
//	RecClHudReset();
//	if (g_pRenderManager)// XDM
//		g_pRenderManager->KillAllSystems();

	gHUD.VidInit();
}


float g_fEntityHighlightEffect = 0;
/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/
void CL_DLLEXPORT HUD_Frame(double time)
{
	DBG_CL_PRINT("HUD_Frame()\n");
//	RecClHudFrame(time);
	/*try
	{
		VIS_Frame(time);
	}
	catch(...)
	{
		conprint(2, "CL: VIS_Frame exception!\n");
	}*/

	if (gHUD.m_iActive)
		GetAllPlayersInfo();

	ServersThink(time);

	if (gHUD.m_iActive)
	{
#if defined (USE_EXCEPTIONS)
		try
		{
#endif
			GetClientVoiceMgr()->Frame(time);
#if defined (USE_EXCEPTIONS)
		}
		catch (...)
		{
			conprintf(2, "GetClientVoiceMgr()->Frame() exception!\n");
		}
#endif
		if (gHUD.m_iPaused == 0)
		{
			g_fEntityHighlightEffect = sinf(gEngfuncs.GetClientTime()*6.0f);
			//conprintf(1, "EHE: %g\n", g_fEntityHighlightEffect);
		}
	}

	//if (gViewPort)// XDM3038a: player data is corrupted when the engine is shutting down
	//	gViewPort->StartFrame(time);

	BGM_StartFrame();
#if defined (_DEBUG)
		ASSERT(g_vecZero.IsZero());// memory corruption detection
#endif
}


/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/
void CL_DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	DBG_CL_PRINT("HUD_VoiceStatus(%d, %d)\n", entindex, bTalking);
////	RecClVoiceStatus(entindex, bTalking);
	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, bTalking);
}

/*
==========================
HUD_DirectorMessage

Called when a director event message was received
==========================
*/
void CL_DLLEXPORT HUD_DirectorMessage(int iSize, void *pbuf)
{
	DBG_CL_PRINT("HUD_DirectorMessage(%d)\n", iSize);
//	RecClDirectorMessage(iSize, pbuf);
	gHUD.m_Spectator.DirectorMessage(iSize, pbuf);
}

/*
==========================
HUD_Shutdown

Called whenever the client disconnects from a server.
==========================
*/
void CL_DLLEXPORT HUD_Shutdown( void )
{
	DBG_CL_PRINT("HUD_Shutdown()\n");
//	RecClShutdown();
	//g_iModelIndexSmkball = 0;
	g_pWorld = NULL;
	g_pViewLeaf = NULL;// XDM3038

	if (g_pRenderManager)
	{
		// in destructor	g_pRenderManager->DeleteAllSystems();
		delete g_pRenderManager;
		g_pRenderManager = NULL;
	}

	hOpenGLDLL = NULL;// XDM3035c: NEVER free this!

	ShutdownInput();

#if defined( _TFC )
	ClearEventList();
#endif

	// XDM3037	CL_UnloadParticleMan();

	BGM_Shutdown();// XDM
}

// XDM: A guess
int	CL_DLLEXPORT HUD_GetPlayerTeam(int playerIndex)
{
	DBG_CL_PRINT("HUD_GetPlayerTeam(%d)\n", playerIndex);
	if (IsValidPlayerIndex(playerIndex))
		return g_PlayerExtraInfo[playerIndex].teamnumber;

	return TEAM_NONE;
}

/*void CL_DLLEXPORT HUD_ClipMoveToEntity(physent_t *pe, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, pmtrace_t *tr)
{
}*/

void CL_UnloadParticleMan( void )
{
	DBG_CL_PRINT("CL_LoadParticleMan()\n");
	Sys_UnloadModule( g_hParticleManModule );

	g_pParticleMan = NULL;
	g_hParticleManModule = NULL;
}

void CL_LoadParticleMan( void )
{
	DBG_CL_PRINT("CL_LoadParticleMan()\n");
	char szPDir[512];

	if ( gEngfuncs.COM_ExpandFilename( PARTICLEMAN_DLLNAME, szPDir, sizeof( szPDir ) ) == FALSE )
	{
		g_pParticleMan = NULL;
		g_hParticleManModule = NULL;
		return;
	}

	g_hParticleManModule = Sys_LoadModule( szPDir );
	CreateInterfaceFn particleManFactory = Sys_GetFactory( g_hParticleManModule );

	if ( particleManFactory == NULL )
	{
		g_pParticleMan = NULL;
		g_hParticleManModule = NULL;
		return;
	}

	g_pParticleMan = (IParticleMan *)particleManFactory( PARTICLEMAN_INTERFACE, NULL);

	if ( g_pParticleMan )
	{
		 g_pParticleMan->SetUp( &gEngfuncs );

		 // Add custom particle classes here BEFORE calling anything else or you will die.
		 g_pParticleMan->AddCustomParticleClassSize ( sizeof ( CBaseParticle ) );
	}
}

// HL112x export, optional, extended API
//#ifdef CLDLL_EXPORT_F

//?cldll_func_dst_t *g_pcldstAddrs;

extern "C" void CL_DLLEXPORT F(void *pv)
{
	DBG_CL_PRINT("CL F()\n");
	cldll_func_t *pcldll_func = (cldll_func_t *)pv;

	// Hack!
//?	g_pcldstAddrs = ((cldll_func_dst_t *)pcldll_func->pHudVidInitFunc);

	cldll_func_t cldll_func = 
	{
	Initialize,
	HUD_Init,
	HUD_VidInit,
	HUD_Redraw,
	HUD_UpdateClientData,
	HUD_Reset,
	HUD_PlayerMove,
	HUD_PlayerMoveInit,
	HUD_PlayerMoveTexture,
	IN_ActivateMouse,
	IN_DeactivateMouse,
	IN_MouseEvent,
	IN_ClearStates,
	IN_Accumulate,
	CL_CreateMove,
	CL_IsThirdPerson,
	CL_CameraOffset,
	KB_Find,
	CAM_Think,
	V_CalcRefdef,
	HUD_AddEntity,
	HUD_CreateEntities,
	HUD_DrawNormalTriangles,
	HUD_DrawTransparentTriangles,
	HUD_StudioEvent,
	HUD_PostRunCmd,
	HUD_Shutdown,
	HUD_TxferLocalOverrides,
	HUD_ProcessPlayerState,
	HUD_TxferPredictionData,
	Demo_ReadBuffer,
	HUD_ConnectionlessPacket,
	HUD_GetHullBounds,
	HUD_Frame,
	HUD_Key_Event,
	HUD_TempEntUpdate,
	HUD_GetUserEntity,
	HUD_VoiceStatus,
	HUD_DirectorMessage,
	HUD_GetStudioModelInterface,
	HUD_ChatInputPosition,
	HUD_GetPlayerTeam// XDM3037
	};

	*pcldll_func = cldll_func;
}
//#endif // CLDLL_EXPORT_F

#include "cl_dll/IGameClientExports.h"

//-----------------------------------------------------------------------------
// Purpose: Exports functions that are used by the gameUI for UI dialogs
//-----------------------------------------------------------------------------
class CClientExports : public IGameClientExports
{
public:
	// returns the name of the server the user is connected to, if any
	virtual const char *GetServerHostName()
	{
		if (gViewPort)
		{
			return gViewPort->m_szServerName;// GetServerName();
		}
		return "";
	}

	// ingame voice manipulation
	virtual bool IsPlayerGameVoiceMuted(int playerIndex)
	{
		if (GetClientVoiceMgr())
			return GetClientVoiceMgr()->IsPlayerBlocked(playerIndex);
		return false;
	}

	virtual void MutePlayerGameVoice(int playerIndex)
	{
		if (GetClientVoiceMgr())
		{
			GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, true);
		}
	}

	virtual void UnmutePlayerGameVoice(int playerIndex)
	{
		if (GetClientVoiceMgr())
		{
			GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, false);
		}
	}
};

EXPOSE_SINGLE_INTERFACE(CClientExports, IGameClientExports, GAMECLIENTEXPORTS_INTERFACE_VERSION);
