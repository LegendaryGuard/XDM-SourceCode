#ifndef BOT_WEAPONS_H
#define BOT_WEAPONS_H

#include "weapondef.h"
#include "protocol.h"

// map XDM weapons back to HL weapons
#define VALVE_WEAPON_NONE			WEAPON_NONE
#define VALVE_WEAPON_CROWBAR		WEAPON_CROWBAR
#define VALVE_WEAPON_GLOCK			WEAPON_GLOCK
#define VALVE_WEAPON_PYTHON			WEAPON_PYTHON
#define VALVE_WEAPON_MP5			WEAPON_MP5
#define VALVE_WEAPON_CROSSBOW		WEAPON_CROSSBOW
#define VALVE_WEAPON_SHOTGUN		WEAPON_SHOTGUN
#define VALVE_WEAPON_RPG			WEAPON_RPG
#define VALVE_WEAPON_GAUSS			WEAPON_GAUSS
#define VALVE_WEAPON_EGON			WEAPON_EGON
#define VALVE_WEAPON_HORNETGUN		WEAPON_HORNETGUN
#define VALVE_WEAPON_HANDGRENADE	WEAPON_HANDGRENADE
#define VALVE_WEAPON_TRIPMINE		WEAPON_TRIPMINE
#define VALVE_WEAPON_SATCHEL		WEAPON_SATCHEL
#define VALVE_WEAPON_SNARK			WEAPON_SNARK

// Matches server's ItemInfo except for classname
typedef struct bot_weapon_s
{
	short	iId;			// weapon ID
	short	iFlags;
	short	iMaxClip;
//	byte	iWeight;
//keep for half-life compatibility #ifdef SERVER_WEAPON_SLOTS// skip 2
	byte	iSlot;			// HUD slot (0 based)
	byte	iPosition;		// slot position
//#endif // SERVER_WEAPON_SLOTS
	char	szClassname[MAX_WEAPON_NAME_LEN];// MAX_ENTITY_STRING?
	short	iAmmo1;			// ammo index for primary ammo
#ifdef OLD_WEAPON_AMMO_INFO// -- GAME_VALVE_DLL still needs this crap
	short	iAmmo1Max;		// max primary ammo
#endif // OLD_WEAPON_AMMO_INFO
	short	iAmmo2;			// ammo index for secondary ammo
#ifdef OLD_WEAPON_AMMO_INFO
	short	iAmmo2Max;		// max secondary ammo
#endif // OLD_WEAPON_AMMO_INFO
} bot_weapon_t;

//extern DLL_GLOBAL char weapon_classnames[MAX_WEAPONS][64];
//extern DLL_GLOBAL ItemInfo weapon_defs[MAX_WEAPONS];
extern DLL_GLOBAL bot_weapon_t weapon_defs[MAX_WEAPONS];
extern DLL_GLOBAL AmmoInfo g_AmmoInfoArray[MAX_AMMO_SLOTS];// XDM3037: now bots have it
extern DLL_GLOBAL int giAmmoIndex;

int AddAmmoToRegistry(const char *szAmmoName, int iMaxCarry);
int GetAmmoIndexFromRegistry(const char *szAmmoName);
int MaxAmmoCarry(int ammoID);
int MaxAmmoCarry(const char *szName);

#endif // BOT_WEAPONS_H
