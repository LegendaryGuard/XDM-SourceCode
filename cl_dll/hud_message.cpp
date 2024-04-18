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
// Message.cpp
//
// implementation of CHudMessage class
//
// old as mammoth's shit, needs to be rewritten
// NOTE: removed a few crashes, rewritten > 50%, but still not as good as it should
// BUGBUG: title text is somehow affecting custom-drawn "console" text color
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_Viewport.h"// XDM3038


DECLARE_MESSAGE(m_Message, HudText)
DECLARE_MESSAGE(m_Message, GameTitle)
DECLARE_MESSAGE(m_Message, TextMsg);

// 1 Global client_textmessage_t for custom messages that aren't in the titles.txt
client_textmessage_t g_pCustomMessage;
const char *g_pCustomName = "Custom";
char g_pCustomText[1024];

int CHudMessage::Init(void)
{
	HOOK_MESSAGE(HudText);
	HOOK_MESSAGE(GameTitle);
	HOOK_MESSAGE(TextMsg);
	gHUD.AddHudElem(this);
	Reset();
	return 1;
}

int CHudMessage::VidInit(void)
{
	m_HUD_title_half = gHUD.GetSpriteIndex("title_half");
	m_HUD_title_life = gHUD.GetSpriteIndex("title_life");
	return 1;
}

void CHudMessage::Reset(void)
{
 	memset(m_pMessages, 0, sizeof(m_pMessages[0]) * maxHUDMessages);
	memset(m_startTime, 0, sizeof(m_startTime[0]) * maxHUDMessages);
	m_iFlags = HUD_INTERMISSION | HUD_DRAW_ALWAYS;// XDM3035
	m_gameTitleTime = 0;
	m_pGameTitle = NULL;
}

float CHudMessage::FadeBlend(const float &fadein, const float &fadeout, const float &hold, const float &localTime)
{
	if (localTime < 0)
		return 0;

	float fadeBlend;
	float fadeTime = fadein + hold;
	if (localTime < fadein)
	{
		fadeBlend = 1 - ((fadein - localTime) / fadein);
	}
	else if (localTime > fadeTime)
	{
		if (fadeout > 0)
			fadeBlend = 1 - ((localTime - fadeTime) / fadeout);
		else
			fadeBlend = 0;
	}
	else
		fadeBlend = 1;

	return fadeBlend;
}

int	CHudMessage::XPosition(const float &x, int width, int totalWidth)
{
	int xPos;
	if (x == -1)
		xPos = (ScreenWidth - width)/2;
	else
	{
		if (x < 0)
			xPos = (1.0f + x) * ScreenWidth - totalWidth;// align to the right
		else
			xPos = x * ScreenWidth;
	}

	if (xPos + width > ScreenWidth)
		xPos = ScreenWidth - width;
	else if (xPos < 0)
		xPos = 0;

	return xPos;
}

int CHudMessage::YPosition(const float &y, int height)
{
	int yPos;
	if (y == -1)// centered
		yPos = (ScreenHeight - height)/2;
	else
	{
		if (y < 0)// align to the bottom
			yPos = (1.0f + y) * ScreenHeight - height;	// Alight bottom
		else // align to the top
			yPos = y * ScreenHeight;
	}

	if (yPos + height > ScreenHeight)
		yPos = ScreenHeight - height;
	else if (yPos < 0)
		yPos = 0;

	return yPos;
}

//-----------------------------------------------------------------------------
// Purpose: Updates m_parms for current character, called by MessageDrawScan
//-----------------------------------------------------------------------------
void CHudMessage::MessageScanNextChar(void)
{
	int srcRed, srcGreen, srcBlue, destRed = 0, destGreen = 0, destBlue = 0;
	int blend;// UNDONE: to float 0...1

	srcRed = m_parms.pMessage->r1;
	srcGreen = m_parms.pMessage->g1;
	srcBlue = m_parms.pMessage->b1;
	blend = 0;// Pure source

	switch (m_parms.pMessage->effect)
	{
	case TEXTMSG_FX_FADE:// Fade-in / Fade-out
		destRed = destGreen = destBlue = 0;
		blend = m_parms.fadeBlend;
		break;
	case TEXTMSG_FX_FLICKER:// flickery
		destRed = m_parms.pMessage->r2;// XDM
		destGreen = m_parms.pMessage->g2;
		destBlue = m_parms.pMessage->b2;
		blend = m_parms.fadeBlend;
		break;
	case TEXTMSG_FX_WRITEOUT:// write out
		m_parms.charTime += m_parms.pMessage->fadein;
		if (m_parms.charTime > m_parms.time)
		{
			srcRed = srcGreen = srcBlue = 0;
			blend = 0;// pure source
		}
		else
		{
			float deltaTime = m_parms.time - m_parms.charTime;
			destRed = destGreen = destBlue = 0;
			if (m_parms.time > m_parms.fadeTime)
				blend = m_parms.fadeBlend;
			else if (deltaTime > m_parms.pMessage->fxtime)
				blend = 0;// pure dest
			else
			{
				destRed = m_parms.pMessage->r2;
				destGreen = m_parms.pMessage->g2;
				destBlue = m_parms.pMessage->b2;
				blend = 255 - (deltaTime * (1.0f/m_parms.pMessage->fxtime) * 255.0f + 0.5f);
			}
		}
		break;
	}
	if (blend > 255)
		blend = 255;
	else if (blend < 0)
		blend = 0;

	m_parms.r = ((srcRed * (255-blend)) + (destRed * blend)) >> 8;
	m_parms.g = ((srcGreen * (255-blend)) + (destGreen * blend)) >> 8;
	m_parms.b = ((srcBlue * (255-blend)) + (destBlue * blend)) >> 8;

	if (m_parms.pMessage->effect == TEXTMSG_FX_FLICKER && m_parms.charTime != 0)
	{
		if (m_parms.x >= 0 && m_parms.y >= 0 && (m_parms.x + gHUD.m_scrinfo.charWidths[m_parms.text]) <= ScreenWidth)
			TextMessageDrawChar(m_parms.x, m_parms.y, m_parms.text, m_parms.pMessage->r2, m_parms.pMessage->g2, m_parms.pMessage->b2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: called by MessageDrawScan
//-----------------------------------------------------------------------------
void CHudMessage::MessageScanStart(void)
{
	switch (m_parms.pMessage->effect)
	{
	case TEXTMSG_FX_FADE:// Fade-in / out with flicker
	case TEXTMSG_FX_FLICKER:
		{
			m_parms.fadeTime = m_parms.pMessage->fadein + m_parms.pMessage->holdtime;

			if (m_parms.time < m_parms.pMessage->fadein)
			{
				m_parms.fadeBlend = ((m_parms.pMessage->fadein - m_parms.time) * (1.0f/m_parms.pMessage->fadein) * 255);
			}
			else if (m_parms.time > m_parms.fadeTime)
			{
				if (m_parms.pMessage->fadeout > 0)
					m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
				else
					m_parms.fadeBlend = 255;// Pure dest (off)
			}
			else
				m_parms.fadeBlend = 0;// Pure source (on)

			m_parms.charTime = 0;

			if (m_parms.pMessage->effect == TEXTMSG_FX_FLICKER && (RANDOM_LONG(0,9) == 0))//(rand()%100) < 10)
				m_parms.charTime = 1;
		}
		break;
	case TEXTMSG_FX_WRITEOUT:
		{
			m_parms.fadeTime = (m_parms.pMessage->fadein * m_parms.length) + m_parms.pMessage->holdtime;
			
			if (m_parms.time > m_parms.fadeTime && m_parms.pMessage->fadeout > 0)
				m_parms.fadeBlend = (int)(((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255.0f);
			else
				m_parms.fadeBlend = 0;
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculates line positions and draws characters
// Input  : *pMessage - 
//			time - 
//-----------------------------------------------------------------------------
void CHudMessage::MessageDrawScan(client_textmessage_t *pMessage, const float &time)
{
	//uint32 i, j;
	//size_t linelength, length, lastspacepos;
	//int width;
	const char *pText;
	//const size_t linelengthmax = 128;// HACK
	size_t linelengthmax = gHUD.m_scrinfo.iWidth/gHUD.m_scrinfo.charWidths['W'];// XDM3038: the widest letter must fit
	size_t maxlines = gHUD.m_scrinfo.iHeight/gHUD.m_scrinfo.iCharHeight;
	ASSERT(maxlines > 0);// && maxlines < MAX_HEAP_SIZE)
	int *szLineWidths = new int[maxlines];
	if (!szLineWidths)
		return;

	pText = pMessage->pMessage;
	// Count lines
	m_parms.time = time;
	m_parms.pMessage = pMessage;
	m_parms.totalWidth = 0;// of the whole text block
	//m_parms.totalHeight = 0;
	m_parms.width = 0;// current line width
	m_parms.lines = 0;
	m_parms.lineLength = 0;// number of symbols
	m_parms.length = 0;// total number of symbols in message
	// XDM3038: Calculate lines. No temp buffers.
	while (*pText)
	{
		if (*pText == '\n'|| m_parms.lineLength >= linelengthmax)// XDM3038: buffer overflow prevention
		{
			m_parms.lineLength = 0;
			if (m_parms.lines >= maxlines)
				break;// can't fit anyway. Don't corrupt the memory!

			szLineWidths[m_parms.lines] = m_parms.width;// store this line's length
			if (m_parms.width > m_parms.totalWidth)
				m_parms.totalWidth = m_parms.width;// remember the widest line to be able to position the text block

			m_parms.width = 0;
			m_parms.lines++;
		}
		else
			m_parms.width += gHUD.m_scrinfo.charWidths[(unsigned char)(*pText)];// line width in pixels

		pText++;
		m_parms.lineLength++;// calculate this to prevent overflow
		m_parms.length++;
	}
	szLineWidths[m_parms.lines] = m_parms.width;// last line
	m_parms.lines += 1;// since we skipped the 1st line while using this variable as an array index
	//ASSERT(m_parms.length <= linelengthmax*maxlines);
	// Draw
	pText = pMessage->pMessage;
	//m_parms.totalWidth is ok
	m_parms.totalHeight = m_parms.lines * gHUD.m_scrinfo.iCharHeight;
	m_parms.x = -1;// reset: dynamic per line
	m_parms.y = YPosition(pMessage->y, m_parms.totalHeight);
	//m_parms.width = 0;
	m_parms.lines = 0;// restart counting to prevent index from getting out of array bounds
	m_parms.lineLength = 0;// start line
	m_parms.charTime = 0;
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		MessageScanStart();
		while (*pText)
		{
			if (*pText == '\n'|| m_parms.lineLength >= linelengthmax)// MUST match previous loop criteria
			{
				m_parms.lineLength = 0;
				if (m_parms.lines >= maxlines)
					break;// can't fit anyway. Don't corrupt the memory!

				m_parms.x = -1;// reset
				m_parms.y += gHUD.m_scrinfo.iCharHeight;
				//m_parms.width = 0;
				m_parms.lines++;// array index!
			}
			else// not a newline
			{
				m_parms.text = *pText;
				if (m_parms.x == -1)
					m_parms.x = XPosition(pMessage->x, szLineWidths[m_parms.lines], m_parms.totalWidth);// line start

				MessageScanNextChar();// update colors

				if (m_parms.x >= 0 && m_parms.y >= 0)
					TextMessageDrawChar(m_parms.x, m_parms.y, m_parms.text, m_parms.r, m_parms.g, m_parms.b);// 'text' is actually char

				m_parms.x += gHUD.m_scrinfo.charWidths[(uint8)*pText];
				//m_parms.width += gHUD.m_scrinfo.charWidths[*pText];// line width in pixels
			}

			pText++;
			m_parms.lineLength++;// calculate this to prevent overflow
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)// prevent memory leaking at all costs
	{
		delete [] szLineWidths;
		szLineWidths = NULL;
		conprintf(1, "CHudMessage::MessageDrawScan(%s) exception!\n", pMessage->pName);
	}
#endif
	if (szLineWidths)
	{
		delete [] szLineWidths;
		szLineWidths = NULL;
	}
	/*m_parms.lines = 1;
	length = 0;
	width = 0;
	linelength = 0;
	while (*pText)
	{
		if (*pText == '\n'|| linelength >= linelengthmax)// XDM3038: buffer overflow prevention
		{
			linelength = 0;
			m_parms.lines++;
			if (width > m_parms.totalWidth)
				m_parms.totalWidth = width;

			width = 0;
		}
		else
			width += gHUD.m_scrinfo.charWidths[*pText];

		pText++;
		length++;
		linelength++;
	}
	m_parms.length = length;
	m_parms.totalHeight = m_parms.lines * gHUD.m_scrinfo.iCharHeight;
	//m_parms.x is dynamic per line
	m_parms.y = YPosition(pMessage->y, m_parms.totalHeight);
	m_parms.charTime = 0;
	pText = pMessage->pMessage;

	MessageScanStart();

	char line[linelengthmax];// UNDONE: TODO: FIXME!!! buffer overrun!
	int nextX;
	for (i = 0; i < m_parms.lines; ++i)
	{
		m_parms.lineLength = 0;
		m_parms.width = 0;
		while (*pText && *pText != '\n' && m_parms.lineLength < linelengthmax)// XDM3038: HACK buffer overflow temporary fix! TODO: don't copy any strings!!
		{
			line[m_parms.lineLength] = *pText;
			m_parms.width += gHUD.m_scrinfo.charWidths[*pText];
			m_parms.lineLength++;
			pText++;
		}
		//if (*pText == '\n')
		pText++;		// Skip LF
		line[m_parms.lineLength] = 0;

		m_parms.x = XPosition(pMessage->x, m_parms.width, m_parms.totalWidth);// line start

		for (j = 0; j < m_parms.lineLength; ++j)
		{
			m_parms.text = line[j];
			nextX = m_parms.x + gHUD.m_scrinfo.charWidths[m_parms.text];
			MessageScanNextChar();// update colors

			if (m_parms.x >= 0 && m_parms.y >= 0 && nextX <= ScreenWidth)
				TextMessageDrawChar(m_parms.x, m_parms.y, m_parms.text, m_parms.r, m_parms.g, m_parms.b);// 'text' is actually char

			m_parms.x = nextX;
		}
		m_parms.y += gHUD.m_scrinfo.iCharHeight;
	}*/
}

int CHudMessage::Draw(const float &flTime)
{
	float endTime;
	int i = 0, drawn = 0;

	if (m_gameTitleTime > 0)
	{
		float brightness;
		float localTime = gHUD.m_flTime - m_gameTitleTime;
		// Maybe timer isn't set yet
		if (m_gameTitleTime > gHUD.m_flTime)
			m_gameTitleTime = gHUD.m_flTime;

		if (localTime > (m_pGameTitle->fadein + m_pGameTitle->holdtime + m_pGameTitle->fadeout))
			m_gameTitleTime = 0;
		else
		{
			brightness = FadeBlend(m_pGameTitle->fadein, m_pGameTitle->fadeout, m_pGameTitle->holdtime, localTime);
			int halfWidth = RectWidth(gHUD.GetSpriteRect(m_HUD_title_half));
			int fullWidth = halfWidth + RectWidth(gHUD.GetSpriteRect(m_HUD_title_life));
			int fullHeight = RectHeight(gHUD.GetSpriteRect(m_HUD_title_half));
			int x = XPosition(m_pGameTitle->x, fullWidth, fullWidth);
			int y = YPosition(m_pGameTitle->y, fullHeight);
			byte r,g,b;// pfnSPR_Set uses int
			r = m_pGameTitle->r1;
			g = m_pGameTitle->g1;
			b = m_pGameTitle->b1;
			ScaleColorsF(r,g,b, brightness);
			SPR_Set(gHUD.GetSprite(m_HUD_title_half), r,g,b);
			SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_title_half));
			SPR_Set(gHUD.GetSprite(m_HUD_title_life), r,g,b);
			SPR_DrawAdditive(0, x + halfWidth, y, &gHUD.GetSpriteRect(m_HUD_title_life));
			drawn = 1;
		}
	}

	client_textmessage_t *pMessage;
	// Fixup level transitions
	for (i = 0; i < maxHUDMessages; ++i)
	{
		if (m_pMessages[i])// Assume m_parms.time contains last time
		{
			pMessage = m_pMessages[i];
			if (m_startTime[i] > gHUD.m_flTime)
				m_startTime[i] = gHUD.m_flTime + m_parms.time - m_startTime[i] + 0.2f;	// Server takes 0.2 seconds to spawn, adjust for this
		}
	}

	for (i = 0; i < maxHUDMessages; ++i)
	{
		if (m_pMessages[i])
		{
			pMessage = m_pMessages[i];
			//conprintf(1, "CHudMessage::Draw(%d '%s' '%s')\n", i, pMessage->pName, pMessage->pMessage);

			// This is when the message is over
			switch (pMessage->effect)
			{
			default:
			//case TEXTMSG_FX_FADE:
			//case TEXTMSG_FX_FLICKER:
					endTime = m_startTime[i] + pMessage->fadein + pMessage->fadeout + pMessage->holdtime;
				break;
			case TEXTMSG_FX_WRITEOUT:// Fade in is per character in scanning messages
					endTime = m_startTime[i] + (pMessage->fadein * strlen( pMessage->pMessage )) + pMessage->fadeout + pMessage->holdtime;
				break;
			}

			if (flTime <= endTime)
			{
				float messageTime = flTime - m_startTime[i];
				MessageDrawScan(pMessage, messageTime);
				++drawn;
			}
			else// The message is over
				m_pMessages[i] = NULL;
		}
	}

	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;
	// Don't call until we get another message
	if (!drawn)
		SetActive(false);

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - in titles.txt
//			fStartTime - time when this message should appear (start)
//-----------------------------------------------------------------------------
void CHudMessage::MessageAdd(const char *pName, float fStartTime)
{
	int i,j;
	client_textmessage_t *tempMessage;
	for (i = 0; i < maxHUDMessages; ++i)
	{
		if (m_pMessages[i] == NULL)
		{
			// Trim off a leading # if it's there
			if (pName[0] == '#') 
				tempMessage = TextMessageGet(pName+1);
			else
				tempMessage = TextMessageGet(pName);
			// If we couldnt find it in the titles.txt, just create it
			if (tempMessage == NULL)
			{
				g_pCustomMessage.effect = TEXTMSG_FX_WRITEOUT;
				UnpackRGB(g_pCustomMessage.r1, g_pCustomMessage.g1, g_pCustomMessage.b1, RGB_GREEN);// XDM
				UnpackRGB(g_pCustomMessage.r2, g_pCustomMessage.g2, g_pCustomMessage.b2, RGB_BLUE);// XDM
				g_pCustomMessage.a1 = 255;
				g_pCustomMessage.a2 = 0;
				g_pCustomMessage.x = -1.0f;// Centered
				g_pCustomMessage.y = 0.7f;
				g_pCustomMessage.fadein = 0.01f;
				g_pCustomMessage.fadeout = 1.5f;
				g_pCustomMessage.fxtime = 0.25f;
				g_pCustomMessage.holdtime = gHUD.m_SayText.m_pCvarSayTextTime->value;
				g_pCustomMessage.pName = g_pCustomName;
				strcpy(g_pCustomText, pName);
				g_pCustomMessage.pMessage = g_pCustomText;
				tempMessage = &g_pCustomMessage;
			}
			else
			{
				if (tempMessage->effect > TEXTMSG_FX_WRITEOUT)// XDM3035c: this check shoud be moved to TextMessageGet()...
				{
					conprintf(1, "Text message %s has invalid effect %d\n", pName, tempMessage->effect);
					tempMessage->effect = TEXTMSG_FX_FADE;
				}
			}
			SetActive(true);// XDM3037

			for (j = 0; j < maxHUDMessages; ++j)
			{
				if (m_pMessages[j])
				{
					// is this message already in the list
					if (!strcmp(tempMessage->pMessage, m_pMessages[j]->pMessage))
						return;

					// get rid of any other messages in same location (only one displays at a time)
					if (fabs(tempMessage->y - m_pMessages[j]->y) < 0.0001)
					{
						if (fabs(tempMessage->x - m_pMessages[j]->x) < 0.0001)
						{
							m_pMessages[j] = NULL;
							m_startTime[i] = 0.0f;// XDM3035
						}
					}
				}
			}
			m_pMessages[i] = tempMessage;
			m_startTime[i] = fStartTime;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newMessage - 
//-----------------------------------------------------------------------------
void CHudMessage::MessageAdd(client_textmessage_t *newMessage)
{
	//conprintf(1, "CHudMessage::MessageAdd(\"%s\", \"%s\")\n", newMessage->pName, newMessage->pMessage);
	m_parms.time = gHUD.m_flTime;

	int free_index = maxHUDMessages;// none
	for (int i = 0; i < maxHUDMessages; ++i)
	{
		if (m_pMessages[i] == newMessage)// XDM3035: this massage already exists!
		{
			m_startTime[i] = gHUD.m_flTime;// update start time as it was a newly added message
			//m_pMessages[i]->holdtime += newMessage->holdtime;
			//m_pMessages[i]->fxtime += newMessage->fxtime;
			return;
		}

		if (m_pMessages[i] == NULL && free_index == maxHUDMessages)
			free_index = i;// must pass ALL indexes for previous check
	}

	if (free_index < maxHUDMessages)
	{
		m_pMessages[free_index] = newMessage;
		m_startTime[free_index] = gHUD.m_flTime;

		// Turn on drawing
		SetActive(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for engine compatibility ("GAMESAVED" text is sent here)
// Warning: iSize is 0 for strings!
//-----------------------------------------------------------------------------
int CHudMessage::MsgFunc_HudText(const char *pszName,  int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHudMessage::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	const char *pString = READ_STRING();
	END_READ();

	MessageAdd(pString, gHUD.m_flTime);
	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	SetActive(true);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for game title image
// NOTE: spites are taken from title_half/life section of hud.txt
// NOTE: drawing parameters are taken from GAMETITLE section of title.txt
//-----------------------------------------------------------------------------
int CHudMessage::MsgFunc_GameTitle(const char *pszName,  int iSize, void *pbuf)
{
	DBG_HUD_PRINTF("CHudMessage::MsgFunc_%s(%d)\n", pszName, iSize);
	//BEGIN_READ(pbuf, iSize);
	//END_READ();
	m_pGameTitle = TextMessageGet("GAMETITLE");
	if (m_pGameTitle)
	{
		m_gameTitleTime = gHUD.m_flTime;
		// Turn on drawing
		SetActive(true);
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for text messages
// displays a string, looking them up from the titles.txt file, which can be localised
// parameters:
//   byte:   message direction  ( HUD_PRINTCONSOLE, HUD_PRINTNOTIFY, HUD_PRINTCENTER, HUD_PRINTTALK )
//   string: message
// optional parameters:
//   string: message parameter 1
//   string: message parameter 2
//   string: message parameter 3
//   string: message parameter 4
// any string that starts with the character '#' is a message name, and is used to look up the real message in titles.txt
// the next (optional) one to four strings are parameters for that string (which can also be message names if they begin with '#')
// Output : int
//-----------------------------------------------------------------------------
int CHudMessage::MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf)
{
	static char szBuf[5][MSG_STRING_BUFFER];
	DBG_HUD_PRINTF("CHudMessage::MsgFunc_%s(%d)\n", pszName, iSize);
	BEGIN_READ(pbuf, iSize);
	byte bdata = READ_BYTE();// BitSplit2x4bit()
	int msg_dest = (int)(bdata & HUD_PRINTDEST_MASK);
	int msg_level = (int)((bdata & HUD_PRINTLEVEL_MASK) >> 4);

	if (g_pCvarDeveloper && g_pCvarDeveloper->value < msg_level)// filter out messages that require higher level of verbosity
	{
		END_READ();
		return 1;
	}

	char *msg_text = LocaliseTextString(READ_STRING(), szBuf[0], MSG_STRING_BUFFER);// XDM3035c
	// UNDONE: TESTME: if we want the msg_dest functionality, test and use this: char *msg_text = LookupString(READ_STRING(), &msg_dest);
	//msg_text = safe_strcpy(szBuf[0], msg_text);

	// keep reading strings and using C format strings for subsituting the strings into the localised text string
	char *sstr1 = LookupString(READ_STRING());
	sstr1 = safe_strcpy(szBuf[1], sstr1, MSG_STRING_BUFFER);

	char *sstr2 = LookupString(READ_STRING());
	sstr2 = safe_strcpy(szBuf[2], sstr2, MSG_STRING_BUFFER);

	char *sstr3 = LookupString(READ_STRING());
	sstr3 = safe_strcpy(szBuf[3], sstr3, MSG_STRING_BUFFER);

	char *sstr4 = LookupString(READ_STRING());
	sstr4 = safe_strcpy(szBuf[4], sstr4, MSG_STRING_BUFFER);

	END_READ();

	if (gViewPort && gViewPort->AllowedToPrintText() == FALSE)
		return 1;

	StripEndNewlineFromString(sstr1);  // these strings are meant for subsitution into the main strings, so cull the automatic end newlines
	StripEndNewlineFromString(sstr2);
	StripEndNewlineFromString(sstr3);
	StripEndNewlineFromString(sstr4);

	char psz[MSG_STRING_BUFFER*4];

	if (msg_dest == HUD_PRINTCENTER)
	{
		safe_sprintf(psz, MSG_STRING_BUFFER, msg_text, sstr1, sstr2, sstr3, sstr4);
		CenterPrint(BufferedLocaliseTextString(ConvertCRtoNL(psz)));// XDM3035: added localization
	}
	else if (msg_dest == HUD_PRINTNOTIFY)
	{
		/*psz[0] = 1;// mark this message to go into the notify buffer
		safe_sprintf(psz+1, MSG_STRING_BUFFER, msg_text, sstr1, sstr2, sstr3, sstr4);
		ConsolePrint(ConvertCRtoNL(psz));*/
		safe_sprintf(psz, MSG_STRING_BUFFER, msg_text, sstr1, sstr2, sstr3, sstr4);
		CON_NPRINTF(0,BufferedLocaliseTextString(ConvertCRtoNL(psz)));// XDM3038c: use notification print function directly
	}
	else if (msg_dest == HUD_PRINTTALK)
	{
		safe_sprintf(psz, MSG_STRING_BUFFER, /*BufferedLocaliseTextString(*/ConvertCRtoNL(msg_text), sstr1, sstr2, sstr3, sstr4);
		gHUD.m_SayText.SayTextPrint(psz, CLIENT_INDEX_INVALID, false);
	}
	else if (msg_dest == HUD_PRINTHUD)
	{
		//if (sstr1 && *sstr1) XDM3038c: process the string fully to get rid of all %s
		{
			safe_sprintf(psz, MSG_STRING_BUFFER, msg_text, sstr1, sstr2, sstr3, sstr4);
			gHUD.m_Message.MessageAdd(BufferedLocaliseTextString(ConvertCRtoNL(psz)), gHUD.m_flTime);// XDM3035: added localization
		}
		//else// a simple message
		//	gHUD.m_Message.MessageAdd(msg_text, gHUD.m_flTime);// XDM3037: special case?
	}
	else// if (msg_dest == HUD_PRINTCONSOLE)
	{
		safe_sprintf(psz, MSG_STRING_BUFFER, msg_text, sstr1, sstr2, sstr3, sstr4);
		ConsolePrint(ConvertCRtoNL(psz));
	}
	return 1;
}
