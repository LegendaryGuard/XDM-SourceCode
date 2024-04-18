//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "hud.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "vgui_Viewport.h"
#include "vgui_SpectatorPanel.h"
#include "hltv.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "parsemsg.h"
// these are included for the math functions
#include "demo_api.h"
#include "event_api.h"
#include "bsputil.h"

// !!! WARNING !!!
// This code is used not only by spectators, but to draw minimap too!

//#pragma warning(disable: 4244)

#define WORLDSIZE 4096.0f// this constant is for overview maps only

extern void V_GetInEyePos(int entity, float *origin, float *angles);
extern void V_ResetChaseCam(void);
extern void V_GetChasePos(cl_entity_t *ent, float *cl_angles, float *origin, float *angles, int flags);// XDM

extern int		iJumpSpectator;
extern vec3_t	vJumpOrigin;
extern vec3_t	vJumpAngles; 

extern vec3_t v_origin;		// last view origin
extern vec3_t v_angles;		// last view angle
extern vec3_t v_cl_angles;	// last client/mouse angle
extern vec3_t v_sim_org;	// last sim origin

void CMD_SpectatorMode(void)
{
	if (CMD_ARGC() <= 1)
	{
		conprintf(0, "usage:  %s <main mode> [inset mode]\n", CMD_ARGV(0));
		return;
	}

	// SetModes() will decide if command is executed on server or local
	if (CMD_ARGC() == 2)
		gHUD.m_Spectator.SetModes(atoi(CMD_ARGV(1)), -1);
	else if (CMD_ARGC() == 3)
		gHUD.m_Spectator.SetModes(atoi(CMD_ARGV(1)), atoi(CMD_ARGV(2)));	
}

void CMD_SpectatorSpray(void)
{
	if (!gEngfuncs.IsSpectateOnly())
		return;

	vec3_t forward;
	AngleVectors(v_angles,forward,NULL,NULL);
	VectorScale(forward, 128, forward);
	VectorAdd(forward, v_origin, forward);
	pmtrace_t * trace = gEngfuncs.PM_TraceLine(v_origin, forward, PM_TRACELINE_PHYSENTSONLY, 2, -1);
	if (trace->fraction != 1.0)
	{
		char string[128];
		_snprintf(string, 128, "drc_spray %.2f %.2f %.2f %i", trace->endpos[0], trace->endpos[1], trace->endpos[2], trace->ent);
		SERVER_COMMAND(string);
	}
}

void CMD_SpectatorHelp(void)
{
	if (gViewPort)
	{
		gViewPort->ShowMenuPanel(MENU_SPECHELP);
	}
	else
	{
  		char *text = BufferedLocaliseTextString("#Spec_Help_Text");
		if (text)
		{
			while (*text)
			{
				if (*text != 13)
					CON_PRINTF("%c", *text);
				++text;
			}
		}
	}
}

void CMD_SpectatorMenu(void)
{
	if (CMD_ARGC() <= 1)
	{
		conprintf(0, "usage: %s <0|1>\n", CMD_ARGV(0));
		return;
	}
	if (gViewPort)
	{
		if (gViewPort->GetSpectatorPanel())
			gViewPort->GetSpectatorPanel()->ShowMenu(atoi(CMD_ARGV(1)) != 0);
	}
}

void CMD_ToggleScores(void)
{
	if (gViewPort)
	{
		if (gViewPort->IsScoreBoardVisible())
			gViewPort->HideScoreBoard();
		else
			gViewPort->ShowScoreBoard();
	}
}

void CMD_SpectatorPIP(void)// XDM
{
	if (CMD_ARGC() <= 1)
	{
		conprintf(0, "usage: %s <inset mode>\n current mode: %d\n", CMD_ARGV(0), gHUD.m_Spectator.m_iInsetMode);
		return;
	}
	gHUD.m_Spectator.SetModes(-1, gHUD.m_Spectator.ToggleInset(atoi(CMD_ARGV(1)), true));
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHudSpectator::Init(void)
{
	DBG_HUD_PRINTF("CHudSpectator::Init()\n");
	gHUD.AddHudElem(this);

	m_iFlags |= (HUD_ACTIVE|HUD_DRAW_ALWAYS);// XDM3035a
	m_flNextObserverInput = 0.0f;
	m_zoomDelta	= 0.0f;
	m_moveDelta = 0.0f;
	m_FOV = DEFAULT_FOV;// HL20130901
	m_chatEnabled = (gHUD.m_SayText.m_pCvarSayText->value > 0.0f);
	iJumpSpectator	= 0;

	memset(&m_OverviewData, 0, sizeof(m_OverviewData));
	memset(&m_OverviewEntities, 0, sizeof(m_OverviewEntities));
	m_lastPrimaryObject = m_lastSecondaryObject = 0;

	ADD_DOMMAND("spec_mode", CMD_SpectatorMode);
	ADD_DOMMAND("spec_decal", CMD_SpectatorSpray);
	ADD_DOMMAND("spec_help", CMD_SpectatorHelp);
	ADD_DOMMAND("spec_menu", CMD_SpectatorMenu);
	ADD_DOMMAND("togglescores", CMD_ToggleScores);
	ADD_DOMMAND("spec_pip", CMD_SpectatorPIP);// XDM

	m_drawnames		= CVAR_CREATE("spec_drawnames","1",FCVAR_CLIENTDLL);
	m_drawcone		= CVAR_CREATE("spec_drawcone","1",FCVAR_CLIENTDLL);
	m_drawstatus	= CVAR_CREATE("spec_drawstatus","1",FCVAR_CLIENTDLL);
	m_autoDirector	= CVAR_CREATE("spec_autodirector","1",FCVAR_CLIENTDLL);
	//m_pip			= CVAR_CREATE("spec_pip","3",FCVAR_CLIENTDLL);
	m_insetBounds	= CVAR_CREATE("v_inset_bounds", "0.3125 0.55 25 25", FCVAR_CLIENTDLL);// XDM3038a
	m_iInsetMode = INSET_OFF;

	if (!m_drawnames || !m_drawcone || !m_drawstatus || !m_autoDirector || !m_insetBounds)// || !m_pip)
	{
		conprintf(0, "ERROR: Couldn't register all spectator variables!\n");
		return 0;
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Loads new icons
//-----------------------------------------------------------------------------
int CHudSpectator::VidInit(void)
{
	DBG_HUD_PRINTF("CHudSpectator::VidInit()\n");
	m_hsprPlayer		= SPR_Load("sprites/iplayer.spr");
	m_hsprPlayerDead	= SPR_Load("sprites/iplayerdead.spr");
	m_hsprGameObject	= SPR_Load("sprites/iobject.spr");
	m_hsprUnkown		= SPR_Load("sprites/iunknown.spr");
	m_hsprUnkownMap		= SPR_Load("sprites/tile.spr");
	m_hsprBeam			= SPR_Load("sprites/laserbeam.spr");
	m_hsprCamera		= SPR_Load("sprites/camera.spr");

	int iCount = 0;
	client_sprite_t *pList = SPR_GetList("sprites/weapon_unknown.txt", &iCount);
	client_sprite_t *pSL = GetSpriteList(pList, "crosshair", gHUD.m_iRes, iCount);
	if (pSL)
	{
		char sz[64];
		_snprintf(sz, 64, "sprites/%s.spr", pSL->szSprite);
		m_hCrosshair = SPR_Load(sz);
		m_crosshairRect = pSL->rc;
	}
	else// hard-coded
	{
		m_hCrosshair = SPR_Load("sprites/crosshairs.spr");
		m_crosshairRect.left = 24;
		m_crosshairRect.right = 48;
		m_crosshairRect.top = 0;
		m_crosshairRect.bottom = 24;
	}
	// HL20130901
	m_lastPrimaryObject = m_lastSecondaryObject = 0;
	m_flNextObserverInput = 0.0f;
	m_lastHudMessage = 0;
	m_iSpectatorNumber = 0;
	iJumpSpectator	= 0;
	g_iUser1 = g_iUser2 = 0;
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Reset HUD
//-----------------------------------------------------------------------------
void CHudSpectator::Reset(void)
{
	DBG_HUD_PRINTF("CHudSpectator::Reset()\n");
	if (strcmp(m_OverviewData.map, GET_LEVEL_NAME()))
	{
		// update level overview if level changed
		ParseOverviewFile();
		LoadMapSprites();// call this even if ParseOverviewFile failed
	}

	memset(&m_OverviewEntities, 0, sizeof(m_OverviewEntities));

	// HL20130901
	m_FOV = DEFAULT_FOV;//90.0f;
	m_IsInterpolating = false;
	m_ChaseEntity = 0;

	SetSpectatorStartPosition();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpectator::InitHUDData(void)
{
	DBG_HUD_PRINTF("CHudSpectator::InitHUDData()\n");
	m_lastPrimaryObject = m_lastSecondaryObject = 0;
	m_flNextObserverInput = 0.0f;
	m_lastHudMessage = 0;
	m_iSpectatorNumber = 0;
	iJumpSpectator = 0;
	g_iUser1 = g_iUser2 = 0;

	memset(&m_OverviewData, 0, sizeof(m_OverviewData));
	memset(&m_OverviewEntities, 0, sizeof(m_OverviewEntities));

	if (gEngfuncs.IsSpectateOnly() || gEngfuncs.pDemoAPI->IsPlayingback())
		m_autoDirector->value = 1.0f;
	else
		m_autoDirector->value = 0.0f;

	Reset();

	SetModes(OBS_CHASE_FREE, -1);// XDM: INSET_OFF
	// HL20130901 suggests this	SetModes( OBS_CHASE_LOCKED, INSET_OFF );

	g_iUser2 = 0; // fake not target until first camera command

	// reset HUD FOV
	gHUD.m_fFOV = 0.0f;// XDM3037a: 0 now means "NO MODIFIERS"// gHUD.m_iFOV = gHUD.GetUpdatedDefaultFOV();// XDM
}

//-----------------------------------------------------------------------------
// Purpose: Draw HUD elements
// Input  : flTime - 
//			intermission - 
//-----------------------------------------------------------------------------
int CHudSpectator::Draw(const float &flTime)
{
	// draw only in spectator mode
	if (!gHUD.IsSpectator())
		return 0;

	size_t lx;
	char string[256];
	byte r,g,b;// XDM

	// if user pressed zoom, aplly changes
	if ((m_zoomDelta != 0.0f) && (g_iUser1 == OBS_MAP_FREE))
	{
		m_mapZoom += m_zoomDelta;

		if (m_mapZoom > 3.0f)
			m_mapZoom = 3.0f;
		else if (m_mapZoom < 0.5f)
			m_mapZoom = 0.5f;
	}

	// if user moves in map mode, change map origin
	if ((m_moveDelta != 0.0f) && (g_iUser1 != OBS_ROAMING))//? && (g_iUser1 != OBS_MAP_FREE))
	{
		Vector right;
		AngleVectors(v_angles, NULL, right, NULL);
		right.NormalizeSelf();
		right *= m_moveDelta;
		m_mapOrigin += right;
	}

	// Only draw the icon names only if map mode is in Main Mode
	if (m_drawnames->value > 0.0f && OverviewIsFullscreen())// XDM3038c
	{
		// make sure we have player info
		GetAllPlayersInfo();

		// loop through all the players and draw additional infos to their sprites on the map
		for (short i = 0; i < MAX_PLAYERS; ++i)
		{
			if (m_vPlayerPos[i][2]<0)	// marked as invisible ?
				continue;

			// check if name would be in inset window
			if (m_iInsetMode != INSET_OFF)
			{
				if (m_vPlayerPos[i][0] > m_OverviewData.insetWindowX &&
					m_vPlayerPos[i][1] > m_OverviewData.insetWindowY &&
					m_vPlayerPos[i][0] < (m_OverviewData.insetWindowX + m_OverviewData.insetWindowWidth) &&
					m_vPlayerPos[i][1] < (m_OverviewData.insetWindowY + m_OverviewData.insetWindowHeight))
					continue;
			}

			// draw the players name
			_snprintf(string, 256, "%s", g_PlayerInfoList[i+1].name);
			string[255] = '\0';
			lx = strlen(string)*3; // 3 is avg. character length :)
			GetPlayerColor(i+1, r,g,b);// XDM3035a
			DrawSetTextColor(r,g,b);
			DrawConsoleString(m_vPlayerPos[i][0]-lx,m_vPlayerPos[i][1], string);
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// SetSpectatorStartPosition(): 
// Get valid map position and 'beam' spectator to this position
//-----------------------------------------------------------------------------
void CHudSpectator::SetSpectatorStartPosition(void)
{
	if (UTIL_FindEntityInMap("info_intermission", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = 1;
	else if (UTIL_FindEntityInMap("info_player_start", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = 1;
	else if (UTIL_FindEntityInMap("info_player_deathmatch", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = 1;
	else if (UTIL_FindEntityInMap("info_player_coop", m_cameraOrigin, m_cameraAngles))
		iJumpSpectator = 1;
	else
	{
		// jump to 0,0,0 if no better position was found
		m_cameraOrigin.Clear();
		m_cameraAngles.Clear();
	}
	vJumpOrigin = m_cameraOrigin;
	vJumpAngles = m_cameraAngles;
	iJumpSpectator = 1;	// jump anyway
}

void CHudSpectator::SetCameraView(const vec3_t &pos, const vec3_t &angle, float fov)
{
	m_FOV = fov;
	vJumpOrigin = pos;
	vJumpAngles = angle;
	gEngfuncs.SetViewAngles(vJumpAngles);
	iJumpSpectator = 1;	// jump anyway
}

void CHudSpectator::AddWaypoint(float time, const vec3_t &pos, const vec3_t &angle, float fov, int flags)
{
	if (!flags == 0 && time == 0.0f)
	{
		// switch instantly to this camera view
		SetCameraView( pos, angle, fov );
		return;
	}

	if (m_NumWayPoints >= MAX_CAM_WAYPOINTS)
	{
		conprintf(1, "CHudSpectator::AddWaypoint() Warning: Too many camera waypoints!\n");
		return;
	}

	m_CamPath[ m_NumWayPoints ].position = pos;
	m_CamPath[ m_NumWayPoints ].angle = angle;
	m_CamPath[ m_NumWayPoints ].flags = flags;
	m_CamPath[ m_NumWayPoints ].fov = fov;
	m_CamPath[ m_NumWayPoints ].time = time;
	conprintf(1, "Added waypoint %i\n", m_NumWayPoints );
	++m_NumWayPoints;
}

void CHudSpectator::SetWayInterpolation(cameraWayPoint_t * prev, cameraWayPoint_t * start, cameraWayPoint_t * end, cameraWayPoint_t * next)
{
	m_WayInterpolation.SetViewAngles( start->angle, end->angle );
	m_WayInterpolation.SetFOVs( start->fov, end->fov );
	m_WayInterpolation.SetSmoothing( (start->flags & DRC_FLAG_SLOWSTART) != 0, (start->flags & DRC_FLAG_SLOWEND) != 0);

	if (prev && next)
		m_WayInterpolation.SetWaypoints(&prev->position, start->position, end->position, &next->position);
	else if (prev)
		m_WayInterpolation.SetWaypoints(&prev->position, start->position, end->position, NULL );
	else if (next)
		m_WayInterpolation.SetWaypoints(NULL, start->position, end->position, &next->position );
	else
		m_WayInterpolation.SetWaypoints(NULL, start->position, end->position, NULL );
}

bool CHudSpectator::GetDirectorCamera(vec3_t &position, vec3_t &angle)
{
	float now = gHUD.m_flTime;
	float fov = DEFAULT_FOV;// XDM3037a

	if ( m_ChaseEntity )
	{
		cl_entity_t	*ent = gEngfuncs.GetEntityByIndex( m_ChaseEntity );
		if (ent)
		{
			vec3_t vt = ent->curstate.origin;
			if ( m_ChaseEntity <= gEngfuncs.GetMaxClients() )
			{			
				if ( ent->curstate.solid == SOLID_NOT )
				{
					// XDM3037a: don't add anything!	vt[2] += PM_DEAD_VIEWHEIGHT;
				}
				else if (ent->curstate.usehull == 1 )
					vt[2] += DUCK_VIEW;
				else
					vt[2] += VEC_VIEW;
			}
			vt -= position;
			VectorAngles(vt, angle);
#if !defined (NOSQB)
			angle[0] = -angle[0];
#endif
			return true;
		}
		else
		{
			return false;
		}
	}

	if ( !m_IsInterpolating )
		return false;

	if ( m_WayPoint < 0 || m_WayPoint >= (m_NumWayPoints-1) )
		return false;

	cameraWayPoint_t * wp1 = &m_CamPath[m_WayPoint];
	cameraWayPoint_t * wp2 = &m_CamPath[m_WayPoint+1];

	if ( now < wp1->time )
		return false;

	while ( now > wp2->time )
	{
		// go to next waypoint, if possible
		++m_WayPoint;

		if ( m_WayPoint >= (m_NumWayPoints-1) )
		{
			m_IsInterpolating = false;
			return false;	// there is no following waypoint
		}

		wp1 = wp2;
		wp2 = &m_CamPath[m_WayPoint+1];

		if ( m_WayPoint > 0 )
		{
			// we have a predecessor

			if ( m_WayPoint < (m_NumWayPoints-1) )
			{
				// we have also a successor
				SetWayInterpolation( &m_CamPath[m_WayPoint-1], wp1, wp2, &m_CamPath[m_WayPoint+2] );
			}
			else
			{
				SetWayInterpolation( &m_CamPath[m_WayPoint-1], wp1, wp2, NULL );
			}
		}
		else if ( m_WayPoint < (m_NumWayPoints-1) )
		{
			// we only have a successor
			SetWayInterpolation( NULL, wp1, wp2, &m_CamPath[m_WayPoint+2] );
		}
		else
		{
			// we have only two waypoints
			SetWayInterpolation( NULL, wp1, wp2, NULL );
		}
	}

	if ( wp2->time <= wp1->time )
		return false;

	float fraction = (now - wp1->time) / (wp2->time - wp1->time);
	if (fraction < 0.0f)
		fraction = 0.0f;
	else if (fraction > 1.0f)
		fraction = 1.0f;

	m_WayInterpolation.Interpolate( fraction, position, angle, &fov );
	// conprintf(2, "Interpolate time: %.2f, fraction %.2f, point : %.2f,%.2f,%.2f\n", now, fraction, position[0], position[1], position[2] );
	SetCameraView( position, angle, fov );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CHudSpectator::GetFOV(void)
{
	return m_FOV;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSize - 
//			*pbuf - 
//-----------------------------------------------------------------------------
void CHudSpectator::DirectorMessage(int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHudSpectator::DirectorMessage(%d)\n", iSize);
	float	f1,f2;
	char *	string;
	vec3_t	v1,v2;
	int		i1,i2,i3;

	BEGIN_READ( pbuf, iSize );
	int cmd = READ_BYTE();
	switch (cmd)	// director command byte 
	{ 
		case DRC_CMD_START:
							// now we have to do some things clientside, since the proxy doesn't know our mod 
							//g_iPlayerClass = 0;
							gHUD.m_iTeamNumber = TEAM_NONE;
							// fake a InitHUD & ResetHUD message
							gHUD.MsgFunc_InitHUD(NULL,0, NULL);
							gHUD.Reset(false);// XDM3038c: TESTME: false
							break;

		case DRC_CMD_EVENT	:	// old director style message
							m_lastPrimaryObject		=	READ_WORD();
							m_lastSecondaryObject	=	READ_WORD();
							m_iObserverFlags		=	READ_LONG();
							if ( m_autoDirector->value )
							{
								if ( (g_iUser2 != m_lastPrimaryObject) || (g_iUser3 != m_lastSecondaryObject) )
									V_ResetChaseCam();	

								g_iUser2 = m_lastPrimaryObject;
								g_iUser3 = m_lastSecondaryObject;
								m_IsInterpolating = false;
								m_ChaseEntity = 0;
							}
							// conprintf(2, "Director Camera: %i %i\n", firstObject, secondObject);
							break;

		case DRC_CMD_MODE:
							if ( m_autoDirector->value )
							{
								SetModes( READ_BYTE(), -1 );
							}
							break;

		case DRC_CMD_CAMERA:
							v1[0] = READ_COORD();	// position
							v1[1] = READ_COORD();
							v1[2] = READ_COORD();	// vJumpOrigin
							v2[0] = READ_COORD();	// view angle
							v2[1] = READ_COORD();   // vJumpAngles
							v2[2] = READ_COORD();
							f1    = READ_BYTE();	// fov
							i1    = READ_WORD();	// target
							if ( m_autoDirector->value )
							{
								SetModes( OBS_ROAMING, -1 );
								SetCameraView(v1, v2, f1);
								m_ChaseEntity = i1;
							}
							break;

		case DRC_CMD_MESSAGE:
							{
								client_textmessage_t * msg = &m_HUDMessages[m_lastHudMessage];
								msg->effect = READ_BYTE();		// effect
								UnpackRGB(msg->r1, msg->g1, msg->b1, READ_LONG() );		// color
								msg->r2 = msg->r1;
								msg->g2 = msg->g1;
								msg->b2 = msg->b1;
								msg->a2 = msg->a1 = 0xFF;	// not transparent
								msg->x = READ_FLOAT();	// x pos
								msg->y = READ_FLOAT();	// y pos
								msg->fadein		= READ_FLOAT();	// fadein
								msg->fadeout	= READ_FLOAT();	// fadeout
								msg->holdtime	= READ_FLOAT();	// holdtime
								msg->fxtime		= READ_FLOAT();	// fxtime;
								strncpy( m_HUDMessageText[m_lastHudMessage], READ_STRING(), 128 );
								m_HUDMessageText[m_lastHudMessage][127]=0;	// text 
								msg->pMessage = m_HUDMessageText[m_lastHudMessage];
								msg->pName	  = "HUD_MESSAGE";
								gHUD.m_Message.MessageAdd( msg );
								m_lastHudMessage++;
								m_lastHudMessage %= MAX_SPEC_HUD_MESSAGES;
							}
							break;

		case DRC_CMD_SOUND:
							string = READ_STRING();
							f1 =  READ_FLOAT();
							// conprintf(2, "DRC_CMD_FX_SOUND: %s %.2f\n", string, value );
							gEngfuncs.pEventAPI->EV_PlaySound(0, v_origin, CHAN_BODY, string, f1, ATTN_NORM, 0, PITCH_NORM );
							break;

		case DRC_CMD_TIMESCALE:
							f1 = READ_FLOAT();
							break;

		case DRC_CMD_STATUS:
							READ_LONG(); // total number of spectator slots
							m_iSpectatorNumber = READ_LONG(); // total number of spectator
							/*m_iProxiesNumber = */READ_WORD(); // total number of relay proxies
							gViewPort->UpdateSpectatorPanel();
							break;

		case DRC_CMD_BANNER:
							// conprintf(1, "GUI: Banner %s\n",READ_STRING() ); // name of banner tga eg gfx/temp/7454562234563475.tga
							if (gViewPort->GetSpectatorPanel())
							{
								gViewPort->GetSpectatorPanel()->SetBanner(READ_STRING());
								gViewPort->UpdateSpectatorPanel();
							}
							break;

		//case DRC_CMD_FADE:		
		//					break;

		case DRC_CMD_STUFFTEXT:
							CLIENT_COMMAND(READ_STRING());
							break;

		case DRC_CMD_CAMPATH:
							v1[0] = READ_COORD();	// position
							v1[1] = READ_COORD();
							v1[2] = READ_COORD();	// vJumpOrigin

							v2[0] = READ_COORD();	// view angle
							v2[1] = READ_COORD();   // vJumpAngles
							v2[2] = READ_COORD();
							f1    = READ_BYTE();	// FOV
							i1    = READ_BYTE();	// flags

							if ( m_autoDirector->value )
							{
								SetModes( OBS_ROAMING, -1 );
								SetCameraView(v1, v2, f1);
							}
							break;

		case DRC_CMD_WAYPOINTS :
							i1 = READ_BYTE();
							m_NumWayPoints = 0;
							m_WayPoint = 0;
							for ( i2=0; i2<i1; i2++ )
							{
								f1 = gHUD.m_flTime + (float)(READ_SHORT())/100.0f;
								v1[0] = READ_COORD();	// position
								v1[1] = READ_COORD();
								v1[2] = READ_COORD();	// vJumpOrigin
								v2[0] = READ_COORD();	// view angle
								v2[1] = READ_COORD();   // vJumpAngles
								v2[2] = READ_COORD();
								f2    = READ_BYTE();	// fov
								i3    = READ_BYTE();	// flags
								AddWaypoint( f1, v1, v2, f2, i3);
							}

							// conprintf(2, "CHudSpectator::DirectorMessage: waypoints %i.\n", m_NumWayPoints );
							if ( !m_autoDirector->value )
							{
								// ignore waypoints
								m_NumWayPoints = 0;
								break;	
							}

							SetModes( OBS_ROAMING, -1 );

							m_IsInterpolating = true;

							if ( m_NumWayPoints > 2 )
							{
								SetWayInterpolation( NULL, &m_CamPath[0], &m_CamPath[1], &m_CamPath[2] );
							}
							else
							{
								SetWayInterpolation( NULL, &m_CamPath[0], &m_CamPath[1], NULL );
							}
							break;

		default: conprintf(1, "CHudSpectator::DirectorMessage: unknown command %i.\n", cmd ); break;
	}
#if defined (_DEBUG)
	END_READ();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bReverse - 
//-----------------------------------------------------------------------------
void CHudSpectator::FindNextPlayer(bool bReverse)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	// if we are NOT in HLTV mode, spectator targets are set on server
	if (!gEngfuncs.IsSpectateOnly())
	{
		if (bReverse)// XDM
			SERVER_COMMAND("followprev");
		else
			SERVER_COMMAND("follownext");

		return;
	}

	CLIENT_INDEX iStart;
	if (g_iUser2)
		iStart = g_iUser2;
	else
		iStart = 1;

	g_iUser2 = 0;

	CLIENT_INDEX iCurrent = iStart;
	CLIENT_INDEX iDir = bReverse ? -1 : 1; 

	// make sure we have player info
	GetAllPlayersInfo();
	cl_entity_t *pEnt = NULL;

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > MAX_PLAYERS)
			iCurrent = 1;
		else if (iCurrent < 1)// XDM3038a: else
			iCurrent = MAX_PLAYERS;

		if (!IsActivePlayer(iCurrent))// XDM3038a
			continue;

		pEnt = gEngfuncs.GetEntityByIndex(iCurrent);
		if (pEnt == gEngfuncs.GetLocalPlayer())// XDM3035a
			continue;

		if (IsTeamGame(gHUD.m_iGameType))
		{
			// XDM3035a: WARNING! current team of a spectator is 0 ("spectators")!
			// last team the player has played on is stored in "playerclass". Hack?
			// XDM3038a: OBSOLETE if (gHUD.m_LocalPlayerState.playerclass != TEAM_NONE && pEnt->curstate.team != gHUD.m_LocalPlayerState.playerclass)
			if (gHUD.m_iTeamNumber != TEAM_NONE && pEnt->curstate.team != gHUD.m_iTeamNumber)
				continue;
		}

		// MOD AUTHORS: Add checks on target here.
		g_iUser2 = iCurrent;
		break;

	} while (iCurrent != iStart);

	// Did we find a target?
	if (g_iUser2 == 0)// not found
	{
		conprintf(1, "No observer targets.\n");
		// take save camera position
		vJumpOrigin = m_cameraOrigin;
		vJumpAngles = m_cameraAngles;
	}
	else
	{
		// use new entity position for roaming
		vJumpOrigin = pEnt->origin;
		vJumpAngles = pEnt->angles;
	}
	iJumpSpectator = 1;
	gViewPort->MsgFunc_ResetFade( NULL, 0, NULL );
}

void CHudSpectator::FindPlayer(CLIENT_INDEX iPlayerIndex)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	// if we are NOT in HLTV mode, spectator targets are set on server
	if ( !gEngfuncs.IsSpectateOnly() )
	{
		char cmdstring[32];
		// forward command to server
		_snprintf(cmdstring, 32, "follow %d\0", iPlayerIndex);
		SERVER_COMMAND(cmdstring);
		return;
	}

	g_iUser2 = 0;

	// make sure we have player info
	GetAllPlayersInfo();

	cl_entity_t * pEnt = NULL;
	for (CLIENT_INDEX i = 1; i < MAX_PLAYERS; ++i)
	{
		if (!IsActivePlayer(i))// XDM3038a
			continue;

		pEnt = gEngfuncs.GetEntityByIndex(i);
		if (IsTeamGame(gHUD.m_iGameType))
		{
			// XDM3035a: WARNING! current team of a spectator is 0 ("spectators")!
			// last team the player has played on is stored in "playerclass". Hack?
			// XDM3038a: OBSOLETE if (gHUD.m_LocalPlayerState.playerclass != TEAM_NONE && pEnt->curstate.team != gHUD.m_LocalPlayerState.playerclass)
			if (gHUD.m_iTeamNumber != TEAM_NONE && pEnt->curstate.team != gHUD.m_iTeamNumber)
				continue;
		}

		//if (!_stricmp(g_PlayerInfoList[pEnt->index].name,name))
		if (i == iPlayerIndex)
		{
			g_iUser2 = i;
			break;
		}
	}

	// Did we find a target?
	if (!g_iUser2)
	{
		conprintf(1, "No observer targets (%d).\n", iPlayerIndex);
		// take save camera position 
		vJumpOrigin = m_cameraOrigin;
		vJumpAngles = m_cameraAngles;
	}
	else
	{
		// use new entity position for roaming
		vJumpOrigin = pEnt->origin;
		vJumpAngles = pEnt->angles;
	}
	iJumpSpectator = 1;
	gViewPort->MsgFunc_ResetFade( NULL, 0, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ButtonPressed - 
//-----------------------------------------------------------------------------
void CHudSpectator::HandleButtonsDown(int ButtonPressed)
{
	// conprintf(1, " HandleButtons:%i\n", ButtonPressed );
	if (!gViewPort)
		return;

	//Not in intermission.
	if (gHUD.m_iIntermission)
		 return;

	if (!gHUD.IsSpectator())
		return; // dont do anything if not in spectator mode

	// don't handle buttons during normal demo playback
	if (gEngfuncs.pDemoAPI->IsPlayingback() && !gEngfuncs.IsSpectateOnly())
		return;

	// Slow down mouse clicks. 
	if (m_flNextObserverInput > gHUD.m_flTime)
		return;

	int newMainMode = -1;
	int newInsetMode = -1;

	// XDM3037a: TODO: move to VGUI code?
	// enable spectator screen
	if (ButtonPressed & IN_DUCK)
		if (gViewPort->GetSpectatorPanel())
			gViewPort->GetSpectatorPanel()->ShowMenu(!gViewPort->GetSpectatorPanel()->m_menuVisible);

	// 'Use' changes inset window mode
	if ((ButtonPressed & IN_USE) && gHUD.m_iGameState == GAME_STATE_ACTIVE)// XDM3038: otherwise it interferes with READY button (+use)
		newInsetMode = ToggleInset(m_iInsetMode + 1, true);

	// if not in HLTV mode, buttons are handled server side
	if (gEngfuncs.IsSpectateOnly())
	{
		// changing target or chase mode not in overviewmode without inset window
		// Jump changes main window modes
		if (ButtonPressed & IN_JUMP)
		{
			newMainMode = g_iUser1 + 1;
			if (newMainMode > OBS_MAP_CHASE)// XDM
				newMainMode = OBS_CHASE_LOCKED;
		}

		// Attack moves to the next player
		if (ButtonPressed & (IN_ATTACK | IN_ATTACK2))
		{
			FindNextPlayer((ButtonPressed & IN_ATTACK2) != 0);

			if (newMainMode == OBS_ROAMING)
			{
				gEngfuncs.SetViewAngles(vJumpAngles);
				iJumpSpectator = 1;
			}
			// release directed mode if player want to see another player
			m_autoDirector->value = 0.0f;
		}
	}

	SetModes(newMainMode, newInsetMode);

	if (g_iUser1 == OBS_MAP_FREE)
	{
		if (ButtonPressed & IN_FORWARD)
			m_zoomDelta =  0.01f;

		if (ButtonPressed & IN_BACK)
			m_zoomDelta = -0.01f;
		
		if (ButtonPressed & IN_MOVELEFT)
			m_moveDelta = -12.0f;

		if (ButtonPressed & IN_MOVERIGHT)
			m_moveDelta =  12.0f;
	}

	m_flNextObserverInput = gHUD.m_flTime + 0.2f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ButtonPressed - 
//-----------------------------------------------------------------------------
void CHudSpectator::HandleButtonsUp(int ButtonPressed)
{
	if (!gViewPort || !gViewPort->GetSpectatorPanel())
		return;

	if (!gViewPort->GetSpectatorPanel()->isVisible())
		return; // dont do anything if not in spectator mode

	if (ButtonPressed & (IN_FORWARD | IN_BACK))
		m_zoomDelta = 0.0f;
	
	if (ButtonPressed & (IN_MOVELEFT | IN_MOVERIGHT))
		m_moveDelta = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iNewMainMode - 
//			iNewInsetMode - 
//-----------------------------------------------------------------------------
void CHudSpectator::SetModes(int iNewMainMode, int iNewInsetMode)
{
	DBG_HUD_PRINTF("CHudSpectator::SetModes(%d %d)\n", iNewMainMode, iNewInsetMode);
	// if value == -1 keep old value
	if (iNewMainMode == -1)
		iNewMainMode = g_iUser1;

	if (iNewInsetMode == -1)
		iNewInsetMode = m_iInsetMode;

	//DBG_PRINTF("CHudSpectator::SetModes(%d %d)\n", iNewMainMode, iNewInsetMode);
	// inset mode is handled only clients side
	m_iInsetMode = iNewInsetMode;
	
	if (iNewMainMode < OBS_CHASE_LOCKED || iNewMainMode > OBS_MAP_CHASE)
	{
		conprintf(0, "Invalid spectator mode %d.\n", iNewMainMode);
		return;
	}

	m_IsInterpolating = false;// HL20130901
	m_ChaseEntity = 0;
	
	// main modes ettings will override inset window settings
	if (iNewMainMode != g_iUser1)
	{
		// if we are NOT in HLTV mode, main spectator mode is set on server
		if (!gEngfuncs.IsSpectateOnly())
		{
			char str[16];
			_snprintf(str, 16, "specmode %d", iNewMainMode);// HL20130901: HL uses 'specmode'
			SERVER_COMMAND(str);
			return;
		}

		if (!g_iUser2 && (iNewMainMode != OBS_ROAMING))	// make sure we have a target
		{
			// choose last Director object if still available
			if (IsActivePlayer(m_lastPrimaryObject))
			{
				g_iUser2 = m_lastPrimaryObject;
				g_iUser3 = m_lastSecondaryObject;
			}
			else
				FindNextPlayer(false); // find any target
		}

		g_iUser1 = iNewMainMode;
		if (g_iUser1 == OBS_ROAMING)// jump to current vJumpOrigin/angle
		{
			if (g_iUser2)
			{
				V_GetChasePos(gEngfuncs.GetEntityByIndex(g_iUser2), v_cl_angles, vJumpOrigin, vJumpAngles, DRC_FLAG_NO_RANDOM);
				gEngfuncs.SetViewAngles(vJumpAngles);
				iJumpSpectator = 1;
			}
		}
		else if (OverviewIsFullscreen())//(g_iUser1 == OBS_MAP_FREE || g_iUser1 == OBS_MAP_CHASE)
		{
			// reset user values
			m_mapZoom = m_OverviewData.zoom;
			m_mapOrigin = m_OverviewData.origin;
		}

		if ((g_iUser1 == OBS_IN_EYE) || (g_iUser1 == OBS_ROAMING)) 
		{
			// XDM3037a
			/*m_crosshairRect.left = 24;
			m_crosshairRect.top = 0;
			m_crosshairRect.right = 48;
			m_crosshairRect.bottom = 24;*/
			SetCrosshair(m_hCrosshair, m_crosshairRect, 255, 255, 255);
		}
		else
		{
			// XDM3037a		memset(&m_crosshairRect,0,sizeof(m_crosshairRect));
			SetCrosshair(0, m_crosshairRect, 0, 0, 0);
		}
		gViewPort->MsgFunc_ResetFade( NULL, 0, NULL );
		/*
		char string[16];
		_snprintf(string, 16, "#Spec_Mode%d", g_iUser1);
		CenterPrint(BufferedLocaliseTextString(string));
		*/
	}
	gViewPort->UpdateSpectatorPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudSpectator::ParseOverviewFile(void)
{
	DBG_HUD_PRINTF("CHudSpectator::ParseOverviewFile()\n");
	char filename[128];
	char levelname[MAX_MAPPATH];
	char token[1024];
	float height;
	char *pfile = NULL;
	char *pfileStart = NULL;

	memset(&m_OverviewData, 0, sizeof(m_OverviewData));

	// fill in standrd values
	m_OverviewData.insetWindowX = 4;	// upper left corner
	m_OverviewData.insetWindowY = 4;
	m_OverviewData.insetWindowWidth = XRES(240);
	m_OverviewData.insetWindowHeight = YRES(180);
	if (m_insetBounds)// XDM3038a
	{
		float wfx, wfy, wfw, wfh;// percent
		if (sscanf(m_insetBounds->string, "%f %f %f %f", &wfx, &wfy, &wfw, &wfh) == 4)
		{
			m_OverviewData.insetWindowX = (int)(((float)ScreenWidth * wfx)/100);
			m_OverviewData.insetWindowY = (int)(((float)ScreenHeight * wfy)/100);
			m_OverviewData.insetWindowWidth = (int)(((float)ScreenWidth * wfw)/100);
			m_OverviewData.insetWindowHeight = (int)(((float)ScreenHeight * wfh)/100);
			if (m_OverviewData.insetWindowX + m_OverviewData.insetWindowWidth > ScreenWidth)
			{
				conprintf(1, "Warning! Bad X or Width specified in \"%s\"!\n", m_insetBounds->name);
				m_OverviewData.insetWindowX = 0;
				m_OverviewData.insetWindowWidth = XRES(240);
			}
			if (m_OverviewData.insetWindowY + m_OverviewData.insetWindowHeight > ScreenHeight)
			{
				conprintf(1, "Warning! Bad Y or Height specified in \"%s\"!\n", m_insetBounds->name);
				m_OverviewData.insetWindowY = 0;
				m_OverviewData.insetWindowHeight = YRES(180);
			}
		}
		else
			conprintf(1, "Warning! Bad value specified in \"%s\"!\n", m_insetBounds->name);
	}
	m_OverviewData.origin[0] = 0.0f;
	m_OverviewData.origin[1] = 0.0f;
	m_OverviewData.origin[2] = 0.0f;
	m_OverviewData.zoom	= 1.0f;
	m_OverviewData.layers = 0;// XDM3038
	m_OverviewData.layersHeights[0] = 0.0f;
	m_OverviewData.layersImages[0][0] = 0;
	strncpy(m_OverviewData.map, GET_LEVEL_NAME(), MAX_MAPPATH);// XDM
	m_OverviewData.map[MAX_MAPPATH-1] = 0;

	if (strlen(m_OverviewData.map) == 0)
		return false; // not active yet

	strncpy(levelname, m_OverviewData.map + 5, MAX_MAPPATH);// maps/
	levelname[strlen(levelname)-4] = '\0';// .bsp
	_snprintf(filename, 128, "overviews/%s.txt", levelname);

	bool result = false;
	pfile = (char *)gEngfuncs.COM_LoadFile(filename, 5, NULL);
	pfileStart = pfile;

	if (!pfile)
	{
		conprintf(0, "Couldn't open \"%s\". Using default values for overiew mode.\n", filename);
		goto func_end;
		//m_mapZoom = m_OverviewData.zoom;
		//m_mapOrigin = m_OverviewData.origin;
		//m_OverviewData.layers = 1;// XDM3038
		//return false;
	}

	while (1)
	{
		pfile = COM_Parse(pfile, token);
		if (!pfile)
			break;

		if (token[0] == '/' && token[1] == '/')// XDM3035a: faster?
			continue;

		token[1023] = 0;
		//if (strbegin(token, "//"))// XDM
		//	continue;
		//else if (strbegin(token, "/*"))

		if (_stricmp(token, "global") == 0)
		{
			// parse the global data
			pfile = COM_Parse(pfile, token);
			if (_stricmp(token, "{")) 
			{
				conprintf(1, "Error parsing overview file \"%s\". (expected { )\n", filename);
				break;//goto func_end;
				//return false;
			}

			pfile = COM_Parse(pfile, token);

			while (_stricmp(token, "}"))
			{
				if (!_stricmp(token, "ZOOM"))
				{
					pfile = COM_Parse(pfile, token);
					m_OverviewData.zoom = atof(token);
				} 
				else if (!_stricmp(token, "ORIGIN"))
				{
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.origin[0] = atof(token);
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.origin[1] = atof(token);
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.origin[2] = atof(token);
				}
				else if (!_stricmp(token, "ROTATED"))
				{
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.rotated = atoi(token);
				}
				else if (!_stricmp(token, "INSET"))
				{
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.insetWindowX = XRES(atoi(token));// XDM3038a: XRES for compatibility
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.insetWindowY = YRES(atoi(token));
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.insetWindowWidth = XRES(atoi(token));
					pfile = COM_Parse(pfile, token); 
					m_OverviewData.insetWindowHeight = YRES(atoi(token));
				}
				else
					conprintf(1, "Warning! Unknown keyword \"%s\" in global section of overview file \"%s\"\n", token, filename);

				pfile = COM_Parse(pfile, token);// parse next token
			}
		}
		else if (!_stricmp(token, "layer"))
		{
			// parse a layer data
			if (m_OverviewData.layers >= OVERVIEW_MAX_LAYERS)
				conprintf(1, "Warning! Too many layers in overview file \"%s\"\n", filename);

			pfile = COM_Parse(pfile, token);

			if (_stricmp(token, "{")) 
			{
				conprintf(1, "Error parsing overview file \"%s\". (expected { )\n", filename);
				break;//goto func_end;
				//return false;
			}

			pfile = COM_Parse(pfile, token);

			while (_stricmp(token, "}"))
			{
				if (!_stricmp(token, "image"))
				{
					pfile = COM_Parse(pfile, token);
					strncpy(m_OverviewData.layersImages[m_OverviewData.layers], token, 256);
				} 
				else if (!_stricmp(token, "height"))
				{
					pfile = COM_Parse(pfile, token); 
					height = atof(token);
					m_OverviewData.layersHeights[m_OverviewData.layers] = height;
				}
				/*else if (!_stricmp(token, "rendermode"))// UNDONE
				{
					m_OverviewData.layersRenderModes[m_OverviewData.layers] = atoi(token);
				}*/
				else
					conprintf(1, "Warning! Unknown keyword \"%s\" in layer %d of overview file \"%s\"\n", token, m_OverviewData.layers, filename);

				pfile = COM_Parse(pfile, token); // parse next token
			}
			m_OverviewData.layers++;
		}
	}

func_end:
	if (pfileStart)
		gEngfuncs.COM_FreeFile(pfileStart);// XDM3035c: pfile);

	if (m_OverviewData.layers == 0)
		m_OverviewData.layers = 1;// XDM3038: default layer

	m_mapZoom = m_OverviewData.zoom;
	m_mapOrigin = m_OverviewData.origin;
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Load images into memory
//-----------------------------------------------------------------------------
void CHudSpectator::LoadMapSprites(void)
{
	m_DefaultSprite = (struct model_s *)gEngfuncs.GetSpritePointer(m_hsprUnkownMap);
	for (size_t i = 0; i < OVERVIEW_MAX_LAYERS; ++i)// iterate through entire array
	{
		if (i < m_OverviewData.layers && m_OverviewData.layersImages[i][0] != 0)// if filename was specified
			m_MapSprite[i] = gEngfuncs.LoadMapSprite(m_OverviewData.layersImages[i]);
		else
			m_MapSprite[i] = NULL;// wipe out all unused layer slots
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reliable
//-----------------------------------------------------------------------------
bool CHudSpectator::OverviewIsFullscreen(void)
{
	return (g_iUser1 == OBS_MAP_FREE || g_iUser1 == OBS_MAP_CHASE);
}

//-----------------------------------------------------------------------------
// Purpose: Draw layers
//-----------------------------------------------------------------------------
void CHudSpectator::DrawOverviewLayer(void)
{
	float screenaspect = 4.0f/3.0f, xs, ys, xStep, yStep, x,y,z;
	int ix,iy,i,xTiles = 0,yTiles = 0,frame;
	model_t *pTexture = NULL;

	xs = m_OverviewData.origin[0];
	ys = m_OverviewData.origin[1];

	// TODO: decide layers order by Z, not by index
	for (size_t current_layer = 0; current_layer < m_OverviewData.layers; ++current_layer)// XDM: multilayer support
	//for (int current_layer = m_OverviewData.layers-1; current_layer >= 0; --current_layer)
	{
		if (m_MapSprite[current_layer] == NULL)// XDM
		{
			if (current_layer != 0)// skip all other layers
				continue;

			pTexture = m_DefaultSprite;//dummySprite;
			xTiles = 8;
			yTiles = 6;
		}
		else
		{
			pTexture = m_MapSprite[current_layer];
			if (pTexture)
			{
				//i = (int)sqrt((float)m_MapSprite[0]->numframes / (4*3));
				i = (int)sqrt((float)pTexture->numframes/(4*3));
				xTiles = i*4;
				yTiles = i*3;
			}
		}
		if (pTexture == NULL)
			continue;

		z = (90.0f - v_angles[0]) / 90.0f;
		z *= m_OverviewData.layersHeights[current_layer]; // gOverviewData.z_min - 32;	

		// i = r_overviewTexture + ( layer*OVERVIEW_X_TILES*OVERVIEW_Y_TILES );
		// XDM3037a: in fullscreen map modes draw all other layers transparent
		if ((OverviewIsFullscreen() && current_layer > 0) || m_OverviewData.layersHeights[current_layer] >= g_vecViewOrigin.z)// XDM3038c: gHUD.m_vecOrigin.z)// XDM3034
		{
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, 0.25);
			//conprintf(1, "OVW: lyr %d\n", current_layer);
		}
		else
		{
			//gEngfuncs.pTriAPI->RenderMode(m_OverviewData.layersRenderModes[current_layer]);
			gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, 1.0);
		}
		gEngfuncs.pTriAPI->CullFace(TRI_NONE);
		//conprintf(1, "OVW: lyr %d: hei: %f, z: %f, origin: %f %f %f\n", current_layer, m_OverviewData.layersHeights[current_layer], z, gHUD.m_vecOrigin[0], gHUD.m_vecOrigin[1], gHUD.m_vecOrigin[2]);

		frame = 0;
		// rotated view ?
		if (m_OverviewData.rotated)// we can use g_pWorld->model->mins and maxs?
		{
			xStep = (2*WORLDSIZE / m_OverviewData.zoom) / xTiles;
			yStep = -(2*WORLDSIZE / (m_OverviewData.zoom * screenaspect)) / yTiles;
			y = ys + (WORLDSIZE / (m_OverviewData.zoom * screenaspect));
			for (iy = 0; iy < yTiles; ++iy)
			{
				x = xs - (WORLDSIZE / (m_OverviewData.zoom));
				for (ix = 0; ix < xTiles; ++ix)
				{
					gEngfuncs.pTriAPI->SpriteTexture(pTexture, frame);
					gEngfuncs.pTriAPI->Begin(TRI_QUADS);
						gEngfuncs.pTriAPI->TexCoord2f(0, 0);
						gEngfuncs.pTriAPI->Vertex3f(x, y, z);

						gEngfuncs.pTriAPI->TexCoord2f(1, 0);
						gEngfuncs.pTriAPI->Vertex3f(x+xStep ,y,  z);

						gEngfuncs.pTriAPI->TexCoord2f(1, 1);
						gEngfuncs.pTriAPI->Vertex3f(x+xStep, y+yStep, z);

						gEngfuncs.pTriAPI->TexCoord2f(0, 1);
						gEngfuncs.pTriAPI->Vertex3f(x, y+yStep, z);
					gEngfuncs.pTriAPI->End();
					if (frame < (pTexture->numframes-1))
						++frame;
					x += xStep;
				}
				y+=yStep;
			}
		} 
		else
		{
			xStep = -(2*WORLDSIZE / m_OverviewData.zoom) / xTiles;
			yStep = -(2*WORLDSIZE / (m_OverviewData.zoom * screenaspect)) / yTiles;
			x = xs + (WORLDSIZE / (m_OverviewData.zoom * screenaspect));
			for (ix = 0; ix < yTiles; ++ix)
			{
				y = ys + (WORLDSIZE / (m_OverviewData.zoom));	
				for (iy = 0; iy < xTiles; ++iy)	
				{
					gEngfuncs.pTriAPI->SpriteTexture(pTexture, frame);
					gEngfuncs.pTriAPI->Begin(TRI_QUADS);
						gEngfuncs.pTriAPI->TexCoord2f(0, 0);
						gEngfuncs.pTriAPI->Vertex3f(x, y, z);

						gEngfuncs.pTriAPI->TexCoord2f(0, 1);
						gEngfuncs.pTriAPI->Vertex3f(x+xStep ,y,  z);

						gEngfuncs.pTriAPI->TexCoord2f(1, 1);
						gEngfuncs.pTriAPI->Vertex3f(x+xStep, y+yStep, z);

						gEngfuncs.pTriAPI->TexCoord2f(1, 0);
						gEngfuncs.pTriAPI->Vertex3f(x, y+yStep, z);
					gEngfuncs.pTriAPI->End();
					if (frame < (pTexture->numframes-1))
						++frame;
					y += yStep;
				}
				x+= xStep;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw entities from m_OverviewEntities
//-----------------------------------------------------------------------------
void CHudSpectator::DrawOverviewEntities(void)
{
	struct model_s *pSpriteModel;
	vec3_t origin, angles, point, forward, right, left, up, world, screen, offset;
	cl_entity_t *ent = NULL;
	float sizeScale;//x,y,z, sizeScale;
	float rmatrix[3][4];// transformation matrix
	float zScale = (90.0f - v_angles[0]) / 90.0f;
	float time = gEngfuncs.GetClientTime();
	short i;
	//?	z = m_OverviewData.layersHeights[0] * zScale;
	byte r,g,b;// XDM

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	// BUGBUG: makes entities totally invisible if this function is called more that once!
	for (i=0; i < MAX_PLAYERS; ++i)
		m_vPlayerPos[i][2] = -1;	// mark as invisible 

	// draw all players
	for (i=0; i < OVERVIEW_MAX_ENTITIES; ++i)
	{
		if (m_OverviewEntities[i].sprite == NULL)
			continue;

		if (m_OverviewEntities[i].killTime > 0.0f && m_OverviewEntities[i].killTime < time)// XDM3038c: disable old entities
		{
			memset(&m_OverviewEntities[i], 0, sizeof(overviewEntity_t));
			continue;
		}
		ent = m_OverviewEntities[i].entity;

		if (ent == NULL)
			continue;

		pSpriteModel = m_OverviewEntities[i].sprite;

		if (ent->player)// XDM: special scale for game objects (hack?)
		{
			//sizeScale = 32.0f;// pSpriteModel->mins/maxs?
			AngleVectors(ent->angles, right, up, NULL);
		}
		else
		{
			//sizeScale = 128.0f;
			//AngleVectors(m_vecViewPortAngles, right, up, NULL);
			//right = g_vecViewRight;
			//up = g_vecViewUp;
			right.x = 1;// always horizontal
			right.y = 0;
			right.z = 0;
			up.x = 0;
			up.y = 1;
			up.z = 0;
		}
		sizeScale = m_OverviewEntities[i].scale;// XDM3038

		//if ((ent->curstate.solid == SOLID_BSP) || (ent->curstate.movetype == MOVETYPE_PUSHSTEP))
		if (ent->model && ent->model->type == mod_brush)// XDM3038a: BSP models usually don't have origin
			origin = (ent->model->mins + ent->model->maxs) * 0.5f;
		else if (gHUD.m_pLocalPlayer != NULL && ent->curstate.messagenum == gHUD.m_pLocalPlayer->curstate.messagenum)// XDM3035c: this entity is updating
			origin = ent->origin;
		else// this one is out of PVS
			origin = ent->baseline.origin;

		if (IsValidPlayerIndex(ent->index))// entity may not be a player, but still belong to a team
			GetPlayerColor(ent->index, r,g,b);
		else
			GetTeamColor(ent->curstate.team, r,g,b);// XDM

		gEngfuncs.pTriAPI->SpriteTexture(pSpriteModel, 0);
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
		gEngfuncs.pTriAPI->Color4ub(r, g, b, 255);// XDM

		if (ent->curstate.eflags & EFLAG_GAMEGOAL)// XDM3038
			gEngfuncs.pTriAPI->Brightness(0.5f + 0.5f*sinf(time*8.0f));

		// see R_DrawSpriteModel
		// draws players sprite
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			VectorMA(origin,  sizeScale, up, point);
			VectorMA(point,   sizeScale, right, point);
			point[2] *= zScale;
			gEngfuncs.pTriAPI->Vertex3fv(point);

			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			VectorMA(origin,  sizeScale, up, point);
			VectorMA(point,  -sizeScale, right, point);
			point[2] *= zScale;
			gEngfuncs.pTriAPI->Vertex3fv(point);

			gEngfuncs.pTriAPI->TexCoord2f(0,1);
			VectorMA(origin, -sizeScale, up, point);
			VectorMA(point,  -sizeScale, right, point);
			point[2] *= zScale;
			gEngfuncs.pTriAPI->Vertex3fv(point);

			gEngfuncs.pTriAPI->TexCoord2f(1,1);
			VectorMA(origin, -sizeScale, up, point);
			VectorMA(point,   sizeScale, right, point);
			point[2] *= zScale;
			gEngfuncs.pTriAPI->Vertex3fv(point);
		gEngfuncs.pTriAPI->End();

		if (!ent->player)
			continue;
		// draw line under player icons
		origin[2] *= zScale;

		pSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer(m_hsprBeam);
		gEngfuncs.pTriAPI->SpriteTexture(pSpriteModel, 0);
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
		gEngfuncs.pTriAPI->Color4ub(r, g, b, 127);// XDM

		// XDM3035a: is this faster?
		gEngfuncs.pTriAPI->Begin(TRI_LINES);
			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			gEngfuncs.pTriAPI->Vertex3f(origin[0], origin[1], origin[2]+HULL_MIN);
			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			gEngfuncs.pTriAPI->Vertex3f(origin[0], origin[1], origin[2]);
		gEngfuncs.pTriAPI->End();

		int playerNum = ent->index - 1;

		// calculate screen position for name and infromation in hud::draw()
		if (gEngfuncs.pTriAPI->WorldToScreen(origin,screen))
		{
			m_vPlayerPos[playerNum][2] = -1;	// mark as invisible 
			continue;	// object is behind viewer
		}

		screen[0] = XPROJECT(screen[0]);
		screen[1] = YPROJECT(screen[1]);
		screen[2] = 0.0f;

		// calculate some offset under the icon
		origin[0]+=32.0f;
		origin[1]+=32.0f;

		gEngfuncs.pTriAPI->WorldToScreen(origin,offset);

		offset[0] = XPROJECT(offset[0]);
		offset[1] = YPROJECT(offset[1]);
		offset[2] = 0.0f;
		offset -= screen;//VectorSubtract(offset, screen, offset);

		m_vPlayerPos[playerNum][0] = screen[0];	
		m_vPlayerPos[playerNum][1] = screen[1] + Length(offset);	
		m_vPlayerPos[playerNum][2] = 1;	// mark player as visible 
	}

	if (m_drawcone->value > 0.0f)
	{
	// get current camera position and angle
	if (g_iUser2 > 0)
	{
		V_GetInEyePos(g_iUser2, origin, angles);
	}
	else
	{
		origin = v_sim_org;
		angles = v_cl_angles;
	}
	/*if (g_iUser1 == OBS_ROAMING || g_iUser1 == OBS_MAP_FREE || (!gHUD.IsSpectator() && gHUD.m_pCvarMiniMap->value > 0.0))// XDM3037a
	{
		VectorCopy(v_sim_org, origin);
		VectorCopy(v_cl_angles, angles);
	}
	else if (m_iInsetMode == INSET_IN_EYE || g_iUser1 == OBS_IN_EYE)
	{
		V_GetInEyePos(g_iUser2, origin, angles);
	}
	else if (m_iInsetMode == INSET_CHASE_FREE  || g_iUser1 == OBS_CHASE_FREE)
	{
		V_GetChasePos(gEngfuncs.GetEntityByIndex(g_iUser2), v_cl_angles, origin, angles, DRC_FLAG_NO_RANDOM);
	}
	else if (g_iUser2 > 0)
		V_GetChasePos(gEngfuncs.GetEntityByIndex(g_iUser2), NULL, origin, angles, DRC_FLAG_NO_RANDOM);*/

	// draw camera sprite
	//x = origin[0];
	//y = origin[1];
	//z = origin[2];
	angles[0] = 0; // always show horizontal camera sprite

	GetTeamColor(gHUD.m_iTeamNumber, r,g,b);// XDM
	pSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer(m_hsprCamera);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	if (gEngfuncs.pTriAPI->SpriteTexture(pSpriteModel, 0))
	{
		gEngfuncs.pTriAPI->Color4ub(r, g, b, 255);// XDM

		AngleVectors(angles, forward, NULL, NULL);
		VectorScale(forward, 512.0f, forward);

		offset[0] =  0.0f; 
		offset[1] = 45.0f; 
		offset[2] =  0.0f; 

		AngleMatrix(g_vecZero, offset, rmatrix);
		VectorTransform(forward, rmatrix, right);

		offset[1]= -45.0f;
		AngleMatrix(g_vecZero, offset, rmatrix);
		VectorTransform(forward, rmatrix, left);

		gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);
			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			gEngfuncs.pTriAPI->Vertex3f(origin.x+right[0], origin.y+right[1], (origin.z+right[2]) * zScale);
			gEngfuncs.pTriAPI->TexCoord2f(0, 1);
			gEngfuncs.pTriAPI->Vertex3f(origin.x, origin.y, origin.z * zScale);
			gEngfuncs.pTriAPI->TexCoord2f(1, 1);
			gEngfuncs.pTriAPI->Vertex3f(origin.x+left[0], origin.y+left[1], (origin.z+left[2]) * zScale);
		gEngfuncs.pTriAPI->End();
	}
	}// m_drawcone
}

//-----------------------------------------------------------------------------
// Called directly from HUD_DrawTransparentTriangles
// Output : bool - drawn
//-----------------------------------------------------------------------------
bool CHudSpectator::DrawOverview(bool bTransparent)
{
	// draw only in sepctator mode
	if (!ShouldDrawOverview())
		return 0;

	if (gHUD.m_iActive == 0)// XDM3038c: prevent more random crashes
		return 0;

	if (!m_OverviewMapDrawPass)// XDM3037a
		return 0;

	if (bTransparent)// XDM3037a
		DrawOverviewEntities();
	else
		DrawOverviewLayer();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: remove old entities
//-----------------------------------------------------------------------------
/*void CHudSpectator::CheckOverviewEntities(void)
{
	float time = gEngfuncs.GetClientTime();
	// removes old entities from list
	for (size_t i = 0; i< OVERVIEW_MAX_ENTITIES; ++i)
	{
		// remove entity from list if it is too old
		if (m_OverviewEntities[i].killTime > 0.0f && m_OverviewEntities[i].killTime < time)// XDM
			memset(&m_OverviewEntities[i], 0, sizeof(overviewEntity_t));
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: For external use
// Input  : *ent - 
//			icon - 
//			lifetime - 0 means show it only this frame;
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudSpectator::AddOverviewEntity(struct cl_entity_s *ent, struct model_s *icon, float scale, float lifetime)
{
	if (ent == NULL)
		return false;

	const model_t *pSprite = NULL;

	if (ent->player)
	{
		if (IsActivePlayer(ent->index))// XDM3038a
			pSprite = gEngfuncs.GetSpritePointer(m_hsprPlayer);
		else
			return false;// spectator

		if (scale <= 0.0f)
			scale = OVERVIEW_ENTSIZE_PLAYER;
	}
	else if (ent->curstate.eflags & EFLAG_GAMEGOAL)
	{
		if (icon)
			pSprite = icon;
		else if (m_hsprUnkown)// XDM3038
			pSprite = gEngfuncs.GetSpritePointer(m_hsprGameObject);
		else
			return false;

		if (scale <= 0.0f)
			scale = OVERVIEW_ENTSIZE_GAMEGOAL;
	}
	else
	{
		if (icon)
			pSprite = icon;
		else if (m_hsprUnkown)// XDM3038
			pSprite = gEngfuncs.GetSpritePointer(m_hsprUnkown);
		else
			return false;

		if (scale <= 0.0f)
			scale = OVERVIEW_ENTSIZE_DEFAULT;
	}

	return AddOverviewEntityToList(ent, pSprite, scale, (lifetime == 0)?0:(gEngfuncs.GetClientTime() + lifetime));// XDM3035c
}

//-----------------------------------------------------------------------------
// Purpose: Internal funciton
// Input  : *ent - 
//			*sprite - 
//			scale - 
//			killTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudSpectator::AddOverviewEntityToList(cl_entity_t *ent, const struct model_s *sprite, float scale, float killTime)
{
	short i;
	short empty_index = OVERVIEW_MAX_ENTITIES;// invalid values
	short found_index = OVERVIEW_MAX_ENTITIES;
	for (i = 0; i < OVERVIEW_MAX_ENTITIES; ++i)
	{
		// find empty entity slot
		if (m_OverviewEntities[i].entity == ent)// XDM3037a
		{
			found_index = i;
			break;
		}
		if (empty_index == OVERVIEW_MAX_ENTITIES && m_OverviewEntities[i].entity == NULL)
			empty_index = i;
	}

	if (found_index < OVERVIEW_MAX_ENTITIES)// XDM3037a: 1st priority: found self
		i = found_index;
	else if (empty_index < OVERVIEW_MAX_ENTITIES)// entity not found, use empty slot
		i = empty_index;
	else
		i = OVERVIEW_MAX_ENTITIES;// no slots whatsoever

	if (i < OVERVIEW_MAX_ENTITIES)
	{
		m_OverviewEntities[i].entity = ent;
		m_OverviewEntities[i].killTime = killTime;
		m_OverviewEntities[i].sprite = (struct model_s *)sprite;
		m_OverviewEntities[i].scale = scale;
		//conprintf(2, "CL: AddOverviewEntityToList: (%d) added entiy %d\n", i, ent->index);
		return true;
	}

	conprintf(1, "AddOverviewEntityToList(%d): error: too many entities! (max %d)\n", ent->index, OVERVIEW_MAX_ENTITIES);
	return false;	// maximum overview entities reached
}

//-----------------------------------------------------------------------------
// Purpose: Place a special mark on the map
// Input  : victim - 
//-----------------------------------------------------------------------------
void CHudSpectator::DeathMessage(int victim)
{
	DBG_HUD_PRINTF("CHudSpectator::DeathMessage(%d)\n", victim);
	if (IsValidPlayer(victim))
	{
		cl_entity_t *pl = GetUpdatingEntity(victim);
		if (pl)
			AddOverviewEntityToList(pl, gEngfuncs.GetSpritePointer(m_hsprPlayerDead), OVERVIEW_ENTSIZE_PLAYER, gHUD.m_flTime + 3.0f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Validate selected modes
//-----------------------------------------------------------------------------
void CHudSpectator::CheckSettings(void)
{
	// disble in intermission screen
	if (gHUD.m_iIntermission)
	{
		m_iInsetMode = INSET_OFF;
	}
	else if (m_iInsetMode != INSET_OFF)
	{
		if (g_iUser2 == 0)// XDM3037a: inset mode with no target
		{
			if (OverviewIsFullscreen())//if (g_iUser1 == OBS_MAP_FREE || g_iUser1 == OBS_MAP_CHASE)// main view: MAP modes
				m_iInsetMode = INSET_ROAMING;// world mode
			else// world modes
				m_iInsetMode = INSET_MAP_FREE;// map mode
		}
		else// disallow same inset mode as main mode:
		{
			if (g_iUser1 == OBS_CHASE_LOCKED)
			{
				RestrictInt(m_iInsetMode, INSET_MAP_FREE, 2, INSET_MAP_FREE, INSET_MAP_CHASE);// WARNING: due to "mode++" algorithm, "iDefault" must be the smallest integer here!!
			}
			else if (g_iUser1 == OBS_CHASE_FREE)
			{
				RestrictInt(m_iInsetMode, INSET_MAP_FREE, 2, INSET_MAP_FREE, INSET_MAP_CHASE);// WARNING: due to "mode++" algorithm, "iDefault" must be the smallest integer here!!
			}
			else if (g_iUser1 == OBS_ROAMING)
			{
				if (m_iInsetMode != INSET_MAP_FREE) m_iInsetMode = INSET_MAP_FREE;//RestrictInt(m_iInsetMode, INSET_MAP_FREE, 1, INSET_MAP_FREE);
			}
			else if (g_iUser1 == OBS_IN_EYE)
			{
				RestrictInt(m_iInsetMode, INSET_MAP_FREE, 2, INSET_MAP_FREE, INSET_MAP_CHASE);// WARNING: due to "mode++" algorithm, "iDefault" must be the smallest integer here!!
			}
			else if (g_iUser1 == OBS_MAP_FREE)
			{
				if (m_iInsetMode != INSET_ROAMING) m_iInsetMode = INSET_ROAMING;//RestrictInt(m_iInsetMode, INSET_ROAMING, 1, INSET_ROAMING);
			}
			else if (g_iUser1 == OBS_MAP_CHASE)
			{
				RestrictInt(m_iInsetMode, INSET_CHASE_FREE, 2, INSET_CHASE_FREE, INSET_IN_EYE);// WARNING: due to "mode++" algorithm, "iDefault" must be the smallest integer here!!
			}
			else if (g_iUser1 == OBS_INTERMISSION)// should never get here
			{
				//DBG_PRINTF("CHudSpectator::CheckSettings(): disabling inset picture during intermission\n");
				m_iInsetMode = INSET_OFF;
			}
		}
	}
	// check chat mode
	if (m_chatEnabled != (gHUD.m_SayText.m_pCvarSayText->value > 0.0f))
	{
		// hud_saytext changed
		m_chatEnabled = (gHUD.m_SayText.m_pCvarSayText->value > 0.0f);
		if (gEngfuncs.IsSpectateOnly())
		{
			// tell proxy our new chat mode
			char chatcmd[16];
			_snprintf(chatcmd, 16, "ignoremsg %i", m_chatEnabled?0:1);
			SERVER_COMMAND(chatcmd);
		}
	}

	// HL/TFC has no oberserver corsshair, so set it client side
	if ((g_iUser1 == OBS_IN_EYE) || (g_iUser1 == OBS_ROAMING)) 
	{
		SetCrosshair(m_hCrosshair, m_crosshairRect, 255, 255, 255);
	}
	else
	{
		//memset(&m_crosshairRect,0,sizeof(m_crosshairRect));
		SetCrosshair(0, m_crosshairRect, 0, 0, 0);
	}

	// if we are a real player on server don't allow inset window
	// in First Person mode since this is our resticted forcecamera mode 2

	if ((gHUD.m_iTeamNumber != TEAM_NONE) && (g_iUser1 == OBS_IN_EYE))
		m_iInsetMode = INSET_OFF;

	// draw small border around inset view, adjust upper black bar
	if (gViewPort->GetSpectatorPanel())
		gViewPort->GetSpectatorPanel()->EnableInsetView(ShouldDrawOverviewWindow());

	//conprintf(2, "CheckSettings: g_iUser1 = %d\n", g_iUser1);
}

//-----------------------------------------------------------------------------
// Purpose: Validate specified new mode by cycling through allowed modes
// Note   : Often called with currentmode++
// Input  : newInsetMode - 
//			allowOff - 
// Output : int - allowed mode
//-----------------------------------------------------------------------------
int CHudSpectator::ToggleInset(int newInsetMode, bool allowOff)
{
	if (!OverviewIsFullscreen())//if (g_iUser1 != OBS_MAP_FREE && g_iUser1 != OBS_MAP_CHASE)// XDM: main mode not fullscreen MAP
	{
		if (g_iUser2 == 0)// no track target
		{
			if (newInsetMode > INSET_OFF && newInsetMode <= INSET_MAP_FREE)
				newInsetMode = INSET_MAP_FREE;// FIRST MAP mode
			else if (allowOff)// && newInsetMode != INSET_OFF)// the only allowed mode is already ON
				newInsetMode = INSET_OFF;
		}
		else// we have target
		{
			if (newInsetMode > INSET_MAP_CHASE)// over LAST MAP mode
			{
				if (allowOff)
					newInsetMode = INSET_OFF;
				else
					newInsetMode = INSET_MAP_FREE;// FIRST MAP mode
			}
		}
	}
	else// main mode fullscreen MAP
	{
		if (g_iUser2 == 0)// no track target
		{
			if (newInsetMode > INSET_OFF && newInsetMode <= INSET_ROAMING)
				newInsetMode = INSET_ROAMING;// FIRST WORLD mode
			else if (allowOff)// && newInsetMode != INSET_OFF)// the only allowed mode is already ON
				newInsetMode = INSET_OFF;	
		}
		else// we have target
		{
			if (newInsetMode > INSET_IN_EYE)// over LAST WORLD mode
			{
				if (allowOff)
					newInsetMode = INSET_OFF;
				else
					newInsetMode = INSET_CHASE_FREE;// FIRST WORLD mode
			}
		}
	}
	//conprintf(1, "CL ToggleInset(%d)\n", newInsetMode);
	return newInsetMode;
}

//-----------------------------------------------------------------------------
// Purpose: debug
//-----------------------------------------------------------------------------
/*void CHudSpectator::DumpOverviewEnts(void)
{
	for (short i = 0; i < OVERVIEW_MAX_ENTITIES; ++i)
	{
		if (m_OverviewEntities[i].entity != NULL)
			conprintf(0, " %d Entity %d spr %d killtime %f\n", i, m_OverviewEntities[i].entity->index, m_OverviewEntities[i].hSprite, m_OverviewEntities[i].killTime);
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: Game logic, NOT RENDERING LOGIC
// Output : Returns true if MAP OVERVIEW (bitmmap+players) should be drawn disregarding render passes, etc.
//-----------------------------------------------------------------------------
bool CHudSpectator::ShouldDrawOverview(void)
{
	if (g_iUser1 == OBS_INTERMISSION)// XDM3035a
		return false;

	if (gHUD.IsSpectator())
	{
		if ((m_iInsetMode == INSET_MAP_FREE) || (m_iInsetMode == INSET_MAP_CHASE))// XDM3037a: windowed map modes
			return true;

		if (OverviewIsFullscreen())//if ((g_iUser1 >= OBS_MAP_FREE) && (g_iUser1 <= OBS_MAP_CHASE))// XDM3035a: fullscreen map modes
			return true;
	}
	else
	{
		if ((gHUD.m_iGameFlags & GAME_FLAG_ALLOW_OVERVIEW) && (gHUD.m_pCvarMiniMap->value > 0) && !gHUD.m_ZoomCrosshair.IsActive())// XDM3037
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Tells us if the overview WINDOW should be visible (doesn't matter what for)
// Output : Returns true if overview WINDOW (any contents) should be drawn disregarding render passes, etc.
//-----------------------------------------------------------------------------
bool CHudSpectator::ShouldDrawOverviewWindow(void)
{
	if (g_iUser1 == OBS_INTERMISSION)// XDM3035a
		return false;

	if (gHUD.IsSpectator())
		return (m_iInsetMode != INSET_OFF);
	else
		return ((gHUD.m_iGameFlags & GAME_FLAG_ALLOW_OVERVIEW) && (gHUD.m_pCvarMiniMap->value > 0) && !gHUD.m_ZoomCrosshair.IsActive());
}

//-----------------------------------------------------------------------------
// OBSOLETE: render pass is dynamic and has no constant values
// Purpose: XDM3037: compare current draw cycle to value returned by this function
// Output : short - the draw cycle overwiew should be drawn at, a CONSTANT VALUE
//-----------------------------------------------------------------------------
/*short CHudSpectator::GetOverviewDrawCycle(void)
{
	if (gHUD.IsSpectator())
	{
		if (g_iUser1 == OBS_MAP_FREE || g_iUser1 == OBS_MAP_CHASE)
			return 0;// draw in main (first) cycle

		if (m_iInsetMode == INSET_MAP_FREE || m_iInsetMode == INSET_MAP_CHASE)
			return 1;// draw in inset (second) cycle
	}
	else
	{
		if (gHUD.m_pCvarMiniMap->value > 0)
			return 1;
	}
	return 255;// some non-realistic value
}*/

void CHudSpectator::GetInsetWindowBounds(int *x, int *y, int *w, int *h)
{
	if (x) *x = m_OverviewData.insetWindowX;
	if (y) *y = m_OverviewData.insetWindowY;
	if (w) *w = m_OverviewData.insetWindowWidth;
	if (h) *h = m_OverviewData.insetWindowHeight;
}

bool CHudSpectator::GetAutoDirector(void)
{
	if (m_autoDirector)
		return (m_autoDirector->value > 0);

	return false;
}

bool CHudSpectator::GetShowStatus(void)
{
	if (m_drawstatus)
		return (m_drawstatus->value > 0);

	return false;
}
