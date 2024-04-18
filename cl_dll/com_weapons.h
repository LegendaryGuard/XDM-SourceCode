//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// com_weapons.h
// Shared weapons common function prototypes
#if !defined( COM_WEAPONSH )
#define COM_WEAPONSH
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "eiface.h"
#include "cl_dll.h"
#include "Exports.h"// HL20130901

void AlertMessage(ALERT_TYPE atype, char *szFmt, ...);
void COM_Log(char *pszFile, char *fmt, ...);
bool CL_IsDead(void);

int UTIL_SharedRandomLong(const unsigned int &seed, const int &low, const int &high);
float UTIL_SharedRandomFloat(const unsigned int &seed, const float &low, const float &high);

void HUD_SendWeaponAnim(int iAnim, int body, int force);
int HUD_GetWeaponAnim(void);
void HUD_PlaySound(char *sound, float volume);
void HUD_PlaybackEvent(int flags, const struct edict_s *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
void HUD_SetMaxSpeed(const struct edict_s *ed, float speed);

int UpdateLocalInventory(const struct weapon_data_s *weapondata);

// Usable client stubs for enginefuncs_t
int stub_PrecacheModel(char *s);
int stub_PrecacheSound(char *s);
void stub_SetModel(struct edict_s *e, const char *m);
int stub_ModelIndex(const char *m);// XDM3037a
int stub_IndexOfEdict(const edict_t *pEdict);// XDM3037a
edict_t *stub_FindEntityByVars(struct entvars_s *pvars);// XDM3037a
const char *stub_NameForFunction(uint32 function);
//int32 stub_RandomLong(int32 lLow, int32 lHigh);// XDM3034
//unsigned short stub_PrecacheEvent(int type, const char *s);
int stub_AllocString(const char *szValue);// XDM3037a


extern bool g_runfuncs;
extern vec3_t v_angles;
extern struct local_state_s *g_finalstate;

#endif
