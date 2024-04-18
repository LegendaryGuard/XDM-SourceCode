#include "hud.h"
#include "cl_util.h"
#include "vgui_Viewport.h"
#include "con_nprint.h"

int CHudGameDisplay::Init(void)
{
	m_iFlags = HUD_ACTIVE|HUD_DRAW_ALWAYS;
	m_pCvarDrawScore = CVAR_CREATE("hud_drawscore", "1", FCVAR_ARCHIVE|FCVAR_CLIENTDLL);
	InitHUDData();
	gHUD.AddHudElem(this);
	return 1;
}

void CHudGameDisplay::InitHUDData(void)
{
	m_iLastScore = 0;
	m_iLastScoreChange = 0;
	m_iLastRespawns = 0;
	m_iAlphaScore = MIN_ALPHA;
	m_iAlphaRespawns = MIN_ALPHA;
}

int CHudGameDisplay::VidInit(void)
{
	m_iIconScore = gHUD.GetSpriteIndex("mp_score");
	m_iIconRespawns = gHUD.GetSpriteIndex("mp_respawns");
	return 1;
}

int CHudGameDisplay::Draw(const float &flTime)
{
	if (gHUD.m_iGameType == GT_SINGLE || m_pCvarDrawScore->value <= 0.0f)
		return 0;

	if (gHUD.m_pLocalPlayer == NULL || gEngfuncs.GetLocalPlayer() == NULL)// sometimes this happen
		return 0;

	// Detect changes
	int idx = 0;
	if (gHUD.IsSpectator())// get value of tracked player
	{
		idx = gHUD.GetSpectatorTarget();
		if (!IsActivePlayer(idx))
			return 0;
	}
	else
		idx = gEngfuncs.GetLocalPlayer()->index;//idx = gHUD.m_pLocalPlayer->index;// in Xash m_pLocalPlayer corrupts A LOT!!!

	int x, y;
	int icon_width, icon_height;
	int sw, sh;// string w/h
	int r,g,b;
	wrect_t *pRect;
	char str[16];

	if (gHUD.m_pCvarUsePlayerColor->value > 0.0f)
	{
		::Color c = GetPlayerColor(idx);
		c.Get(r,g,b,x);// x is temporary, there's no alpha anyway
	}
	else
		UnpackRGB(r, g, b, RGB_GREEN);

	// if (gHUD.m_iScoreLimit) ?
	pRect = &gHUD.GetSpriteRect(m_iIconScore);
	icon_width = RectWidth(*pRect);
	icon_height = RectHeight(*pRect);
	x = ScreenWidth - icon_width - STATUS_ICONS_MARGIN;
	y = (int)(ScreenHeight/4)*3;

	if (m_iLastScore != g_PlayerExtraInfo[idx].score)
	{
		m_iAlphaScore = MAX_ALPHA;
		m_iLastScoreChange = g_PlayerExtraInfo[idx].score - m_iLastScore;
	}
	else if (m_iAlphaScore > MIN_ALPHA)
		m_iAlphaScore -= (float)(gHUD.m_flTimeDelta * SCORE_ICON_FADE_RATE);
	else
	{
		m_iAlphaScore = MIN_ALPHA;
		m_iLastScoreChange = 0;
	}
	ScaleColors(r,g,b,m_iAlphaScore);
	SPR_Set(gHUD.GetSprite(m_iIconScore), r, g, b);
	SPR_DrawAdditive(0, x, y, pRect);
	y += icon_height;
	if (gViewPort && gViewPort->AllowedToPrintText())
	{
		_snprintf(str, 16, "%d\0", g_PlayerExtraInfo[idx].score);
		DrawSetTextColor(r,g,b);
		gEngfuncs.pfnDrawConsoleStringLen(str, &sw, &sh);
		DrawConsoleString(x + icon_width - sw, y, str);
		/* FAIL because of position if (m_iLastScore != g_PlayerExtraInfo[idx].score)// detected once
		{
			con_nprint_t nprintinfo;
			nprintinfo.index = 10;
			nprintinfo.time_to_live = 3.0f;
			nprintinfo.color[0] = (g_PlayerExtraInfo[idx].score > m_iLastScore)?0:1;
			nprintinfo.color[1] = (g_PlayerExtraInfo[idx].score > m_iLastScore)?1:0;
			nprintinfo.color[2] = 0;
			m_iAlphaScore = MAX_ALPHA;
			//pNPrintText = "%+d\n";
			CON_NXPRINTF(&nprintinfo, "%+d\n", g_PlayerExtraInfo[idx].score-m_iLastScore);
		}*/
		y += sh;

		if (/*m_iAlphaScore > MIN_ALPHA && */m_iLastScoreChange != 0)// XDM3038c
		{
			_snprintf(str, 16, "%+d\0", m_iLastScoreChange);
			DrawSetTextColor((m_iLastScoreChange > 0)?0:255, (m_iLastScoreChange > 0)?255:0, 0);
			gEngfuncs.pfnDrawConsoleStringLen(str, &sw, &sh);
			DrawConsoleString(x - sw - STATUS_ICONS_MARGIN, y-sh-sh, str);
		}
	}

	// if (gHUD.m_iDeathLimit) ?
	pRect = &gHUD.GetSpriteRect(m_iIconRespawns);
	icon_width = RectWidth(*pRect);
	icon_height = RectHeight(*pRect);
	x = ScreenWidth - icon_width - STATUS_ICONS_MARGIN;
	//y = y;
	if (gHUD.m_pCvarUsePlayerColor->value > 0.0f)
	{
		::Color c = GetPlayerColor(idx);
		r = c.r;
		g = c.g;
		b = c.b;
	}
	else
		UnpackRGB(r, g, b, RGB_GREEN);

	if (m_iLastRespawns != g_PlayerExtraInfo[idx].deaths)
		m_iAlphaRespawns = MAX_ALPHA;
	else if (m_iAlphaRespawns > MIN_ALPHA)
		m_iAlphaRespawns -= (float)(gHUD.m_flTimeDelta * SCORE_ICON_FADE_RATE);
	else
		m_iAlphaRespawns = MIN_ALPHA;

	ScaleColors(r,g,b,m_iAlphaRespawns);
	SPR_Set(gHUD.GetSprite(m_iIconRespawns), r, g, b);
	SPR_DrawAdditive(0, x, y, pRect);
	y += icon_height;
	if (gViewPort && gViewPort->AllowedToPrintText())
	{
		_snprintf(str, 16, "%d\0", g_PlayerExtraInfo[idx].deaths);
		DrawSetTextColor(r,g,b);
		gEngfuncs.pfnDrawConsoleStringLen(str, &sw, &sh);
		DrawConsoleString(x + icon_width - sw, y, str);
		//DrawConsoleString(ScreenWidth - icon_width/2 - sw/2, y, str);
		y += sh;
	}

	m_iLastScore = g_PlayerExtraInfo[idx].score;
	m_iLastRespawns = g_PlayerExtraInfo[idx].deaths;
	m_iWidth = x;
	m_iHeight = y;
	return 1;
}
