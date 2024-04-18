#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_Viewport.h"

DECLARE_MESSAGE(m_DomDisplay, DomInfo);

int CHudDomDisplay::Init(void)
{
	HOOK_MESSAGE(DomInfo);
	m_iFlags = HUD_INTERMISSION|HUD_DRAW_ALWAYS;// XDM3035
	InitHUDData();
	gHUD.AddHudElem(this);
	return 1;
}

// Don't reset precached resource indexes here!
void CHudDomDisplay::InitHUDData(void)
{
	m_iNumDomPoints = 0;
	for (size_t i = 0; i < DOM_MAX_POINTS; ++i)
	{
		m_iDomPointEnts[i] = 0;
		m_iDomPointTeam[i] = TEAM_NONE;
		m_fDomPointFade[i] = 0;// XDM3037
		m_szDomPointNames[i][0] = 0;
		//memset(m_szDomPointNames[i], 0, sizeof(m_szDomPointNames));
	}
	memset(m_szMessage, 0, sizeof(m_szMessage));
	memset(&m_Message, 0, sizeof(client_textmessage_t));
}

int CHudDomDisplay::VidInit(void)
{
	// NO!	if (gHUD.m_iGameType != GT_DOMINATION)
	//		return 0;

	m_iIconPoint = gHUD.GetSpriteIndex("dompoint");
	m_iIconHighlight = gHUD.GetSpriteIndex("icon_highlight");
	//m_hsprOvwIcon = SPR_Load("sprites/iobject.spr");
	return 1;
}

int CHudDomDisplay::MsgFunc_DomInfo(const char *pszName, int iSize, void *pbuf)
{
	DBG_PRINTF("MsgFunc_%s(%d)\n", pszName, iSize);
	if (m_iNumDomPoints >= DOM_MAX_POINTS)
	{
		conprintf(1, "MsgFunc_%s() too many control points! (max. %d reached)\n", pszName, m_iNumDomPoints);
		return 1;
	}

	//if (m_iGameType == GT_DOMINATION)
	SetActive(true);

	BEGIN_READ(pbuf, iSize);

	int entindex = READ_SHORT();
	TEAM_ID team = READ_BYTE();// XDM3033

	for (size_t i=0; i < m_iNumDomPoints; ++i)// don't add existing points
	{
		if (m_iDomPointEnts[i] == entindex)
		{
			conprintf(1, "MsgFunc_%s: ERROR: control point (%d %d %s) already registered!\n", pszName, entindex, team, READ_STRING());
			END_READ();
			return 1;
		}
	}
	m_iDomPointEnts[m_iNumDomPoints] = entindex;
	m_iDomPointTeam[m_iNumDomPoints] = team;// XDM3033
	//ok	strncpy(m_szDomPointNames[m_iNumDomPoints], READ_STRING(), DOM_NAME_LENGTH);
	//buggy!	LocaliseTextString(READ_STRING(), m_szDomPointNames[m_iNumDomPoints], DOM_NAME_LENGTH);

	char *pName = m_szDomPointNames[m_iNumDomPoints];
	memset(pName, 0, DOM_NAME_LENGTH);
	strncpy(pName, LookupString(READ_STRING()), DOM_NAME_LENGTH);
	StripEndNewlineFromString(pName);
	pName[DOM_NAME_LENGTH-1] = 0;
	//conprintf(1, " CL  MsgFunc_DomInfo: %d %s\n", m_iDomPointEnts[m_iNumDomPoints], m_szDomPointNames[m_iNumDomPoints]);
	END_READ();
	m_iNumDomPoints ++;
	return 1;
}

int CHudDomDisplay::Draw(const float &flTime)
{
	wrect_t *pRect = &gHUD.GetSpriteRect(m_iIconPoint);// same icon for all control points
	wrect_t *pRectHl = &gHUD.GetSpriteRect(m_iIconHighlight);
	int r,g,b, sw,sh;
	int w = RectWidth(*pRect);// pRect->right - pRect->left;
	int h = RectHeight(*pRect);//pRect->bottom - pRect->top;
	int x = ScreenWidth - w;
	int y = (ScreenHeight - h*m_iNumDomPoints)/2;// DOM_MAX_POINTS
	int hl_w = RectWidth(*pRectHl);
	int hl_h = RectHeight(*pRectHl);
	int hl_x = (x+w/2)-(hl_w/2);
	int hl_y, hr,hg,hb;

	for (size_t i=0; i < m_iNumDomPoints; ++i)
	{
		GetTeamColor(m_iDomPointTeam[i], r,g,b);
		if (m_fDomPointFade[i] > 0)
		{
			if (m_fDomPointFade[i] > DOM_ICON_FADE_TIME)
				m_fDomPointFade[i] = DOM_ICON_FADE_TIME;

			m_fDomPointFade[i] -= (float)(gHUD.m_flTimeDelta * 20);
			if (m_fDomPointFade[i] <= 0)
			{
				//k = 0;
				m_fDomPointFade[i] = 0;
			}
			else
			{
				float k = m_fDomPointFade[i]/DOM_ICON_FADE_TIME;
				/*r = min(255, r+(int)((float)127*k));
				g = min(255, g+(int)((float)127*k));
				b = min(255, b+(int)((float)127*k));*/
				hr = r; hg = g; hb = b;
				ScaleColorsF(hr,hg,hb,k);
				//hl_x = (x+w/2)-(hl_w/2);
				hl_y = (y+h/2)-(hl_h/2);
				SPR_Set(gHUD.GetSprite(m_iIconHighlight), hr,hg,hb);
				SPR_DrawAdditive(0, hl_x, hl_y, pRectHl);
			}
		}
		SPR_Set(gHUD.GetSprite(m_iIconPoint), r, g, b);
		SPR_DrawAdditive(0, x, y, pRect);

		if (gViewPort && gViewPort->AllowedToPrintText())
		{
			DrawSetTextColor(r,g,b);
			gEngfuncs.pfnDrawConsoleStringLen(m_szDomPointNames[i], &sw, &sh);
			//conprintf(0, "w %d  h %d  sw %d  sh %d\n", w,h,sw,sh);
			DrawConsoleString(sw<=w?x:(ScreenWidth-sw), y+h, m_szDomPointNames[i]);
			y += sh;
		}

		cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(m_iDomPointEnts[i]);
		if (pEntity)
		{
			pEntity->curstate.team = m_iDomPointTeam[i];// XDM3037: force set team because entity network updates may not be available
			// XDM3038	gHUD.m_Spectator.AddOverviewEntityToList(pEntity, gEngfuncs.GetSpritePointer(m_hsprOvwIcon), OVERVIEW_ENTSCALE_GAMEGOAL, 0);
		}
		y += h;
	}
	return 1;
}

void CHudDomDisplay::SetEntTeam(int entindex, TEAM_ID teamindex)
{
	for (size_t i=0; i < m_iNumDomPoints; ++i)
	{
		if (m_iDomPointEnts[i] == entindex)
		{
			if (m_iDomPointTeam[i] != teamindex)// XDM3037: only if changed
			{
				m_iDomPointTeam[i] = teamindex;
				m_fDomPointFade[i] = DOM_ICON_FADE_TIME;// XDM3037
				if (gViewPort)
				{
					// get text FORMAT from localized string and store it in m_szMessage
					client_textmessage_t *msg = TextMessageGet("DOM_SETPOINT");
					if (msg)
					{
						byte r,g,b;
						GetTeamColor(teamindex, r,g,b);
						_snprintf(m_szMessage, DOM_MSG_LENGTH, msg->pMessage, m_szDomPointNames[i], GetTeamName(teamindex));
						m_szMessage[DOM_MSG_LENGTH-1] = 0;
						memcpy(&m_Message, msg, sizeof(client_textmessage_t));
						//m_Message = *msg;// copy localized message
						m_Message.x = -1;// override some parameters
						//m_Message.y = 0.9;
						m_Message.r1 = r;
						m_Message.g1 = g;
						m_Message.b1 = b;
						m_Message.a1 = 220;
						m_Message.holdtime = 3.0;
						m_Message.pName = "DOM_MSG";
						m_Message.pMessage = m_szMessage;
						gHUD.m_Message.MessageAdd(&m_Message);
					}
				}
			}
		}
	}
}
