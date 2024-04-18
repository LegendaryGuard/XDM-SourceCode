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
//
// hud_redraw.cpp
//
#include <math.h>
#include "hud.h"
#include "cl_util.h"
#include "in_defs.h"
#include "vgui_Viewport.h"
#include "vgui_ScorePanel.h"
#include "bench.h"

//-----------------------------------------------------------------------------
// Purpose: Step through the local data, placing the appropriate graphics & text as appropriate
// Input  : flTime - client time in seconds
//			intermission - 1/0
// Output : returns 1 if they've changed, 0 otherwise
//-----------------------------------------------------------------------------
int CHud::Redraw(const float &flTime, const int &intermission)
{
	m_fOldTime = m_flTime;// save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - (double)m_fOldTime;

	// Clock was reset, reset delta
	if (m_flTimeDelta < 0)
		m_flTimeDelta = 0;

	int iRet = 0;
	// Bring up the scoreboard during intermission
	if (m_iIntermission && !intermission)
	{
		// Have to do this here so the scoreboard goes away
		m_iIntermission = intermission;
		IntermissionEnd();
		m_flIntermissionStartTime = 0;
		++iRet;
	}
	else if (!m_iIntermission && intermission)
	{
		m_iIntermission = intermission;
		m_flIntermissionStartTime = flTime;
		IntermissionStart();
		++iRet;
		// Take a screenshot if the client's got the cvar set
		if (m_pCvarTakeShots->value > 0.0f)
			m_flShotTime = flTime + 1.0f;// Take a screenshot in a second
	}
	else
		m_iIntermission = intermission;

	// Bad, accumulates error.	if (m_flTimeLeft > 0 && m_flTimeDelta > 0)
	//m_flTimeLeft -= m_flTimeDelta;//only float!!
	if (m_iGameState == GAME_STATE_ACTIVE)// allow negative here?
	{
		if (m_iTimeLimit != 0)
			m_flTimeLeft = ((float)m_iTimeLimit - (m_flTime - m_flGameStartTime));
		else
			m_flTimeLeft = 0;
	}
	else if (m_iGameState == GAME_STATE_WAITING)
	{
		if (m_iJoinTime != 0)
			m_flTimeLeft = ((float)m_iJoinTime - (m_flTime - m_flGameStartTime));
		else
			m_flTimeLeft = 0;
	}
	else if (intermission)//m_iGameState == GAME_STATE_FINISHED)
	{
		if (m_iTimeLimit != 0)
			m_flTimeLeft = ((float)m_iTimeLimit - (m_flTime - m_flIntermissionStartTime));
		else
			m_flTimeLeft = 0;
	}

	if (m_iPaused == 0)// allow in demo && m_iActive > 0)
	{
		if (gViewPort->GetScoreBoard())
			gViewPort->GetScoreBoard()->UpdateCounters();
	}

	if (m_flShotTime && m_flShotTime < flTime)// time to do the automatic screenshot
	{
		CLIENT_COMMAND("snapshot\n");
		m_flShotTime = 0;
	}

	// draw all registered HUD elements
	if (m_iActive && m_pCvarDraw->value)// XDM3038c: added m_iActive check to avoid more Xash3D crashes
	{
		HUDLIST *pList = m_pHudList;
		while (pList)
		{
#if defined (ENABLE_BENCKMARK)
			if (!Bench_Active())// HL20130901
			{
#endif
				if (pList->p->IsActive())// XDM3037: for all
				{
					if (intermission == 0)
					{
						if (!(m_iHideHUDDisplay & HIDEHUD_ALL) || (pList->p->m_iFlags & HUD_DRAW_ALWAYS))
							iRet += pList->p->Draw(flTime);
					}
					else// it's an intermission,  so only draw hud elements that are set to draw during intermissions
					{
						if (pList->p->m_iFlags & HUD_INTERMISSION)
							iRet += pList->p->Draw(flTime);
					}
				}
#if defined (ENABLE_BENCKMARK)
			else
			{
				if ((pList->p == &m_Benchmark) && (pList->p->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL))
					iRet += pList->p->Draw(flTime);
			}
#endif
			pList = pList->pNext;
		}
	}

	if (m_iLogo)// draw looping animated logo in the top corner
	{
		if (m_hsprLogo == 0)
			m_hsprLogo = LoadSprite("sprites/%d_logo.spr");

		if (m_hsprLogo)
		{
			SPR_Set(m_hsprLogo, 255,255,255);
			SPR_DrawAdditive((int)(flTime * 20.0f) % SPR_Frames(m_hsprLogo), ScreenWidth - SPR_Width(m_hsprLogo, 0), SPR_Height(m_hsprLogo, 0)/2, NULL);
			++iRet;
		}
	}

	::Color rgb(255,255,255,255);
	float fr,fg,fb,fa;
	UnpackRGB(rgb, RGB_GREEN);
	rgb.Get4f(fr,fg,fb,fa);
	SET_TEXT_COLOR(fr,fg,fb);// XDM3038c: fix for text color being affected by HUD messages

	if (g_iMouseManipulationMode != MMM_NONE)// XDM3036: show active mouse manipulation mode
	{
		char szMMModeMsg[16];
		_snprintf(szMMModeMsg, 16, "#MMMODE%d", g_iMouseManipulationMode);
		szMMModeMsg[15] = '\0';
		DrawConsoleString(4, ScreenHeight/2, LookupString(szMMModeMsg, NULL));
		++iRet;//?
	}

	/* test only
	char seqstr[128];
	studiohdr_t *pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(m_pLocalPlayer->model);
	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pStudioHeader + pStudioHeader->seqindex) + m_pLocalPlayer->curstate.sequence;
	mstudioseqdesc_t *pseqdesc2 = (mstudioseqdesc_t *)((byte *)pStudioHeader + pStudioHeader->seqindex) + m_pLocalPlayer->curstate.gaitsequence;
	_snprintf(seqstr, 128, "seq: %d (%s), gsq: %d (%s)\n", m_pLocalPlayer->curstate.sequence, pseqdesc->label, m_pLocalPlayer->curstate.gaitsequence, pseqdesc2->label);
	DrawConsoleString(1, ScreenHeight/2, seqstr);*/
	return iRet;
}

//-----------------------------------------------------------------------------
// Purpose: Y all bottom text should be centered around
// Output : int Y
//-----------------------------------------------------------------------------
int CHud::GetHUDBottomLine(void)
{
	return ScreenHeight - (gHUD.m_iFontHeight * 1.5);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : xpos ypos - 
//			iMaxX - 
//			*szIt - 
//			r g b - 
// Output : int
//-----------------------------------------------------------------------------
int CHud::DrawHudString(int xpos, int ypos, int iMaxX, const char *szIt, int r, int g, int b)
{
#if defined (CLDLL_NEWFUNCTIONS)
	return xpos + gEngfuncs.pfnDrawString(xpos, ypos, szIt, r, g, b);// HL20130901
#else
	// draw the string until we hit the null character or a newline character
	int next;
	for (; *szIt != 0 && *szIt != '\n'; ++szIt)
	{
		next = xpos + gHUD.m_scrinfo.charWidths[*szIt]; // variable-width fonts look cool
		if (next > iMaxX)
			return xpos;

		TextMessageDrawChar(xpos, ypos, *szIt, r, g, b);
		xpos = next;		
	}
	return xpos;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: draws a string from right to left (right-aligned)
// Input  : xpos ypos - 
//			iMinX - 
//			*szString - 
//			r g b - 
// Output : int
//-----------------------------------------------------------------------------
int CHud::DrawHudStringReverse(int xpos, int ypos, int iMinX, const char *szString, const int &r, const int &g, const int &b)
{
#if defined (CLDLL_NEWFUNCTIONS)
	return xpos - gEngfuncs.pfnDrawStringReverse(xpos, ypos, szString, r, g, b);// HL20130901
#else
	const char *szIt;
	int next;
	// find the end of the string
	for (szIt = szString; *szIt != 0; ++szIt)
	{ // we should count the length?		
	}
	// iterate throug the string in reverse
	for (szIt--;  szIt != (szString-1);  --szIt)	
	{
		next = xpos - gHUD.m_scrinfo.charWidths[*szIt]; // variable-width fonts look cool
		if (next < iMinX)
			return xpos;

		xpos = next;
		TextMessageDrawChar(xpos, ypos, *szIt, r, g, b);
	}
	return xpos;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Number as textmessage
// Input  : xpos ypos - 
//			iMinX - 
//			iNumber - 
//			r g b - 
// Output : int x after drawing all digits
//-----------------------------------------------------------------------------
int CHud::DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, const int &r, const int &g, const int &b)
{
	char szString[32];
	_snprintf(szString, 32, "%d", iNumber);
	return DrawHudStringReverse(xpos, ypos, iMinX, szString, r, g, b);
}

//-----------------------------------------------------------------------------
// Purpose: LARGE number
// Input  : x y - 
//			iFlags - DHN_
//			iNumber - 
//			r g b - 
// Output : int x after drawing all digits
//-----------------------------------------------------------------------------
int CHud::DrawHudNumber(int x, int y, int iFlags, int iNumber, const int &r, const int &g, const int &b)
{
	int iWidth = RectWidth(GetSpriteRect(m_HUD_number_0));//GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	if (iNumber > 0)
	{
		int k;
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			k = iNumber/100;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
			SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive(0, x, y, &rc);
			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100)/10;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
			SPR_DrawAdditive(0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive(0, x, y, &rc);
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b);
		SPR_DrawAdditive(0,  x, y, &GetSpriteRect(m_HUD_number_0 + k));
		x += iWidth;
	} 
	else if (iFlags & DHN_DRAWZERO) 
	{
		SPR_Set(GetSprite(m_HUD_number_0), r, g, b);

		// SPR_Draw 100's
		if (iFlags & (DHN_3DIGITS))
		{
			//SPR_DrawAdditive(0, x, y, &rc );
			x += iWidth;
		}

		if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			//SPR_DrawAdditive(0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		SPR_DrawAdditive(0,  x, y, &GetSpriteRect(m_HUD_number_0));
		x += iWidth;
	}
	return x;
}

//-----------------------------------------------------------------------------
// Purpose: Count number of digits in a number, which may be overridden by flags
// Input  : iNumber - 
//			iFlags - DHN_
// Output : int
//-----------------------------------------------------------------------------
int CHud::GetNumWidth(const int &iNumber, const int &iFlags)
{
	if (iFlags & (DHN_3DIGITS))
		return 3;

	if (iFlags & (DHN_2DIGITS))
		return 2;

	if (iNumber <= 0)
	{
		if (iFlags & (DHN_DRAWZERO))
			return 1;
		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;
}	
