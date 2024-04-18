#ifndef CL_DLL_H
#define CL_DLL_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);

#include "../common/platform.h"
#include "../common/vector.h"
#include "../common/cdll_dll.h"
#include "../engine/cdll_int.h"
#include "../engine/eiface.h"

#if defined (_DEBUG_CLIENTDLL)
#define DBG_CL_PRINT	DBG_PrintF
#else
#define DBG_CL_PRINT
#endif

// Global Client-DLL functions
void CL_Precache(void);
void CL_RegisterVariables(void);
void CL_RegisterCommands(void);
void CL_RegisterMessages(void);
void CL_TempEntPlaySound(struct tempent_s *pTemp, float damp);

void EV_HookEvents(void);

float HUD_GetFOV(void);


// Global Client-DLL variables
extern float g_lastFOV;

// Console variables
extern cvar_t *g_pCvarLW;
extern cvar_t *g_pCvarSuitVolume;
extern cvar_t *g_pCvarDeveloper;
extern cvar_t *g_pCvarServerZMax;
extern cvar_t *g_pCvarParticles;
extern cvar_t *g_pCvarEffects;
extern cvar_t *g_pCvarEffectsDLight;
extern cvar_t *g_pCvarScheme;
extern cvar_t *g_pCvarTFX;
extern cvar_t *g_pCvarDeathView;
extern cvar_t *g_pCvarPickupVoice;
extern cvar_t *g_pCvarAmmoVoice;
extern cvar_t *g_pCvarAnnouncer;
extern cvar_t *g_pCvarAnnouncerEvents;
extern cvar_t *g_pCvarAnnouncerLCombo;
extern cvar_t *g_pCvarHLPlayers;
extern cvar_t *g_pCvarViewDistance;
extern cvar_t *g_pCvarFlashLightMode;
extern cvar_t *g_pCvarDefaultFOV;
extern cvar_t *g_pCvarZSR;
extern cvar_t *g_pCvarCameraAngles;
extern cvar_t *g_pCvarLogStats;
extern cvar_t *g_pCvarLODDist;
extern cvar_t *g_pCvarRenderSystem;
extern cvar_t *g_pCvarPSAutoRate;
extern cvar_t *g_pCvarVoiceIconOffset;
extern cvar_t *g_pCvarMasterServerFile;
extern cvar_t *g_pCvarTmp;
#if defined (_DEBUG_ANGLES)
extern cvar_t *g_pCvarDbgAngles;
extern cvar_t *g_pCvarDbgAnglesClient;
extern cvar_t *g_pCvarDbgAnglesMult;
#endif
#if defined (_DEBUG)
extern cvar_t *cl_test1;
extern cvar_t *cl_test2;
extern cvar_t *cl_test3;
#endif

#endif // CL_DLL_H
