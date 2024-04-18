#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "event_api.h"
#include "vgui_Viewport.h"
#include "vgui_ScorePanel.h"
#include "cl_fx.h"
#if defined(CLIENT_WEAPONS)// XDM
#include "com_weapons.h"
#endif
#include "particleman.h"
extern IParticleMan *g_pParticleMan;

/*
extern BEAM *pBeam;
extern BEAM *pBeam2;
*/

//-----------------------------------------------------------------------------
// Called sometime
//-----------------------------------------------------------------------------
int CHud::MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
	m_iSkyMode = 0;
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	m_Ammo.UpdateCrosshair(CROSSHAIR_OFF);// XDM3037a
	m_iFogMode = 0;
	ResetFog();// XDM: clear out the fog!

	if (m_iGameType != GT_SINGLE)// XDM3037: change levels without resetting this data
	{
		m_flNextAnnounceTime = 0.0f;// XDM3035
		m_iDistortMode = 0;// XDM3037
		m_fDistortValue = 0.0f;
		m_bFrozen = 0;
	}
	//Probably not a good place to put this.

/*#if defined(_TFC)
	ClearEventList();

	// catch up on any building events that are going on
	gEngfuncs.pfnServerCmd("sendevents");
#endif*/

	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

//#if !defined(_TFC)
	//Probably not a good place to put this.
// XDM	pBeam = pBeam2 = NULL;
//#endif

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Called by respawning code
//-----------------------------------------------------------------------------
int CHud::MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
#if defined (_DEBUG)
	conprintf(1, "MsgFunc_ResetHUD()\n");
#endif
	ASSERT(iSize == 0);
	Reset(false);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Camera mode sent by the server
//-----------------------------------------------------------------------------
int CHud::MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	m_iCameraMode = READ_BYTE();
	END_READ();
	return 1;
}

/* XDM3037a: OBSOLETE
extern float g_lastFOV;

int CHud::MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int newfov = READ_BYTE();
	END_READ();

#if defined(CLIENT_WEAPONS)// XDM: WTF?!
	//Weapon prediction already takes care of changing the fov. ( g_lastFOV ).
	if (g_pCvarLW && g_pCvarLW->value > 0.0f)
		return 1;
#endif// XDM

	SetFOV((float)newfov);// XDM3037
	// the clients fov is actually set in the client data update section of the hud
	return 1;
}*/

//-----------------------------------------------------------------------------
// Purpose: Enable/disable looping logo sprite in the corner of the screen
//-----------------------------------------------------------------------------
int CHud::MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	// update Train data
	m_iLogo = READ_BYTE();
	END_READ();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Inform player of current game rules, mode and limits
// WARNING: Must arrive before any other game-related messages!!!
//-----------------------------------------------------------------------------
int CHud::MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	m_iGameType			= READ_BYTE();
	m_iGameMode			= READ_BYTE();
	short iGameState	= READ_BYTE();// XDM3037a
	m_iGameSkillLevel	= READ_BYTE();
	m_iGameFlags		= READ_BYTE();// "gamedefs.h"
	m_iRevengeMode		= READ_BYTE();
	uint32 iTimeLimit	= READ_UINT32();// cvar (minutes) 
	uint32 iScoreLimit	= READ_UINT32();// unique per-rule vaule
	uint32 iDeathLimit	= READ_UINT32();// XDM3038a
	m_iRoundsLimit		= READ_BYTE();
	m_iRoundsPlayed		= READ_BYTE();
	m_iPlayerMaxHealth	= READ_BYTE();
	m_iPlayerMaxArmor	= READ_BYTE();// XDM3038
	m_flGameStartTime	= READ_FLOAT();// XDM3038a
	END_READ();

	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%hd, %hd, %hd, %hd, %hd, %hd, %u, %u, %u, %d, %d, %d, %d, %g)\n", pszName, m_iGameType, m_iGameMode, iGameState,
		m_iGameSkillLevel, m_iGameFlags, m_iRevengeMode,
		iTimeLimit, iScoreLimit, iDeathLimit,
		m_iRoundsLimit, m_iRoundsPlayed,
		m_iPlayerMaxHealth, m_iPlayerMaxArmor,
		m_flGameStartTime);

	if (iGameState != m_iGameState)// XDM3038a
	{
		if (m_iGameType > GT_SINGLE)
		{
			if (iGameState == GAME_STATE_ACTIVE)
				GameRulesEvent(GAME_EVENT_START, m_iGameType, m_iGameMode);//m_SayText.SayTextPrint(BufferedLocaliseTextString("#GAME_START\0"), 0);
		}
		m_iGameState = iGameState;
		m_flNextAnnounceTime = 0.0f;
		// XDM3038a: TODO?
		/*if (iGameState == GAME_STATE_WAITING)
		{
			m_iIntermission = 0;
			//? iTimeLimit = 0;
		}
		else if (m_iGameState == GAME_STATE_SPAWNING)
			m_iIntermission = 0;
		else if (m_iGameState == GAME_STATE_ACTIVE)
			m_iIntermission = 0;
		else if (m_iGameState == GAME_STATE_FINISHED)
			m_iIntermission = 1;
		else */if (m_iGameState == GAME_STATE_LOADING)
		{
			m_iIntermission = 1;
			iTimeLimit = 0;
			iScoreLimit = 0;
			iDeathLimit = 0;
			return 1;// TESTME!!!
		}
	}

	if (iTimeLimit != m_iTimeLimit)
	{
		if (m_iGameType != 0 && gEngfuncs.GetMaxClients() > 1 && m_iGameState != GAME_STATE_FINISHED)
		{
			char str[MAX_CHARS_PER_LINE];
			_snprintf(str, MAX_CHARS_PER_LINE, "! #Time_limit #changed_to %u\n", iTimeLimit);
			m_SayText.SayTextPrint(BufferedLocaliseTextString(str), 0);
		}
		m_iTimeLimit = iTimeLimit;
		//m_flTimeLeft = ???
		m_flNextAnnounceTime = 0.0f;// XDM3038a
	}
	if (iScoreLimit != m_iScoreLimit)
	{
		if (m_iGameType != 0 && gEngfuncs.GetMaxClients() > 1)
		{
			char str[MAX_CHARS_PER_LINE];
			_snprintf(str, MAX_CHARS_PER_LINE, "! #Score_limit #changed_to %u\n", iScoreLimit);
			m_SayText.SayTextPrint(BufferedLocaliseTextString(str), 0);
		}
		m_iScoreLimit = iScoreLimit;
		m_flNextAnnounceTime = 0.0f;// XDM3038a
	}
	if (iDeathLimit != m_iDeathLimit)// XDM3038a
	{
		if (m_iGameType != 0 && gEngfuncs.GetMaxClients() > 1)
		{
			char str[MAX_CHARS_PER_LINE];
			_snprintf(str, MAX_CHARS_PER_LINE, "! #Death_limit #changed_to %u\n", iDeathLimit);
			m_SayText.SayTextPrint(BufferedLocaliseTextString(str), 0);
		}
		m_iDeathLimit = iDeathLimit;
		m_flNextAnnounceTime = 0.0f;
	}
	//g_pCvarTimeLeft = CVAR_GET_POINTER("mp_timeleft");// XDM: now it's ready
	//g_pCvarScoreLeft = CVAR_GET_POINTER("mp_fragsleft");

	if (gViewPort)
	{
		gViewPort->UpdateOnPlayerInfo();
		//gViewPort->m_pScoreBoard->UpdateTitle();// XDM3035a: check this: movet into UpdateOnPlayerInfo()
		//gViewPort->m_iAllowSpectators = (m_iGameFlags & GAME_FLAG_ALLOW_SPECTATORS)?:1:0;
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Game rules-related updates.
// Note   : Sent very often, optimize as much as possible!
//-----------------------------------------------------------------------------
int CHud::MsgFunc_GRInfo(const char *pszName, int iSize, void *pbuf)
{
	//DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	CLIENT_INDEX bestplayer = READ_BYTE();
	TEAM_ID bestteam = READ_BYTE();
	m_iScoreLeft = READ_SHORT();
	float flTimeLeft = (float)READ_LONG();// XDM3038
	END_READ();
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d %hd %u %g)\n", pszName, bestplayer, bestteam, m_iScoreLeft, flTimeLeft);

	float d = fabs(m_flTimeLeft - flTimeLeft);
	if (d > 2.0 && m_iIntermission == 0)
	{
		conprintf(1, "CL: warning: time left counter out of sync by %g seconds.\n", d);
	}
	m_flTimeLeft = flTimeLeft;

	if (m_iGameState == GAME_STATE_WAITING)// XDM3038a: 20150813 relies on server InitHUD code
		m_iJoinTime = flTimeLeft;

	if (gViewPort)
	{
		gViewPort->GetScoreBoard()->m_iServerBestPlayer = bestplayer;
		gViewPort->GetScoreBoard()->m_iServerBestTeam = bestteam;
	}
	// checked inside if (m_iIntermission == 0)
	CheckRemainingScoreAnnouncements();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Game rules-related events like GAME_EVENT_AWARD
//-----------------------------------------------------------------------------
int CHud::MsgFunc_GREvent(const char *pszName, int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	int gameevent = READ_BYTE();
	short data1 = READ_SHORT();
	short data2 = READ_SHORT();
	END_READ();
	GameRulesEvent(gameevent, data1, data2);
	return 1;
}

//-----------------------------------------------------------------------------
// XDM3038: now these can be verified and saved on the server
//-----------------------------------------------------------------------------
int CHud::MsgFunc_HUDEffects(const char *pszName, int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHud::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	short iDistortMode = READ_BYTE();
	short value = READ_BYTE();
	END_READ();

	if ((iDistortMode & HUD_DISTORT_COLOR) && !(m_iDistortMode & HUD_DISTORT_COLOR))// something new added
	{
		if (value > 0 && m_flNextSuitSoundTime <= gHUD.m_flTime)// TODO: queue messages
		{
			PlaySoundSuit("!HEV_E3");
			m_flNextSuitSoundTime = gHUD.m_flTime + 5.0;
		}
		m_iDistortMode |= HUD_DISTORT_COLOR;
	}

	if ((iDistortMode & HUD_DISTORT_SPRITE) && !(m_iDistortMode & HUD_DISTORT_SPRITE))// something new added
	{
		if (value > 0 && m_flNextSuitSoundTime <= gHUD.m_flTime)
		{
			PlaySoundSuit("!HEV_E2");
			m_flNextSuitSoundTime = gHUD.m_flTime + 5.0;
		}
		m_iDistortMode |= HUD_DISTORT_SPRITE;
	}

	if ((iDistortMode & HUD_DISTORT_POS) && !(m_iDistortMode & HUD_DISTORT_POS))// something new added
	{
		if (value > 0 && m_flNextSuitSoundTime <= gHUD.m_flTime)
		{
			PlaySoundSuit("!HEV_E4");
			m_flNextSuitSoundTime = gHUD.m_flTime + 5.0;
		}
		m_iDistortMode |= HUD_DISTORT_POS;
	}

	m_iDistortMode = iDistortMode;// just in case something went missing
	m_fDistortValue = (float)value/255.0f;
	return 1;
}
