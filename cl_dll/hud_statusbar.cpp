// XDM: nothing left from HLSDK code :D
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
/*#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"*/

DECLARE_MESSAGE(m_StatusBar, StatusData);

int CHudStatusBar::Init(void)
{
	gHUD.AddHudElem(this);
	HOOK_MESSAGE(StatusData);
	Reset();
	m_iFlags = HUD_INTERMISSION;// XDM3037
	m_pCvarCenterId = CVAR_CREATE("hud_centerid", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	return 1;
}

void CHudStatusBar::Reset(void)
{
	size_t i = 0;
	m_szStatusText[0] = 0;
	memset(m_iStatusValues, 0, sizeof m_iStatusValues);
	m_iStatusValues[0] = 1;// 0 is the special index, which always returns true
	// reset our colors for the status bar lines (yellow is default)
	for (i = 0; i < MAX_STATUSBAR_LINES; ++i)
	{
		m_LineColors[i].Set(127,127,127,255);
		m_szStatusBar[i][0] = '\0';
	}
	//m_pflNameColors[i] = g_ColorDefault;
	SetActive(false);// start out inactive
}

// m_szStatusText -> localize, parse -> m_szStatusBar[]
void CHudStatusBar::ParseStatusString(void)
{
	// localise string first
	char szBuffer[MAX_STATUSBAR_LINELEN];
	memset(szBuffer, 0, sizeof szBuffer);
	LocaliseTextString(m_szStatusText, szBuffer, MAX_STATUSBAR_LINELEN);

	// parse m_szStatusText & m_iStatusValues into m_szStatusBar
	size_t line_num = 0;
	int valindex = 0;
	char *src = szBuffer;
	char *src_start = szBuffer;
	char *dst;
	char *dst_start;
	char szRepString[MAX_PLAYER_NAME_LENGTH];// we shall replace tag with this string

	while (*src != 0)
	{
		while (*src == '\n')
			src++;// skip over any newlines

		// No space left, prevent overflow
		if ((src - src_start) >= MAX_STATUSBAR_LINELEN)
			break;

		memset(m_szStatusBar[line_num], 0, MAX_STATUSBAR_LINELEN);
		dst = m_szStatusBar[line_num];
		dst_start = dst;
		//Int2RGB(gHUD.m_iDrawColorMain, m_pflLineColors[line_num][0], m_pflLineColors[line_num][1], m_pflLineColors[line_num][2]);
		m_LineColors[line_num].Set(gHUD.m_iDrawColorMain);

		// copy the text, char by char, until we hit a % or a \n
		while (*src != '\n' && *src != 0 && ((dst - dst_start) < MAX_STATUSBAR_LINELEN))
		{
			if (*src != '%')// just copy the character
			{
				*dst = *src;
				++dst, ++src;
			}
			else// found tag ("%i1")
			{
				// get the descriptor
				char valtype = *(++src); // move over %
				if (valtype == '%')// if it's just a %, draw a % sign
				{
					*dst = valtype;
					++dst, ++src;
					continue;
				}
				// move over descriptor, then get and move over the index
				valindex = atoi(++src);
				while (*src >= '0' && *src <= '9')// skip what we've just parsed
					++src;

				// get the string to substitute in place of the %XX
				szRepString[0] = 0;// !!!
				if (valindex >= 0 && valindex < SBAR_NUMVALUES)
				{
					switch (valtype)
					{
					case 'c':
						{
							int entindex = m_iStatusValues[valindex];// XDM3038a
							if (IsValidPlayerIndex(entindex))// faster than IsValidPlayer(indexval))
								m_LineColors[line_num] = GetPlayerColor(entindex);//GetPlayerColor(entindex, m_pflLineColors[line_num][0], m_pflLineColors[line_num][1], m_pflLineColors[line_num][2]);// XDM3035
							else
								m_LineColors[line_num].Set(gHUD.m_iDrawColorMain);//Int2RGB(gHUD.m_iDrawColorMain, m_pflLineColors[line_num][0], m_pflLineColors[line_num][1], m_pflLineColors[line_num][2]);

							// safe? continue;
						}
						break;
					case 'n':// player/monster name
						{
							GetEntityPrintableName(m_iStatusValues[valindex], szRepString, MAX_PLAYER_NAME_LENGTH);
						}
						break;
					case 'd':// a decimal number
						_snprintf(szRepString, MAX_PLAYER_NAME_LENGTH, "%hd", m_iStatusValues[valindex]);
						break;
					case 'i':// an ID
						// anyone cares? gEngfuncs.GetPlayerUniqueID(m_iStatusValues[valindex], szRepString);
						_snprintf(szRepString, MAX_PLAYER_NAME_LENGTH, "%hd", m_iStatusValues[valindex]);
						break;
					default:
						szRepString[0] = 0;
					}
				}
				else
				{
					strcat(szRepString, "error\0");
					return;
				}
				// copy temporary string over the source string tag
				for (char *cp = szRepString; *cp != 0 && ((dst - dst_start) < MAX_STATUSBAR_LINELEN); cp++, dst++)
					*dst = *cp;
			}
		}
		++line_num;
	}
	for (; line_num<MAX_STATUSBAR_LINES; ++line_num)// clear remaining lines
		m_szStatusBar[line_num][0] = 0;
}

int CHudStatusBar::Draw(const float &flTime)
{
	if (m_bReparseString)
	{
		ParseStatusString();
		m_bReparseString = false;
	}
	if (m_szStatusText[0] == 0)// the text was reset
		return 0;

	// Draw the status bar lines
	int TextHeight, TextWidth;
	int x, y;
	int y_start;

	for (int i = 0; i < MAX_STATUSBAR_LINES; ++i)
	{
		DrawConsoleStringLen(m_szStatusBar[i], &TextWidth, &TextHeight);

		if (m_pCvarCenterId->value > 0)// let user set status ID bar centering
		{
			y_start = min(ScreenHeight * (0.5f + clamp(m_pCvarCenterId->value,1,100)/200.0f), ScreenHeight - TextHeight*MAX_STATUSBAR_LINES);// XDM3038: limit value to keep all lines on the screen
			x = max(0, max(2, ScreenWidth-TextWidth) / 2);
		}
		else
		{
			y_start = gHUD.m_SayText.m_iHeight + TextHeight;//YRES(6);
		// y_start = ScreenHeight - YRES(32/* + 4*/)
			x = XRES(8);
		}
		y = y_start + TextHeight * i;// XDM3038a
		DrawSetTextColor(m_LineColors[i].r, m_LineColors[i].g, m_LineColors[i].b);//DrawSetTextColor(m_pflLineColors[i][0], m_pflLineColors[i][1], m_pflLineColors[i][2]);
		DrawConsoleString(x, y, m_szStatusBar[i]);
	}
	/*UNDONE: or is this useful at all? server may send anything
	if (gHUD.m_iGameType > GT_SINGLE)// XDM3035: Multiplayer: client team ID string
	{
		pmtrace_t pmtrace;
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetTraceHull(HULL_POINT);
		Vector end = g_vecViewOrigin + g_vecViewForward*512.0f;
		gEngfuncs.pEventAPI->EV_PlayerTrace(g_vecViewOrigin, end, PM_STUDIO_BOX|PM_GLASS_IGNORE, -1, &pmtrace);
		// since we can't check BSP leafs and nodes (all NULL), we use this simple method
		if (pmtrace.fraction != 1.0f)
		{
			//int entindex = gEngfuncs.pEventAPI->EV_IndexFromTrace(&pmtrace);
			//if (entindex >= 1 && entindex <= gEngfuncs.GetMaxClients())
			{
				//cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(entindex);
				physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( pmtrace.ent );
				if (pEntity && pe->player && pe->team == gHUD.m_iTeamNumber)
				{
					BYTE r,g,b;
					GetTeamColor(pe->team, r,g,b);
					DrawSetTextColor(r,g,b);
					
					//_snprintf(m_szClientTraceText, ASD, " = %s: Health: %d\0", g_PlayerInfoList[entindex].name, pEntity->curstate.health);
					//GetConsoleStringSize(m_szStatusBar[i], &TextWidth, &TextHeight);
					x = 4;
					y = (ScreenHeight / 2) + (TextHeight*m_pCvarCenterId->value);
					DrawConsoleString(x, y, m_szClientTraceText);
				}
			}
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}*/
	return 1;
}

// XDM3037: single message
//
// Format:
// byte 0, 1 or 255
// format string
// {
// byte parameter index
// short parameter value
// }
int CHudStatusBar::MsgFunc_StatusData(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	// required section
	byte numlines = READ_BYTE();
	if (numlines == SBAR_MSG_CLEAR)// means clear
	{
		m_szStatusText[0] = 0;
		for (size_t i=0; i<MAX_STATUSBAR_LINES; ++i)// clear
		{
			m_LineColors[i].Set(127,127,127,255);
			m_szStatusBar[i][0] = '\0';
		}
	}
	else if (numlines > 0)// XDM3038a: string section: there's only one line
	{
		strncpy(m_szStatusText, READ_STRING(), SBAR_STRING_SIZE);
		m_szStatusText[SBAR_STRING_SIZE-1] = 0;// ensure it's null terminated (strncpy() won't null terminate if read string too long)
	}
	// values section
	while (READ_REMAINING() > 0)
	{
		byte param = READ_BYTE();
		m_iStatusValues[param] = READ_SHORT();
	}
	END_READ();
	m_bReparseString = true;
	SetActive(true);
	return 1;
}
