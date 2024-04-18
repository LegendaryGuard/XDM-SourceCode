//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_Viewport.h"

DECLARE_MESSAGE(m_Timer, HUDTimer);

int CHudTimer::Init(void)
{
	HOOK_MESSAGE(HUDTimer);
	m_iFlags = HUD_DRAW_ALWAYS | HUD_NORESET;
	Reset();// call it manually once
	InitHUDData();
	gHUD.AddHudElem(this);
	return 1;
}

int CHudTimer::VidInit(void)
{
	int iSpriteTimer = gHUD.GetSpriteIndex("timer");
	if (iSpriteTimer != HUDSPRITEINDEX_INVALID)
	{
		m_hIconTimer = gHUD.GetSprite(iSpriteTimer);
		m_prcIconTimer = &gHUD.GetSpriteRect(iSpriteTimer);
	}
	else
	{
		m_hIconTimer = HUDSPRITEINDEX_INVALID;
		m_prcIconTimer = NULL;
	}
	return 1;
}

void CHudTimer::Reset(void)
{
	//m_iSeconds = 0;
	m_fEndTime = 0;
	m_iSecondsLast = 0;
	m_iAlpha = 0;
	m_szTitle[0] = 0;
}

int CHudTimer::Draw(const float &flTime)
{
	if (m_hIconTimer == NULL || m_prcIconTimer == NULL)
		return 0;

	int r,g,b;
	int iSeconds = (int)(m_fEndTime - gHUD.m_flTime);
	if (iSeconds < 0)
	{
		Reset();
		SetActive(false);
		return 0;
	}
	else if (iSeconds < 30)
		UnpackRGB(r, g, b, RGB_RED);
	else if (iSeconds < 60)
		UnpackRGB(r, g, b, RGB_YELLOW);
	else
		UnpackRGB(r, g, b, RGB_GREEN);

	if (iSeconds != m_iSecondsLast)
		m_iAlpha = MAX_ALPHA;
	else if (m_iAlpha > MIN_ALPHA)
		m_iAlpha -= (float)(gHUD.m_flTimeDelta * TIMER_DIGIT_FADE_RATE);
	else
		m_iAlpha = MIN_ALPHA;

	int x, y;
	int icon_width, icon_height;
	int iDigitWidth = RectWidth(gHUD.GetSpriteRect(gHUD.m_HUD_number_0));

	icon_width = RectWidth(*m_prcIconTimer);
	icon_height = RectHeight(*m_prcIconTimer);
	x = ScreenWidth/2 - icon_width - iDigitWidth;
	y = gHUD.GetHUDBottomLine();

	// Draw text before we ScaleColors
	if (gViewPort && gViewPort->AllowedToPrintText() && m_szTitle[0] != '\0')
	{
		int sw, sh;// string w/h
		DrawSetTextColor(r,g,b);
		gEngfuncs.pfnDrawConsoleStringLen(m_szTitle, &sw, &sh);
		DrawConsoleString(x - sw/2, y - sh, m_szTitle);
	}

	ScaleColors(r,g,b,m_iAlpha);
	SPR_Set(m_hIconTimer, r, g, b);
	SPR_DrawAdditive(0, x, y, m_prcIconTimer);
	x += icon_width;

	//if (gHUD.m_pCvarDrawNumbers->value > 0) critical!
		x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, iSeconds, r, g, b);

	m_iWidth = x;
	m_iHeight = y;
	m_iSecondsLast = iSeconds;
	return 1;
}

int CHudTimer::MsgFunc_HUDTimer(const char *pszName,  int iSize, void *pbuf)
{
	DBG_PRINTF("MsgFunc_%s(%d)\n", pszName, iSize);
	Reset();
	BEGIN_READ(pbuf, iSize);
	short seconds = READ_SHORT();
	if (seconds > 0)
	{
		m_fEndTime = gHUD.m_flTime + seconds;
		//memset(m_szTitle, 0, sizeof(m_szTitle));
		// requires # LocaliseTextString(READ_STRING(), m_szTitle, 512);
		const char *pTitle = READ_STRING();
		if (pTitle && *pTitle)
		{
			client_textmessage_t *clmsg = TextMessageGet(pTitle);
			if (clmsg == NULL || clmsg->pMessage == NULL)
				strncpy(m_szTitle, pTitle, 512);
			else
				strncpy(m_szTitle, clmsg->pMessage, 512);
		}
		else
			m_szTitle[0] = '\0';

		SetActive(true);
	}
	else
	{
		// already^ Reset();//m_fEndTime = 0;
		SetActive(false);
	}
	END_READ();
	return 1;
}
