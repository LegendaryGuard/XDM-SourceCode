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
#ifndef HUD_H
#define HUD_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#ifndef _WIN32
#define _cdecl 
#endif

#include "cl_dll.h"
#include "gamedefs.h"// XDM
#include "playerstats.h"// XDM3037
#include "player_info.h"// HL20130901
#include "ammo.h"

#if defined(_DEBUG_HUD)
#define DBG_HUD_PRINTF		DBG_PrintF
#else
#define DBG_HUD_PRINTF
#endif

// Utilities for angles/pitch/SQB debugging
#if defined (_DEBUG_ANGLES)
#define DBG_ANGLES_NPRINT							gEngfuncs.Con_NPrintf
#define DBG_ANGLES_DRAW(num, origin, angles, client, text)	{ if (g_pCvarDbgAngles->value > 0.0f && (g_pCvarDbgAnglesClient->value == 0 || (int)g_pCvarDbgAnglesClient->value == client)) {if (num > 0 && g_pCvarDbgAngles->value == num){angles[PITCH] *= g_pCvarDbgAnglesMult->value; UTIL_DebugAngles(origin, angles, 0.5, 32);} gEngfuncs.Con_NPrintf(num, "%s%s: %f", (g_pCvarDbgAngles->value == num)?"> ":"  ", text, angles[PITCH]);} }
#else
#define DBG_ANGLES_NPRINT
#define DBG_ANGLES_DRAW
#endif

// HL CL legacy
enum hud_color_id_e
{
	RGB_DEFAULT = 0,
	RGB_GREEN,
	RGB_YELLOW,
	RGB_BLUE,
	RGB_RED,
	RGB_CYAN,
};

// Flags for drawing large dprite digits
#define DHN_DRAWZERO				1
#define DHN_2DIGITS					2
#define DHN_3DIGITS					4

#define MAX_ALPHA					255// used in rare occasions like change of hud elements
#define MIN_ALPHA					127// used during normal hud state
#define WEAPONLIST_ALPHA			191// normal alpha to draw weapons with
#define FADE_TIME					100
#define HUD_FADE_SPEED				20// default fade rate

#define MAX_SPRITE_NAME_LENGTH		24

#define HUD_DISTORT_POS_MAX_RADIUS	4

#define ANNOUNCEMENT_MSG_LENGTH		128

#define HUDSPRITEINDEX_INVALID		-1


//
//-----------------------------------------------------
//
// CHudBase flags
#define HUD_ACTIVE					0x01
#define HUD_INTERMISSION			0x02// draw even during intermission
#define HUD_DRAW_ALWAYS				0x04// XDM: ignore HIDEHUD_ALL
#define HUD_NORESET					0x08// XDM: ignore Reset()


class CHudBase
{
public:
	CHudBase() { m_iFlags = 0; m_iWidth = 0; m_iHeight = 0; }
	virtual ~CHudBase() {}
	virtual int Init(void) {return 0;}
	virtual int VidInit(void) {return 0;}
	virtual int Draw(const float &flTime) {return 0;}// valid place to draw HUD sprites
	virtual void Think(void) {}// called every frame by CHud::Think()
	virtual void Reset(void) {}// called by CHud::Reset()
	virtual void InitHUDData(void) {}// called by Msg InitHUD
	virtual bool IsActive(void);
	//virtual int GetWidth(void) { return m_iWidth; }
	//virtual int GetHeight(void) { return m_iHeight; }
	void SetActive(bool active);

	//POSITION m_pos;
	uint32 m_iFlags;
	int m_iWidth;// uint32 is incompatible with HL code
	int m_iHeight;
};

struct HUDLIST {
	CHudBase	*p;
	HUDLIST		*pNext;
};

//
//-----------------------------------------------------
//
#include "hud_spectator.h"// CHudSpectator
#include "health.h"// CHudHealth

//
//-----------------------------------------------------
//
#include "ammohistory.h"

#define HUD_WEAPON_SELECTION_MARGIN		8
#define HUD_WEAPON_SELECTION_SPACING_X	5
#define HUD_WEAPON_SELECTION_SPACING_Y	5
#define HUD_WEAPON_SELECTION_TIME		5

// XDM3037a
enum crosshair_mode_e
{
	CROSSHAIR_OFF = 0,
	CROSSHAIR_NORMAL,
	CROSSHAIR_AUTOAIM
};

class CHudAmmo: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual void InitHUDData(void);
	virtual void Reset(void);
	virtual int Draw(const float &flTime);
	virtual void Think(void);

	void DrawAmmoBar(HUD_WEAPON *p, int x, int y, int width, int height);
	int DrawWList(const float &flTime);
	void SetCurrentCrosshair(HLSPRITE hspr, wrect_t &rc, int r, int g, int b);// XDM3038c
	void UpdateCrosshair(int mode);// XDM3038c
	int InitWeaponSlots(void);// XDM3035b
	void UpdateWeaponBits(void);// const int &iNewWeaponBits);// XDM3038c
	void UpdateCurrentWeapon(const int &iID);// XDM3038
	void OnWeaponChanged(const int &iID);// XDM3038b
	int GetWeaponSlotPos(const int &iId, short &wslot, short &wpos);
	int GetNextBestItem(const int iCurrentID);// XDM3037
	int GetActiveWeapon(void) { return m_iActiveWeapon; }
	void SelectItem(const int iItemID);// XDM3037
	bool ShouldSwitchWeapon(const int iNewItemID);// XDM3037
	//bool LoadAmmoSprite(const byte &iAmmoID);// XDM3037
	uint32 LoadAmmoSprites(void);// XDM3038c
	short MaxAmmoCarry(const byte &iAmmoID);// XDM3037: must be signed
	HLSPRITE AmmoSpriteHandle(const byte &iAmmoID);// XDM3037
	wrect_t &AmmoSpriteRect(const byte &iAmmoID);// XDM3037
	//int GetAmmoDisplayPosY(void) { return m_iAmmoDisplayY; }
	void SlotInput(short iSlot);
	void SlotClose(void);
	void WeaponPickup(const int &iID);// XDM3038a
	void WeaponRemove(const int &iID);// XDM3038c

	int MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoList(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ItemPickup(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HideWeapon(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_UpdWeapons(const char *pszName, int iSize, void *pbuf);//XDM3035
	int MsgFunc_UpdAmmo(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_SelBestItem(const char *pszName, int iSize, void *pbuf);// XDM3037

	void _cdecl UserCmd_Slot1(void);
	void _cdecl UserCmd_Slot2(void);
	void _cdecl UserCmd_Slot3(void);
	void _cdecl UserCmd_Slot4(void);
	void _cdecl UserCmd_Slot5(void);
	void _cdecl UserCmd_Slot6(void);
	void _cdecl UserCmd_Slot7(void);
	void _cdecl UserCmd_Slot8(void);
	void _cdecl UserCmd_Close(void);
	void _cdecl UserCmd_SelectWeapon(void);// XDM3038
	void _cdecl UserCmd_NextWeapon(void);
	void _cdecl UserCmd_PrevWeapon(void);
	void _cdecl UserCmd_InvUp(void);// XDM
	void _cdecl UserCmd_InvDown(void);
	void _cdecl UserCmd_InvLast(void);// XDM3038
	void _cdecl UserCmd_Holster(void);// XDM3038
//#if defined (_DEBUG)
	void _cdecl UserCmd_DumpInventory(void);// XDM3038c
//#endif

	cvar_t	*m_pCvarDrawHistoryTime;
	cvar_t	*m_pCvarDrawAccuracy;// XDM3038c
	cvar_t	*m_pCvarFastSwitch;
	cvar_t	*m_pCvarWeaponPriority;// XDM3037
	cvar_t	*m_pCvarWeaponSlots;// XDM3035b
	cvar_t	*m_pCvarSwitchOnPickup;// XDM3037
	cvar_t	*m_pCvarWeaponSelectionTime;// XDM3037

	WeaponsResource gWR;// XDM3038a: hack?
	HistoryResource gHR;// XDM3038c

protected:
#if !defined(SERVER_WEAPON_SLOTS)
	char m_szWeaponPriorityConfig[128];
	char m_szWeaponSlotConfig[128];
	char m_szToken[1024];// XDM3037: for internal parser
#endif
	HUD_AMMO m_AmmoInfoArray[MAX_AMMO_SLOTS];// XDM3037
	HUD_WEAPON *m_pWeapon;
	//HUD_WEAPON *m_pLastWeapon;// XDM3038
	HUD_WEAPON *m_pActiveSel;// NULL means off, wNoSelection means just the menu bar, otherwise this points to the active weapon menu item
	//HUD_WEAPON *m_pLastSel;// Last weapon menu selection 
	HLSPRITE m_hCurrentCrosshair;// XDM3038c
	wrect_t m_CurrentCrosshairRect;// XDM3038c
	uint32 m_iAmmoTypes;// XDM3037
	int	m_HUD_bucket0;
	int	m_HUD_bucket_flat;// XDM3037
	int m_HUD_selection;
	int m_iHUDSpriteSpread;
	wrect_t m_rcSpread;
	uint32 m_iOldWeaponBits;// XDM3038c: to detect difference
	int m_iActiveWeapon;// XDM3038
	int m_iPrevActiveWeapon;// XDM3038
	int m_iRecentPickedWeapon;// for announcements
	//int m_iAmmoDisplayY;// XDM3037
	int m_iBucketHeight;
	int m_iBucketWidth;
	int m_iABHeight;// Ammo Bar width and height
	int m_iABWidth;
	int m_iSelectNextBestItemCache;// XDM3038a: a trick to wait for gHUD.m_iWeaponBits to arrive
	float m_fShootSpread;// XDM3038c
	float m_fFade;
	float m_fSelectorShowTime;// XDM3037
};

//
//-----------------------------------------------------
//
/*class CHudAmmoSecondary: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual void Reset(void);
	virtual int Draw(const float &flTime);

//	int MsgFunc_SecAmmoVal(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_SecAmmoIcon(const char *pszName, int iSize, void *pbuf);

protected:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};*/


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	virtual void Reset(void);
	int MsgFunc_Geiger(const char *pszName, int iSize, void *pbuf);

protected:
	int m_iGeigerRange;
};

//
//-----------------------------------------------------
//
class CHudTrain: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	int MsgFunc_Train(const char *pszName, int iSize, void *pbuf);

protected:
	HLSPRITE m_hSprite;
	int m_iPos;
};

//
//-----------------------------------------------------
//
#define MAX_STATUSBAR_LINES		4// client HUD lines to draw
#define MAX_STATUSBAR_LINELEN	128

class CHudStatusBar : public CHudBase
{
public:
	virtual int Init(void);
	virtual int Draw(const float &flTime);
	virtual void Reset(void);

	void ParseStatusString(void);
	int MsgFunc_StatusData(const char *pszName, int iSize, void *pbuf);// XDM3037

	cvar_t	*m_pCvarCenterId;

protected:
	::Color m_LineColors[MAX_STATUSBAR_LINES];// an array of colors...one color for each line
	char m_szStatusText[SBAR_STRING_SIZE];// unlocalized string name "#MYTITLE"
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSBAR_LINELEN];// after localization, one string may become many
	//char m_szClientTraceText[MAX_STATUSBAR_LINELEN];// XDM3035
	// UNDONE	short m_iStatusValues[MAX_STATUSBAR_LINES][SBAR_NUMVALUES];// an array of values for use in the status bar
	short m_iStatusValues[SBAR_NUMVALUES];// an array of values for use in the status bar
	bool m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated
};

//
//-----------------------------------------------------
//
#define DEATHNOTICE_MSG_LENGTH		128// XDM3038c
#define DEATHNOTICE_MSG_HEIGHT		YRES(10)
//#define KILL_AWARD_TIME				3.0// XDM: UT FX
#define MAX_DEATHNOTICES			32// XDM3035: absolute maximum
//#define DEATHNOTICE_TOP				32

// Death notices
// These are for display only, not for network.
typedef enum cl_killtype_e
{
	KILL_NORMAL = 0,// somebody killed somebody
	KILL_LOCAL,		// local player killed somebody
	KILL_SELF,		// somebody killed self
	KILL_TEAM,		// somebody killed a teammate
	KILL_MONSTER,	// monster killed somebody
	KILL_THISPLAYER,// somebody killed local player
	KILL_WORLD,		// XDM3037a: world killed somebody
	KILL_UNKNOWN,
} cl_killtype_t;

struct DeathNoticeItem
{
	char szKiller[MAX_PLAYER_NAME_LENGTH*2];
	char szVictim[MAX_PLAYER_NAME_LENGTH*2];
	int iSpriteIndex;// must be signed for -1
	::Color KillerColor;
	::Color VictimColor;
	::Color WeaponColor;
	float flDisplayTime;
	short bDrawKiller;// XDM3037a: explicit
	short iKillType;// CL_KILLTYPE
	short bActive;// XDM3038c: 0 = free, 1 = active
};

class CHudDeathNotice : public CHudBase
{
public:
	CHudDeathNotice();
	virtual ~CHudDeathNotice();
	virtual int Init(void);
	virtual void InitHUDData(void);
	virtual int VidInit(void);
	virtual void Reset(void);
	virtual int Draw(const float &flTime);

	void FreeData(void);
	void SetupWeaponColor(::Color &rgb, const byte &iKillType);// XDM: don't bother optimizing to byte[3] because engine still requires integers
#ifdef OLD_DEATHMSG
	uint32 AddNotice(short killer_index, short victim_index, char *killedwith);
#else
	uint32 AddNotice(short iKiller, short iVictim, short iKillerIs, short iVictimIs, short iRelation, int iWeaponID, char *noticestring);
#endif
	int MsgFunc_DeathMsg(const char *pszName, int iSize, void *pbuf);

	cvar_t	*m_pCvarDNNum;// XDM3035
	cvar_t	*m_pCvarDNTime;
	cvar_t	*m_pCvarDNEcho;
	cvar_t	*m_pCvarDNTop;// XDM3035

protected:
	int		m_iDefaultSprite;
	char	m_szScoreMessage[DEATHNOTICE_MSG_LENGTH];// XDM
	char	m_szMessage[DEATHNOTICE_MSG_LENGTH];// XDM
	client_textmessage_t m_ScoreMessage;
	client_textmessage_t m_Message;

private:
	DeathNoticeItem *m_pDeathNoticeList;// XDM
	uint32	m_iNumDeathNotices;// XDM
};

//
//-----------------------------------------------------
//
#define MAX_LINES			32
#define MAX_CHARS_PER_LINE	256/* it can be less than this, depending on char size */
// Left margin
#define LINE_START			10
// allow 20 pixels on either side of the text
#define MAX_LINE_WIDTH		(ScreenWidth - 40)


class CHudSayText : public CHudBase
{
friend class CHudSpectator;
public:
	CHudSayText();
	virtual ~CHudSayText();
	virtual int Init(void);
	virtual void InitHUDData(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);

	void FreeData(void);
	int ScrollTextUp(void);
	void SayTextPrint(const char *pszBuf, byte clientIndex, bool teamonly = false);
	void EnsureTextFitsInOneLineAndWrapIfHaveTo(int line);

	int MsgFunc_SayText(const char *pszName, int iSize, void *pbuf);

	char	m_szLocalizedTeam[MAX_TEAMNAME_LENGTH];// XDM: localized "TEAM" word
	float	m_flScrollTime;// the time at which the lines next scroll up
	int		Y_START;
	int		line_height;
	cvar_t	*m_pCvarSayText;
	cvar_t	*m_pCvarSayTextTime;
	cvar_t	*m_pCvarSayTextHighlight;
	cvar_t	*m_pCvarSayTextLines;

protected:
	struct SayTextItem
	{
		size_t iNameLength;// it'll be converted to size_t in string operations anyway
		char szLineBuffer[MAX_CHARS_PER_LINE];
		::Color StringColor;
		::Color NameColor;
	};

	SayTextItem *m_pSayTextList;// XDM
	size_t		m_iMaxLines;// XDM
};

//
//-----------------------------------------------------
//
class CHudBattery: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual void Reset(void);
	virtual int Draw(const float &flTime);
	//int GetBatteryValue(void) { return m_iBat; }
	int MsgFunc_Battery(const char *pszName, int iSize, void *pbuf);

	int		m_iBat;

protected:
	HLSPRITE	m_hSpriteEmpty;
	HLSPRITE	m_hSpriteFull;
	HLSPRITE	m_hSpriteDivider;
	wrect_t		*m_prcEmpty;
	wrect_t		*m_prcFull;
	wrect_t		*m_prcDivider;
	float		m_fFade;
};

//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	virtual void Reset(void);

	void LongJump(void);// did not want to create a separate class for just one icon
	int MsgFunc_Flashlight(const char *pszName, int iSize, void *pbuf);

protected:
	HLSPRITE m_hSpriteMain;
	HLSPRITE m_hSpriteFull;
	HLSPRITE m_hSpriteOn;
	HLSPRITE m_hLJM;
	wrect_t *m_prcMain;
	wrect_t *m_prcFull;
	wrect_t *m_prcBeam;
	wrect_t *m_prcLJM;
	float	m_flBat;	
	float	m_fFadeLJ;
	int		m_fOn;
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;

struct message_parms_t
{
	client_textmessage_t *pMessage;
	float time;
	int x, y;
	int	totalWidth, totalHeight;
	int width;
	uint32 lines;
	uint32 lineLength;
	uint32 length;
	int r, g, b;// engine eats integers anyway...
	int text;// a char actually
	int fadeBlend;
	float charTime;
	float fadeTime;
};

// client_textmessage_t name length
#define MAX_TITLE_NAME		32

//
//-----------------------------------------------------
//
/* OBSOLETE class CHudTextMessage: public CHudBase
{
public:
	virtual int Init(void);
};*/

//
//-----------------------------------------------------
//
class CHudMessage: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	virtual void Reset(void);

	float FadeBlend(const float &fadein, const float &fadeout, const float &hold, const float &localTime);
	int	XPosition(const float &x, int width, int lineWidth);
	int YPosition(const float &y, int height);

	void MessageAdd(const char *pName, float fStartTime);
	void MessageAdd(client_textmessage_t *newMessage);
	void MessageDrawScan(client_textmessage_t *pMessage, const float &time);
	void MessageScanStart(void);
	void MessageScanNextChar(void);

	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
//?	int MsgFunc_HudTextPro(const char *pszName, int iSize, void *pbuf);// HL20130901
	int MsgFunc_GameTitle(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);// XDM3038: somplified, optimized

protected:
	client_textmessage_t	*m_pMessages[maxHUDMessages];
	float					m_startTime[maxHUDMessages];
	message_parms_t			m_parms;
	float					m_gameTitleTime;
	client_textmessage_t	*m_pGameTitle;
	int						m_HUD_title_life;
	int						m_HUD_title_half;
};

//
//-----------------------------------------------------
//
#define STATUS_ICONS_MARGIN		YRES(4)
#define MAX_ICONSPRITES			8

class CHudStatusIcons: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual void Reset(void);
	virtual int Draw(const float &flTime);
	void EnableIcon(const char *pszIconName, byte r, byte g, byte b, short holdtime, short fadetime);
	void DisableIcon(const char *pszIconName);

	int MsgFunc_StatusIcon(const char *pszName, int iSize, void *pbuf);

	cvar_t	*m_pCvarDrawStatusIcons;// XDM3037a

private:
	typedef struct icon_sprite_s
	{
		char szSpriteName[MAX_SPRITE_NAME_LENGTH];
		HLSPRITE spr;
		wrect_t rc;
		int frame;// XDM3037a
		float endholdtime;// XDM3037a
		float endfadetime;// XDM3037a
		// UNDONE	short shakeamplitude;
		::Color color;// XDM3038c: there was padding here anyway
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES];
};

//
//-----------------------------------------------------
//
class CHudZoomCrosshair: public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	virtual void Reset(void);
	virtual void InitHUDData(void);
	virtual void Think(void);
	void SetParams(int spr_idx, int rendermode, int soundindex, float finalfov);// rendermode -1 to disable

private:
	float m_fFinalFOV;
	int m_iRenderMode;
	int m_iTextureIndex;
	int m_iSoundIndex;// XDM3038a: almost fail
	float m_fPlaySoundTime;
	struct model_s *m_pTexture;
};

//
//-----------------------------------------------------
//
class CHudRocketScreen : public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);

private:
	HLSPRITE	m_hSprText;
	int m_iOffset;
	int m_iFrames;
	int m_iCurFrame;
};

// XDM3038a
//-----------------------------------------------------
//
#define SCORE_ICON_FADE_RATE	30

class CHudGameDisplay : public CHudBase
{
public:
	virtual int Init(void);
	virtual void InitHUDData(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);

	cvar_t *m_pCvarDrawScore;
private:
	int	m_iIconScore;
	int	m_iIconRespawns;
	int m_iLastScore;
	int m_iLastScoreChange;
	int m_iLastRespawns;
	int m_iAlphaScore;
	int m_iAlphaRespawns;
};

//
//-----------------------------------------------------
//
#define DOM_NAME_LENGTH		64
#define DOM_MSG_LENGTH		(64 + DOM_NAME_LENGTH)
#define DOM_ICON_FADE_TIME	40

class CHudDomDisplay : public CHudBase
{
public:
	virtual int Init(void);
	virtual void InitHUDData(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	void SetEntTeam(int entindex, TEAM_ID teamindex);
	int MsgFunc_DomInfo(const char *pszName, int iSize, void *pbuf);

private:
	int	m_iIconPoint;
	int	m_iIconHighlight;
	size_t m_iNumDomPoints;
	int m_iDomPointEnts[DOM_MAX_POINTS];
	TEAM_ID m_iDomPointTeam[DOM_MAX_POINTS];
	float m_fDomPointFade[DOM_MAX_POINTS];
	char m_szDomPointNames[DOM_MAX_POINTS][DOM_NAME_LENGTH];
	char m_szMessage[DOM_MSG_LENGTH];
	client_textmessage_t m_Message;
	//HLSPRITE m_hsprOvwIcon;
};

//
//-----------------------------------------------------
//
#define MAX_CAPTURE_ENTS	2
#define CTF_ICON_FADE_TIME	30

class CHudFlagDisplay : public CHudBase
{
public:
	virtual int Init(void);
	virtual void InitHUDData(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	virtual void Reset(void);
	int MsgFunc_FlagInfo(const char *pszName, int iSize, void *pbuf);
	void SetEntState(int entindex, int team, int state);

private:
	int	m_iIconNormal;
	int	m_iIconTaken;
	int	m_iIconDropped;
	int	m_iIconHighlight;
	size_t m_iNumFlags;
	int m_iFlagEnts[MAX_CAPTURE_ENTS];
	TEAM_ID m_iFlagTeam[MAX_CAPTURE_ENTS];
	int m_iFlagState[MAX_CAPTURE_ENTS];
	float m_fFlagFade[MAX_CAPTURE_ENTS];
	//HLSPRITE m_hsprOvwIcon;
};

// XDM3038c
//-----------------------------------------------------
//
#define TIMER_DIGIT_FADE_RATE	60

class CHudTimer : public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	virtual void Reset(void);
	int MsgFunc_HUDTimer(const char *pszName, int iSize, void *pbuf);

private:
	HLSPRITE m_hIconTimer;
	wrect_t *m_prcIconTimer;
	//int m_iSeconds;
	//float m_fStartTime;
	float m_fEndTime;
	int m_iSecondsLast;
	int m_iAlpha;
	char m_szTitle[512];
};

//
//-----------------------------------------------------
//
class CHudBenchmark : public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);// XDM3038
	virtual void Think(void);

	void SetScore(float score);
	void StartNextSection(int section);
	void CountFrame(float dt);
	int GetObjects(void) { return m_nObjects; };
	void SetCompositeScore(void);
	void Restart(void);
	int Bench_ScoreForValue(int stage, float raw);
	int MsgFunc_Bench(const char *pszName, int iSize, void *pbuf);

private:
	float	m_fDrawTime;
	float	m_fDrawScore;
	float	m_fAvgScore;

	float   m_fSendTime;
	float	m_fReceiveTime;

	int		m_nFPSCount;
	float	m_fAverageFT;
	float	m_fAvgFrameRate;

	int		m_nSentFinish;
	float	m_fStageStarted;

	float	m_StoredLatency;
	float	m_StoredPacketLoss;
	int		m_nStoredHopCount;
	int		m_nTraceDone;

	int		m_nObjects;

	int		m_nScoreComputed;
	int 	m_nCompositeScore;
};

//
//-----------------------------------------------------
//


class CHud
{
public:
	CHud();
	~CHud();

	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Think(void);
	virtual void Reset(bool bForce);
	virtual int Redraw(const float &flTime, const int &intermission);
	virtual int UpdateClientData(client_data_t *cdata, const float &time);
	virtual void OnGamePaused(const int &paused);// XDM
	virtual void OnGameActivated(const int &active);// XDM

	bool LoadHUDSprites(const char *listname, bool additional);
	void AddHudElem(CHudBase *p);
	int GetHUDBottomLine(void);// XDM3038c

	int DrawHudString(int x, int y, int iMaxX, const char *szString, int r, int g, int b);
	int DrawHudStringReverse(int xpos, int ypos, int iMinX, const char *szString, const int &r, const int &g, const int &b);
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, const int &r, const int &g, const int &b);
	int DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, const int &r, const int &g, const int &b);
	int GetNumWidth(const int &iNumber, const int &iFlags);
	void SetFOV(const float &fFieldOfView);// XDM3037
	float GetCurrentFOV(void);// XDM3037a
	float GetUpdatedDefaultFOV(void);// XDM
	float GetSensitivity(void);

	HLSPRITE GetSprite(const int &index) { return (index <= HUDSPRITEINDEX_INVALID) ? 0 : m_rghSprites[index]; }
	wrect_t &GetSpriteRect(const int &index) { return m_rgrcRects[index]; }
	int GetSpriteIndex(const char *SpriteName);	// gets a sprite index, for use in the m_rghSprites[] array
	int GetSpriteIndex(HLSPRITE hSprite);// XDM3037a
	int GetSpriteFrame(const int &index);// XDM3037a

	void IntermissionStart(void);// XDM3035
	void IntermissionEnd(void);// XDM3035
	bool IsSpectator(void);
	int GetSpectatorMode(void);// XDM3038a
	int GetSpectatorTarget(void);// XDM3038a: cl_entity_t *?
	bool PlayerHasSuit(void);// XDM3038: should be the most reliable check
	bool PlayerIsAlive(void);// XDM3038a
	void DistortionReset(void);// XDM3038

	void CheckRemainingScoreAnnouncements(void);// XDM3035a
	void CheckRemainingTimeAnnouncements(void);// XDM3035a
	void GameRulesEndGame(void);// XDM3035c
	void GameRulesEvent(int gameevent, short data1, short data2);// XDM3035c

	// user messages
	int _cdecl MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_Logo(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_GRInfo(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_GREvent(const char *pszName, int iSize, void *pbuf);
	//int _cdecl MsgFunc_Concuss(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_HUDEffects(const char *pszName, int iSize, void *pbuf);// XDM3038

	CHudAmmo m_Ammo;
	CHudHealth m_Health;
	CHudSpectator m_Spectator;
	CHudGeiger m_Geiger;
	CHudBattery	m_Battery;
	CHudTrain m_Train;
	CHudFlashlight m_Flash;
	CHudMessage m_Message;
	CHudStatusBar m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText m_SayText;
	//CHudAmmoSecondary m_AmmoSecondary;
	// XDM3038: OBSOLETE	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	//CHudLightLevel m_LightLevel;// XDM
	CHudZoomCrosshair m_ZoomCrosshair;
	//CHudRocketScreen m_RocketScreen;
	CHudGameDisplay m_GameDisplay;// XDM3038a
	CHudDomDisplay m_DomDisplay;
	CHudFlagDisplay m_FlagDisplay;
	CHudTimer m_Timer;// XDM3038c
#if defined (ENABLE_BENCKMARK)
	CHudBenchmark m_Benchmark;
#endif

	// Screen information
	SCREENINFO	m_scrinfo;
	int			m_iRes;
	int			m_iFontHeight;
	// sprite indexes
	int			m_iHideHUDDisplay;
	int			m_HUD_number_0;
	int			m_iLogo;
	//HLSPRITE		m_hsprCursor;
	HLSPRITE		m_hsprLogo;
	// engine state
	float		m_flTime;// the current client time
	float		m_fOldTime;// last Redraw() time
	double		m_flTimeDelta;// the difference between flTime and fOldTime
	// HUD data
	client_textmessage_t m_MessageAward;
	client_textmessage_t m_MessageCombo;
	client_textmessage_t m_MessageTimeLeft;
	client_textmessage_t m_MessageScoreLeft;
	client_textmessage_t m_MessageAnnouncement;
	client_textmessage_t m_MessageExtraAnnouncement;
	uint32		m_iDrawColorMain;// 3 bytes RGB for fast conversion/usage
	uint32		m_iDrawColorRed;
	uint32		m_iDrawColorBlue;
	uint32		m_iDrawColorCyan;
	uint32		m_iDrawColorYellow;
	float		m_fFOV;// XDM3037a: UPD: this SHOULD normally be zero :)
	float		m_fFOVServer;// XDM3038a: sent from the server
	uint32		m_iDistortMode;// XDM3037: flags
	float		m_fDistortValue;// XDM3037: 0...1
	//float		m_fDistortEndTime;// XDM3037: I don't mind, but... how can this be restored over time?!
	cvar_t		*m_pCvarStealMouse;
	cvar_t		*m_pCvarDraw;
	cvar_t		*m_pCvarDrawNumbers;
	cvar_t		*m_pCvarEventIconTime;
	cvar_t		*m_pCvarUseTeamColor;
	cvar_t		*m_pCvarUsePlayerColor;
	cvar_t		*m_pCvarMiniMap;
	cvar_t		*m_pCvarColorMain;
	cvar_t		*m_pCvarColorRed;
	cvar_t		*m_pCvarColorBlue;
	cvar_t		*m_pCvarColorCyan;
	cvar_t		*m_pCvarColorYellow;
	cvar_t		*m_pCvarTakeShots;
	// world data
	int			m_iFogMode;
	int			m_iSkyMode;
	int			m_iCameraMode;
	vec3_t		m_vecSkyPos;
	float		m_flFogStart;
	float		m_flFogEnd;
	// player data
	//vec3_t		m_vecOrigin;
	//vec3_t		m_vecAngles;
	int			m_iKeyBits;
	uint32		m_iWeaponBits;// required at least for the suit
	int			m_iPlayerMaxHealth;
	int			m_iPlayerMaxArmor;// XDM3038
	int			m_iRandomSeed;// XDM3037
	//byte		m_fPlayerDead;
	short		m_bFrozen;// XDM3037
	TEAM_ID		m_iTeamNumber;
	// player data structures
	// SLOW entity_state_t	m_LocalPlayerState;// XDM
	// SLOW	clientdata_t	m_ClientData;// XDM3037a
	cl_entity_t		*m_pLocalPlayer;// XDM3037
	//Vector	m_vecViewOffset;// XDM3037: EV_LocalPlayerViewheight() does the same

	// multiplayer data
	short		m_iGameType;
	short		m_iGameMode;
	short		m_iGameState;// XDM3037a
	short		m_iGameSkillLevel;
	short		m_iGameFlags;
	short		m_iRevengeMode;
	int			m_fLastScoreAward;// XDM: last type of award (n in n-kill)
	char		m_szMessageAward[DEATHNOTICE_MSG_LENGTH];// XDM3035
	char		m_szMessageCombo[DEATHNOTICE_MSG_LENGTH];// XDM3035
	char		m_szMessageTimeLeft[ANNOUNCEMENT_MSG_LENGTH];
	char		m_szMessageScoreLeft[ANNOUNCEMENT_MSG_LENGTH];
	char		m_szMessageAnnouncement[ANNOUNCEMENT_MSG_LENGTH];
	char		m_szMessageExtraAnnouncement[ANNOUNCEMENT_MSG_LENGTH];
	float		m_flShotTime;
	float		m_flNextAnnounceTime;// XDM3035
	float		m_flNextSuitSoundTime;// XDM3038
	float		m_flGameStartTime;// XDM3038a
	float		m_flIntermissionStartTime;// XDM3038a
	float		m_flTimeLeft;// XDM3037
	uint32		m_iJoinTime;// XDM3038a: fixed time
	uint32		m_iTimeLimit;// XDM3038: fixed non-negative time in seconds
	uint32		m_iScoreLeft;// XDM3038: fixed non-negative score
	uint32		m_iDeathLimit;// XDM3038a
	uint32		m_iTimeLeftLast;
	uint32		m_iScoreLeftLast;
	uint32		m_iScoreLimit;
	int			m_iRoundsPlayed;
	int			m_iRoundsLimit;
	byte		m_iActive;// game is active (not loading or playing demo)
	byte		m_iPaused;// game is paused
	byte		m_iIntermission;// intermission is active (showing scoreboard, etc.)
	byte		m_iHardwareMode;// hardware renderer is used

private:
	HUDLIST			*m_pHudList;
	client_sprite_t	*m_pSpriteList;
	int				m_iSpriteCount;// cannot set these to uint because of legacy engine shit
	int				m_iSpriteCountAllRes;
	float			m_flMouseSensitivity;
	// these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded. freed in ~CHud()
	HLSPRITE		*m_rghSprites;	/*[HUD_SPRITE_COUNT]*/// the sprites loaded from hud.txt
	wrect_t			*m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char			*m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/
	int				*m_iszSpriteFrames;// [m_iSpriteCount] XDM3037a
};


extern CHud gHUD;

// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight	(gHUD.m_scrinfo.iHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth		(gHUD.m_scrinfo.iWidth)

#define BASE_XRES 640.f

// Use this to set any co-ords in 640x480 space
#define XRES(x)			((int)(float(x) * ((float)ScreenWidth / 640.0f) + 0.5f))
#define YRES(y)			((int)(float(y) * ((float)ScreenHeight / 480.0f) + 0.5f))
// HL20130901 from bad to worse
//#define XRES(x)			(x  * ((float)ScreenWidth / 640))
//#define YRES(y)			(y  * ((float)ScreenHeight / 480))

// use this to project world coordinates to screen coordinates
#define XPROJECT(x)		((1.0f+(x))*ScreenWidth*0.5f)
#define YPROJECT(y)		((1.0f-(y))*ScreenHeight*0.5f)
// Here x should belong to [-ScreenResPx/2...+ScreenResPx/2]
#define XUNPROJECT(x)	(2*(x/ScreenWidth))
#define YUNPROJECT(y)	(2*(y/ScreenHeight))

//extern int g_iAlive;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;

#endif // HUD_H
