#ifndef CL_UTIL_H
#define CL_UTIL_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "cvardef.h"
#include "cl_enginefuncs.h"
#include "util_vector.h"
#include "util_common.h"
#include "gamedefs.h"
#include "color.h"

/*
#ifdef _MSC_VER
//#pragma warning(disable: 4244)// 'possible loss of data converting float to int'
#pragma warning(disable: 4305)// 'truncation from 'const double' to 'float'
#endif // _MSC_VER
*/

#define CBSENTENCENAME_MAX 16

inline int IS_DEDICATED_SERVER(void) {return 0;}// XDM3038c: a stub, there is no dedicated server on a client side!

// Macros to hook function calls into the HUD object
/*#if defined (_DEBUG)
#define DECLARE_MESSAGE(y, x) int __MsgFunc_##x(const char *pszName, int iSize, void *pbuf) \
							{ \
								CON_DPRINTF("%s::%s(%s, %d)\n", " ##y", " ##x", pszName, iSize); \
								return gHUD.y.MsgFunc_##x(pszName, iSize, pbuf); \
							}
#else*/
#define DECLARE_MESSAGE(y, x) int __MsgFunc_##x(const char *pszName, int iSize, void *pbuf) { return gHUD.y.MsgFunc_##x(pszName, iSize, pbuf); }
//#endif

#if defined (_DEBUG)
#define HOOK_MESSAGE(x) gEngfuncs.Con_Printf("HOOK_MESSAGE(%s): %d\n", #x, gEngfuncs.pfnHookUserMsg(#x, __MsgFunc_##x));
#else
#define HOOK_MESSAGE(x) gEngfuncs.pfnHookUserMsg(#x, __MsgFunc_##x);
#endif

#define DECLARE_COMMAND(y, x) void __CmdFunc_##x(void) \
							{ \
								gHUD.y.UserCmd_##x(); \
							}
#define HOOK_COMMAND(x, y) gEngfuncs.pfnAddCommand(x, __CmdFunc_##y);


inline void DrawSetTextColor(const byte &r, const byte &g, const byte &b)
{
	gEngfuncs.pfnDrawSetTextColor((float)r/255.0f, (float)g/255.0f, (float)b/255.0f);
}

inline int ConsoleStringLen(const char *string)
{
	int _width, _height;
	gEngfuncs.pfnDrawConsoleStringLen(string, &_width, &_height);
	return _width;
}

inline int RectWidth(const struct rect_s &r)
{
	return r.right - r.left;
}

inline int RectHeight(const struct rect_s &r)
{
	return r.bottom - r.top;
}

// HL20130901
inline char *safe_strcpy(char *dst, const char *src, int len_dst)
{
	if (len_dst <= 0)
		return NULL; // this is bad

	strncpy(dst,src,len_dst);
	dst[len_dst-1] = '\0';
	return dst;
}

inline int safe_sprintf(char *dst, int len_dst, const char *format, ...)
{
	if (len_dst <= 0)
		return -1; // this is bad

	va_list v;
    va_start(v, format);
	int iret = _vsnprintf(dst,len_dst,format,v);
	va_end(v);
	dst[len_dst-1] = '\0';
	return iret;
}

// sound functions
// Made inlines to avoid conflict with system macros and for overloading
inline void PlaySound(char *szSound, float vol) { gEngfuncs.pfnPlaySoundByName(szSound, vol); }
//inline void PlaySound(int iSound, float vol) { gEngfuncs.pfnPlaySoundByIndex(iSound, vol); }
void PlaySoundSuit(char *szSound);
void PlaySoundAnnouncer(char *szSound, float duration);

float GetSensitivityModByFOV(const float &newfov);
HLSPRITE LoadSprite(const char *pszName);
client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *pszName, int iRes, int iCount);
int SPR_FindFrame(HLSPRITE hSprite);// XDM3037a
void SPR_Set(HLSPRITE hPic, int r, int g, int b);// XDM3037
void SPR_Draw(int frame, int x, int y, const wrect_t *prc);// XDM3037a
void SPR_DrawHoles(int frame, int x, int y, const wrect_t *prc);// XDM3037a
void SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc);// XDM3037

void CL_ScreenFade(byte r, byte g, byte b, byte alpha, float fadeTime, float fadeHold, int flags);// XDM3037

void UnpackRGB(int &r, int &g, int &b, unsigned short colorindex);
void UnpackRGB(byte &r, byte &g, byte &b, unsigned short colorindex);
void UnpackRGB(::Color &rgb, unsigned short colorindex);

const ::Color &GetTeamColor(TEAM_ID team);
void GetTeamColor(TEAM_ID team, byte &r, byte &g, byte &b);
void GetTeamColor(TEAM_ID team, int &r, int &g, int &b);
const ::Color &GetPlayerColor(CLIENT_INDEX client);
bool GetPlayerColor(CLIENT_INDEX client, byte &r, byte &g, byte &b);
void GetMeterColor(const float &fMeterValue, byte &r, byte &g, byte &b);

int SpriteRenderMode(struct msprite_s *pHeader);

char *LocaliseTextString(const char *msg, char *dst_buffer, const size_t buffer_size);
char *BufferedLocaliseTextString(const char *msg);
char *LookupString(const char *msg_name, int *msg_dest = NULL);
//char *StripEndNewlineFromString(char *str);
void StripEndNewlineFromString(char *str);
char *ConvertCRtoNL(char *str);

const char *GetMapName(bool bIncludeSubdir = false);// XDM

//void ExtractFileName(const char *fullpath, char *dir, char *name, char *ext);// XDM3030
//int LoadModel(const char *pszName, struct model_s *pModel = NULL);

void GetAllPlayersInfo(void);// XDM3037a
void RebuildTeams(void);// XDM3038a
bool CL_IsDead(void);

bool IsActiveTeam(const TEAM_ID &team_id);
bool IsValidTeam(const TEAM_ID &team_id);

//bool IsActivePlayer(cl_entity_t *ent);
bool IsActivePlayer(const CLIENT_INDEX &idx);
bool IsValidPlayer(const CLIENT_INDEX &idx);
bool IsValidPlayerIndex(const CLIENT_INDEX &idx);
bool IsSpectator(const CLIENT_INDEX &idx);

bool IsTeamGame(const short &gamegules);
bool IsRoundBasedGame(const short &gamerules);// XDM3037
bool IsExtraScoreBasedGame(const short &gamerules);// XDM3037a

short GetGameMode(void);
short GetGameFlags(void);
bool IsMultiplayer(void);// XDM3038a
bool IsGameOver(void);// XDM3038a

const char *GetGameDescription(const short &gamerules);
const char *GetGameRulesIntroText(const short &gametype, const short &gamemode);
const char *GetTeamName(const TEAM_ID &team_id);

float UTIL_PointViewDist(const Vector &point);// XDM3035c
bool UTIL_PointIsFar(const Vector &point);// XDM3035c
bool UTIL_PointIsVisible(const Vector &point, bool check_backplane);// XDM3035

void GetEntityPrintableName(int entindex, char *output, const size_t max_len);// XDM3035c
const char *GetEntityPrintableName(int entindex);// XDM3038a
cl_entity_t *GetUpdatingEntity(int entindex);// XDM3035c
const Vector &GetCEntAttachment(const struct cl_entity_s *pEntity, const int attachment);// XDM3038c

void V_PunchAxis(int axis, float angle);// view.cpp



extern wrect_t nullrc;// XDM3037

extern double g_cl_gravity;// XDM3035

extern int g_iDrawCycle;// XDM3037a
extern short g_ThirdPersonView;
extern Vector g_vecViewOrigin;// XDM: real view point
extern Vector g_vecViewAngles;
extern Vector g_vecViewForward;
extern Vector g_vecViewRight;
extern Vector g_vecViewUp;
// bad extern struct ref_params_s *g_pRefParams;
extern struct cl_entity_s *g_pWorld;

#endif // CL_UTIL_H
