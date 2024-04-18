#ifndef __AMMO_H__
#define __AMMO_H__
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "weapondef.h"
#include "com_model.h"

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

//#define MAX_WEAPON_NAME 128

#define WEAPON_FLAGS_SELECTONEMPTY	1// must be same as ITEM_FLAG_SELECTONEMPTY!

#define WEAPON_IS_ONTARGET 0x40

// XDM3037: same as AmmoInfo, but with server index and HUD data
struct HUD_AMMO
{
	HLSPRITE hAmmoSprite;
	wrect_t rcAmmoSprite;
	char	szName[MAX_AMMO_NAME_LEN];
	short	iMax;// must be signed
	// Ammo ID is the index in the array byte	iAmmoID;
};

// Don't forget to fix MsgFunc_WeaponList if you modify this (and also hwNoSelection)
// resembles ItemInfo on server
struct HUD_WEAPON
{
	int		iId;
	short	iFlags;
	short	iMaxClip;
	short	iSlot;
	short	iSlotPos;
	short	iAmmoType;
	short	iAmmo2Type;
#if defined (OLD_WEAPON_AMMO_INFO)
	short	iMax1;
	short	iMax2;
#endif
	short	iClip;
	short	iState;// XDM3038a
	short	iZoomMode;// XDM3038a
	short	iPriority;// XDM3037
	char	szName[MAX_WEAPON_NAME_LEN];
	HLSPRITE hActive;
	wrect_t rcActive;
	HLSPRITE hInactive;
	wrect_t rcInactive;
#if defined (OLD_WEAPON_AMMO_INFO)
	HLSPRITE hAmmo;
	wrect_t rcAmmo;
	HLSPRITE hAmmo2;
	wrect_t rcAmmo2;
#endif
	HLSPRITE hCrosshair;
	wrect_t rcCrosshair;
	HLSPRITE hAutoaim;
	wrect_t rcAutoaim;
	HLSPRITE hZoomedCrosshair;
	wrect_t rcZoomedCrosshair;
	HLSPRITE hZoomedAutoaim;
	wrect_t rcZoomedAutoaim;
	//char	szViewModel[MAX_MODEL_NAME];
};

struct HIST_ITEM
{
	int type;
	float DisplayTime;  // the time at which this item should be removed from the history
	int iCount;
	int iId;
};

#endif // __AMMO_H__
