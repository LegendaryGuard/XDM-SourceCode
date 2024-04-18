#ifndef GLOBALS_H
#define GLOBALS_H
#if defined (_WIN32)
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

void LinkUserMessages(void);
void PrecacheEvents(void);

// Events
extern DLL_GLOBAL	unsigned short g_usBulletImpact;
extern DLL_GLOBAL	unsigned short g_usBeamImpact;
extern DLL_GLOBAL	unsigned short g_usExplosion;
extern DLL_GLOBAL	unsigned short g_usFlame;
extern DLL_GLOBAL	unsigned short g_usFlameTrail;
extern DLL_GLOBAL	unsigned short g_usGrenExp;
extern DLL_GLOBAL	unsigned short g_usSparkShower;
extern DLL_GLOBAL	unsigned short g_usTrail;
extern DLL_GLOBAL	unsigned short g_usWarpBall;

extern DLL_GLOBAL	unsigned short g_usCaptureObject;
extern DLL_GLOBAL	unsigned short g_usDomPoint;
extern DLL_GLOBAL	unsigned short g_usItemSpawn;
extern DLL_GLOBAL	unsigned short g_usPlayerSpawn;
//extern DLL_GLOBAL	unsigned short g_usTeleport;
extern DLL_GLOBAL	unsigned short g_usNuclearDevice;
extern DLL_GLOBAL	unsigned short g_usMonFx;

extern DLL_GLOBAL	unsigned short g_usPM_Fall;
extern DLL_GLOBAL	unsigned short g_usPM_Longjump;


// Variables
//extern DLL_GLOBAL globalvars_t *gpGlobals;
//extern DLL_GLOBAL ULONG		g_ulFrameCount;
extern DLL_GLOBAL ULONG			g_ulModelIndexPlayer;
extern DLL_GLOBAL Vector		g_vecAttackDir;
//extern DLL_GLOBAL short			g_iSkillLevel;
//extern DLL_GLOBAL int			g_Language;
extern DLL_GLOBAL int			g_iWeaponBoxCount;
extern DLL_GLOBAL int			g_iProtocolVersion;
extern DLL_GLOBAL bool			g_ServerActive;
extern DLL_GLOBAL bool			g_SilentItemPickup;
extern DLL_GLOBAL unsigned char	g_ClientShouldInitialize[];
extern DLL_GLOBAL class CWorld	*g_pWorld;// XDM: don't forget to check for NULL!


// Messages
extern int gmsgInitHUD;
extern int gmsgResetHUD;
//extern int gmsgCurWeapon;
extern int gmsgShake;
extern int gmsgFade;
extern int gmsgFlashlight;
//extern int gmsgFlashBattery;
extern int gmsgShowGameTitle;
//XDM3037aextern int gmsgHealth;
extern int gmsgDamage;
extern int gmsgDamageFx;
extern int gmsgBattery;
extern int gmsgTrain;
extern int gmsgSayText;
extern int gmsgTextMsg;
extern int gmsgWeaponList;
extern int gmsgAmmoList;
extern int gmsgDeathMsg;
extern int gmsgScoreInfo;
extern int gmsgTeamInfo;
extern int gmsgTeamScore;
extern int gmsgDomInfo;
extern int gmsgFlagInfo;
extern int gmsgGameMode;
extern int gmsgGRInfo;
extern int gmsgGREvent;
extern int gmsgMOTD;
extern int gmsgServerName;
//OBSOLETE extern int gmsgAmmoPickup;
//extern int gmsgWeapPickup;
extern int gmsgItemPickup;
extern int gmsgHideWeapon;
extern int gmsgSetCurWeap;
//XDM3037aextern int gmsgSetFOV;
//extern int gmsgViewMode;
extern int gmsgShowMenu;
extern int gmsgGeigerRange;
extern int gmsgTeamNames;
extern int gmsgStatusData;
//extern int gmsgStatusText;
//extern int gmsgStatusValue;
extern int gmsgStatusIcon;
extern int gmsgViewModel;
extern int gmsgParticles;
extern int gmsgNXPrintf;
extern int gmsgWallSprite;
extern int gmsgPartSys;
extern int gmsgRenderSys;
extern int gmsgSpectator;
extern int gmsgSetFog;
extern int gmsgSetSky;
extern int gmsgSetRain;
extern int gmsgAudioTrack;
extern int gmsgFireBeam;
extern int gmsgSpawnGibs;
//extern int gmsgEgonSparks;
extern int gmsgUpdWeapons;
extern int gmsgUpdAmmo;
extern int gmsgStaticEnt;
extern int gmsgStaticSprite;
extern int gmsgTEModel;
extern int gmsgBubbles;
extern int gmsgSpeakSnd;
extern int gmsgPickedEnt;
extern int gmsgItemSpawn;
extern int gmsgSelBestItem;
extern int gmsgPlayerStats;
extern int gmsgTeleport;
extern int gmsgHUDEffects;
extern int gmsgHUDTimer;

#endif // GLOBALS_H
