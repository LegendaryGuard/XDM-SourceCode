#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_Viewport.h"

// Half-Life-compatible saytext printing system

DECLARE_MESSAGE(m_SayText, SayText);

CHudSayText::CHudSayText() : CHudBase()
{
	m_pSayTextList = NULL;
	m_iMaxLines = 0;
}

CHudSayText::~CHudSayText()
{
	FreeData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSayText::FreeData(void)
{
	DBG_HUD_PRINTF("CHudSayText::FreeData()\n");
	if (m_pSayTextList)
	{
		delete [] m_pSayTextList;
		m_pSayTextList = NULL;
		m_iMaxLines = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int 
//-----------------------------------------------------------------------------
int CHudSayText::Init(void)
{
	DBG_HUD_PRINTF("CHudSayText::Init()\n");
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(SayText);

	m_pCvarSayText			= CVAR_CREATE("hud_saytext", "1", FCVAR_ARCHIVE|FCVAR_CLIENTDLL);
	m_pCvarSayTextTime		= CVAR_CREATE("hud_saytext_time", "5", FCVAR_ARCHIVE|FCVAR_CLIENTDLL);
	m_pCvarSayTextHighlight	= CVAR_CREATE("hud_saytext_hlight", "1", FCVAR_ARCHIVE|FCVAR_CLIENTDLL);
	m_pCvarSayTextLines		= CVAR_CREATE("hud_saytext_lines", "6", FCVAR_ARCHIVE|FCVAR_CLIENTDLL);

	FreeData();
	InitHUDData();

	m_iFlags |= (HUD_INTERMISSION|HUD_DRAW_ALWAYS); // is always drawn during an intermission // XDM
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSayText::InitHUDData(void)
{
	DBG_HUD_PRINTF("CHudSayText::InitHUDData()\n");
	if (m_pSayTextList == NULL)
	{
		if (m_pCvarSayTextLines->value < 2)
			m_pCvarSayTextLines->value = 2;
		else if (m_pCvarSayTextLines->value > MAX_LINES)
			m_pCvarSayTextLines->value = MAX_LINES;

		m_iMaxLines = (short)m_pCvarSayTextLines->value;
		m_pSayTextList = new SayTextItem[m_iMaxLines];// XDM3035: this should really be a queue, not an array!
	}

	if (m_pSayTextList)
		memset(m_pSayTextList, 0, sizeof(SayTextItem)*m_iMaxLines);// XDM3036

	m_flScrollTime = 0;// the time at which the lines next scroll up
	line_height = YRES(6);// just for start
	Y_START = (int)((float)ScreenHeight * 0.865f) - (line_height * m_iMaxLines);//(7/8);
	m_iHeight = Y_START + line_height*m_iMaxLines;// for external use
	LocaliseTextString(" (#TEAM)", m_szLocalizedTeam, MAX_TEAMNAME_LENGTH);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CHudSayText::VidInit(void)
{
	DBG_HUD_PRINTF("CHudSayText::VidInit()\n");
	FreeData();
	InitHUDData();
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int number of lines moved
//-----------------------------------------------------------------------------
int CHudSayText::ScrollTextUp(void)
{
	size_t i;
	size_t iNumLinesToErase = 1;
	for (i=iNumLinesToErase; i<m_iMaxLines; ++i)// count lines that are actually one wrapped line
	{
		if (m_pSayTextList[0].szLineBuffer[0] == ' ')// continuation of the wrapped line
			++iNumLinesToErase;
		else
			break;// only count contingous sequence
	}

	for (i=0; i<iNumLinesToErase; ++i)// print lines that are about to be deleted
		ConsolePrint(m_pSayTextList[i].szLineBuffer);

	memmove(&m_pSayTextList[0], &m_pSayTextList[iNumLinesToErase], sizeof(SayTextItem) * (m_iMaxLines-iNumLinesToErase));// move memory range from second line to last line over the first line, last line must be erased.

	for (i=0; i<iNumLinesToErase; ++i)// erase remaining (duplicate) lines
		m_pSayTextList[m_iMaxLines-1-i].szLineBuffer[0] = 0;

	return iNumLinesToErase;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &flTime - 
// Output : int
//-----------------------------------------------------------------------------
int CHudSayText::Draw(const float &flTime)
{
	if (m_pSayTextList == NULL)
		return 0;

	if ((gViewPort && !gViewPort->AllowedToPrintText()) || m_pCvarSayText->value <= 0.0f)
		return 1;

	int y = Y_START;

	// make sure the scrolltime is within reasonable bounds,  to guard against the clock being reset
	m_flScrollTime = min(m_flScrollTime, flTime + m_pCvarSayTextTime->value);

	if (m_flScrollTime <= flTime)
	{
		if (m_pSayTextList[0].szLineBuffer[0])
		{
			m_flScrollTime = flTime + m_pCvarSayTextTime->value;
			// push the console up
			ScrollTextUp();
		}
		else// buffer is empty,  just disable drawing of this section
			SetActive(false);
	}

	for (size_t i = 0; i < m_iMaxLines; ++i)
	{
		if (m_pSayTextList[i].szLineBuffer[0])
		{
			if (m_pSayTextList[i].iNameLength > 0)
			{
				char replaced = m_pSayTextList[i].szLineBuffer[m_pSayTextList[i].iNameLength];
				m_pSayTextList[i].szLineBuffer[m_pSayTextList[i].iNameLength] = 0;
				DrawSetTextColor(m_pSayTextList[i].NameColor.r, m_pSayTextList[i].NameColor.g, m_pSayTextList[i].NameColor.b);
				int x = DrawConsoleString(LINE_START, y, m_pSayTextList[i].szLineBuffer);
				m_pSayTextList[i].szLineBuffer[m_pSayTextList[i].iNameLength] = replaced;
				//int x = DrawConsoleString(LINE_START, y, buf);

				// color is reset after each string draw
				// XDM: this prints the real saytext
				if (m_pSayTextList[i].StringColor[0] + m_pSayTextList[i].StringColor[1] + m_pSayTextList[i].StringColor[2] > 0)// faster than || ?
					DrawSetTextColor(m_pSayTextList[i].StringColor.r, m_pSayTextList[i].StringColor.g, m_pSayTextList[i].StringColor.b);

				//DrawConsoleString(x, y, m_pSayTextList[i].szLineBuffer + l);
				DrawConsoleString(x, y, m_pSayTextList[i].szLineBuffer + m_pSayTextList[i].iNameLength);
			}
			else// normal draw
				DrawConsoleString(LINE_START, y, m_pSayTextList[i].szLineBuffer);
		}
		y += line_height;// this should not change because of empty lines
	}
	m_iHeight = y+line_height;// XDM3038a: constant
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3035: now we don't send sender name explicitly in the text, just the index (which is 0 for announcements)
// Input  : *pszBuf - 
//			clientIndex -
//			teamonly -
// Output : int
//-----------------------------------------------------------------------------
void CHudSayText::SayTextPrint(const char *pszBuf, byte clientIndex, bool teamonly)
{
	const char *pSayTest = (pszBuf[0] == 2)?pszBuf+1:pszBuf;// XDM: HL HACK! TODO: move outside to msg procedure?

	if (gViewPort && gViewPort->AllowedToPrintText() == FALSE)// Not allowed to print it on the screen, redirect to the console
	{
		if (IsValidPlayerIndex(clientIndex))
			CON_PRINTF("%s: %s%s\n", g_PlayerInfoList[clientIndex].name, pSayTest, (teamonly?m_szLocalizedTeam:""));
		else
			ConsolePrint(pSayTest);

		return;
	}

	size_t i = 0;
	for (i = 0; i < m_iMaxLines; ++i)
	{
		if (m_pSayTextList[i].szLineBuffer[0] == 0)// empty string slot
			break;
	}
	if (i == m_iMaxLines)// MAX_LINES
	{
		// force scroll buffer up
		i = m_iMaxLines-ScrollTextUp();
	}

	memset(&m_pSayTextList[i], 0, sizeof(SayTextItem));// XDM

	int localindex = gEngfuncs.GetLocalPlayer()->index;
	ASSERT(localindex >= 1 && localindex <= MAX_PLAYERS);

	bool local_highlighted = false;// XDM3035
	const char *pSenderName = "";

	// if it's a say message, search for the players name in the string
	// XDM3038a: ENOUGH OF THIS SHIT! if (*pszBuf == 2 && IsValidPlayerIndex(clientIndex))
	if (IsValidPlayerIndex(clientIndex))
	{
		pSayTest = pszBuf;// XDM3038a: ENOUGH OF THIS SHIT!pszBuf+1;
		GetPlayerInfo(clientIndex, &g_PlayerInfoList[clientIndex]);
		pSenderName = g_PlayerInfoList[clientIndex].name;
		if (pSenderName)
		{
			m_pSayTextList[i].iNameLength = strlen(pSenderName);// + (nameInString - pszBuf);

			if (gHUD.m_pCvarUsePlayerColor->value > 0.0f)
				m_pSayTextList[i].NameColor = GetPlayerColor(clientIndex);//GetPlayerColor(clientIndex, m_pSayTextList[i].NameColor[0], m_pSayTextList[i].NameColor[1], m_pSayTextList[i].NameColor[2]);
			else
				m_pSayTextList[i].NameColor = GetTeamColor(g_PlayerExtraInfo[clientIndex].teamnumber);//GetTeamColor(g_PlayerExtraInfo[clientIndex].teamnumber, m_pSayTextList[i].NameColor[0], m_pSayTextList[i].NameColor[1], m_pSayTextList[i].NameColor[2]);// XDM

			if (IsTeamGame(gHUD.m_iGameType) || clientIndex == localindex)// XDM3035: highlight the whole string if sent by teammate
			{
				if (g_PlayerExtraInfo[localindex].teamnumber != TEAM_NONE && g_PlayerExtraInfo[clientIndex].teamnumber == g_PlayerExtraInfo[localindex].teamnumber)// XDM3037a: TEAM_NONE check
					m_pSayTextList[i].StringColor = m_pSayTextList[i].NameColor;
			}
		}

		// XDM3035: chat highlights
		if (m_pCvarSayTextHighlight->value > 0.0f)// highlight if others talking about me ot this is my message
		{
			const char *pLocalName = g_PlayerInfoList[localindex].name;// get local player name
			if (pLocalName)
			{
				if (clientIndex == localindex || strstr(pSayTest, pLocalName))// UNDONE: invent strIstr()
				{
					local_highlighted = true;
					UnpackRGB(m_pSayTextList[i].StringColor, RGB_YELLOW);
				}
			}
		}
	}
	else
		pSayTest = pszBuf;

	if (IsValidPlayerIndex(clientIndex))
		_snprintf(m_pSayTextList[i].szLineBuffer, MAX_CHARS_PER_LINE, "%s: %s%s\n\0", pSenderName, pSayTest, (teamonly?m_szLocalizedTeam:""));
	else
		strncpy(m_pSayTextList[i].szLineBuffer, pSayTest, max(strlen(pszBuf), MAX_CHARS_PER_LINE));// HL20130901

	EnsureTextFitsInOneLineAndWrapIfHaveTo(i);

	if (i == 0)
		m_flScrollTime = gHUD.m_flTime + m_pCvarSayTextTime->value;

	SetActive(true);

	// XDM3035: don't interrupt the announcer
	if (g_pCvarAnnouncer->value <= 0.0f || gHUD.m_flNextAnnounceTime < gHUD.m_flTime)
	{
		if (local_highlighted && clientIndex != localindex)// don't react on my own message
			PlaySound("misc/talk_hl.wav", 1.0f);
		else
			PlaySound("misc/talk.wav", 1.0f);
	}

	//Y_START = (int)( (int)((float)ScreenHeight*(7/8)) - (line_height * m_iMaxLines));// 0.875 == (1 - 1/8)
	//Y_START = ScreenHeight * (7/8) - (line_height * m_iMaxLines);// 0.875 == (1 - 1/8)
	Y_START = (int)((float)ScreenHeight * 0.865f) - (line_height * m_iMaxLines);//(7/8);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : line - 
//-----------------------------------------------------------------------------
void CHudSayText::EnsureTextFitsInOneLineAndWrapIfHaveTo(int line)
{
	int line_width = 0;
	DrawConsoleStringLen(m_pSayTextList[line].szLineBuffer, &line_width, &line_height);

	if ((line_width + LINE_START) > MAX_LINE_WIDTH)// string is too long to fit on line
	{
		// scan the string until we find what word is too long, and wrap the end of the sentence after the word
		int length = LINE_START;
		int tmp_len = 0;
		char *last_break = NULL;
		for (char *x = m_pSayTextList[line].szLineBuffer; *x != 0; ++x)
		{
			// check for a color change, if so skip past it
			if (x[0] == '/' && x[1] == '(')
			{
				x += 2;
				// skip forward until past mode specifier
				while (*x != 0 && *x != ')')
					x++;

				if (*x != 0)
					x++;

				if (*x == 0)
					break;
			}

			char buf[2];
			buf[1] = 0;

			if (*x == ' ' && x != m_pSayTextList[line].szLineBuffer)// isspace(*x)? // store each line break, except for the very first character
				last_break = x;

			buf[0] = *x;  // get the length of the current character
			DrawConsoleStringLen(buf, &tmp_len, &line_height);
			length += tmp_len;

			if (length > MAX_LINE_WIDTH)// needs to be broken up
			{
				if (!last_break)
					last_break = x-1;

				x = last_break;

				// find an empty string slot
				size_t j;
				do 
				{
					for (j = 0; j < m_iMaxLines; ++j)// MAX_LINES
					{
						if (m_pSayTextList[j].szLineBuffer[0] == 0)
							break;
					}
					if (j == m_iMaxLines)// MAX_LINES
					{
						// need to make more room to display text, scroll stuff up then fix the pointers
						int linesmoved = ScrollTextUp();
						line -= linesmoved;
						last_break = last_break - (MAX_CHARS_PER_LINE * linesmoved);
					}
				}
				while (j == m_iMaxLines);// MAX_LINES

				// copy remaining string into next buffer, making sure it starts with a space character
				if ((char)*last_break == (char)' ')// if (isspace(*last_break))
				{
					size_t linelen = strlen(m_pSayTextList[j].szLineBuffer);
					size_t remaininglen = strlen(last_break);
					if ((linelen - remaininglen) <= MAX_CHARS_PER_LINE)
						strcat(m_pSayTextList[j].szLineBuffer, last_break);
				}
				else
				{
					if ((strlen(m_pSayTextList[j].szLineBuffer) - strlen(last_break) - 2) < MAX_CHARS_PER_LINE)
					{
						strcat(m_pSayTextList[j].szLineBuffer, " ");
						strcat(m_pSayTextList[j].szLineBuffer, last_break);
					}
				}
				*last_break = '\0';// cut off the last string
				EnsureTextFitsInOneLineAndWrapIfHaveTo(j);
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Incoming saytext from server
// WARNING: HL engine uses this message directly without registering!
// Input  : 
// Output : int
//-----------------------------------------------------------------------------
int CHudSayText::MsgFunc_SayText(const char *pszName, int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHudSayText::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	byte client_index = READ_BYTE();		// the client who spoke the message
	bool teamonly = false;
	if (client_index & SAYTEXT_BIT_TEAMONLY)
	{
		teamonly = true;
		client_index &= ~SAYTEXT_BIT_TEAMONLY;
	}
	char *pBuf = READ_STRING();
	END_READ();
	SayTextPrint(pBuf, client_index, teamonly);
	//SayTextPrint(BufferedLocaliseTextString(pBuf), client_index, teamonly);// XDM3035c: NO!
	//TEST CL_ScreenFade(255,20,0, test1->value, test2->value, test3->value, 0);// 255, 1, 0.1
	return 1;
}
