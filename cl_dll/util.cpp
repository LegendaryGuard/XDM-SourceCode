#include "hud.h"
#include <stdio.h>
#include <assert.h>
#include "cl_util.h"
#include "com_model.h"
#include "vgui_Viewport.h"
#include "pm_defs.h"
#include "weapondef.h"
#include "event_api.h"
#include "screenfade.h"
#include "shake.h"
#include "demo.h"
#include "demo_api.h"
/*#if _MSC_VER > 1200
#include <crtdbg.h>
#endif*/

wrect_t nullrc = {0,0,0,0};

// WARNING! we count from 1 because entindex 0 is the world, so MAX_PLAYERS (32) is a valid index!
hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS+1];// player info from the engine
extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS+1];// additional player info sent directly to the client dll
//byte				g_IsSpectator[MAX_PLAYERS+1];// separate array because it is filled externally at unpredictable time
team_info_t			g_TeamInfo[MAX_TEAMS+1];// by team index, including TEAM_NONE
long				g_PlayerStats[MAX_PLAYERS+1][STAT_NUMELEMENTS];// by player index, 0 is unused
short				g_iNumberOfTeams = 0;// XDM3037: byte addressing is still bad on x86 systems, so use 2
float				g_lastFOV = 0.0f;
//extern				cvar_t *sensitivity;

// IMPORNANT: gamedefs.h
gametype_prefix_t gGameTypePrefixes[] =
{
	{"DM",		GT_DEATHMATCH},
	{"CO",		GT_COOP},
	{"CTF",		GT_CTF},
	{"DOM",		GT_DOMINATION},
	{"LMS",		GT_LMS},
	{"TM",		GT_TEAMPLAY},
	{"RM",		GT_ROUND},
	{"AS",		GT_ROUND},// assault
	{"OP4CTF",	GT_CTF},
	{NULL,		0}// IMPORTANT! Must end with a null-terminator.
};

//-----------------------------------------------------------------------------
// Purpose: Uses suit volume AND ONLY if player has suit on
// UNDONE : Message queue
// Input  : *szSound - 
//-----------------------------------------------------------------------------
void PlaySoundSuit(char *szSound)
{
	if (gHUD.PlayerHasSuit())// XDM3038
		PlaySound(szSound, (g_pCvarSuitVolume?g_pCvarSuitVolume->value:(float)VOL_NORM));
}

//-----------------------------------------------------------------------------
// Purpose: Special wrapper with common parameters
// Input  : *szSound - 
//-----------------------------------------------------------------------------
void PlaySoundAnnouncer(char *szSound, float duration)
{
	if (g_pCvarAnnouncer->value > 0.0f)
	{
		PlaySound(szSound, g_pCvarAnnouncer->value);

#if defined(CLDLL_NEWFUNCTIONS)
		gHUD.m_flNextAnnounceTime = gHUD.m_flTime + gEngfuncs.pfnGetApproxWavePlayLen(szSound);
#else
		gHUD.m_flNextAnnounceTime = gHUD.m_flTime + duration;
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get actual FOV value
// Output : float - FOV
//-----------------------------------------------------------------------------
float HUD_GetFOV(void)
{
	if (gEngfuncs.pDemoAPI->IsRecording())
	{
		// Write it
		int i = 0;
		unsigned char buf[8];
		// Active
		*(float *)&buf[i] = g_lastFOV;
		i += sizeof(float);
		Demo_WriteBuffer(TYPE_ZOOM, i, buf);
	}
	if (gEngfuncs.pDemoAPI->IsPlayingback())
		g_lastFOV = g_demozoom;

	return g_lastFOV;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate mouse sensitivity for modified FOV
// Note   : This formula is linear, but the real ratio is probably not
// Input  : newfov - FOV in degrees
// Output : float - sensitivity multiplier, measured in "times"
//-----------------------------------------------------------------------------
float GetSensitivityModByFOV(const float &newfov)
{
	float def_fov = gHUD.GetUpdatedDefaultFOV();
	//if (newfov == def_fov)
	//	return 0.0f;

	float s = 1.0f;
	float k = 0.0f;

	if (newfov > def_fov)
	{
		// k = 4	@ 360
		// k = 2	@ 180
		// k = 1	@ 90
		// k = 1/2	@ 45
		// k = 0	@ 0 (0 is physically impossible)
		if (newfov == 0.0f)// same as "newfov = def_fov"
			k = 1;
		else
			k = newfov/def_fov;

		s /= k;
	}
	else if (newfov < def_fov && g_pCvarZSR->value != 0.0f)// zooming
		s *= g_pCvarZSR->value;

	//conprintf(1, "FOV = %g,\tk = %g,\ts = %g\n", newfov, k, s);
	return s;
}

//-----------------------------------------------------------------------------
// Purpose: Simple resolution-aware sprite loading function
// Input  : *pszName - should include one %d for screen width value
// Output : HLSPRITE
//-----------------------------------------------------------------------------
HLSPRITE LoadSprite(const char *pszName)
{
	char sz[256]; 
	_snprintf(sz, 256, pszName, gHUD.m_iRes);
	sz[255] = '\0';
	return SPR_Load(sz);
}

//-----------------------------------------------------------------------------
// Purpose: Find the matching sprite in the list
// Input  : *pList - sprite list to search
//			pszName - sprite name
//			iRes - screen X resolution
//			iCount - number of items in the pList
// Output : client_sprite_t * - sprite
//-----------------------------------------------------------------------------
client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *pszName, int iRes, int iCount)
{
	if (pList == NULL)// !!! IMPORTANT! Must be here!
		return NULL;

	int i = iCount;
	client_sprite_t *p = pList;
	while (i)
	{
		if ((strcmp(pszName, p->szName) == 0) && (p->iRes == iRes))
			return p;

		++p;
		--i;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037a: allows HUD sprites to use different frames
// Input  : hSprite - 
// Output : int
//-----------------------------------------------------------------------------
int SPR_FindFrame(HLSPRITE hSprite)
{
	int index = gHUD.GetSpriteIndex(hSprite);
	if (index != HUDSPRITEINDEX_INVALID)
		return gHUD.GetSpriteFrame(index);

	return 0;
}

static HLSPRITE g_CurrentSprite = 0;
static wrect_t g_CurrentSpriteRect;

//-----------------------------------------------------------------------------
// Purpose: Additional layer between the game and the engine to inject some effects
// Input  : hPic - 
//			r g b - 
//-----------------------------------------------------------------------------
void SPR_Set(HLSPRITE hPic, int r, int g, int b)
{
	g_CurrentSprite = hPic;
	if (gHUD.m_iDistortMode & HUD_DISTORT_COLOR)// distort sprite drawing color on screen
	{
		float h = 0.5*gHUD.m_fDistortValue;
		ScaleColorsF(r,g,b, RANDOM_FLOAT(0.5, 0.5+h));
	}
	gEngfuncs.pfnSPR_Set(hPic, r, g, b);
}

//-----------------------------------------------------------------------------
// Purpose: Draws a the current sprite as solid
// Input  : frame - sprite frame to draw
//			x y - screen output coordinates
//			*prc - sprite area to draw (can be NULL)
//-----------------------------------------------------------------------------
void SPR_Draw(int frame, int x, int y, const wrect_t *prc)
{
	if (gHUD.m_iDistortMode & HUD_DISTORT_POS)// distort sprite drawing position on screen
	{
		x += (int)(RANDOM_FLOAT(-1,1) * gHUD.m_fDistortValue * HUD_DISTORT_POS_MAX_RADIUS);
		y += (int)(RANDOM_FLOAT(-1,1) * gHUD.m_fDistortValue * HUD_DISTORT_POS_MAX_RADIUS);
	}
	//if (g_CurrentSprite && frame == 0)
	//SLOW	frame = SPR_FindFrame(g_CurrentSprite);

	gEngfuncs.pfnSPR_Draw(frame, x, y, prc);
}

//-----------------------------------------------------------------------------
// Purpose: Draws the current sprites, with color index 255 transparent
// Input  : frame - sprite frame to draw
//			x y - screen output coordinates
//			*prc - sprite area to draw (can be NULL)
//-----------------------------------------------------------------------------
void SPR_DrawHoles(int frame, int x, int y, const wrect_t *prc)
{
	if (gHUD.m_iDistortMode & HUD_DISTORT_POS)// distort sprite drawing position on screen
	{
		x += (int)(RANDOM_FLOAT(-1,1) * gHUD.m_fDistortValue * HUD_DISTORT_POS_MAX_RADIUS);
		y += (int)(RANDOM_FLOAT(-1,1) * gHUD.m_fDistortValue * HUD_DISTORT_POS_MAX_RADIUS);
	}
	//if (g_CurrentSprite && frame == 0)
	//SLOW	frame = SPR_FindFrame(g_CurrentSprite);

	gEngfuncs.pfnSPR_DrawHoles(frame, x, y, prc);
}

//-----------------------------------------------------------------------------
// Purpose: Additional layer between the game and the engine to inject some effects
// Note   : Optimize this as much as possible!
// Input  : frame - sprite frame to draw
//			x y - screen output coordinates
//			*prc - sprite area to draw (can be NULL)
//-----------------------------------------------------------------------------
void SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc)
{
	if (gHUD.m_iDistortMode & HUD_DISTORT_POS)// distort sprite drawing position on screen
	{
		x += (int)(RANDOM_FLOAT(-1,1) * gHUD.m_fDistortValue * HUD_DISTORT_POS_MAX_RADIUS);
		y += (int)(RANDOM_FLOAT(-1,1) * gHUD.m_fDistortValue * HUD_DISTORT_POS_MAX_RADIUS);
	}
	//if (g_CurrentSprite && frame == 0)
	//SLOW	frame = SPR_FindFrame(g_CurrentSprite);

	if ((gHUD.m_iDistortMode & HUD_DISTORT_SPRITE) && prc != NULL)// distort internal sprite crop coordinates
	{
		int w = (prc->right - prc->left);//RectWidth(*prc);
		int h = (prc->bottom - prc->top);//RectHeight(*prc);
		//int xr = min(prc->left, SPR_Width(g_CurrentSprite, frame)-w);
		//int yr = min(prc->top, SPR_Height(g_CurrentSprite, frame)-h);
		//int sw = SPR_Width(g_CurrentSprite, frame);
		//int sh = SPR_Height(g_CurrentSprite, frame);

		//g_CurrentSpriteRect.left = clamp(prc->left + (int)(sw * (RANDOM_FLOAT(0, gHUD.m_fDistortValue)-0.5f)), 0, sw-w);
		//g_CurrentSpriteRect.top = clamp(prc->top + (int)(sh * (RANDOM_FLOAT(0, gHUD.m_fDistortValue)-0.5f)), 0, sh-h);

		//wtf g_CurrentSpriteRect.top = prc->top + (int)((RANDOM_FLOAT(0, gHUD.m_fDistortValue)-0.5f) * (float)(SPR_Height(g_CurrentSprite, frame)-h));

		//g_CurrentSpriteRect.left = prc->left + (int)((RANDOM_FLOAT(0, gHUD.m_fDistortValue)-0.5f) * (float)(SPR_Width(g_CurrentSprite, frame)-w));
		//g_CurrentSpriteRect.top = prc->top + (int)((RANDOM_FLOAT(0, gHUD.m_fDistortValue)-0.5f) * (float)(SPR_Height(g_CurrentSprite, frame)-h));

		// Centered around original rect (but slower). Double calls to RANDOM_FLOAT are required
		/*g_CurrentSpriteRect.left = prc->left
			-(prc->left - 0)*RANDOM_FLOAT(0,gHUD.m_fDistortValue)
			+(SPR_Width(g_CurrentSprite, frame)-prc->left-RectWidth(*prc))*RANDOM_FLOAT(0,gHUD.m_fDistortValue);

		g_CurrentSpriteRect.top = prc->top
			-(prc->top - 0)*RANDOM_FLOAT(0,gHUD.m_fDistortValue)
			+(SPR_Height(g_CurrentSprite, frame)-prc->top-RectHeight(*prc))*RANDOM_FLOAT(0,gHUD.m_fDistortValue);*/

		// simplified
		g_CurrentSpriteRect.left = (int)(prc->left
			-prc->left*RANDOM_FLOAT(0,gHUD.m_fDistortValue)
			+(SPR_Width(g_CurrentSprite, frame)-prc->right)*RANDOM_FLOAT(0,gHUD.m_fDistortValue));

		g_CurrentSpriteRect.top = (int)(prc->top
			-prc->top*RANDOM_FLOAT(0,gHUD.m_fDistortValue)
			+(SPR_Height(g_CurrentSprite, frame)-prc->bottom)*RANDOM_FLOAT(0,gHUD.m_fDistortValue));

		// OK, but too rough
		//ok	g_CurrentSpriteRect.left = RANDOM_LONG(0, SPR_Width(g_CurrentSprite, frame)-w);
		//ok	g_CurrentSpriteRect.top = RANDOM_LONG(0, SPR_Height(g_CurrentSprite, frame)-h);

		// keep original size
		g_CurrentSpriteRect.right = g_CurrentSpriteRect.left + w;// prc->right + (int)((RANDOM_FLOAT(0, gHUD.m_fDistortValue)-0.5f) * (float)gEngfuncs.pfnSPR_Width(g_CurrentSprite, frame));
		g_CurrentSpriteRect.bottom = g_CurrentSpriteRect.top + h;// prc->bottom + (int)((RANDOM_FLOAT(0, gHUD.m_fDistortValue)-0.5f) * (float)gEngfuncs.pfnSPR_Height(g_CurrentSprite, frame));
		gEngfuncs.pfnSPR_DrawAdditive(frame, x, y, &g_CurrentSpriteRect);
	}
	else
		gEngfuncs.pfnSPR_DrawAdditive(frame, x, y, prc);
}

//-----------------------------------------------------------------------------
// Purpose: Client version of UTIL_ScreenFade // TESTME!
// Input  : r g b alpha - 
//			fadeTime - gradient time
//			fadeHold - time to hold solid color
//			flags - FFADE_IN, etc.
//-----------------------------------------------------------------------------
void CL_ScreenFade(byte r, byte g, byte b, byte alpha, float fadetime, float fadehold, int flags)
{
	float fTime;
	if (gHUD.m_iActive == 0)
		fTime = 0;
	else
		fTime = gHUD.m_flTime;
		//return;

	//conprintf(1, "CL_ScreenFade(%d %d %d %d, %g, %g, %d)\n", r,g,b,alpha, fadetime, fadehold, flags);
	screenfade_t sf;
	//right now, we have no blending code, so we don't need to copy: ?
	GET_SCREEN_FADE(&sf);
	//sf.fadeSpeed = 0;//test1->value;//100.0;
	//sf.fadeEnd = test1->value;// gradient fade period
	//sf.fadeTotalEnd = test2->value;
	//sf.fadeReset = test3->value;// solid color period
	//sf.fadeEnd = fadetime;// gradient fade period
	sf.fadeTotalEnd = fTime + fadetime + fadehold;
	//sf.fadeReset = fadehold;// solid color period
	sf.fader = r;
	sf.fadeg = g;
	sf.fadeb = b;
	sf.fadealpha = alpha;
	sf.fadeFlags = flags;
	if (fadetime > 0)// guess what? it's HL2 SDK code
	{
		if (flags & FFADE_OUT)// - from transparent to color. 1) fade, 2) hold
		{
			if (fadetime > 0)
				sf.fadeSpeed = -(float)sf.fadealpha / fadetime;

			sf.fadeEnd = fTime + fadetime;
			sf.fadeReset = sf.fadeEnd + fadehold;
		}
		else// FFADE_IN - from color to transparent. 1) hold, 2) fade
		{
			if (fadetime > 0)
				sf.fadeSpeed = (float)sf.fadealpha / fadetime;

			sf.fadeReset = fTime + fadehold + fadetime;// was without fadetime
			sf.fadeEnd = sf.fadeReset;//fTime + fadehold + fadetime;
		}
	}
	else// XDM: support for solid overlay without fading
	{
		sf.fadeSpeed = 0;
		sf.fadeReset += fTime;
		sf.fadeEnd = sf.fadeReset;
	}
	SET_SCREEN_FADE(&sf);
}

//-----------------------------------------------------------------------------
// Purpose: Old Half-Life inheritance
// Input  : &r &g &b - output
//			colorindex - 
//-----------------------------------------------------------------------------
void UnpackRGB(::Color &rgb, unsigned short colorindex)
{
	switch (colorindex)
	{
	default:
		//conprintf(0, "UnpackRGB(%d): bad color!\n", colorindex);
	case RGB_GREEN:
		{
			if (gHUD.m_pCvarUseTeamColor->value > 0.0f)// HACK? RGB_GREEN may be used not just to draw HUD elements
				rgb = GetTeamColor(gHUD.m_iTeamNumber);
			else
				rgb.Set(gHUD.m_iDrawColorMain);
		}
		break;
	case RGB_YELLOW:rgb.Set(gHUD.m_iDrawColorYellow); break;
	case RGB_BLUE:	rgb.Set(gHUD.m_iDrawColorBlue); break;
	case RGB_RED:	rgb.Set(gHUD.m_iDrawColorRed); break;
	case RGB_CYAN:	rgb.Set(gHUD.m_iDrawColorCyan); break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Old Half-Life inheritance
// Input  : &r &g &b - output
//			colorindex - 
//-----------------------------------------------------------------------------
void UnpackRGB(byte &r, byte &g, byte &b, unsigned short colorindex)
{
	static ::Color rgb;
	UnpackRGB(rgb, colorindex);
	r = rgb.r;
	g = rgb.g;
	b = rgb.b;
}

//-----------------------------------------------------------------------------
// Purpose: Old Half-Life inheritance
// Input  : &r &g &b - output
//			colorindex - 
//-----------------------------------------------------------------------------
void UnpackRGB(int &r, int &g, int &b, unsigned short colorindex)
{
	static byte br,bg,bb;
	UnpackRGB(br,bg,bb,colorindex);
	r = br;
	g = bg;
	b = bb;
}

//-----------------------------------------------------------------------------
// Purpose: Get team color
// Input  : team - TEAM_ID
// Output : Color
//-----------------------------------------------------------------------------
const ::Color &GetTeamColor(TEAM_ID team)
{
	if (team < TEAM_NONE || team > MAX_TEAMS || !IsTeamGame(gHUD.m_iGameType))//iNumberOfTeamColors)
		team = TEAM_NONE;

	return g_TeamInfo[team].color;
}

//-----------------------------------------------------------------------------
// Purpose: Get team RGB colors
// Input  : team - TEAM_ID
//			&r &g &b - output
//-----------------------------------------------------------------------------
void GetTeamColor(TEAM_ID team, byte &r, byte &g, byte &b)
{
	if (IsTeamGame(gHUD.m_iGameType))
	{
		if (team < TEAM_NONE || team > MAX_TEAMS)//iNumberOfTeamColors)
			team = TEAM_NONE;

		r = g_TeamInfo[team].color[0];
		g = g_TeamInfo[team].color[1];
		b = g_TeamInfo[team].color[2];
	}
	else
	{
		//UnpackRGB(r,g,b, RGB_GREEN);// STACK OVERFLOW!!! Infinite loop!
		//if (sscanf(g_pCvarColorMain->string, "%d %d %d", &r, &g, &b) == 3)
		//	return false;
		//else
			//team = 0;

		Int2RGB(gHUD.m_iDrawColorMain, r,g,b);// XDM3034 // UNDONE: this is not a right place for this
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get team RGB colors, integer version
// Input  : team - TEAM_ID
//			&r &g &b - output
//-----------------------------------------------------------------------------
void GetTeamColor(TEAM_ID team, int &r, int &g, int &b)
{
	byte br=0,bg=0,bb=0;
	GetTeamColor(team, br,bg,bb);
	r = br;
	g = bg;
	b = bb;
}

//-----------------------------------------------------------------------------
// Purpose: Get player personal color for all game types
// Input  : client - player ID/entindex
// Output : Color
//-----------------------------------------------------------------------------
const ::Color &GetPlayerColor(CLIENT_INDEX client)
{
	if (IsValidPlayerIndex(client))// ActivePlayer is better, but a lot slower
	{
		if (IsTeamGame(gHUD.m_iGameType))
			return GetTeamColor(g_PlayerExtraInfo[client].teamnumber);
		else
		{
			static ::Color c;
			colormap2RGB(g_PlayerInfoList[client].topcolor, false, c.r,c.g,c.b);
			c.a = 255;
			return c;
		}
	}
	return g_TeamInfo[TEAM_NONE].color;
}

//-----------------------------------------------------------------------------
// Purpose: Get player personal color for all game types as RGB
// Input  : client - player ID/entindex
//			&r &g &b - output
//-----------------------------------------------------------------------------
bool GetPlayerColor(CLIENT_INDEX client, byte &r, byte &g, byte &b)
{
	if (IsValidPlayerIndex(client))// ActivePlayer is better, but a lot slower
	{
		if (IsTeamGame(gHUD.m_iGameType))
			GetTeamColor(g_PlayerExtraInfo[client].teamnumber, r,g,b);
		else
		{
			//HSL2RGB(((float)g_PlayerInfoList[client].topcolor/255.0f)*360.0f, 1.0f, 1.0f, r,g,b);
			colormap2RGB(g_PlayerInfoList[client].topcolor, false, r,g,b);
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Get color gradient by the meter value: red->yellow->green
// Input  : fMeterValue - 0...1
//			&r &g &b - output red...green
//-----------------------------------------------------------------------------
void GetMeterColor(const float &fMeterValue, byte &r, byte &g, byte &b)
{
	byte value = (byte)(255.0f * clamp(fMeterValue, 0,1));// convert to byte 0...255
	r = 255 - value;
	g = value;
	b = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Choose render mode best suited for a certain sprite
// Input  : *pHeader - model_t.cache.data
// Output : int - rendermode
//-----------------------------------------------------------------------------
int SpriteRenderMode(msprite_t *pHeader)
{
	if (pHeader)
	{
		if (pHeader->texFormat == SPR_NORMAL)
			return kRenderNormal;
		else if (pHeader->texFormat == SPR_ALPHTEST)
			return kRenderTransAlpha;
		//else
		//	return kRenderTransAdd;
	}
	return kRenderTransAdd;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Searches through the string for any msg names (indicated by a '#')
// any found are looked up in titles.txt and the new message substituted
// the new value is pushed into dst_buffer
// Input  : *msg - 
//			*dst_buffer - 
//			buffer_size - 
// Output : char *dst_buffer
//-----------------------------------------------------------------------------
char *LocaliseTextString(const char *msg, char *dst_buffer, const size_t buffer_size)
{
	if (msg == dst_buffer)// writing to the source?
	{
		DBG_FORCEBREAK
		return dst_buffer;// XDM3035c: this function does not support writing to the source!
	}
	char msgname[MAX_TITLE_NAME];
	size_t msgnamelen = 0;
	size_t buffer_remaining = buffer_size;
	const char *msgnamestart = NULL;
	char *dst = dst_buffer;

	for (const char *src = msg; *src != 0 && buffer_remaining > 0; --buffer_remaining)
	{
		if (*src == '#')// && *(src+1) != '#')?
		{
			msgnamestart = src;
			++src;// skip
			msgnamelen = 0;

			while ((isalnum(*src) || ((*src) == '_')) && msgnamelen < MAX_TITLE_NAME)// XDM3038c: don't include spaces!
			{
				msgname[msgnamelen] = *src;
				++msgnamelen;
				++src;
			}
			msgname[min(msgnamelen, MAX_TITLE_NAME-1)] = 0;

			// lookup msg name in titles.txt
			client_textmessage_t *clmsg = TextMessageGet(msgname);
			if (clmsg == NULL || clmsg->pMessage == NULL)
			{
				src = msgnamestart;// revert back to the '#', copy it and skip it
				*dst = *src;
				dst++,
				src++;
				continue;
			}
			else
			{
				// copy string into message over the msg name
				for (const char *wsrc = clmsg->pMessage; *wsrc != 0; ++wsrc, ++dst)
					*dst = *wsrc;

				if (*(dst-1) == '\r' || *(dst-1) == '\n')// XDM3035: HACK remove last newline symbol because we need 
				{
					--dst;
					*dst = 0;
				}
			}
			*dst = 0;
		}
		else// just copy character by character, move both the source and destionation pointers
		{
			*dst = *src;
			++dst; ++src;
			//WTF?	*dst = 0;
		}
	}
	*dst = 0;
	dst_buffer[buffer_size-1] = '\0'; // ensure null termination
	return dst_buffer;
}

#define LOCALISE_BUFFER_SIZE		1024
//-----------------------------------------------------------------------------
// Purpose: As above, but with a local static buffer
// Input  : *msg - 
// Output : char * - buffered localized string, valid until next call
//-----------------------------------------------------------------------------
char *BufferedLocaliseTextString(const char *msg)
{
	static char dst_buffer[LOCALISE_BUFFER_SIZE];
	LocaliseTextString(msg, dst_buffer, LOCALISE_BUFFER_SIZE);
	//dst_buffer[LOCALISE_BUFFER_SIZE-1] = 0;// LocaliseTextString takes care of it
	return dst_buffer;
}

//-----------------------------------------------------------------------------
// Purpose: Simplified version of LocaliseTextString;  assumes string is only one word
// Input  : *msg - title #name, starting with '#'
//			*msg_dest - HUD_PRINT enum
// Output : char * - a temporary pointer to a structure inside the engine!!
//-----------------------------------------------------------------------------
char *LookupString(const char *msg, int *msg_dest)
{
	if (msg == NULL)
		return "";

	// '#' character indicates this is a reference to a string in titles.txt, and not the string itself
	if (msg[0] == '#')
	{
		// this is a message name, so look up the real message
		client_textmessage_t *clmsg = TextMessageGet(msg+1);
		if (!clmsg || !(clmsg->pMessage))
			return (char *)msg; // lookup failed, so return the original string

		if (msg_dest)
		{
			// check to see if titles.txt info overrides msg destination
			// if clmsg->effect is less than 0, then clmsg->effect holds -1 * message_destination
			if (clmsg->effect < 0)
				*msg_dest = -clmsg->effect;
		}

		return (char *)clmsg->pMessage;
	}
	else// nothing special about this message, so just return the same string
		return (char *)msg;
}

//-----------------------------------------------------------------------------
// Purpose: StripEndNewlineFromString
// Input  : *str - i/o
//-----------------------------------------------------------------------------
// THIS CAUSES WEIRD MEMORY CORRUPTION! char *StripEndNewlineFromString(char *str)
void StripEndNewlineFromString(char *str)
{
	if (str && *str)
	{
		size_t s = strlen(str) - 1;
		if (str[s] == '\n' || str[s] == '\r')
			str[s] = '\0';
	}
}

//-----------------------------------------------------------------------------
// Purpose: converts all '\r' characters to '\n', so that the engine can deal with the properly
// Input  : *str - 
// Output : char * pointer to str
//-----------------------------------------------------------------------------
char *ConvertCRtoNL(char *str)
{
	char *ch = NULL;
	for (ch = str; *ch != 0; ++ch)
		if (*ch == '\r')
			*ch = '\n';
	return str;
}

char g_sMapName[MAX_MAPPATH] = "\0\0\0\0";// XDM3038a

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: get current map name (without extension)
// Note   : GET_LEVEL_NAME returns "maps/test/tst_fog.bsp"
// Note   : Who resets m_sMapName?
// Input  : bIncludeSubdir - return "test/tst_fog" or just "tst_fog"
// Output : const char *
//-----------------------------------------------------------------------------
const char *GetMapName(bool bIncludeSubdir)
{
	if (g_sMapName[0] == 0)// Holds "test/tst_fog"
	{
		const char *pLevelName = GET_LEVEL_NAME();
		//if (strncmp(pLevelName, "maps/", 5) == 0)
		//	return (pLevelName + 5);// HACK?

		memset(g_sMapName, 0, sizeof(g_sMapName));
		//char dir[_MAX_DIR];
		//char fname[_MAX_FNAME];
		//_splitpath(pLevelName, NULL, dir, fname, NULL);

		size_t offset = strbegin(pLevelName, "maps/");// not a /maps/ directory?
		//if (strncmp(pLevelName, "maps/", 5) == 0)// not a /maps/ directory?
		//	offset = 5;
		////else
		////	offset = (strrchr(pLevelName, '/')-pLevelName);

		//_snprintf(m_sMapName, "%s%s", dir+offset, fname);
		const char *pLastDot = strrchr(pLevelName, '.');
		if (pLastDot)
			strncpy(g_sMapName, pLevelName+offset, pLastDot-(pLevelName+offset));
		else
			strncpy(g_sMapName, pLevelName+offset, MAX_MAPPATH);// no extension? we're prepared.

		g_sMapName[MAX_MAPPATH-1] = '\0';
	}

	if (bIncludeSubdir)// XDM3037
		return g_sMapName;// "test/tst_fog"

	char *pLastSeparator = strrchr(g_sMapName, '/');
	if (pLastSeparator)
		return pLastSeparator+1;// "tst_fog"
	else
		return g_sMapName;
}


//-----------------------------------------------------------------------------
// Purpose: Obsolete: use _splitpath
// No buffer overflow checks!
// Input  : *fullpath - 
//			*dir - 
//			*name - 
//			*ext - 
//-----------------------------------------------------------------------------
/*void ExtractFileName(const char *fullpath, char *dir, char *name, char *ext)
{
	//Pathname_Convert(fullpath);
	unsigned int i = 0;
	unsigned int offset = 0;
	unsigned int l = 0;
	unsigned int dot = l;// assume file has no extension

	while (fullpath[i] != 0)
	{
		if (fullpath[i] == '/' || fullpath[i] == '\\')
			offset = i+1;

		i++;
	}
	l = i;
	for (i=offset; i<l; i++)
	{
		if (fullpath[i] == '.')
			dot = i;
	}
	if (dir)
	{
		strncpy(dir, fullpath, offset-1);
		dir[offset-1] = 0;
	}
	if (name)
	{
		strncpy(name, fullpath+offset, dot-offset);
		name[dot-offset] = 0;
	}
	if (ext && dot < l)
	{
		strncpy(ext, fullpath+dot+1, l-dot);
		ext[l-dot] = 0;
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: Get structure data from the engine
//-----------------------------------------------------------------------------
void GetAllPlayersInfo(void)
{
	for (CLIENT_INDEX i = 1; i <= MAX_PLAYERS; ++i)// XDM3035a: include index 32!!!
		GetPlayerInfo(i, &g_PlayerInfoList[i]);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Recalculate the existing teams in the match
//-----------------------------------------------------------------------------
void RebuildTeams(void)
{
	size_t i = 0;
	// clear out player counts from teams
	for (i = 0; i < MAX_TEAMS+1; ++i)// XDM3035: clear all and spectators even in non-teamplay
		g_TeamInfo[i].players = 0;

	// rebuild the team list
	GetAllPlayersInfo();

	for (i = 1; i <= MAX_PLAYERS; ++i)
	{
		if (!IsValidPlayer(i))
			continue;
		if (IsValidTeam(g_PlayerExtraInfo[i].teamnumber))// XDM: 0 means spectators
			g_TeamInfo[g_PlayerExtraInfo[i].teamnumber].players++;
	}

	// clear out any empty teams
	for (i = 0; i <= MAX_TEAMS; ++i)
	{
	// XDM: DON'T CLEAR NAME!!			memset(&g_TeamInfo[i], 0, sizeof(team_info_t));
		if (g_TeamInfo[i].players < 1)
		{
			g_TeamInfo[i].score = 0;
			g_TeamInfo[i].deaths = 0;
			g_TeamInfo[i].scores_overriden = 0;
			g_TeamInfo[i].ping = 0;
			g_TeamInfo[i].packetloss = 0;
		}
	}
	// Update the scoreboard?
}

//-----------------------------------------------------------------------------
// Purpose: Is local player dead?
// Warning: TODO: make a more reliable method?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CL_IsDead(void)
{
	//if (gHUD.m_ClientData.health > 0)// XDM3037a: extra precision
	//if (gHUD.m_Health.m_iHealth > 0)// XDM3038a: same source - HUD_TxferLocalOverrides
	//	return false;
	//if (gHUD.m_pLocalPlayer->curstate.health > 0)
	return (gHUD.m_Health.m_iHealth <= 0) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Is this a real, playable team? // TEAM_NONE must be invalid here!
// Input  : &team_id - TEAM_ID
// Output : Returns true if [TEAM_1 to TEAM_4] and active
//-----------------------------------------------------------------------------
bool IsActiveTeam(const TEAM_ID &team_id)
{
	return (team_id > TEAM_NONE && team_id <= g_iNumberOfTeams);
}

//-----------------------------------------------------------------------------
// Purpose: Is this a valid team ID? (GLOBALLY, DISREGARDING GAME RULES!)
// Warning: ACCEPTS TEAM_NONE!
// Input  : &team_id - TEAM_ID
// Output : Returns true if [TEAM_NONE to TEAM_4]
//-----------------------------------------------------------------------------
bool IsValidTeam(const TEAM_ID &team_id)
{
	return (team_id >= TEAM_NONE && team_id <= MAX_TEAMS);
}

//-----------------------------------------------------------------------------
// Purpose: Connected and not a spectator
// Input  : *ent - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/* XDM3038a: LE BIG WTF! ent->player is often 0!!!!!!!!! bool IsActivePlayer(cl_entity_t *ent)
{
	if (ent == NULL)
		return false;

	return (ent &&
			ent->player &&// let's rely on ent->player IsValidPlayer(ent->index) &&//ent->index > 0 && ent->index < MAX_PLAYERS &&
			g_PlayerExtraInfo[ent->index].observer == 0 &&//old !g_IsSpectator[ent->index] &&
			g_PlayerInfoList[ent->index].name != NULL);
}*/

//-----------------------------------------------------------------------------
// Purpose: Real player, connected and not a spectator?
// Input  : idx - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsActivePlayer(const CLIENT_INDEX &idx)
{
	/*if (!IsValidPlayerIndex(idx))
		return false;
	if (g_PlayerInfoList[idx].name == NULL)
		return false;*/
	if (!IsValidPlayer(idx))
		return false;
	if (g_PlayerExtraInfo[idx].observer != 0)// UNDONE TODO TESETME Can active players have this flag set in some cases?
		return false;

	return true;//IsActivePlayer(gEngfuncs.GetEntityByIndex(idx));
}

//-----------------------------------------------------------------------------
// Purpose: Real player, connected, MAY be dead or spectating
// Input  : idx - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsValidPlayer(const CLIENT_INDEX &idx)
{
	if (!IsValidPlayerIndex(idx))
		return false;

	//GetPlayerInfo(idx, &g_PlayerInfoList[idx]);
	if (g_PlayerInfoList[idx].name == NULL)
		return false;

	return (gEngfuncs.GetEntityByIndex(idx) != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Is this a possible index for a player? (NO A PLAYER, JUST INDEX!)
// Input  : idx - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsValidPlayerIndex(const CLIENT_INDEX &idx)
{
	if (idx >= 1 && idx <= gEngfuncs.GetMaxClients())
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Is this a spectator?
// Input  : idx - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsSpectator(const CLIENT_INDEX &idx)
{
	if (IsValidPlayerIndex(idx))
	{
		//old	return (g_IsSpectator[idx] > 0);
		return (g_PlayerExtraInfo[idx].observer > 0);
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: GameRules->IsTeamplay()
// Input  : &gamerules - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsTeamGame(const short &gamerules)
{
	if (gamerules >= GT_TEAMPLAY)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &gamerules - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsRoundBasedGame(const short &gamerules)
{
	if (gamerules == GT_ROUND)
		return true;
	//else if (gamerules == GT_ASSAULT)
	//	return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &gamerules - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsExtraScoreBasedGame(const short &gamerules)
{
	if (gamerules == GT_CTF)
		return true;
	else if (gamerules == GT_DOMINATION)
		return true;
	//else if (gamerules == GT_ASSAULT)
	//	return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: COOP_MODE_SWEEP
// Output : short
//-----------------------------------------------------------------------------
short GetGameMode(void)
{
	return gHUD.m_iGameMode;
}

//-----------------------------------------------------------------------------
// Purpose: GAME_FLAG_NOSHOOTING
// Output : short
//-----------------------------------------------------------------------------
short GetGameFlags(void)
{
	return gHUD.m_iGameFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Abstraction between both DLLs
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsMultiplayer(void)
{
	return (gEngfuncs.GetMaxClients() > 1);
}

//-----------------------------------------------------------------------------
// Purpose: Abstraction between both DLLs
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool IsGameOver(void)
{
	return (gHUD.m_iGameState == GAME_STATE_FINISHED || gHUD.m_iGameState == GAME_STATE_LOADING);
}

//-----------------------------------------------------------------------------
// Purpose: GetGameDescription
// Input  : &gamerules - 
// Output : const char
//-----------------------------------------------------------------------------
const char *GetGameDescription(const short &gamerules)
{
	char stringname[16];
	_snprintf(stringname, 16, "#%s%d\0", GAMETITLE_STRING_NAME, gamerules);
	stringname[15] = '\0';
	const char *str = BufferedLocaliseTextString(stringname);
	if (str)
		return str;

	return GAMETITLE_DEFAULT_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: Get localized game rules description text (now has its own buffer)
// Input  : &gametype - 
//			&gamemode - 
// Output : const char *
//-----------------------------------------------------------------------------
const char *GetGameRulesIntroText(const short &gametype, const short &gamemode)
{
	static char dst_buffer[LOCALISE_BUFFER_SIZE];
	char *msg = NULL;
	switch (gametype)
	{
	/*default:
	//case GT_SINGLE:
		{
			return "";// safe?
			break;
		}*/
	case GT_COOP:
		{
			if (gamemode == COOP_MODE_MONSTERFRAGS)
				msg = "#INTRO_COOP_MON";//"--- CoOperative monster hunting! ---\n\nGet more monsterfrags!");
			else if (gamemode == COOP_MODE_LEVEL)
				msg = "#INTRO_COOP_LVL";//"--- CoOperative level playing! ---\n\nAll players must reach the end of level!");
			else // if (gamemode == COOP_MODE_SWEEP)
				msg = "#INTRO_COOP_SWP";//"--- CoOperative monster sweeping! ---\n\nClear this level off monsters!");

		}
		break;
	case GT_DEATHMATCH:
		{
			msg = "#INTRO_DM";// it's probably better to automatically generate these string IDs
		}
		break;
	case GT_LMS:
		{
			msg = "#INTRO_LMS";
		}
		break;
	case GT_TEAMPLAY:
		{
			msg = "#INTRO_TDM";
		}
		break;
	case GT_CTF:
		{
			msg = "#INTRO_CTF";
		}
		break;
	case GT_DOMINATION:
		{
			msg = "#INTRO_DOM";
		}
		break;
	case GT_ROUND:
		{
			msg = "#INTRO_RD";
		}
		break;
	}

	if (msg)
	{
		LocaliseTextString(msg, dst_buffer, LOCALISE_BUFFER_SIZE);
		return dst_buffer;
	}
	else
		return "";
}

//-----------------------------------------------------------------------------
// Purpose: GetTeamName
// Input  : &team_id - 
// Output : char
//-----------------------------------------------------------------------------
const char *GetTeamName(const TEAM_ID &team_id)
{
	if (IsActiveTeam(team_id))//if (IsValidTeam(team_id))
		return g_TeamInfo[team_id].name;

	//DBG_FORCEBREAK
	return "";
}

//-----------------------------------------------------------------------------
// Purpose: Check for user vis.distance setting (for simple manual LOD)
// Input  : &point - point to check
// Output : float - 0...1 is in cl_viewdist, 2 is double cl_viewdist, etc.
//-----------------------------------------------------------------------------
float UTIL_PointViewDist(const Vector &point)
{
	vec_t fLen = (point-g_vecViewOrigin).Length();
	float v = (g_pCvarViewDistance->value * (gHUD.GetUpdatedDefaultFOV()/gHUD.GetCurrentFOV()));// XDM3037a: gHUD.m_iFOV));// 90/60
	if (v != 0.0f)
		return fLen/v;

	return 100.0f;// ?
}

//-----------------------------------------------------------------------------
// Purpose: Simple check for user setting (for simple manual LOD)
// Input  : &point - point to check
// Output : Returns true if point is farther than cl_viewdist, false otherwise
//-----------------------------------------------------------------------------
bool UTIL_PointIsFar(const Vector &point)
{
	//return (l > v);
	return (UTIL_PointViewDist(point) > 1.0f);
}

//-----------------------------------------------------------------------------
// Purpose: is provided vector visible by local client?
//			OPTIMIZE AS HARD AS POSSIBLE!!
// Input  : &point - point to check
//			check_backplane - check if it's behind viewport
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UTIL_PointIsVisible(const vec3_t &point, bool check_backplane)
{
	static Vector vDir;
	vDir = point;
	vDir -= g_vecViewOrigin;
	vec_t fLen = vDir.NormalizeSelf();

	if (g_pCvarServerZMax && fLen >= g_pCvarServerZMax->value*0.9f)// clipped by sv_zmax
		return false;

	if ((gHUD.m_iFogMode > 0) && (gHUD.m_flFogEnd > 32.0f) && (fLen >= gHUD.m_flFogEnd))// clipped by fog
		return false;

	if (check_backplane)
	{
		vec_t fDot = DotProduct(vDir, g_vecViewForward);// point must NOT be behind view plane
		if (fDot < cosf(gHUD.GetCurrentFOV()*0.5f))// XDM3037a: new FOV mechanism
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get entity name or at least something useful...
// Input  : entindex - name source
//			*output - print here
//			max_len - maximum length of output string (0 will be put at this-1)
//-----------------------------------------------------------------------------
void GetEntityPrintableName(int entindex, char *output, const size_t max_len)
{
	if (entindex > 0 && output && max_len > 1)
	{
		if (IsValidPlayerIndex(entindex))
		{
			GetPlayerInfo(entindex, &g_PlayerInfoList[entindex]);
			if (IsValidPlayer(entindex))// XDM3038
				strncpy(output, g_PlayerInfoList[entindex].name, max_len);
			else
				_snprintf(output, max_len, "PID %d\0", entindex);
		}
		//else if (gHUD.m_iGameType == GT_COOP)
		//	_snprintf(output, max_len, "monster %d", data1);
		else
		{
			cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(entindex);
			if (pEntity && pEntity->model)
				_snprintf(output, max_len, "EID %d (%s)\0", entindex, UTIL_StripFileNameExt(pEntity->model->name));
			else
				_snprintf(output, max_len, "EID %d\0", entindex);// there are no classnames or targetnames on client side :(
		}
		output[max_len-1] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get entity name or at least something useful... v2
// Input  : entindex - name source
// Output : const char *temporary buffer
//-----------------------------------------------------------------------------
const char *GetEntityPrintableName(int entindex)
{
	static char szNameBuffer[64];
	GetEntityPrintableName(entindex, szNameBuffer, 64);
	return szNameBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: Returns pointer to an enitity ONLY if it is updated (has origin and other properties)
// Warning: Returns NULL very often!
// Input  : entindex
// Output : cl_entity_t
//-----------------------------------------------------------------------------
cl_entity_t *GetUpdatingEntity(int entindex)
{
	cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(entindex);
	if (pEntity && gHUD.m_pLocalPlayer)// may be outside PVS // WARNING: gHUD.m_pLocalPlayer may be NULL!!
	{
		if (pEntity->curstate.messagenum == gHUD.m_pLocalPlayer->curstate.messagenum)
			return pEntity;// already culled by server
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Search array for specified value
// Input  : *pArray - []
//			&value - value
// Output : int - index in array or -1 if not found
//-----------------------------------------------------------------------------
int Array_FindInt(const int *pArray, const int &value)// const int terminator, const int badreturn)
{
	if (pArray)
	{
		int index = 0;
		while (pArray[index] != 0)// terminator
		{
			if (pArray[index] == value)
				return index;

			++index;
		}
	}
	return -1;// badreturn
}

//-----------------------------------------------------------------------------
// Purpose: HACK to avoid cleared attachment[]s! We really hate GoldSource, do we?
// Input  : *pEntity - 
//			attachment - 
// Output : Vector - world coordinates
//-----------------------------------------------------------------------------
const Vector &GetCEntAttachment(const struct cl_entity_s *pEntity, const int attachment)
{
	if (pEntity == NULL)
		return g_vecZero;

	if (pEntity == gHUD.m_pLocalPlayer && g_ThirdPersonView == 0)
		return gEngfuncs.GetViewModel()->attachment[attachment];

#if defined (CLDLL_FIX_PLAYER_ATTACHMENTS)
	if (pEntity->player)// this HL bullshit only affects players
	{
		if (attachment == 0)
			return pEntity->baseline.vuser1;
		else if (attachment == 1)
			return pEntity->baseline.vuser2;
		else if (attachment == 2)
			return pEntity->baseline.vuser3;
		else// if (attachment == 3)
			return pEntity->baseline.vuser4;
	}
#endif
	return pEntity->attachment[attachment];
}
