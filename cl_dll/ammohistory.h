#ifndef	AMMOHISTORY_H
#define	AMMOHISTORY_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "protocol.h"


#define MAX_WEAPON_POSITIONS		8// max number of items in each bucket

//-----------------------------------------------------------------------------
// This is the actual inventory
//-----------------------------------------------------------------------------
class WeaponsResource
{
public:
	void Init(void);
	void Reset(void);
	void LoadWeaponSprites(HUD_WEAPON *pWeapon);
	void LoadAllWeaponSprites(void);

	void AddWeapon(HUD_WEAPON *pWeapon);
	void PickupWeapon(HUD_WEAPON *pWeapon);
	void DropWeapon(HUD_WEAPON *pWeapon);

	HUD_WEAPON *GetWeaponStruct(const int iId);
	HUD_WEAPON *GetWeaponSlot(uint16 iSlot, uint16 iSlotPos) { return m_rgSlots[iSlot][iSlotPos]; }
	HUD_WEAPON *GetFirstPos(uint16 iSlot);
	HUD_WEAPON *GetNextActivePos(uint16 iSlot, uint16 iSlotPos);

	bool HasWeapons(void);
	bool HasUsableWeapons(void);
	bool HasWeapon(const int &iItemID);
	bool HasAmmo(HUD_WEAPON *pWeapon);
	bool IsSelectable(HUD_WEAPON *pWeapon);

	void SetAmmo(const int iId, short iCount);
	short CountAmmo(const int iId);

#if defined (OLD_WEAPON_AMMO_INFO)
	HLSPRITE *GetAmmoPicFromWeapon(const int &iAmmoId, wrect_t &rect);
#endif

protected:
	HUD_WEAPON m_rgWeapons[MAX_WEAPONS];// right now index == ID
	HUD_WEAPON *m_rgSlots[MAX_WEAPON_SLOTS+1][MAX_WEAPON_POSITIONS+1];// The slots currently in use by weapons.  The value is a pointer to the weapon;  if it's NULL, no weapon is there
	short m_riAmmo[MAX_AMMO_SLOTS];// count of each ammo type
};

extern HUD_WEAPON *g_pwNoSelection;
// XDM3038a extern WeaponsResource gWR;


//-----------------------------------------------------------------------------
#define PICKUP_HISTORY_MARGIN		YRES(8)
#define PICKUP_HISTORY_MAX			256

enum
{
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
};


//-----------------------------------------------------------------------------
// Pickup history
//-----------------------------------------------------------------------------
class HistoryResource
{
public:
	void Init(void);
	void Reset(void);
	void AddToHistory(const int &iType, const int &iId, const int &iCount = 0);
	//void AddToHistory(const int &iType, const char *szName, const int &iCount = 0);
	void CheckClearHistory(void);
	int DrawAmmoHistory(const float &flTime);

private:
	size_t iCurrentHistorySlot;
	HIST_ITEM rgAmmoHistory[PICKUP_HISTORY_MAX];
};

// XDM3038c extern HistoryResource gHR;

#endif // AMMOHISTORY_H
