#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_Viewport.h"
#include "gamedefs.h"
//#include "RenderManager.h"
//#include "RenderSystem.h"
//#include "RSLight.h"

DECLARE_MESSAGE(m_FlagDisplay, FlagInfo);

int CHudFlagDisplay::Init(void)
{
	HOOK_MESSAGE(FlagInfo);

	m_iFlags = HUD_INTERMISSION|HUD_DRAW_ALWAYS;// XDM3035
	InitHUDData();

	gHUD.AddHudElem(this);
	return 1;
}

void CHudFlagDisplay::InitHUDData(void)
{
	m_iNumFlags = 0;
	for (size_t i = 0; i < MAX_CAPTURE_ENTS; ++i)
	{
		m_iFlagEnts[i] = 0;
		m_iFlagTeam[i] = TEAM_NONE;
		m_iFlagState[i] = 0;
		m_fFlagFade[i] = 0;// XDM3037
	}
}

int CHudFlagDisplay::VidInit(void)
{
	// NO!	if (gHUD.m_iGameType != GT_CTF)
	//		return 0;

	m_iIconNormal = gHUD.GetSpriteIndex("flag");
	m_iIconTaken = gHUD.GetSpriteIndex("flag_t");
	m_iIconDropped = gHUD.GetSpriteIndex("flag_d");
	m_iIconHighlight = gHUD.GetSpriteIndex("icon_highlight");
	//m_hsprOvwIcon = SPR_Load("sprites/iobject.spr");
	return 1;
}

int CHudFlagDisplay::MsgFunc_FlagInfo(const char *pszName,  int iSize, void *pbuf)
{
	DBG_PRINTF("MsgFunc_%s(%d)\n", pszName, iSize);
	if (m_iNumFlags >= MAX_CAPTURE_ENTS)
	{
		conprintf(1, "MsgFunc_%s() too many flags! (max. %d reached)\n", pszName, m_iNumFlags);
		return 1;
	}
	//if (m_iGameType == GT_CTF)
	SetActive(true);

	BEGIN_READ(pbuf, iSize);
	int entindex = READ_SHORT();
	TEAM_ID team = READ_BYTE();
	END_READ();

	for (size_t i=0; i < m_iNumFlags; ++i)// don't add existing points
	{
		if (m_iFlagEnts[i] == entindex)
		{
			conprintf(1, "MsgFunc_%s: ERROR: flag (%d %d) already registered!\n", pszName, entindex, team);
			END_READ();
			return 1;
		}
	}

	byte r,g,b;
	GetTeamColor(team, r,g,b);
	m_iFlagEnts[m_iNumFlags] = entindex;
	m_iFlagTeam[m_iNumFlags] = team;
	//if (g_pRenderManager)
	// XDM3038a		g_pRenderManager->AddSystem(new CRSLight(Vector(0,0,2048), r,g,b, 128, NULL, 0.0, 0.0), RENDERSYSTEM_FLAG_NOCLIP, m_iFlagEnts[m_iNumFlags], RENDERSYSTEM_FFLAG_ICNF_NODRAW);

	// doesn't seem to work
	if (!m_iIconNormal)
		m_iIconNormal = gHUD.GetSpriteIndex("flag");

	// Useless because StartObserver() sends InitHUD message that forces m_Spectator to reset data
	//if (!gHUD.m_Spectator.AddOverviewEntityToList(gHUD.GetSprite(m_iFlagNormal), gEngfuncs.GetEntityByIndex(m_iFlagEnts[m_iNumFlags]), 0))
	//	conprintf(0, "CL: WARNING: unable to add flag %d to the overview entlist!\n", m_iFlagEnts[m_iNumFlags]);

	//conprintf(1, " >>>>>> Registered flag %d for team %d!\n", m_iFlagEnts[m_iNumFlags], m_iFlagTeam[m_iNumFlags]);
	m_iNumFlags ++;
	return 1;
}

int CHudFlagDisplay::Draw(const float &flTime)
{
	int r,g,b;
	int spr = m_iIconNormal;

	wrect_t *pRect = &gHUD.GetSpriteRect(spr);// same rect for all icons
	wrect_t *pRectHl = &gHUD.GetSpriteRect(m_iIconHighlight);
	int icon_width = RectWidth(*pRect);//pRect->right - pRect->left;
	int icon_height = RectHeight(*pRect);//pRect->top - pRect->bottom;
	int dig_width = RectWidth(gHUD.GetSpriteRect(gHUD.m_HUD_number_0));// XDM3037a
	int x = ScreenWidth - icon_width;
	int y = (ScreenHeight - icon_height*MAX_CAPTURE_ENTS)/2;
	int hl_w = RectWidth(*pRectHl);
	int hl_h = RectHeight(*pRectHl);
	int hl_x = (x+icon_width/2)-(hl_w/2);
	int hl_y, hr,hg,hb;

	for (size_t i=0; i < m_iNumFlags; i++)
	{
		if (m_iFlagState[i] == CTF_EV_TAKEN)// our flag
			spr = m_iIconTaken;
		else if (m_iFlagState[i] == CTF_EV_DROP)
			spr = m_iIconDropped;
		else
			spr = m_iIconNormal;

		GetTeamColor(m_iFlagTeam[i], r,g,b);
		if (m_fFlagFade[i] > 0)
		{
			if (m_fFlagFade[i] > CTF_ICON_FADE_TIME)
				m_fFlagFade[i] = CTF_ICON_FADE_TIME;

			m_fFlagFade[i] -= (float)(gHUD.m_flTimeDelta * 20);
			if (m_fFlagFade[i] > 0)
			{
				hr = r; hg = g; hb = b;
				ScaleColorsF(hr,hg,hb, m_fFlagFade[i]/CTF_ICON_FADE_TIME);
				//hl_x = (x+icon_width/2)-(hl_w/2);
				hl_y = (y+icon_height/2)-(hl_h/2);
				SPR_Set(gHUD.GetSprite(m_iIconHighlight), hr,hg,hb);
				SPR_DrawAdditive(0, hl_x, hl_y, pRectHl);
			}
			else
				m_fFlagFade[i] = 0;
		}
		pRect = &gHUD.GetSpriteRect(spr);// !!
		SPR_Set(gHUD.GetSprite(spr), r, g, b);
		SPR_DrawAdditive(0, x, y, pRect);

		cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(m_iFlagEnts[i]);
		if (pEntity)
		{
			pEntity->curstate.team = m_iFlagTeam[i];// XDM3037: force set team because entity network updates may not be available
			// XDM3038	gHUD.m_Spectator.AddOverviewEntityToList(pEntity, gEngfuncs.GetSpritePointer(m_hsprOvwIcon), OVERVIEW_ENTSCALE_GAMEGOAL, 0);
		}

		gHUD.DrawHudNumber(x-dig_width*2, y+gHUD.m_iFontHeight/2, DHN_2DIGITS | DHN_DRAWZERO, g_TeamInfo[m_iFlagTeam[i]].scores_overriden, r, g, b);
		y += icon_height;
	}
	return 1;
}

void CHudFlagDisplay::SetEntState(int entindex, int team, int state)
{
	for (size_t i=0; i < m_iNumFlags; i++)
	{
		if (m_iFlagEnts[i] == entindex)
		{
			if (m_iFlagState[i] != state)
			{
				m_iFlagState[i] = state;
				m_fFlagFade[i] = CTF_ICON_FADE_TIME;// XDM3037
				//conprintf(1, " Flag %d state changed to %d\n", entindex, state);
			}
			//else
			//	conprintf(1, " Flag %d duplicate state %d message!\n", entindex, state);
		}
	}
}

void CHudFlagDisplay::Reset(void)
{
	//for (int i=0; i < m_iNumFlags; ++i)
	// GetEntityByIndex() may return NULL now		gHUD.m_Spectator.AddOverviewEntityToList(m_hsprOvwIcon, gEngfuncs.GetEntityByIndex(m_iFlagEnts[i]), 0);
}
