#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "hud_servers.h"
#include "vgui_Viewport.h"
#include "musicplayer.h"
#include "RenderManager.h"// XDM
#include "cl_fx.h"
#include "bsputil.h"
#include "pm_defs.h"
#include "pm_shared.h"

// Supposed-to-be-obsolete code, changed significantly

//-----------------------------------------------------------------------------
// CHud constructor
// Prepare default values here
//-----------------------------------------------------------------------------
CHud::CHud(): m_iSpriteCount(0), m_pHudList(NULL)
{
	DBG_HUD_PRINTF("CHud::CHud()\n");
	m_flTime = 0.0f;
	m_iActive = 0;
	m_iPaused = 0;
	m_iIntermission = 0;
	m_iHardwareMode = 0;

	m_iDistortMode = 0;
	m_fDistortValue = 0;

	m_iFogMode = 0;
	m_iSkyMode = 0;
	m_iCameraMode = 0;
	m_vecSkyPos.Clear();
	m_flFogStart = 0;
	m_flFogEnd = 0;
	//m_vecOrigin.Clear();
	//m_vecAngles.Clear();
	m_iKeyBits = 0;
	m_iWeaponBits = 0;
	//m_iActiveWeapon = WEAPON_NONE;

	m_bFrozen = 0;
	m_iTeamNumber = TEAM_NONE;
	m_iGameType = 0;
	m_iGameMode = 0;
	m_iGameState = 0;
	m_iGameSkillLevel = 0;
	m_iGameFlags = 0;
	m_iRevengeMode = 0;
	m_fLastScoreAward = 0;
	m_flShotTime = 0.0f;
	m_flNextAnnounceTime = 0;// XDM3035
	m_flNextSuitSoundTime = 0;
	m_flGameStartTime = 0;
	m_flIntermissionStartTime = 0;
	m_flTimeLeft = 0;
	m_iJoinTime = 0;// XDM3038a
	m_iTimeLimit = 0;
	m_iScoreLeft = 0;
	m_iDeathLimit = 0;
	m_iTimeLeftLast = 0;
	m_iScoreLeftLast = 0;
	m_iScoreLimit = 0;
	m_iRoundsPlayed = 0;
	m_iRoundsLimit = 0;

	m_pHudList = NULL;
	m_pSpriteList = NULL;
	m_iSpriteCount = 0;
	m_iSpriteCountAllRes = 0;
	m_flMouseSensitivity = 1.0f;
	m_rghSprites = NULL;
	m_rgrcRects = NULL;
	m_rgszSpriteNames = NULL;
	m_iszSpriteFrames = NULL;
}

//-----------------------------------------------------------------------------
// CHud destructor
// Cleans up memory allocated for m_rg* arrays
//-----------------------------------------------------------------------------
CHud::~CHud()
{
	DBG_HUD_PRINTF("CHud::~CHud()\n");
	if (m_rghSprites)
	{
		delete [] m_rghSprites;
		m_rghSprites = NULL;
	}
	if (m_rgrcRects)
	{
		delete [] m_rgrcRects;
		m_rgrcRects = NULL;
	}
	if (m_rgszSpriteNames)
	{
		delete [] m_rgszSpriteNames;
		m_rgszSpriteNames = NULL;
	}
	if (m_iszSpriteFrames)
	{
		delete [] m_iszSpriteFrames;// XDM3037a
		m_iszSpriteFrames = NULL;
	}
	if (m_pHudList)
	{
		HUDLIST *pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}

	ServersShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: This is called every time the DLL is loaded by HUD_Init()
//-----------------------------------------------------------------------------
void CHud::Init(void)
{
	DBG_HUD_PRINTF("CHud::Init()\n");
	memset(m_szMessageAward, 0, sizeof(m_szMessageAward));
	memset(m_szMessageCombo, 0, sizeof(m_szMessageCombo));
	memset(m_szMessageTimeLeft, 0, sizeof(m_szMessageTimeLeft));
	memset(m_szMessageAnnouncement, 0, sizeof(m_szMessageAnnouncement));
	memset(m_szMessageExtraAnnouncement, 0, sizeof(m_szMessageExtraAnnouncement));
	memset(&m_MessageAward, 0, sizeof(client_textmessage_t));
	memset(&m_MessageCombo, 0, sizeof(client_textmessage_t));
	memset(&m_MessageTimeLeft, 0, sizeof(client_textmessage_t));
	memset(&m_MessageAnnouncement, 0, sizeof(client_textmessage_t));
	memset(&m_MessageExtraAnnouncement, 0, sizeof(client_textmessage_t));

	// XDM3036: only HUD-vars here
	m_pCvarColorMain		= CVAR_CREATE("hud_grn",			"159 159 255",	FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// RGB_GREEN
	m_pCvarColorRed			= CVAR_CREATE("hud_red",			"255 0 0",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// RGB_RED
	m_pCvarColorBlue		= CVAR_CREATE("hud_blu",			"0 0 255",		FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// RGB_BLUE
	m_pCvarColorCyan		= CVAR_CREATE("hud_cyn",			"0 255 255",	FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// RGB_CYAN
	m_pCvarColorYellow		= CVAR_CREATE("hud_yel",			"255 255 0",	FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// RGB_YELLOW
	m_pCvarStealMouse		= CVAR_CREATE("hud_capturemouse",	"1",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarDraw				= CVAR_CREATE("hud_draw",			"1",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarDrawNumbers		= CVAR_CREATE("hud_drawnumbers",	"1",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarEventIconTime	= CVAR_CREATE("hud_eventicontime",	"0",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM3037a
	m_pCvarUseTeamColor		= CVAR_CREATE("hud_useteamcolor",	"0",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM
	m_pCvarUsePlayerColor	= CVAR_CREATE("hud_useplayercolor",	"1",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarMiniMap			= CVAR_CREATE("hud_minimap",		"0",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarTakeShots		= CVAR_CREATE("hud_takesshots",		"0",			FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// controls whether or not to automatically take screenshots at the end of a round

	m_flTimeLeft = 0.0f;
	m_iScoreLeftLast = 0;// XDM3035
	m_iTimeLeftLast = 0;
	m_flNextAnnounceTime = 0.0f;// XDM3035
	DistortionReset();// XDM3038
	m_bFrozen = 0;

	m_pSpriteList = NULL;
	m_iFogMode = 0;// XDM3035
	m_iSkyMode = 0;
	m_iLogo = 0;
	m_fFOV = 0;// XDM3037a m_iFOV = DEFAULT_FOV;
	//m_iCameraMode = 0;

	m_flFogStart = 0.0f;
	m_flFogEnd = 0.0f;

	// Clear any old HUD list
	if (m_pHudList)
	{
		HUDLIST *pList;
		while (m_pHudList)
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free(pList);
		}
		m_pHudList = NULL;
	}
	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;
	// These are self-registering here
	m_ZoomCrosshair.Init();// XDM: must be first!
	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	// XDM3038: OBSOLETE	m_TextMessage.Init();
	m_StatusIcons.Init();
	//m_RocketScreen.Init();// XDM
	m_GameDisplay.Init();// XDM3038a
	m_DomDisplay.Init();
	m_FlagDisplay.Init();
	m_Timer.Init();// XDM3038c
#if defined (ENABLE_BENCKMARK)
	m_Benchmark.Init();
#endif
	ServersInit();
	Reset(true);// XDM3038c: TESTME: true
}

//-----------------------------------------------------------------------------
// Purpose: GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// Input  : *SpriteName - 
// Output : int index into the gHUD.m_rghSprites[] array, 0 if sprite not found
//-----------------------------------------------------------------------------
int CHud::GetSpriteIndex(const char *SpriteName)
{
	if (SpriteName && *SpriteName != 0)
	{
		// look through the loaded sprite name list for SpriteName
		for (int i = 0; i < m_iSpriteCount; ++i)
		{
			if (strncmp(SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH) == 0)
				return i;
		}
	}
	DBG_PRINTF("CHud::GetSpriteIndex(\"%s\") failed!\n", SpriteName);
	return HUDSPRITEINDEX_INVALID; // invalid sprite
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hSprite - HLSPRITE
// Output : int - index in HUD array
//-----------------------------------------------------------------------------
int CHud::GetSpriteIndex(HLSPRITE hSprite)
{
	// look through the loaded sprite name list for SpriteName
	for (int i = 0; i < m_iSpriteCount; ++i)
	{
		if (m_rghSprites[i] == hSprite)
			return i;
	}
	DBG_PRINTF("CHud::GetSpriteIndex(%d) failed!\n", hSprite);
	return HUDSPRITEINDEX_INVALID; // invalid sprite
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: extra layer of abstraction and protection
// Input  : index - 
// Output : int
//-----------------------------------------------------------------------------
int CHud::GetSpriteFrame(const int &index)
{
	if (index < m_iSpriteCount)
		return m_iszSpriteFrames[index];

	return 0;// invalid index
}

//-----------------------------------------------------------------------------
// Purpose: Called when the client DLL initializes and whenever the vid_mode is changed (as if HL can switch modes... :-/ )
//-----------------------------------------------------------------------------
void CHud::VidInit(void)
{
	DBG_HUD_PRINTF("CHud::VidInit()\n");
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);
	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	m_iFogMode = 0;// XDM: clear out the fog!
	ResetFog();
	m_hsprLogo = 0;	

	LoadHUDSprites("hud", false);// load main file once

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	const char *pFontSpriteName = "number_0";
	m_HUD_number_0 = GetSpriteIndex(pFontSpriteName);
	ASSERT(m_HUD_number_0 != HUDSPRITEINDEX_INVALID);
	if (m_HUD_number_0 != HUDSPRITEINDEX_INVALID)
		m_iFontHeight = RectHeight(m_rgrcRects[m_HUD_number_0]);//m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;
	else
	{
		m_iFontHeight = 32;// ERROR!!!!
		conprintf(0, "CHud::VidInit() error: sprite \"%s\" not found in list!\n", pFontSpriteName);
	}

	HUDLIST *pList = m_pHudList;// XDM3038c
	while (pList)
	{
		if (pList->p)
			pList->p->VidInit();
		pList = pList->pNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add some CHudBase-derived member to a linked list
// Warning: Memory for each element is allocated by malloc()!
// Input  : *phudelem - 
//-----------------------------------------------------------------------------
bool CHud::LoadHUDSprites(const char *listname, bool additional)
{
	DBG_HUD_PRINTF("CHud::LoadHUDSprites(%s)\n", listname);
	char szFilePath[128];
	int j = 0;
	_snprintf(szFilePath, 128, "sprites/%s.txt", listname);
	szFilePath[127] = '\0';
	if (m_pSpriteList == NULL)
	{
		conprintf(0, "Loading %s\n", szFilePath);
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList(szFilePath, &m_iSpriteCountAllRes);
		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			for (j = 0; j < m_iSpriteCountAllRes; ++j)
			{
				if (p->iRes == m_iRes)
					++m_iSpriteCount;
				++p;
			}
			// allocated memory for sprite handle arrays
 			m_rghSprites = new HLSPRITE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];
			m_iszSpriteFrames = new int[m_iSpriteCount];// XDM3037a
			p = m_pSpriteList;
			int index = 0;
			for (j = 0; j < m_iSpriteCountAllRes; ++j)
			{
				if (p->iRes == m_iRes)
				{
					//char sz[256];
					_snprintf(szFilePath, 128, "sprites/%s.spr", p->szSprite);
					szFilePath[127] = '\0';
					m_rghSprites[index] = SPR_Load(szFilePath);
					m_rgrcRects[index] = p->rc;
					strncpy(&m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH);
					m_iszSpriteFrames[index] = 0;// XDM3037a
					++index;
				}
				++p;
			}
		}
		else
		{
			conprintf(0, "HUD: Unable to load \"%s\"!\n", szFilePath);
			return false;
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;
		int index = 0;
		for (j = 0; j < m_iSpriteCountAllRes; ++j)
		{
			if (p->iRes == m_iRes)
			{
				char sz[256];
				_snprintf(sz, 256, "sprites/%s.spr", p->szSprite);
				m_rghSprites[index] = SPR_Load(sz);
				++index;
			}
			++p;
		}
	}

	// XDM3037a: load sprite frames. NOTE: this file is optional
	_snprintf(szFilePath, 128, "sprites/%s_frames.txt", listname);
	FILE *pf = LoadFile(szFilePath, "r");//fopen(szFramesFilePath, "r");
	if (pf)
	{
		char szString[128];
		//char szSprName[96]; reuse szFilePath
		int iRes, iFrame;
		conprintf(0, "Loading %s\n", szFilePath);
		while (fgets(szString, 128, pf) != NULL)
		{
			if (!strncmp(szString, "//", 2) || szString[0] == '\n')
				continue;// skip comments or blank lines

			/*char *ch = strchr(szString, '\n');
			if (ch == NULL)// try windows-style
				strchr(szString, '\r');

			if (ch)// force all strings to end only with \0
				*ch = '\0';*/

			iFrame = 0;
			if (sscanf(szString, "%95s %d %d", szFilePath, &iRes, &iFrame) == 3)
			{
				if (iRes == m_iRes)
				{
					j = GetSpriteIndex(szFilePath);
					if (j != HUDSPRITEINDEX_INVALID)
						m_iszSpriteFrames[j] = min(iFrame, SPR_Frames(m_rghSprites[j])-1);
					else
						conprintf(0, " sprite \"%s\" is not listed!\n", szString);
				}
			}
			else
				conprintf(0, " bad line: \"%s\"!\n", szString);
		}
		fclose(pf);
	}
	else
		conprintf(0, "HUD: Unable to load \"%s\"!\n", szFilePath);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Add some CHudBase-derived member to a linked list
// Warning: Memory for each element is allocated by malloc()!
// Input  : *phudelem - 
//-----------------------------------------------------------------------------
void CHud::AddHudElem(CHudBase *phudelem)
{
	if (!phudelem)
		return;

	//DBG_HUD_PRINTF("CHud::AddHudElem()\n");
	//phudelem->Think();
	HUDLIST *pdl, *ptemp;

	pdl = (HUDLIST *)malloc(sizeof(HUDLIST));
	if (pdl == NULL)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: centralized. Updates sensitivity.
// Input  : &fFieldOfView - 
//-----------------------------------------------------------------------------
void CHud::SetFOV(const float &fFieldOfView)
{
	//DBG_HUD_PRINTF("CHud::SetFOV(%g)\n", fFieldOfView);
	if (fFieldOfView == 0)
		g_lastFOV = GetUpdatedDefaultFOV();
	else
		g_lastFOV = fFieldOfView;

	// XDM3037a:	m_iFOV = (int)g_lastFOV;
	m_fFOV = g_lastFOV;
	m_flMouseSensitivity = GetSensitivityModByFOV(g_lastFOV);// XDM
}

//-----------------------------------------------------------------------------
// Purpose: Returns value suitable for calculations (never 0)
// Output : float 90 This function should NEVER return 0!
//-----------------------------------------------------------------------------
float CHud::GetCurrentFOV(void)// XDM3037a
{
	// XDM3037a: Server-set FOV has highest priority
	//if (m_ClientData.fov > 0)// && m_ClientData.fov != DEFAULT_FOV)
	if (m_fFOVServer > 0)// XDM3038a
		return m_fFOVServer;//m_ClientData.fov;
	else if (m_fFOV > 0)// XDM3037a: new concept: m_iFOV is an OVERRIDE, normally it should be 0
		return m_fFOV;
	else
		return GetUpdatedDefaultFOV();
}

//-----------------------------------------------------------------------------
// Purpose: Returns value suitable for calculations (never 0) and validates default FOV cvar
// Output : float 90 This function should NEVER return 0!
//-----------------------------------------------------------------------------
float CHud::GetUpdatedDefaultFOV(void)// XDM
{
	if (g_pCvarDefaultFOV->value == 0.0f)
	{
		g_pCvarDefaultFOV->value = DEFAULT_FOV;
		CVAR_SET_FLOAT(g_pCvarDefaultFOV->name, DEFAULT_FOV);// slow
	}
	return g_pCvarDefaultFOV->value;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CHud::GetSensitivity(void)
{
	return m_flMouseSensitivity;
}

//-----------------------------------------------------------------------------
// Purpose: XDM: called when game paused state is changed
// Input  : paused - 
//-----------------------------------------------------------------------------
void CHud::OnGamePaused(const int &paused)
{
	DBG_HUD_PRINTF("CHud::OnGamePaused(%d)\n", paused);
	m_iPaused = paused;
	BGM_GamePaused(paused);
}

//-----------------------------------------------------------------------------
// Purpose: XDM: called when game loading state is changed
// Input  : active - 
//-----------------------------------------------------------------------------
void CHud::OnGameActivated(const int &active)
{
	DBG_HUD_PRINTF("CHud::OnGameActivated(%d)\n", active);
	m_iActive = active;
	if (active)
	{
		g_pWorld = gEngfuncs.GetEntityByIndex(0);
		//fail :(	gViewPort->Initialize();

		// XDM3037a: exec client-side map-related config
		char cmd[MAX_MAPPATH];
		_snprintf(cmd, MAX_MAPPATH, "exec maps/%s_cl.cfg\n", GetMapName(true));
		CLIENT_COMMAND(cmd);

		/* does not work	char *s = gEngfuncs.pfnGetCvarString("bottomcolor");
		gEngfuncs.PlayerInfo_SetValueForKey("bottomcolor", s);// XDM3038a: read these back from cvars in case game rules change from team-based to free for all
		s = gEngfuncs.pfnGetCvarString("topcolor");
		gEngfuncs.PlayerInfo_SetValueForKey("topcolor", s);*/

		g_iMsgRS_UID_Postfix = 0;// XDM3038c
	}
	else
	{
		if (m_iGameType != GT_SINGLE)// XDM3038: change levels without resetting this data
		{
			DistortionReset();
			m_bFrozen = 0;
		}
		if (g_pRenderManager)// XDM3035a
			g_pRenderManager->DeleteAllSystems();

		g_pWorld = NULL;// XDM3035c
		g_pViewLeaf = NULL;// XDM3038

		m_flNextAnnounceTime = 0;// XDM3035
		//no m_flNextSuitSoundTime = 0;
		m_fLastScoreAward = 0;
		m_flTimeLeft = 0;// XDM3038
		m_iTimeLimit = 0;
		m_iScoreLeft = 0;
		m_iTimeLeftLast = 0;
		m_iScoreLeftLast = 0;
		m_iScoreLimit = 0;
		m_iRoundsPlayed = 0;
		m_iRoundsLimit = 0;

		m_Ammo.UpdateCrosshair(CROSSHAIR_OFF);// XDM3038
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: Does local player have the HEV suit on?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHud::PlayerHasSuit(void)
{
	return FBitSet(m_iWeaponBits, 1<<WEAPON_SUIT);// eventually this should be replaced
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Is local player alive?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHud::PlayerIsAlive(void)
{
	if (IsSpectator())
	{
		if (g_iUser2 > 0)
		{
			cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex(g_iUser2);
			if (pEnt && pEnt->curstate.health > 0)
				return true;
		}
		return false;// ?
	}
	else if (pmove)
		return (pmove->dead == false);
	// BAD! CRASH when gmsgDamage is called too early (CO_AI )else if (gHUD.m_pLocalPlayer)
	//	return (gHUD.m_pLocalPlayer->curstate.health > 0);
	else
		return (gHUD.m_Health.m_iHealth > 0);//return (gHUD.m_ClientData.health > 0);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038
//-----------------------------------------------------------------------------
void CHud::DistortionReset(void)
{
	DBG_HUD_PRINTF("CHud::DistortionReset()\n");
	m_iDistortMode = 0;// XDM3037
	m_fDistortValue = 0.0f;
}





//-----------------------------------------------------------------------------
// Purpose: Active means we have to draw it
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudBase::IsActive(void)
{
	return (m_iFlags & HUD_ACTIVE);
}

//-----------------------------------------------------------------------------
// Purpose: Abstraction layer to toggle state
// Input  : active - 
//-----------------------------------------------------------------------------
void CHudBase::SetActive(bool active)
{
	if (active)
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;
}
