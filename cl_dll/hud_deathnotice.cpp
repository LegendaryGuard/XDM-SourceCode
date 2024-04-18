#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_Viewport.h"
#include <ctype.h>

// UNDONE: server-decided notice types
// TODO: remove all memory movement in favor of skipping and searching for empty entries? Right now memmove is required to use index as drawing position.

DECLARE_MESSAGE(m_DeathNotice, DeathMsg);

const int DEATHNOTICE_TEXTSPACE = 5;
/*const color24 DN_COLOR_TEAMMATE = {255,63,0};
const color24 DN_COLOR_UNKNOWN = {127,127,127};
const color24 DN_COLOR_WORLD = {191,191,191};
const color24 DN_COLOR_W_MONSTER = {255,127,31};*/
#define DN_COLOR_TEAMMATE		RGBA2INT(255,63,0,255)
#define DN_COLOR_UNKNOWN		RGBA2INT(127,127,127,255)
#define DN_COLOR_WORLD			RGBA2INT(191,191,191,255)
#define DN_COLOR_W_MONSTER		RGBA2INT(255,127,31,255)

const char nullstring[] = "NULL\0";

CHudDeathNotice::CHudDeathNotice() : CHudBase()
{
	m_pDeathNoticeList = NULL;
	m_iNumDeathNotices = 0;
}

CHudDeathNotice::~CHudDeathNotice()
{
	FreeData();
}

void CHudDeathNotice::FreeData(void)
{
	memset(m_szMessage, 0, sizeof(m_szMessage));
	memset(m_szScoreMessage, 0, sizeof(m_szMessage));
	memset(&m_Message, 0, sizeof(client_textmessage_t));
	memset(&m_ScoreMessage, 0, sizeof(client_textmessage_t));

	if (m_pDeathNoticeList)
	{
		delete [] m_pDeathNoticeList;
		m_pDeathNoticeList = NULL;
		m_iNumDeathNotices = 0;
	}
}

int CHudDeathNotice::Init(void)
{
	HOOK_MESSAGE(DeathMsg);

	m_pCvarDNNum = CVAR_CREATE("hud_deathnotice_num", "8", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarDNTime = CVAR_CREATE("hud_deathnotice_time", "6", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarDNEcho = CVAR_CREATE("hud_deathnotice_echo", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_pCvarDNTop = CVAR_CREATE("hud_deathnotice_top", "0.075", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	m_iFlags |= HUD_DRAW_ALWAYS | HUD_INTERMISSION;// XDM

	FreeData();
	gHUD.AddHudElem(this);
	return 1;
}

void CHudDeathNotice::InitHUDData(void)
{
	if (m_pDeathNoticeList == NULL)
	{
		if (m_pCvarDNNum->value < 2)
			m_pCvarDNNum->value = 2;
		else if (m_pCvarDNNum->value > MAX_DEATHNOTICES)
			m_pCvarDNNum->value = MAX_DEATHNOTICES;

		m_iNumDeathNotices = (short)m_pCvarDNNum->value;
		m_pDeathNoticeList = new DeathNoticeItem[m_iNumDeathNotices];// XDM3035: this should really be a queue, not an array!
		if (m_pDeathNoticeList)
			memset(m_pDeathNoticeList, 0, sizeof(DeathNoticeItem)*m_iNumDeathNotices);// XDM3036
	}
}

int CHudDeathNotice::VidInit(void)
{
	FreeData();
	m_iDefaultSprite = gHUD.GetSpriteIndex("d_default");
	InitHUDData();
	Reset();
	return 1;
}

// Warning: Called during respawn, do not do anything here!
void CHudDeathNotice::Reset(void)
{
}

int CHudDeathNotice::Draw(const float &flTime)
{
	if (m_pDeathNoticeList == NULL)
		return 0;

	int x, y;
	for (uint32 i = 0; i < m_iNumDeathNotices; ++i)
	{
		if (m_pDeathNoticeList[i].bActive == false)//if (m_pDeathNoticeList[i].iSpriteIndex == 0)// with current concept this is true.
			break;  // we've gone through them all

		if (m_pDeathNoticeList[i].flDisplayTime < flTime)// display time has expired
		{
			// remove the current item from the list and move all following items to its position
			memmove(&m_pDeathNoticeList[i], &m_pDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (m_iNumDeathNotices-i-1));// XDM3038c: fix
			memset(&m_pDeathNoticeList[m_iNumDeathNotices-1], 0, sizeof(DeathNoticeItem));// XDM: clear last freed position <- iSpriteIndex == bActive == 0 after this!
			//m_pDeathNoticeList[m_iNumDeathNotices-1].iSpriteIndex = HUDSPRITEINDEX_INVALID;
			--i;  // continue on the next item;  stop the counter getting incremented
			continue;
		}
		m_pDeathNoticeList[i].flDisplayTime = min(m_pDeathNoticeList[i].flDisplayTime, gHUD.m_flTime + m_pCvarDNTime->value);

		// Only draw if the viewport allows it
		if (gViewPort && gViewPort->AllowedToPrintText())
		{
			// Draw the death notice
			y = (int)((float)ScreenHeight*m_pCvarDNTop->value)/* + 2*/ + (DEATHNOTICE_MSG_HEIGHT * i);  //!!! // default DNTop->value is YRES(32)

			int iSpr = (m_pDeathNoticeList[i].iSpriteIndex == HUDSPRITEINDEX_INVALID) ? m_iDefaultSprite : m_pDeathNoticeList[i].iSpriteIndex;
			x = ScreenWidth - ConsoleStringLen(m_pDeathNoticeList[i].szVictim) - (gHUD.GetSpriteRect(iSpr).right - gHUD.GetSpriteRect(iSpr).left);

			if (m_pDeathNoticeList[i].bDrawKiller)// XDM3037a: m_pDeathNoticeList[i].iKillType != KILL_SELF)
			{
				x -= (DEATHNOTICE_TEXTSPACE + ConsoleStringLen(m_pDeathNoticeList[i].szKiller));
				// Draw killers name
				DrawSetTextColor(m_pDeathNoticeList[i].KillerColor[0], m_pDeathNoticeList[i].KillerColor[1], m_pDeathNoticeList[i].KillerColor[2]);
				x = DEATHNOTICE_TEXTSPACE + DrawConsoleString(x, y, m_pDeathNoticeList[i].szKiller);
			}
			// Draw weapon
			SPR_Set(gHUD.GetSprite(iSpr), m_pDeathNoticeList[i].WeaponColor[0], m_pDeathNoticeList[i].WeaponColor[1], m_pDeathNoticeList[i].WeaponColor[2]);
			SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(iSpr));
			x += (gHUD.GetSpriteRect(iSpr).right - gHUD.GetSpriteRect(iSpr).left);

			// Draw victims name (if it was a player that was killed)
			DrawSetTextColor(m_pDeathNoticeList[i].VictimColor[0], m_pDeathNoticeList[i].VictimColor[1], m_pDeathNoticeList[i].VictimColor[2]);
			x = DrawConsoleString(x, y, m_pDeathNoticeList[i].szVictim);
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Special color for the weapon icon
// Input  : *rgb - int[3] RGB
//			iKillType - KILLTYPE
//-----------------------------------------------------------------------------
void CHudDeathNotice::SetupWeaponColor(::Color &rgb, const byte &iKillType)
{
	int c = RGB_BLUE;
	switch (iKillType)
	{
	case KILL_NORMAL:	c = RGB_BLUE; break;
	case KILL_LOCAL:	c = RGB_CYAN; break;
	case KILL_SELF:		c = RGB_YELLOW; break;// no killer is drawn, so we need a different color
	case KILL_TEAM:		c = RGB_RED; break;
	case KILL_MONSTER:
		{
			rgb.Set(DN_COLOR_W_MONSTER);
			/*rgb[0] = DN_COLOR_W_MONSTER.r;
			rgb[1] = DN_COLOR_W_MONSTER.g;
			rgb[2] = DN_COLOR_W_MONSTER.b;*/
			return;// don't unpack
			break;
		}
	case KILL_THISPLAYER:	c = RGB_RED; break;
	case KILL_WORLD:
	case KILL_UNKNOWN:
	default:
		{
			rgb.Set(DN_COLOR_UNKNOWN);
			/*rgb[0] = DN_COLOR_UNKNOWN.r;
			rgb[1] = DN_COLOR_UNKNOWN.g;
			rgb[2] = DN_COLOR_UNKNOWN.b;*/
			return;// don't unpack
			break;
		}
	}
	//UnpackRGB(rgb[0], rgb[1], rgb[2], c);
	UnpackRGB(rgb, c);
}

#if defined (OLD_DEATHMSG)
//-----------------------------------------------------------------------------
// Purpose: XDM3035: real code now goes here
// TODO: decide kill type/flags/hints on the server!
// Input  : killer_index - entindex
//			victim_index - entindex
//			*killedwith - weapon name. When killer is a monster, monster's name.
// Warning: killedwith will be MODIFIED!
// Output : int
//-----------------------------------------------------------------------------
uint32 CHudDeathNotice::AddNotice(short killer_index, short victim_index, char *killedwith)
{
	if (m_pDeathNoticeList == NULL)
		return m_iNumDeathNotices;// since 0 is valid

	int prefix = 2;// "d_"
	const char *killer_name = NULL;
	const char *victim_name = NULL;
	uint32 i = 0;
	bool killer_player = false;
	bool victim_player = false;

	for (i = 0; i < m_iNumDeathNotices; ++i)
	{
		if (m_pDeathNoticeList[i].bActive == false)//if (m_pDeathNoticeList[i].iSpriteIndex <= 0)
			break;
	}
	if (i == m_iNumDeathNotices)// move the rest of the list forward to make room for this item
	{
		// move everything from and after position 1 to position 0 to free the last position
		memmove(&m_pDeathNoticeList[0], &m_pDeathNoticeList[1], sizeof(DeathNoticeItem) * (m_iNumDeathNotices-1));
		i = m_iNumDeathNotices - 1;
	}
	memset(&m_pDeathNoticeList[i], 0, sizeof(DeathNoticeItem));// XDM
	m_pDeathNoticeList[i].iKillType = KILL_UNKNOWN;
	m_pDeathNoticeList[i].iSpriteIndex = HUDSPRITEINDEX_INVALID;// sprite index
	m_pDeathNoticeList[i].bActive = true;

// -- Get the Victim's name
	// If victim is -1, the killer killed a specific, non-player object (like a sentrygun)
	if (IsValidPlayerIndex(victim_index))
	{
		victim_player = true;
		victim_name = g_PlayerInfoList[victim_index].name;
		GetTeamColor(g_PlayerExtraInfo[victim_index].teamnumber, m_pDeathNoticeList[i].VictimColor[0], m_pDeathNoticeList[i].VictimColor[1], m_pDeathNoticeList[i].VictimColor[2]);
		//m_pDeathNoticeList[i].VictimColor = GetClientColor(victim);

		if (victim_name)
			strncpy(m_pDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH*2);
		else
			conprintf(1, "CL: AddNotice(ki %d vi %d kw %s): victim with no name!\n", killer_index, victim_index, killedwith);

		m_pDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH*2-1] = 0;
	}
	else// victim is not a player (CoOp)
	{
		victim_player = false;
		_snprintf(m_pDeathNoticeList[i].szVictim, MAX_PLAYER_NAME_LENGTH*2, "monster %d\0", victim_index);
		m_pDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH*2-1] = 0;
		victim_name = m_pDeathNoticeList[i].szVictim;
		m_pDeathNoticeList[i].VictimColor.Set(DN_COLOR_W_MONSTER);
		/*m_pDeathNoticeList[i].VictimColor[0] = DN_COLOR_W_MONSTER.r;// Don't confuse with team color!
		m_pDeathNoticeList[i].VictimColor[1] = DN_COLOR_W_MONSTER.g;
		m_pDeathNoticeList[i].VictimColor[2] = DN_COLOR_W_MONSTER.b;*/
	}

// -- Get the Killer's name
	if (IsValidPlayerIndex(killer_index))// IMPORTANT: index fits into player info array bounds! (g_PlayerInfoList, etc.)
	{
		killer_player = true;
		killer_name = g_PlayerInfoList[killer_index].name;

		if (g_PlayerInfoList[killer_index].thisplayer && !IsTeamGame(gHUD.m_iGameType))// XDM3035
			Int2RGB(gHUD.m_iDrawColorCyan, m_pDeathNoticeList[i].KillerColor[0], m_pDeathNoticeList[i].KillerColor[1], m_pDeathNoticeList[i].KillerColor[2]);
		else// don't use the same cyan color for name highlighting in teamplay because it may match a team color
			GetTeamColor(g_PlayerExtraInfo[killer_index].teamnumber, m_pDeathNoticeList[i].KillerColor[0], m_pDeathNoticeList[i].KillerColor[1], m_pDeathNoticeList[i].KillerColor[2]);
		// we could probably use GetPlayerColor() here, but that's against the concept (colors have important significance here)

		if (killer_name)
			strncpy(m_pDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH*2);

		m_pDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
		m_pDeathNoticeList[i].iKillType = KILL_NORMAL;// default, overridden later
		m_pDeathNoticeList[i].bDrawKiller = true;// XDM3037a
	}
	else// killer is not a player
	{
		killer_player = false;
		if (strbegin(killedwith, "d_monster") > 0)
		{
			m_pDeathNoticeList[i].iKillType = KILL_MONSTER;
			// color
			//UnpackRGB(m_pDeathNoticeList[i].KillerColor[0], m_pDeathNoticeList[i].KillerColor[1], m_pDeathNoticeList[i].KillerColor[2], RGB_YELLOW);
			/*m_pDeathNoticeList[i].KillerColor[0] = DN_COLOR_W_MONSTER.r;// Don't confuse with team color!
			m_pDeathNoticeList[i].KillerColor[1] = DN_COLOR_W_MONSTER.g;
			m_pDeathNoticeList[i].KillerColor[2] = DN_COLOR_W_MONSTER.b;*/
			m_pDeathNoticeList[i].KillerColor.Set(DN_COLOR_W_MONSTER);
			// name
			killer_name = killedwith + 10;// skip "d_monster_"
			//strncpy(m_pDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH);
			_snprintf(m_pDeathNoticeList[i].szKiller, MAX_PLAYER_NAME_LENGTH*2-1, "%s %d\0", killer_name, killer_index);
			m_pDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH*2-1] = 0;
			//*killedwith = NULL;
			killedwith[9] = 0;// "d_monster" for icon
			//killer_index = -1;// NO! This fails suicide check
			m_pDeathNoticeList[i].bDrawKiller = true;// XDM3037a
		}
		else if (strcmp(killedwith, "d_worldspawn") == 0)
		{
			killer_name = nullstring;
			m_pDeathNoticeList[i].szKiller[0] = 0;
			m_pDeathNoticeList[i].iKillType = KILL_WORLD;// like in original HL
			/*m_pDeathNoticeList[i].KillerColor[0] = DN_COLOR_WORLD.r;// Don't confuse with team color!
			m_pDeathNoticeList[i].KillerColor[1] = DN_COLOR_WORLD.g;
			m_pDeathNoticeList[i].KillerColor[2] = DN_COLOR_WORLD.b;*/
			m_pDeathNoticeList[i].KillerColor.Set(DN_COLOR_WORLD);
			m_pDeathNoticeList[i].bDrawKiller = false;// XDM3037a
		}
		else// right now we have no means to get entity name, so we can only get something from killedwith string
		{
			killer_name = nullstring;
			strncpy(m_pDeathNoticeList[i].szKiller, killedwith+2, MAX_PLAYER_NAME_LENGTH*2);// XDM3037a: skip "d_"
			m_pDeathNoticeList[i].iKillType = KILL_UNKNOWN;
			/*m_pDeathNoticeList[i].KillerColor[0] = DN_COLOR_UNKNOWN.r;// Don't confuse with team color!
			m_pDeathNoticeList[i].KillerColor[1] = DN_COLOR_UNKNOWN.g;
			m_pDeathNoticeList[i].KillerColor[2] = DN_COLOR_UNKNOWN.b;*/
			m_pDeathNoticeList[i].KillerColor.Set(DN_COLOR_UNKNOWN);
			m_pDeathNoticeList[i].bDrawKiller = false;// XDM3037a
		}
	}

// -- Determine which iKillType is it
	if (killer_index == victim_index)// suicide -- CHECK FIRST!
	{
		m_pDeathNoticeList[i].iKillType = KILL_SELF;
		m_pDeathNoticeList[i].bDrawKiller = false;// XDM3037a
	}
	else if (killer_player && g_PlayerInfoList[killer_index].thisplayer)// local player scores
	{
		m_pDeathNoticeList[i].bDrawKiller = true;// XDM3037a
		client_textmessage_t *msg = TextMessageGet("KILL_LOCAL");
		if (msg)
		{
			_snprintf(m_szScoreMessage, DEATHNOTICE_MSG_LENGTH, msg->pMessage, victim_name);// should be "you killed %s"
			m_szScoreMessage[DEATHNOTICE_MSG_LENGTH-1] = '\0';// XDM3038c
			memcpy(&m_ScoreMessage, msg, sizeof(client_textmessage_t));// copy all message parameters
			//m_ScoreMessage = *msg;// copy localized message
			//m_ScoreMessage.pName = "KILL_LOCAL";
			m_ScoreMessage.pMessage = m_szScoreMessage;// point to a real existing string
			gHUD.m_Message.MessageAdd(&m_ScoreMessage);
		}

		// XDM3037: moved from server :) WARNING: victim_player && killer_player must be true! We use g_PlayerExtraInfo[] array!
		if (victim_player && IsTeamGame(gHUD.m_iGameType) && g_PlayerExtraInfo[killer_index].teamnumber != TEAM_NONE && g_PlayerExtraInfo[killer_index].teamnumber == g_PlayerExtraInfo[victim_index].teamnumber)
		{
			m_pDeathNoticeList[i].iKillType = KILL_TEAM;
			m_Message.r1 = RGB2R(DN_COLOR_TEAMMATE);// override color defined in titles.txt
			m_Message.g1 = RGB2G(DN_COLOR_TEAMMATE);
			m_Message.b1 = RGB2B(DN_COLOR_TEAMMATE);
			/*m_Message.r1 = DN_COLOR_TEAMMATE.r;// override color defined in titles.txt
			m_Message.g1 = DN_COLOR_TEAMMATE.g;
			m_Message.b1 = DN_COLOR_TEAMMATE.b;*/
		}
		else
			m_pDeathNoticeList[i].iKillType = KILL_LOCAL;
	}
	else if (victim_player && g_PlayerInfoList[victim_index].thisplayer)// local player was killed
	{
		m_pDeathNoticeList[i].iKillType = KILL_THISPLAYER;
		if (killer_player)// don't print "you were killed by world" or by a monster
		{
			//m_iLastKilledBy = killer_index;
			client_textmessage_t *msg = TextMessageGet("KILL_THISPLAYER");
			if (msg)
			{
				_snprintf(m_szMessage, DEATHNOTICE_MSG_LENGTH, msg->pMessage, killer_name, killedwith+prefix);
				m_szMessage[DEATHNOTICE_MSG_LENGTH-1] = '\0';// XDM3038c
				//m_Message = *msg;// copy localized message
				memcpy(&m_Message, msg, sizeof(client_textmessage_t));
				//m_Message.pName = "KILL_THISPLAYER";
				m_Message.pMessage = m_szMessage;
				gHUD.m_Message.MessageAdd(&m_Message);
			}
		}
		//else
		//	m_iLastKilledBy = 0;
	}
	else if (!killer_player && !victim_player)// probably a monster died
	{
		// don't change, default was set	m_pDeathNoticeList[i].iKillType = KILL_UNKNOWN;
		if (m_pDeathNoticeList[i].szVictim[0] == 0)
			strncpy(m_pDeathNoticeList[i].szVictim, killedwith+prefix, MAX_PLAYER_NAME_LENGTH*2);
			//strcpy(m_pDeathNoticeList[i].szVictim, killedwith+prefix);
	}
	//else// some other player scored/died/etc,
	//	conprintf(1, "CL: AddNotice(%d, %d, %s) error: unknown type!\n", killer_index, victim_index, killedwith);

// -- Setup weapon icon color (pre-setup for faster drawing!)
	SetupWeaponColor(m_pDeathNoticeList[i].WeaponColor, m_pDeathNoticeList[i].iKillType);

// -- Print everything
	// Find the sprite in the list
	m_pDeathNoticeList[i].iSpriteIndex = gHUD.GetSpriteIndex(killedwith);
	m_pDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + m_pCvarDNTime->value;// XDM3033

	// UNDONE: deal with all of the '\r' (13) and UTF-8 in these strings!
	// record the death notice in the console
	if (m_pCvarDNEcho->value > 0.0f)// 3033 XDM: SPAMSPAM!
	{
		const char *szFmt = NULL;
		const size_t iConsoleStringLen = 80;
		char szConsoleString[iConsoleStringLen];

		if (m_pDeathNoticeList[i].iKillType == KILL_WORLD)
		{
			client_textmessage_t *msg = TextMessageGet("DEATHNOTICE_WORLD");
			if (msg) szFmt = msg->pMessage; else szFmt = "* %s died";
			_snprintf(szConsoleString, iConsoleStringLen, szFmt, m_pDeathNoticeList[i].szVictim);
			szConsoleString[iConsoleStringLen-1] = '\0';
		}
		else if (m_pDeathNoticeList[i].iKillType == KILL_UNKNOWN)
		{
			if (m_pDeathNoticeList[i].szKiller && *m_pDeathNoticeList[i].szKiller)
			{
				client_textmessage_t *msg = TextMessageGet("DEATHNOTICE_UNKNOWN_W");
				if (msg) szFmt = msg->pMessage; else szFmt = "* %s caused %s's death";
				_snprintf(szConsoleString, iConsoleStringLen, szFmt, m_pDeathNoticeList[i].szKiller, m_pDeathNoticeList[i].szVictim);
				szConsoleString[iConsoleStringLen-1] = '\0';
			}
			else
			{
				client_textmessage_t *msg = TextMessageGet("DEATHNOTICE_UNKNOWN");
				if (msg) szFmt = msg->pMessage; else szFmt = "* %s died mysteriously";
				_snprintf(szConsoleString, iConsoleStringLen, szFmt, m_pDeathNoticeList[i].szVictim);
				szConsoleString[iConsoleStringLen-1] = '\0';
			}
		}
		else
		{
			if (m_pDeathNoticeList[i].iKillType == KILL_SELF)
			{
				client_textmessage_t *msg = TextMessageGet("DEATHNOTICE_SELF");
				if (msg) szFmt = msg->pMessage; else szFmt = "* %s killed self";
				_snprintf(szConsoleString, iConsoleStringLen, szFmt, m_pDeathNoticeList[i].szVictim);
				szConsoleString[iConsoleStringLen-1] = '\0';
			}
			else if (m_pDeathNoticeList[i].iKillType == KILL_TEAM)
			{
				client_textmessage_t *msg = TextMessageGet("DEATHNOTICE_ALLY");
				if (msg) szFmt = msg->pMessage; else szFmt = "* %s killed ally %s";
				_snprintf(szConsoleString, iConsoleStringLen, szFmt, m_pDeathNoticeList[i].szKiller, m_pDeathNoticeList[i].szVictim);
				szConsoleString[iConsoleStringLen-1] = '\0';
			}
			//else if (m_pDeathNoticeList[i].iKillType == KILL_UNKNOWN)
			//{
			//}
			else// if (m_pDeathNoticeList[i].szKiller[0])
			{
				client_textmessage_t *msg = TextMessageGet("DEATHNOTICE_NORMAL");
				if (msg) szFmt = msg->pMessage; else szFmt = "* %s killed %s";
				_snprintf(szConsoleString, iConsoleStringLen, szFmt, m_pDeathNoticeList[i].szKiller, m_pDeathNoticeList[i].szVictim);
				//_snprintf(szConsoleString, l, "* %s killed %s", m_pDeathNoticeList[i].szKiller, m_pDeathNoticeList[i].szVictim);
				szConsoleString[iConsoleStringLen-1] = '\0';
			}

			if (killedwith && *killedwith && isprint(*killedwith) &&
				m_pDeathNoticeList[i].iKillType != KILL_WORLD &&
				m_pDeathNoticeList[i].iKillType != KILL_MONSTER)
				// && strcmp(killedwith, "d_worldspawn") != 0 && strcmp(killedwith, "d_monster") != 0)
			{
				// UNDONE: should be "with %s", but requires more buffering
				client_textmessage_t *msg = TextMessageGet("DEATHNOTICE_WITH");
				if (msg) szFmt = msg->pMessage; else szFmt = " with ";
				strncat(szConsoleString, szFmt, iConsoleStringLen);//strncat(szConsoleString, " with ", iConsoleStringLen);
				szConsoleString[iConsoleStringLen-1] = '\0';
				strncat(szConsoleString, killedwith+prefix, iConsoleStringLen);// skip over the "d_" part
				szConsoleString[iConsoleStringLen-1] = '\0';
			}
		}
		strncat(szConsoleString, "\n\0", 2);
		ConsolePrint(szConsoleString);
	}
	return i;
}
#else // OLD_DEATHMSG
#error TODO: UNDONE: new deathmsg not implemented

//-----------------------------------------------------------------------------
// Purpose: XDM3035: real code now goes here
// TODO: decide kill type/flags/hints on the server!
// Input  : killer_index - entindex
//			victim_index - entindex
//			*killedwith - weapon name. When killer is a monster, monster's name.
// Warning: killedwith will be MODIFIED!
// Output : int
//-----------------------------------------------------------------------------
uint32 CHudDeathNotice::AddNotice(short iKiller, short iVictim, short iKillerIs, short iVictimIs, short iRelation, int iWeaponID, char *noticestring)
{
	if (m_pDeathNoticeList == NULL)
		return m_iNumDeathNotices;// since 0 is valid

	uint32 i = 0;
	for (i = 0; i < m_iNumDeathNotices; ++i)
	{
		if (m_pDeathNoticeList[i].bActive == false)//if (m_pDeathNoticeList[i].iSpriteIndex <= 0)
			break;
	}
	if (i == m_iNumDeathNotices)// move the rest of the list forward to make room for this item
	{
		// move everything from and after position 1 to position 0 to free the last position
		memmove(&m_pDeathNoticeList[0], &m_pDeathNoticeList[1], sizeof(DeathNoticeItem) * (m_iNumDeathNotices-1));
		i = m_iNumDeathNotices - 1;
	}
	memset(&m_pDeathNoticeList[i], 0, sizeof(DeathNoticeItem));// XDM
	m_pDeathNoticeList[i].iKillType = KILL_UNKNOWN;
	m_pDeathNoticeList[i].iSpriteIndex = HUDSPRITEINDEX_INVALID;// sprite index

	if (iKillerIs == ENTIS_PLAYER)
	{
	}
	else if (iKillerIs == ENTIS_HUMAN || iKillerIs == ENTIS_MONSTER)
	{
	}
	return i;
}

#endif // OLD_DEATHMSG


//-----------------------------------------------------------------------------
// This message handler may be better off elsewhere
// WARNING: indexes are NOT ALWAYS player indexes!
//-----------------------------------------------------------------------------
int CHudDeathNotice::MsgFunc_DeathMsg(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	short killer_index = READ_SHORT();// WARNING: incompatibility: XDM3035a
	short victim_index = READ_SHORT();// WARNING! killer index is an ENTITY INDEX!

#if defined (OLD_DEATHMSG)
	char noticestring[32];
	noticestring[0] = '\0';
	char *pString = READ_STRING();
	strcpy(noticestring, "d_");
	if (pString && *pString != '\0')
		strncat(noticestring, pString, 32);
	else
		strncat(noticestring, "unknown", 8);

	//conprintf(1, "CL: DeathMsg(ki %hd, vi %hd, kw %s)\n", killer_index, victim_index, noticestring);
#else
#error TODO: UNDONE:
	short killertype = READ_BYTE();
	short victimtype = READ_BYTE();
	short relation = READ_BYTE();
	int weaponindex = READ_BYTE();
	//if (weaponindex == WEAPON_NONE)
	char *pString = READ_STRING();

	conprintf(1, "CL: DeathMsg(ki %hd, vi %hd, kt %hd, vt %hd, rl %hd, w %d, str %s)\n", killer_index, victim_index, killertype, victimtype, relation, weaponindex, noticestring);
#endif
	END_READ();

	SetActive(true);

	if (IsValidPlayerIndex(killer_index))
	{
		GetPlayerInfo(killer_index, &g_PlayerInfoList[killer_index]);
		g_PlayerExtraInfo[killer_index].lastscoretime = gHUD.m_flTime;
	}
	if (IsValidPlayerIndex(victim_index))
	{
		GetPlayerInfo(victim_index, &g_PlayerInfoList[victim_index]);
		gHUD.m_Spectator.DeathMessage(victim_index);
	}

	if (gViewPort)
		gViewPort->DeathMsg(killer_index, victim_index);

#if defined (OLD_DEATHMSG)
	AddNotice(killer_index, victim_index, noticestring);
#else
	AddNotice(killer_index, victim_index, killertype, victimtype, weaponindex, noticestring);
#endif
	return 1;
}
