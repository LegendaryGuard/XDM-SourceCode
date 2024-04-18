//
// XDM: F*** OFF WITH THIS SHIT!
//
// HL code was SO crappy, hacky and disgusting that people at valve should have died of shame!
// The fact that it somehow did work is a pure miracle!
//
// NOTE: this system has no access to view model whatsoever and proper animation timings are unknown!
//

// GENERAL NOTES:
// Pay close attention at which point the owner may be destroyed by the weapon itself! Weapon should not access m_hOwner after his death!
// Do not precache projectiles inside weapons (UTIL_PrecacheOther()) because it is slow, do it in W_Precache()!
// Weapons are not sent to client as entities, only as weapon_data_t and pev->weaponmodel.
// Code is balanced between s2c data size and customization ability.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "globals.h"// XDM
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "usercmd.h"
#if defined(CLIENT_DLL)
#include "cl_dll.h"
#include "hud.h"
#include "com_weapons.h"
#include "r_studioint.h"
#include "studio_util.h"

extern CBasePlayer g_ClientPlayer;
#endif // CLIENT_DLL

#define ITEM_DEFAULT_HEALTH	100

bool g_SilentItemPickup = false;// delete item if it hasn't been picked up, don't show messages, no respawning

DLL_GLOBAL ItemInfo g_ItemInfoArray[MAX_WEAPONS] =
{
#if defined (SERVER_WEAPON_SLOTS)
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, 0, 0, NULL, NULL, 0, NULL, 0, }
	...
#error "write 32 lines youself"
#else
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },

	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },

	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },

	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL },
	{ WEAPON_NONE, 0, WEAPON_NOCLIP, 0, "", NULL, NULL }
#endif
};

//-----------------------------------------------------------------------------
// Purpose: Send weapon static data, e.g. type, max amount, etc. but NOT the inventory!
// Warning: Track message size limit! Also, this needs to be sent on EVERY map
// Input  : *pPlayer - 
// Output : int - number of messages generated
//-----------------------------------------------------------------------------
size_t SendWeaponsRegistry(CBaseEntity *pPlayer)
{
	if (pPlayer == NULL)
		return 0;

	size_t nummessages = 0;
#if !defined(CLIENT_DLL)
#if defined (USE_EXCEPTIONS)
	try
	{
#endif // USE_EXCEPTIONS
	for (size_t i = 0; i < MAX_WEAPONS; ++i)// XDM3038c: logical fix
	{
		ItemInfo &II = g_ItemInfoArray[i];
		if (II.iId == WEAPON_NONE)
		{
//#if defined (_DEBUG_ITEMS)
//			conprintf(2, "SendWeaponsRegistry(): skipping weapon %u with no ID\n", i);
//#endif // _DEBUG_ITEMS
			continue;
		}
		const char *pszName;
		if (II.szName[0])// XDM3038a if (II.pszName)
		{
			pszName = II.szName;
			//ASSERT(pszName[0] != '\0');
		}
		else
			pszName = "Empty";

		MESSAGE_BEGIN(MSG_ONE, gmsgWeaponList, NULL, pPlayer->edict());
			WRITE_STRING(pszName);					// string	weapon name
			WRITE_BYTE(GetAmmoIndexFromRegistry(II.pszAmmo1));// byte		Ammo Type
#if defined(OLD_WEAPON_AMMO_INFO)
			WRITE_BYTE(II.iMaxAmmo1);				// byte     Max Ammo 1
#endif
			WRITE_BYTE(GetAmmoIndexFromRegistry(II.pszAmmo2));// byte		Ammo2 Type
#if defined(OLD_WEAPON_AMMO_INFO)
			WRITE_BYTE(II.iMaxAmmo2);				// byte     Max Ammo 2
#endif
#if defined(SERVER_WEAPON_SLOTS)
			WRITE_BYTE(II.iSlot);					// byte		bucket
			WRITE_BYTE(II.iPosition);				// byte		bucket pos
#endif
			WRITE_BYTE(II.iId);						// byte		id
			WRITE_BYTE(II.iMaxClip);				// byte		iMaxClip XDM3037a
			WRITE_BYTE((II.iFlags)&0x00FF);			// byte		Flags XDM3037a
		MESSAGE_END();
		++nummessages;
	}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		DBG_PRINTF("*** SendWeaponsRegistry() exception!\n");
		DBG_FORCEBREAK
	}
#endif // USE_EXCEPTIONS
	//conprintf(2, "SendWeaponsRegistry([%d]): generated %u messages\n", pPlayer->entindex(), nummessages);
#endif // !defined(CLIENT_DLL)
	return nummessages;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038b: Loads a v_ model without precaching it!
// Input  : szViewModel - 
//			m_pPlayer -
//-----------------------------------------------------------------------------
void SetViewModel(const char *szViewModel, CBasePlayer *m_pPlayer)
{
	// XDM3037a: RECHECK!!!
//#if defined (SERVER_VIEWMODEL)
	if (m_pPlayer)
		m_pPlayer->pev->viewmodel = MAKE_STRING(szViewModel);// XDM3037: was idx;// WTF?! assigning index to a string? Who uses that!?
#if defined(CLIENT_DLL)
	cl_entity_t *pViewModel = gEngfuncs.GetViewModel();
	if (pViewModel)
		pViewModel->model = IEngineStudio.Mod_ForName(szViewModel, 0);// XDM3038b
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Precaches the weapon and queues the weapon info for sending to clients
// Warning: ammo_entities do not represent all possible ammo types, so this cannot be avoided :(
// Input  : *szClassname - 
//-----------------------------------------------------------------------------
void UTIL_PrecacheWeapon(const char *szClassname)
{
	UTIL_PrecacheOther(szClassname, iStringNull);
}

//-----------------------------------------------------------------------------
// Purpose: Precaches ammo resources and registers ammo type in ammo registry
// Input  : *szClassname - 
//-----------------------------------------------------------------------------
void UTIL_PrecacheAmmo(const char *szClassname)
{
	UTIL_PrecacheOther(szClassname, iStringNull);
}

//-----------------------------------------------------------------------------
// Purpose: called by worldspawn
//-----------------------------------------------------------------------------
void W_Precache(void)
{
	memset(g_ItemInfoArray, 0, sizeof(g_ItemInfoArray));
	memset(g_AmmoInfoArray, 0, sizeof(g_AmmoInfoArray));
	giAmmoIndex = 0;

#if !defined(CLIENT_DLL)
#if !defined(OLD_WEAPON_AMMO_INFO)
	SERVER_PRINT(" Loading ammo registry...\n");
	RegisterAmmoTypes(NULL);// load default ammo types
	char customammofile[128];
	_snprintf(customammofile, 128, "maps/%s_ammotypes.txt\0", STRING(gpGlobals->mapname));
	RegisterAmmoTypes(customammofile);// load custom ammo types
#endif

	// Common entities
	if (sv_precacheitems.value > 0)// XDM3038: experimental
	{
	SERVER_PRINT(" Precaching items...\n");
	UTIL_PrecacheOther("item_suit");
	UTIL_PrecacheOther("item_battery");
	//UTIL_PrecacheOther("item_antidote");
	//UTIL_PrecacheOther("item_security");
	UTIL_PrecacheOther("item_longjump");
	//UTIL_PrecacheOther("item_airtank");
	UTIL_PrecacheOther("item_healthkit");
	}
	// These are special
	UTIL_PrecacheOther("item_flare");
	UTIL_PrecacheOther("flare");

	// If not using OLD_WEAPON_AMMO_INFO, order of precaching is not important anymore: ammo indexes are generated while reading text files.
	if (sv_precacheammo.value > 0)// XDM3038: experimental
	{
	SERVER_PRINT(" Precaching ammo...\n");
	UTIL_PrecacheAmmo("ammo_9mmclip");
	UTIL_PrecacheAmmo("ammo_9mmAR");
	// rare	UTIL_PrecacheAmmo("ammo_9mmbox");
	UTIL_PrecacheAmmo("ammo_ARgrenades");
	UTIL_PrecacheAmmo("ammo_buckshot");
	UTIL_PrecacheAmmo("ammo_357");
	UTIL_PrecacheAmmo("ammo_gaussclip");
	UTIL_PrecacheAmmo("ammo_rpgclip");
	UTIL_PrecacheAmmo("ammo_crossbow");
	UTIL_PrecacheAmmo("ammo_alauncher");
	UTIL_PrecacheAmmo("ammo_glauncher");
	UTIL_PrecacheAmmo("ammo_lightp");
	UTIL_PrecacheAmmo("ammo_fueltank");
	UTIL_PrecacheAmmo("ammo_sniper");
	UTIL_PrecacheAmmo("ammo_razorblades");// XDM3038c
	}

	if (sv_precacheweapons.value > 0)// XDM3038: experimental
	{
	SERVER_PRINT(" Precaching weapons...\n");
	UTIL_PrecacheWeapon("weapon_crowbar");
	UTIL_PrecacheWeapon("weapon_9mmhandgun");
	UTIL_PrecacheWeapon("weapon_9mmAR");
	UTIL_PrecacheWeapon("weapon_shotgun");
	UTIL_PrecacheWeapon("weapon_357");
	UTIL_PrecacheWeapon("weapon_gauss");
	UTIL_PrecacheWeapon("weapon_rpg");
	UTIL_PrecacheWeapon("weapon_crossbow");
	UTIL_PrecacheWeapon("weapon_egon");
	UTIL_PrecacheWeapon("weapon_tripmine");
	UTIL_PrecacheWeapon("weapon_satchel");
	UTIL_PrecacheWeapon("weapon_handgrenade");
	UTIL_PrecacheWeapon("weapon_snark");
	UTIL_PrecacheWeapon("weapon_hornetgun");
	UTIL_PrecacheWeapon("weapon_alauncher");
	UTIL_PrecacheWeapon("weapon_glauncher");
	UTIL_PrecacheWeapon("weapon_chemgun");
	UTIL_PrecacheWeapon("weapon_flame");
	UTIL_PrecacheWeapon("weapon_plasma");
	UTIL_PrecacheWeapon("weapon_displacer");
	UTIL_PrecacheWeapon("weapon_sword");
	UTIL_PrecacheWeapon("weapon_strtarget");
	UTIL_PrecacheWeapon("weapon_beamrifle");
	UTIL_PrecacheWeapon("weapon_sniperrifle");
	UTIL_PrecacheWeapon("weapon_bhg");
	//UTIL_PrecacheWeapon("weapon_translocator");
	UTIL_PrecacheWeapon("weapon_dlauncher");// XDM3038c
	UTIL_PrecacheOther("weaponbox");// container for dropped deathmatch weapons
	}

	// XDM3035a: moved all calls to UTIL_PrecacheOther() here to speed up the code and decrease runtime edict use (weapons respawn and Precache() often!)
	UTIL_PrecacheOther("grenade");// XDM: some things were moved from here to CGrenade::Precache()
	UTIL_PrecacheOther("a_grenade");
	UTIL_PrecacheOther("ar_grenade");// XDM3038c
	UTIL_PrecacheOther("bolt");
	UTIL_PrecacheOther("hornet");
	UTIL_PrecacheOther("l_grenade");
	UTIL_PrecacheOther("lightp");
	UTIL_PrecacheOther("laser_spot");
	UTIL_PrecacheOther(SATCHEL_CLASSNAME);
	UTIL_PrecacheOther(SQUEAK_CLASSNAME);
	UTIL_PrecacheOther(TRIPMINE_CLASSNAME);
	UTIL_PrecacheOther("plasmaball");
	UTIL_PrecacheOther("rpg_rocket");
	UTIL_PrecacheOther("squeakbox");
	UTIL_PrecacheOther("strtarget");
	UTIL_PrecacheOther("teleporter");
	UTIL_PrecacheOther("razordisk");// XDM3038c
	UTIL_PrecacheOther("blackball");// XDM3038c
	UTIL_PrecacheOther("monster_mortar");// from mtarget.cpp

	conprintf(1, " Precaching common resources...\n");

	PRECACHE_SOUND(DEFAULT_PICKUP_SOUND_AMMO);
	PRECACHE_SOUND(DEFAULT_PICKUP_SOUND_WEAPON);// player picks up a gun
	PRECACHE_SOUND(DEFAULT_PICKUP_SOUND_CONTAINER);// player picks up a weapon box
	PRECACHE_SOUND(DEFAULT_RESPAWN_SOUND_ITEM);// item/weapon respawn sound
	PRECACHE_SOUND(DEFAULT_DROP_SOUND_AMMO);// falls to the ground
	PRECACHE_SOUND(DEFAULT_DROP_SOUND_ITEM);
	PRECACHE_SOUND(DEFAULT_DROP_SOUND_WEAPON);
	PRECACHE_SOUND("weapons/bullet_hit1.wav");
	PRECACHE_SOUND("weapons/bullet_hit2.wav");
	PRECACHE_SOUND("weapons/bullet_hit3.wav");
	PRECACHE_SOUND("weapons/bullet_hit4.wav");
	PRECACHE_SOUND("weapons/dryfire1.wav");
	PRECACHE_SOUND("weapons/explode3.wav");// XDM3035
	PRECACHE_SOUND("weapons/explode4.wav");
	PRECACHE_SOUND("weapons/explode5.wav");
	PRECACHE_SOUND("weapons/flame_burn.wav");
#endif // CLIENT_DLL
}




TYPEDESCRIPTION CBasePlayerItem::m_SaveData[] =
{
	DEFINE_FIELD(CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayerItem, m_hLastOwner, FIELD_EHANDLE),
	DEFINE_FIELD(CBasePlayerItem, m_iId, FIELD_INTEGER),
	//DEFINE_FIELD(CBasePlayerItem, m_iModelIndexView, FIELD_MODELINDEX),
	//DEFINE_FIELD(CBasePlayerItem, m_iModelIndexWorld, FIELD_MODELINDEX),
	//DEFINE_FIELD(CBasePlayerItem, m_iItemState, FIELD_INTEGER), pev->impulse
};
IMPLEMENT_SAVERESTORE(CBasePlayerItem, CBaseAnimating);


TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
{
#if defined(CLIENT_WEAPONS)
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextAmmoBurn, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_FLOAT),
 	DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_FLOAT),
#else
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextAmmoBurn, FIELD_TIME),
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME),
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME),
	DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME),
#endif // CLIENT_WEAPONS
	//DEFINE_FIELD(CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER),// don't save
	//DEFINE_FIELD(CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER),// don't save
	DEFINE_FIELD(CBasePlayerWeapon, m_iClip, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_fInReload, FIELD_INTEGER),// XDM3035
	DEFINE_FIELD(CBasePlayerWeapon, m_fInSpecialReload, FIELD_INTEGER),// XDM3035
	DEFINE_FIELD(CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iAmmoContained1, FIELD_UINT32),// XDM3038c
	DEFINE_FIELD(CBasePlayerWeapon, m_iAmmoContained2, FIELD_UINT32),
	//DEFINE_FIELD(CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER),// don't save
	//DEFINE_FIELD(CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER),// don't save
};
IMPLEMENT_SAVERESTORE(CBasePlayerWeapon, CBasePlayerItem);

//-----------------------------------------------------------------------------
// Purpose: // XDM3038c: useless
//-----------------------------------------------------------------------------
/*void CBasePlayerItem::SetObjectCollisionBox(void)
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16);
}*/

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: allow to be used by players directly in certain cases
// Output : int - flags
//-----------------------------------------------------------------------------
int	CBasePlayerItem::ObjectCaps(void)
{
	return (CBaseAnimating::ObjectCaps() | FCAP_MUST_SPAWN | (pev->impulse == ITEM_STATE_DROPPED?FCAP_IMPULSE_USE:0));
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: Special Precache and Registration function for weapons
//-----------------------------------------------------------------------------
void CBasePlayerItem::Precache(void)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::Precache() (id %d)\n", entindex(), GetID());
	SetBits(pev->flags, FL_IMMUNE_WATER|FL_IMMUNE_SLIME|FL_IMMUNE_LAVA);// XDM3038c: Set these to prevent engine from distorting entvars!
	//if (GetID() != WEAPON_NONE && g_ItemInfoArray[GetID()].iId != WEAPON_NONE)// already filled
	//	return;

	int iID = GetID();
	if (iID > WEAPON_NONE && iID < MAX_WEAPONS)
	{
		if (g_ItemInfoArray[iID].iId != iID)// We have no information on this weapon kind yet
		{
			ASSERT(g_ItemInfoArray[iID].iId == WEAPON_NONE);
			if (GetItemInfo(&g_ItemInfoArray[iID]) > 0)// WARNING!!!! DONT SAVE POINTERS!! OBJECT IS DELETED!!!
			{
				//g_ItemInfoArray[iID].iId = GetID();// XDM3038a
				strncpy(g_ItemInfoArray[iID].szName, GetWeaponName(), MAX_WEAPON_NAME_LEN);// XDM3038a
			}
			else
				conprintf(1, "CBasePlayerItem(%d %s)::Precache() ERROR! Failed to GetItemInfo()!\n", entindex(), GetWeaponName());
		}
	}
	else
		conprintf(1, "CBasePlayerItem(%d %s)::Precache() ERROR! Bad ID: %d!\n", entindex(), GetWeaponName(), iID);

	if (!FStringNull(pev->viewmodel))// XDM3038a
		m_iModelIndexView = PRECACHE_MODEL(STRINGV(pev->viewmodel));

	if (pev->dmg == 0.0f)
		conprintf(2, "CBasePlayerItem(%d)::Precache(): Warning: \"%s\" ID %d has no damage!\n", entindex(), GetWeaponName(), GetID());

	CBaseAnimating::Precache();
	m_iModelIndexWorld = pev->modelindex;// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Spawn. Also called while restoring!
//-----------------------------------------------------------------------------
void CBasePlayerItem::Spawn(void)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::Spawn() (id %d)\n", entindex(), GetID());
	pev->takedamage = DAMAGE_NO;
	if (IsCarried())
	{
		ASSERT(pev->impulse != ITEM_STATE_WORLD);
		ASSERT(pev->impulse != ITEM_STATE_DROPPED);
	}
	else
	{
		m_pPlayer = NULL;// XDM3035a

		if (FBitSet(pev->spawnflags, SF_ITEM_NOFALL))// XDM3038c
			pev->movetype = MOVETYPE_NONE;
		else if (pev->impulse == ITEM_STATE_WORLD || pev->impulse == ITEM_STATE_DROPPED)// XDM3038c: don't rely on these states too much because they sometimes cannot be set at the right time
			pev->movetype = MOVETYPE_TOSS;

		if (pev->scale <= 0.0f)
			pev->scale = UTIL_GetWeaponWorldScale();// XDM3035b

		pev->health = ITEM_DEFAULT_HEALTH;
	}

	if (IsMultiplayer())
		SetBits(pev->flags, FL_HIGHLIGHT);// XDM3037

	CBaseAnimating::Spawn();// sets model, starts animation, Precache(), Materialize()

	if (!IsCarried())
	{
		pev->sequence = LookupSequence("stayput");
		if (pev->sequence < 0)
			pev->sequence = 1;// XDM3037: all p_models MUST have proper sequence named "stayput"! No exceptions!

		if (pev->movetype == MOVETYPE_TOSS && !FBitSet(pev->effects, EF_NODRAW))// falling
		{
			pev->solid = SOLID_BBOX;
			ClearBits(pev->flags, FL_ONGROUND);
			UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground
			if (!FBitSet(pev->spawnflags, SF_NOREFERENCE))
			{
				if (DROP_TO_FLOOR(edict()) == 0)
				{
					conprintf(0, "Design error: %s[%d] fell out of level at %g %g %g!\n", STRING(pev->classname), entindex(), pev->origin.x, pev->origin.y, pev->origin.z);
					Destroy();
					return;
				}
			}
			SetThink(&CBasePlayerItem::FallThink);// XDM3035c: reuse more code
			SetNextThink(0.25);
			//SetTouch(&CBasePlayerItem::DefaultTouch);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set solidity, origin, size, etc.
// Warning: Called during Spawn, Respawn, FallThink... maybe somewhere else. Do not change movetype here!
//-----------------------------------------------------------------------------
void CBasePlayerItem::Materialize(void)
{
#if defined (_DEBUG_ITEMS)
	DBG_ITM_PRINT("CBasePlayerItem(%d)::Materialize() (id %d)\n", entindex(), GetID());
	ASSERT(IsCarried() == false);
#endif
	if (IsCarried())
		return;

	//if (pev->movetype == MOVETYPE_NONE)// not falling
	{
		pev->solid = SOLID_TRIGGER;
		pev->owner = NULL;// XDM3038c: TESTME! enable collisions
		UTIL_SetSize(this, VEC_HULL_ITEM_MIN, VEC_HULL_ITEM_MAX);// XDM3038c: should we?
		SetTouch(&CBasePlayerItem::DefaultTouch);
		//SetThinkNull();DontThink();// XDM3038a
		if (pev->impulse != ITEM_STATE_DROPPED)// only keep "dropped" state
			pev->impulse = ITEM_STATE_WORLD;// XDM3038
		//pev->groupinfo = 0;// XDM3038c: TESTME! enable collisions
		//UTIL_UnsetGroupTrace();// XDM3038c: TESTME!
	}
	/*else if (pev->movetype == MOVETYPE_TOSS)
	{
		ClearBits(pev->flags, FL_ONGROUND);
		pev->solid = SOLID_BBOX;
		UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground
		if (pev->impulse != ITEM_STATE_DROPPED)// only keep "dropped" state
			pev->impulse = ITEM_STATE_WORLD;// XDM3038
		//FallInit();
	}*/
	CBaseAnimating::Materialize();//UTIL_SetOrigin(this, pev->origin);// link into world.
}

//-----------------------------------------------------------------------------
// Purpose: THINK Items check landing and materialize here
// Note   : Item should not stop thinking in case it gets pushed or falls from breakable, etc.
//-----------------------------------------------------------------------------
void CBasePlayerItem::FallThink(void)
{
#if defined (_DEBUG)
	if (IsCarried())
	{
		conprintf(1, "%s[%d]::FallThink() called while IsCarried()!\n", STRING(pev->classname), entindex());
		SetThinkNull();
		return;
	}
#endif

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (pev->solid != SOLID_TRIGGER)// m_pfnTouch != &CBasePlayerItem::DefaultTouch // if (!pev->vuser3.IsZero())// is not materialized yet
		{
			if (pev->impulse == ITEM_STATE_DROPPED && FBitSet(pev->spawnflags, SF_NORESPAWN) && !(iFlags() & ITEM_FLAG_IMPORTANT))
			{
				int pc = POINT_CONTENTS(pev->origin);// Remove items that fall into unreachable locations
				if (pc == CONTENTS_SLIME || pc == CONTENTS_LAVA || pc == CONTENTS_SKY)
				{
					Destroy();
					return;
				}
				else
				{
					EMIT_SOUND_DYN(edict(), CHAN_VOICE, DEFAULT_DROP_SOUND_WEAPON, VOL_NORM, ATTN_STATIC, 0, RANDOM_LONG(100,105));
					//SetModelCollisionBox();// XDM3035b: TODO
					AlignToFloor();// XDM3036
				}
			}
			Materialize();
			pev->vuser3.Clear();
		}
		SetNextThink(0.25);
	}
	else// not on ground
	{
		if (pev->vuser3.IsZero())
		{
			pev->solid = SOLID_BBOX;
			UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground
		}
		pev->vuser3 = pev->velocity;// like m_flFallVelocity
		SetNextThink(0.1);// check faster
	}
}

//-----------------------------------------------------------------------------
// Purpose: This item is being picked up. Create an identical copy that will "respawn" later.
// Warning: DO NOT REMOVE OR EVEN DO ANYTHING WITH <this>!
// Warning: Right now this should be denied for UTIL_Remove()! See DefaultTouch()
// Output : CBaseEntity - duplicated item
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayerItem::StartRespawn(void)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::StartRespawn() (id %d)\n", entindex(), GetID());
	ASSERT(pev->impulse != ITEM_STATE_DROPPED);

#if defined(CLIENT_DLL)
	if (IsMultiplayer())// STUB
		return this;
	else
		return NULL;
#else

	// Make a copy of this weapon that is invisible and inaccessible to players (no touch function).
	// The weapon spawn/respawn code will decide when to make the weapon visible and touchable.
	//CBaseEntity *pNewWeapon = CreateCopy(STRING(pev->classname), this, pev->spawnflags, true);// XDM3038c
	CBaseEntity *pNewWeapon = CreateCopy(STRING(pev->classname), this, pev->spawnflags, false);// XDM3038c: Spawn() will be called by SUB_Respawn()
	if (pNewWeapon)
	{
		pNewWeapon->m_vecSpawnSpot = g_pGameRules->GetWeaponRespawnSpot(this);// IMPORTANT!
		//pNewWeapon->m_hOwner = m_hOwner;// NO! After picking up this item's host/owner is a player!
		SetBits(pNewWeapon->pev->flags, FL_NOTARGET);// XDM3038a: test: no pickup
		SetBits(pNewWeapon->pev->effects, EF_NODRAW);
		pNewWeapon->SetTouchNull();// no touch
		pNewWeapon->SetUseNull();
		pNewWeapon->SetBlockedNull();
		pNewWeapon->SetThink(&CBasePlayerItem::AttemptToRespawn);
		DROP_TO_FLOOR(pNewWeapon->edict());// FIX
		pNewWeapon->SetNextThink(g_pGameRules->GetWeaponRespawnDelay(this));
		// XDM3038c: we need different mechanism for now	pNewWeapon->ScheduleRespawn(g_pGameRules->GetWeaponRespawnDelay(this));
	}
	else
		conprintf(1, "CBasePlayerItem(%d)::StartRespawn() failed to create \"%s\"!\n", entindex(), STRING(pev->classname));

	return pNewWeapon;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after successfully respawning
//-----------------------------------------------------------------------------
void CBasePlayerItem::OnRespawn(void)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::OnRespawn() (id %d)\n", entindex(), GetID());

	if (pev->impulse != ITEM_STATE_WORLD)
	{
		conprintf(1, "%s[%d]::OnRespawn(): warning: state != ITEM_STATE_WORLD!\n", STRING(pev->classname), entindex());
		pev->impulse = ITEM_STATE_WORLD;// XDM3038
	}
	SetBits(pev->effects, EF_MUZZLEFLASH);
	ClearBits(pev->flags, FL_NOTARGET);// clear "don't pickup" flags for AI
	if (IsMultiplayer())
		SetBits(pev->flags, FL_HIGHLIGHT);

#if !defined(CLIENT_DLL)
	MESSAGE_BEGIN(MSG_PVS, gmsgItemSpawn, pev->origin+Vector(0,0,4));
		WRITE_BYTE(EV_ITEMSPAWN_WEAPON);
		WRITE_SHORT(entindex());
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_ANGLE(pev->angles.x);
		WRITE_ANGLE(pev->angles.y);
		WRITE_ANGLE(pev->angles.z);
		WRITE_SHORT(pev->modelindex);
		WRITE_BYTE((int)(pev->scale*10.0f));
		WRITE_BYTE(pev->body);
		WRITE_BYTE(pev->skin);
		WRITE_BYTE(pev->sequence);
	MESSAGE_END();
	// event does not work for badly placed items!
	// PLAYBACK_EVENT_FULL(0, edict(), g_usItemSpawn, 0.0f, pev->origin, pev->angles, pev->scale, pev->sequence, EV_ITEMSPAWN_WEAPON, pev->modelindex, pev->body, pev->skin);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: THINK wrapper for SUB_Respawn() that allows postpruning in case of edict flood
//-----------------------------------------------------------------------------
void CBasePlayerItem::AttemptToRespawn(void)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::AttemptToRespawn() (id %d)\n", entindex(), GetID());
#if defined(CLIENT_DLL)
	float fDelay = 1.0f;// infinite
#else
	float fDelay = g_pGameRules?g_pGameRules->OnWeaponTryRespawn(this):0.0f;
#endif
	if (fDelay == 0.0f)
	{
		SUB_Respawn();
	}
	else if (fDelay < 0.0f)
	{
		//conprintf(1, "%s[%d]::AttemptToRespawn(): removing.\n", STRING(pev->classname), entindex());
		Destroy();
	}
	else
		SetNextThink(fDelay);
}

//-----------------------------------------------------------------------------
// Purpose: Detect weapons dropped in lava or hit by explosions
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayerItem::TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	DBG_PRINT_ENT_TAKEDAMAGE;
	if (pev->takedamage == DAMAGE_NO)
		return 0;
	if (GetHost())
		return 0;

	if (FBitSet(pev->spawnflags, SF_NORESPAWN) && pev->impulse == ITEM_STATE_DROPPED)// dropped by somebody
	{
		pev->health -= flDamage;
		if (pev->health <= 0.0f)
		{
			Killed(pInflictor, pAttacker, GIB_NORMAL);
		}
#if !defined(CLIENT_DLL)
		else if (g_pGameRules && g_pGameRules->FAllowEffects())
		{
			if (RANDOM_LONG(0,3) == 0)
				UTIL_Sparks(Center());
		}
#endif
		return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A common method that can be used outside
// Input  : *pInflictor - 
//			*pAttacker - 
//			iGib - 
//-----------------------------------------------------------------------------
void CBasePlayerItem::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)// XDM3035c
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::Killed() (id %d)\n", entindex(), GetID());
	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: Don't collide with the player that has just dropped me
// Input  : *pOther - 
// Output : 1 = yes, 0 = no
//-----------------------------------------------------------------------------
int CBasePlayerItem::ShouldCollide(CBaseEntity *pOther)
{
	if (FBitSet(pev->spawnflags, SF_NORESPAWN) && pev->impulse == ITEM_STATE_DROPPED)
	{
		if (!FBitSet(pev->flags, FL_ONGROUND) && pOther->IsPlayer())
		{
			if (m_hLastOwner.Get() && (m_hLastOwner.Get() == pOther->edict()))
				return 0;// Don't touch the player that has just dropped this item
		}
	}
	return CBaseEntity::ShouldCollide(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: World item touched by someone
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBasePlayerItem::DefaultTouch(CBaseEntity *pOther)
{
	DBG_PRINT_ENT_TOUCH(DefaultTouch);
	if (!pOther->IsPlayer())// if it's not a player, ignore (FUTURE: allow monsters to pick up and use items?)
		return;
	if (IsGameOver())// XDM3037
		return;
	if (IsRemoving())
	{
#if defined (_DEBUG_ITEMS)
		conprintf(1, "CBasePlayerItem(%d)::DefaultTouch(%d %s) Error: id %d is marked for removal!\n", entindex(), pOther->entindex(), STRING(pOther->pev->netname), GetID());
#endif
		return;
	}
	if (FBitSet(pev->spawnflags, SF_NORESPAWN) && pev->impulse == ITEM_STATE_DROPPED)
	{
		if (!FBitSet(pev->flags, FL_ONGROUND))
		{
			if (m_hLastOwner.Get() && m_hLastOwner == pOther)// XDM
				return;// Don't touch the player that has just dropped this box
		}
	}
	DBG_ITM_PRINT("CBasePlayerItem(%d)::DefaultTouch(%d %s) id %d\n", entindex(), pOther->entindex(), STRING(pOther->pev->netname), GetID());

	if (GetHost())
	{
#if defined (_DEBUG_ITEMS)
		conprintf(1, "CBasePlayerItem(%d)::DefaultTouch(%d %s) Error: id %d is already attached to a host (%d %s), exiting.\n", entindex(), pOther->entindex(), STRING(pOther->pev->netname), GetID(), GetHost()->entindex(), STRING(GetHost()->pev->netname));
#endif
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

#if !defined(CLIENT_DLL)
	if (g_pGameRules)
	{
		if (/*m_hOwner.Get() && */!g_pGameRules->CanPickUpOwnedThing(pPlayer, this))// XDM3038c
		{
			DBG_ITM_PRINT("CBasePlayerItem(%d)::DefaultTouch(%d %s) id %d: rejected by owning policy.\n", entindex(), pOther->entindex(), STRING(pOther->pev->netname), GetID());
			return;
		}
	}
#endif

	//int prevstatus = pev->impulse;
	bool bDestroy = g_SilentItemPickup;// || FBitSet(pev->spawnflags, SF_NOREFERENCE|SF_NORESPAWN);// XDM3038: sf

#if !defined(CLIENT_DLL)
	if (g_pGameRules && !g_pGameRules->CanHavePlayerItem(pPlayer, this))
	{
		DBG_ITM_PRINT("CBasePlayerItem(%d)::DefaultTouch(%d %s) id %d: rejected by game rules.\n", entindex(), pOther->entindex(), STRING(pOther->pev->netname), GetID());
		if (bDestroy)
		{
			// Unsafe to call from Touch()! Destroy();
			pev->solid = SOLID_NOT;
			pev->effects = EF_NODRAW;
			SetTouchNull();
			SetThink(&CBaseEntity::SUB_Remove);
			SetNextThink(0.001);// XDM3038a
		}
		return;
	}
#endif

	// Don't pick up through walls
	//if (!pOther->FVisible(pev->origin))// XDM3038c: that one is bad
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pOther->Center(), ignore_monsters, ignore_glass, edict(), &tr);
	if (tr.flFraction != 1.0f)
		return;// Line of sight is not established

	int result = pOther->AddPlayerItem(this);// this calls AddToPlayer()
	DBG_ITM_PRINT("CBasePlayerItem(%d)::DefaultTouch(%d %s): AddPlayerItem result: %d\n", entindex(), pOther->entindex(), STRING(pOther->pev->netname), result);
	if (result != ITEM_ADDRESULT_NONE)
	{
		//m_flRemoveTime = 0;// XDM3038a: !!!IMPORTANT!!!
		if (!g_SilentItemPickup)// means created by console/maker/etc.
		{
			//m_flDelay = 0.1f;
			//if (prevstatus == ITEM_STATE_WORLD)// XDM3038a: don't do this for dropped items?
			if (!FBitSet(pev->spawnflags, SF_NOREFERENCE))// XDM3038c
				SUB_UseTargets(pOther, USE_TOGGLE, 0); // UNDONE: when should this happen?

#if !defined(CLIENT_DLL)
			if (!FBitSet(pev->spawnflags, SF_NOREFERENCE))// XDM3038c
				CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, ITEM_PICKUP_VOLUME, 1.0f);// XDM3038c
#endif
			if (ShouldRespawn())
				StartRespawn();// Warning: create respawning duplicate before origin is modified!

			SetBits(pev->spawnflags, SF_NORESPAWN);// and don't create more respawning copies!
		}

		if (result == ITEM_ADDRESULT_PICKED)// not "extracted"
		{
			AttachTo(pPlayer);// this changes our origin!
			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, DEFAULT_PICKUP_SOUND_WEAPON, VOL_NORM, ATTN_IDLE);
		}

		pev->target = iStringNull;// XDM3038c: After cloning! Clear this do targets won't be fired if the weapon is dropped and picked up again.
		//SetTouchNull();
/*#if defined (_DEBUG)
		if (m_pfnTouch != NULL)
		{
			conprintf(1, "CBasePlayerItem(%d)::DefaultTouch(%d %s) %d TOUCH STILL ACTIVE! 2\n", entindex(), pOther->entindex(), STRING(pOther->pev->netname), GetID());
			DBG_FORCEBREAK
		}
#endif*/
		//ASSERT(m_pPlayer != NULL);
		//if (m_pPlayer->m_pActiveItem == NULL)
		//	m_pPlayer->SelectItem(this);
	}

	// if this item was not moved to player's inventory, remove it
	if (result == ITEM_ADDRESULT_EXTRACTED ||
		(result == ITEM_ADDRESULT_NONE && bDestroy))// extracted, or was created temporarily
	{
		// Unsafe to call from Touch()! Destroy();
		pev->solid = SOLID_NOT;
		pev->effects = EF_NODRAW;// prevent disintegration
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.001);// XDM3038a
	}
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: allow player to grab it (in case item is stuck somewhere visually reachable)
// Input  : *pActivator - ...
//-----------------------------------------------------------------------------
void CBasePlayerItem::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	DBG_PRINT_ENT_USE(Use);
	if (pev->impulse == ITEM_STATE_DROPPED && pActivator->IsPlayer() && pCaller == pActivator)
	{
		if (FVisible(pActivator->pev->origin))
			Touch(pActivator);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Selected, but put down
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::IsHolstered(void) const// XDM3035
{
	if (m_pPlayer)
	{
#if defined (WEAPON_MODELATTACH)// TODO: attach weapons like flags in CTF
		if (pev->impulse == ITEM_STATE_HOLSTERED && FStringNull(m_pPlayer->pev->weaponmodel))
#else
		if (pev->impulse == ITEM_STATE_HOLSTERED && FBitSet(pev->effects, EF_NODRAW))
#endif
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Overload
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::ShouldRespawn(void) const
{
	if (FBitSet(pev->spawnflags, /*SF_NOREFERENCE|*/SF_NORESPAWN))
		return false;
	if (pev->impulse == ITEM_STATE_DROPPED)// XDM3038: other values are possible
		return false;
#if !defined(CLIENT_DLL)
	if (g_pGameRules)
	{
		if (!g_pGameRules->FWeaponShouldRespawn(this))
			return false;
	}
#endif
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Affected by forces only if dropped by someone
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::IsPushable(void)
{
	if (iFlags() & ITEM_FLAG_IMPORTANT)// ?
		return false;
	if (pev->impulse == ITEM_STATE_DROPPED)// XDM3038c: don't push items placed by the editor
		if (FBitSet(pev->spawnflags, SF_NORESPAWN) && GetExistenceState() == ENTITY_EXSTATE_WORLD && !IsCarried())// dropped by monster AND not picked up and carried
			return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Show important items on map so players may find them
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::ShowOnMap(CBasePlayer *pPlayer) const
{
	if (iFlags() & ITEM_FLAG_IMPORTANT)
		return true;

	return CBaseAnimating::ShowOnMap(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CBasePlayerItem::ReportState(int printlevel)
{
	CBaseAnimating::ReportState(printlevel);
	conprintf(printlevel, "WeaponID: %d, Player: %d, LastOwner: %d\n", GetID(), m_pPlayer?m_pPlayer->entindex():0, m_hLastOwner.Get()?m_hLastOwner->entindex():0);
}

//-----------------------------------------------------------------------------
// Purpose: Is carried by owner (in inventory)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::IsCarried(void) const
{
#if defined (WEAPON_MODELATTACH)
	if (FBitSet(pev->effects, EF_NODRAW) && pev->aiment && GetHost())
#else
	if (GetExistenceState() == ENTITY_EXSTATE_CARRIED)
#endif
	{
		if (pev->aiment)
		{
			if (GetHost())
			{
				if (pev->aiment == GetHost()->edict())
				{
					//ASSERT(GetExistenceState() == ENTITY_EXSTATE_CARRIED);
					ASSERT(pev->impulse != ITEM_STATE_WORLD);
					ASSERT(pev->impulse != ITEM_STATE_DROPPED);
					return true;
				}
				else
					conprintf(1, "%s[%d]::IsCarried() error: aiment != host!!\n", GetWeaponName(), entindex());// BREAKPOINT
			}
			else
				conprintf(1, "%s[%d]::IsCarried() error: ENTITY_EXSTATE_CARRIED without host!!\n", GetWeaponName(), entindex());// BREAKPOINT
		}
		else
			conprintf(1, "%s[%d]::IsCarried() error: ENTITY_EXSTATE_CARRIED without aiment!!\n", GetWeaponName(), entindex());// BREAKPOINT
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038a: Deploy
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::Deploy(void)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::Deploy() (id %d)\n", entindex(), GetID());
	if (m_pPlayer)
	{
		//if (pev->impulse == ITEM_STATE_HOLSTERED)
		pev->impulse = ITEM_STATE_EQUIPPED;
		m_iExistenceState = ENTITY_EXSTATE_CARRIED;// XDM3038
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Put away, don't select anything else from here
// Warning: Call this last! (from derived classes)
// Warning: m_pPlayer can be NULL!
// Input  : skiplocal - 1 if client is predicting weapon animations
//-----------------------------------------------------------------------------
void CBasePlayerItem::Holster(int skiplocal /* = 0 */)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::Holster() (id %d)\n", entindex(), GetID());
	if (m_pPlayer)
	{
// No, animation must finish!		m_pPlayer->pev->viewmodel = iStringNull;
#if defined (WEAPON_MODELATTACH)
		m_pPlayer->pev->weaponmodel = iStringNull;
#else
		SetBits(pev->effects, EF_NODRAW);
#endif
	}
	pev->impulse = ITEM_STATE_HOLSTERED;// XDM3038
	// NO! m_iExistenceState = ENTITY_EXSTATE_CONTAINER;// XDM3038c: NO! Still CARRIED!
	SetThinkNull();// XDM3038a
	DontThink();// XDM3038a
}

//-----------------------------------------------------------------------------
// Purpose: Item is being added to the inventory, BUT player may just extract ammo and remove it!
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::AddToPlayer(CBasePlayer *pPlayer)
{
	if (pPlayer == NULL)
	{
#if defined (_DEBUG_ITEMS)
		conprintf(1, "CBasePlayerItem(%d)::AddToPlayer(NULL)!!! id %d\n", entindex(), GetID());
		DBG_FORCEBREAK
#endif
		return false;
	}
/*#if defined (_DEBUG)
	if (FBitSet(iFlags(), ITEM_FLAG_EXHAUSTIBLE))// XDM3038c: to avoid this, add ammo first!
	{
		if (pPlayer->AmmoInventory(GetAmmoIndexFromRegistry(pszAmmo1())) <= 0 &&
			pPlayer->AmmoInventory(GetAmmoIndexFromRegistry(pszAmmo2())) <= 0)
		{
			DBG_PRINTF("%s[%d]::AddToPlayer(%d) warning: EXHAUSTIBLE, player has no ammo!\n", STRING(pev->classname), entindex(), pPlayer->entindex());
			//NO! Players do not have ammo when picking up new weapon! return false;
		}	
	}
#endif*/
	DBG_ITM_PRINT("%s[%d]::AddToPlayer(%d)\n", STRING(pev->classname), entindex(), pPlayer->entindex());
	SetOwner(pPlayer);// required by ExtractAmmo()!
	// XDM3038c pev->impulse = ITEM_STATE_INVENTORY;// XDM3038
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called through the newly-touched weapon's instance. The existing player weapon is pOriginal
// Input  : *pOriginal - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerItem::AddDuplicate(CBasePlayerItem *pOriginal)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::AddDuplicate(%d) id %d\n", entindex(), pOriginal?pOriginal->entindex():0, GetID());
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: AttachTo player/container physically
// Warning: No calls to player functions from here!
// Note   : Calls SetOwner(pHost);
// Input  : *pHost - a container: player, weaponbox or anything else
//-----------------------------------------------------------------------------
void CBasePlayerItem::AttachTo(CBaseEntity *pHost)
{
	if (pHost == NULL)
	{
#if defined (_DEBUG_ITEMS)
		conprintf(1, "CBasePlayerItem(%d)::AttachTo(NULL)!!! id %d\n", entindex(), GetID());
		DBG_FORCEBREAK
#endif
		return;
	}
	else
		DBG_ITM_PRINT("CBasePlayerItem(%d)::AttachTo(%d %s) (id %d)\n", entindex(), pHost->entindex(), STRING(pHost->pev->netname), GetID());

	SetTouchNull();
	SetThinkNull();// XDM3035c: should we???
	if (pHost->IsPlayer() || pHost->IsMonster())// someone alive that can USE this item
	{
#if defined (WEAPON_MODELATTACH)// when we use pPlayer->pev->weaponmodel
		pev->effects = EF_NODRAW;// ??
#endif
		if (pHost->IsPlayer())
			SetOwner((CBasePlayer *)pHost);

		m_iExistenceState = ENTITY_EXSTATE_CARRIED;// XDM3038c: CARRIED is set only when carried by a player/monster/bot/etc.
	}
	else
	{
		pev->effects = EF_NODRAW;
		SetOwner(NULL);
		m_iExistenceState = ENTITY_EXSTATE_CONTAINER;// XDM3038c: STORED!
	}
	ClearBits(pev->flags, FL_ONGROUND);// XDM3038
	ClearBits(pev->spawnflags, SF_ITEM_NOFALL/* WTF!? | SF_NORESPAWN*/);// XDM3038a: don't clear SF_NOREFERENCE so player won't drop it if not allowed!
	pev->origin = pHost->pev->origin;
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->impulse = ITEM_STATE_INVENTORY;
	pev->aiment = pHost->edict();
	// XDM3038a	pev->modelindex = 0;// server won't send down to clients if modelindex == 0
	// XDM3038a	pev->model = iStringNull;
	//pev->owner = pev->aiment;// same pHost->edict();
	pev->takedamage = DAMAGE_NO;// XDM3035: RadiusDamage still affects weapons!!
	DontThink();//SetNextThink(0.1);// UTIL_WeaponTimeBase()?
	UTIL_SetOrigin(this, pev->origin);
	pev->playerclass = 1;// XDM3035a: mark as glass so trace won't hit it?
	m_flRemoveTime = 0;// XDM3038a: !!!IMPORTANT!!!
	OnAttached();// XDM3038c: call when it's safe
}

//-----------------------------------------------------------------------------
// Purpose: Detach from the host physically
// Warning: No calls to player functions from here!
// Undone : Should reset most weapon vars to their default state... as if we have them!
//-----------------------------------------------------------------------------
void CBasePlayerItem::DetachFromHost(void)
{
	OnDetached();// XDM3038c: call while it's safe
	if (m_pPlayer)
	{
		DBG_ITM_PRINT("CBasePlayerItem(%d)::DetachFromHost(%d) %d\n", entindex(), m_pPlayer->entindex(), GetID());

		if (m_pPlayer->m_pLastItem == this)
			m_pPlayer->m_pLastItem = NULL;

		if (m_pPlayer->m_pNextItem == this)// XDM
			m_pPlayer->m_pNextItem = NULL;
	}
	else
	{
		conprintf(1, "CBasePlayerItem(%d)::DetachFromHost(NULL) %d NULL player!\n", entindex(), GetID());
		DBG_FORCEBREAK
	}
	pev->aiment = NULL;
	//pev->owner = NULL;// better keep this to prevent players from getting stuck
	SetOwner(NULL);
	SetThink(&CBasePlayerItem::FallThink);// FallInit();?
	SetNextThink(0.25);
	SetTouchNull();//NO! Don't let players touch it here!	SetTouch(&CBasePlayerItem::DefaultTouch);
	//SetUseNull();
	pev->modelindex = m_iModelIndexWorld;// set initial weapon model
	pev->sequence = LookupSequence("stayput");// XDM3038
	if (pev->sequence < 0)
		pev->sequence = 1;// XDM3037: all p_models MUST have proper sequence named "stayput"! No exceptions!
 
	//pev->model = ???
	ClearBits(pev->effects, EF_NODRAW);
	//pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_TOSS;
	pev->takedamage = DAMAGE_NO;
	pev->playerclass = 0;
	SetBits(pev->spawnflags, SF_NORESPAWN);// don't respawn detached items!
	ClearBits(pev->spawnflags, SF_NOREFERENCE|SF_ITEM_NOFALL);// XDM3038a
	ClearBits(pev->flags, FL_ONGROUND);
	pev->impulse = ITEM_STATE_DROPPED;// XDM3038
	m_iExistenceState = ENTITY_EXSTATE_WORLD;// XDM3038
}

//-----------------------------------------------------------------------------
// Purpose: Destroy and remove from world (safely)
// Note   : Variables are stored in cache-friendly order.
//-----------------------------------------------------------------------------
void CBasePlayerItem::Destroy(void)
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::Destroy()(id %d)\n", entindex(), GetID());

	ASSERT(FBitSet(iFlags(), ITEM_FLAG_IMPORTANT) == false);// XDM3038c
#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (m_pPlayer)// if attached to a player, remove.
			ASSERT(m_pPlayer->RemovePlayerItem(this));// this calls DetachFromHost
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		printf("*** CBasePlayerItem(%d)::Destroy() exception!\n", GetID());
		DBG_FORCEBREAK
	}
#endif
#if defined (_DEBUG)
	pev->targetname = MAKE_STRING("???REMOVEDWEAPON???");
#else
	pev->targetname = iStringNull;
#endif
	pev->target = iStringNull;
	pev->solid = SOLID_NOT;
	//pev->owner = NULL;
	//SetID(WEAPON_NONE);// XDM3035: mark as erased
	m_iId = WEAPON_NONE;
	SetTouchNull();
	SetThinkNull();
	SetUseNull();
	DontThink();// XDM3038a
	SetOwner(NULL);// XDM3035a
	pev->impulse = ITEM_STATE_WORLD;// XDM3038
	//REMOVE_ENTITY(edict());// XDM3035b: TESTME
	pev->effects = EF_NODRAW;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	CBaseAnimating::Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: entity is about to be erased from world and server memory
//-----------------------------------------------------------------------------
void CBasePlayerItem::UpdateOnRemove(void)// XDM3035
{
	DBG_ITM_PRINT("CBasePlayerItem(%d)::UpdateOnRemove()(id %d)\n", entindex(), GetID());

	// true when game ends ASSERT(!(iFlags() & ITEM_FLAG_IMPORTANT));
	if (iFlags() & ITEM_FLAG_IMPORTANT)
		conprintf(1, "CBasePlayerItem(%d)::UpdateOnRemove()(id %d) warning: removing important item!\n", entindex(), GetID());

#if defined (USE_EXCEPTIONS)
	try
	{
#endif
		if (m_pPlayer)// if attached to a player, remove.
		{
			DBG_PRINTF("CBasePlayerItem(%d)::UpdateOnRemove()(id %d): still owned by player(%d)!\n", entindex(), GetID(), m_pPlayer->entindex());
			bool bRemovedPlayerItem = m_pPlayer->RemovePlayerItem(this);// this calls DetachFromHost
			ASSERT(bRemovedPlayerItem == true);// pulled call out of the ASSERT to ensure it is made
			m_pPlayer = NULL;
		}
#if defined (USE_EXCEPTIONS)
	}
	catch (...)
	{
		printf("*** CBasePlayerItem(%d)::Destroy() exception!\n", GetID());
		DBG_FORCEBREAK
	}
#endif
	m_iId = WEAPON_NONE;// XDM3038a: next function erases classname so it's ok
	CBaseAnimating::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: entity is about to be erased from world and server memory
//-----------------------------------------------------------------------------
void CBasePlayerItem::OnFreePrivateData(void)// XDM3035
{
#if defined (_DEBUG_ITEMS)
	//DBG_PRINTF("CBasePlayerItem(%d)::OnFreePrivateData()(id %d)\n", entindex(), GetID());
	if (g_ServerActive)
	{
		ASSERT(GetID() == WEAPON_NONE);
	}
#endif
	CBaseAnimating::OnFreePrivateData();
	//m_iId = 0;// TEST
	//m_pPlayer = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Safe method to set m_hOwner, m_pPlayer and all other stuff
// Warning: This sets internal 'owner' which is not the same as m_hOwner!
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CBasePlayerItem::SetOwner(CBaseEntity *pOwner)
{
/*#if defined (_DEBUG_ITEMS)
	if (pPlayer == NULL)
		conprintf(1, "CBasePlayerItem(%d)::SetOwner(NULL) %d\n", entindex(), GetID());
#endif*/
	m_hOwner = pOwner;// XDM3037
	if (pOwner == NULL || pOwner->IsPlayer())
		m_pPlayer = (CBasePlayer *)pOwner;

	if (pOwner)// remember only real players
	{
		m_hLastOwner = pOwner;// this is not a previous owner, but a place to store current owner after pev->owner/m_pPlayer gets erased
		pev->owner = pOwner->edict();// XDM3037: for collisions mostly
	}
	else
		pev->owner = NULL;// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: Safe method of getting the owner, checks may be performed here
// Note   : XDM3038c: renamed to avoid conflicts and misconception
// Output : CBasePlayer	*
//-----------------------------------------------------------------------------
CBasePlayer	*CBasePlayerItem::GetHost(void) const
{
	return m_pPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: Get weapon name to store it in the database
// Warning: DO NOT USE ItemInfoArray HERE!!! 
// Output : const char	* - classname except for weapon_custom
//-----------------------------------------------------------------------------
const char *CBasePlayerItem::GetWeaponName(void) const
{
	return STRING(pev->classname);
}








//-----------------------------------------------------------------------------
// Purpose: Customization done by the map designer is processed here
// Output : pkvd->fHandled
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::KeyValue(KeyValueData *pkvd)
{
	/* OLD if (FStrEq(pkvd->szKeyName, "defaultammo"))
	{
		m_iDefaultAmmo = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else */if (FStrEq(pkvd->szKeyName, "ammocontained1"))
	{
		m_iAmmoContained1 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ammocontained2"))
	{
		m_iAmmoContained2 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBasePlayerItem::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: Default Spawn, always called by subclasses
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::Spawn(void)
{
	m_iClientWeaponState = wstate_undefined;// impossible state that differs from GetWeaponState(), so the client gets update right away
	CBasePlayerItem::Spawn();// Precache -> GetItemInfo
	m_iPrimaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo1());// XDM3038c: requires GetItemInfo
	m_iSecondaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo2());
	if (pev->impulse == ITEM_STATE_WORLD && m_iDefaultAmmo > 0)// Old COMPATIBILITY hack
	{
		if (m_iAmmoContained1 == 0)
		{
			m_iAmmoContained1 = m_iDefaultAmmo;
			if (m_iAmmoContained2 == 0 && (iFlags() & ITEM_FLAG_ADDDEFAULTAMMO2))// only if both m_iAmmoContained are 0
				m_iAmmoContained2 = m_iDefaultAmmo;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*int CBasePlayerWeapon::Save(CSave &save)
{
	if (CBasePlayerItem::Save(save) == 0)
		return 0;

#if defined(CLIENT_DLL)
	return 1;
#else
	return save.WriteFields("CBasePlayerWeapon", this, m_SaveData, ARRAYSIZE(m_SaveData));
#endif
}*/

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*int CBasePlayerWeapon::Restore(CRestore &restore)
{
	if (CBasePlayerItem::Restore(restore) == 0)
		return 0;

#if defined(CLIENT_DLL)
	return 1;
#else
	int status = restore.ReadFields("CBasePlayerWeapon", this, m_SaveData, ARRAYSIZE(m_SaveData));
	if (status != 0)
	{
		Initialize();// XDM3038a
	}
	return status;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Every subclassed weapon must call this in Spawn()!
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::Initialize(void)
{
//#if defined (_DEBUG_ITEMS)
//	conprintf(1, "CBasePlayerWeapon(%d)::Initialize()(id %d)\n", entindex(), GetID());
//#endif
	m_iPrimaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo1());
	m_iSecondaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo2());
	CBasePlayerItem::Initialize();
}*/

//-----------------------------------------------------------------------------
// Purpose: Called from player to check&add this weapon to inventory. 'this' is being moved to inventory, no duplication.
// Input  : *pPlayer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::AddToPlayer(CBasePlayer *pPlayer)
{
	DBG_ITM_PRINT("%s[%d]::AddToPlayer(%d %s) (id %d)\n", GetWeaponName(), entindex(), pPlayer->entindex(), STRING(pPlayer->pev->netname), GetID());

	if (GetHost() != NULL)
		conprintf(1, "%s[%d]::AddToPlayer(%d %s) (id %d) error: owned by %s[%d] %s!\n", GetWeaponName(), entindex(), pPlayer->entindex(), STRING(pPlayer->pev->netname), GetID(), STRING(GetHost()->pev->classname), GetHost()->entindex(), STRING(GetHost()->pev->netname));

#if defined (_DEBUG)
	if (FBitSet(iFlags(), ITEM_FLAG_EXHAUSTIBLE))// XDM3038c: to avoid this, add ammo first!
	{
		if (m_iAmmoContained1 == 0 && m_iAmmoContained2 == 0 &&//if (pPlayer->AmmoInventory(PrimaryAmmoIndex()) == 0 && pPlayer->AmmoInventory(SecondaryAmmoIndex()) == 0)
			pPlayer->AmmoInventory(GetAmmoIndexFromRegistry(pszAmmo1())) <= 0 &&
			pPlayer->AmmoInventory(GetAmmoIndexFromRegistry(pszAmmo2())) <= 0)
		{
			DBG_PRINTF("%s[%d]::AddToPlayer(%d) failed: EXHAUSTIBLE with no ammo!\n", GetWeaponName(), entindex(), pPlayer->entindex());
			// NO! Weaponboxes have all weapons empty! return false;
		}	
	}
#endif

	if (CBasePlayerItem::AddToPlayer(pPlayer))
	{
		// update ammo indexes
		m_iPrimaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo1());
		m_iSecondaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo2());

		// extract ammo from weapon to inventory
		//if (m_iAmmoContained1 > 0 || m_iAmmoContained2 > 0)// XDM3038c
			ExtractAmmo(this);
		//else// a dead player dropped this.
		//	ExtractClipAmmo(this);
		// NOTE: even if partial pickup is allowed, we cannot duplicate a weapon (it's illogical), we can only drop some ammo instead :)
		return true;// don't fail even if we could not add ammo, because player can pickup weapon when his inventory is full of ammo
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called through the newly-touched weapon's instance. The existing player weapon is pOriginal
// Input  : *pOriginal - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::AddDuplicate(CBasePlayerItem *pOriginal)
{
	DBG_ITM_PRINT("CBasePlayerWeapon(%d)::AddDuplicate(%d) id %d\n", entindex(), pOriginal?pOriginal->entindex():0, GetID());
	if (pOriginal)
	{
		//if (m_iAmmoContained1 > 0 || m_iAmmoContained2 > 0)// XDM3038c
			return ExtractAmmo(pOriginal->GetWeaponPtr()) > 0;
		//else// a dead player dropped this.
		//	return ExtractClipAmmo(pOriginal->GetWeaponPtr()) > 0;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c Entity is attached to a new owner
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::OnAttached(void)
{
	CBasePlayerItem::OnAttached();
	m_flNextAmmoBurn = 0.0;
	m_flTimeWeaponIdle = 0.0;
	m_flPumpTime = 0.0;
	m_fInReload = 0;
	m_fInSpecialReload = 0;
	m_iClientClip = 0;
	m_iClientWeaponState = wstate_undefined;// XDM3038c: FIX: force update when added to player or container
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c Entity is detached from its former owner
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::OnDetached(void)
{
	CBasePlayerItem::OnDetached();
	m_flNextAmmoBurn = 0.0;
	m_flTimeWeaponIdle = 0.0;
	m_flPumpTime = 0.0;
	m_fInReload = 0;
	m_fInSpecialReload = 0;
	m_iClientClip = 0;
	m_iClientWeaponState = wstate_undefined;// XDM3038c: FIX: force update when added to player or container
}

//-----------------------------------------------------------------------------
// Purpose: Can this weapon perform attack? (based on many restrictions)
// Warning: Overloaded CanAttack in all derived classes must call this one!
// Input  : &attack_time - time to check - m_flNextSecondaryAttack, etc.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::CanAttack(const float &attack_time)
{
	if (GetHost())
	{
		if (m_pPlayer->CanAttack() == false)
			return false;

		if (IsHolstered())// XDM3035: new
			return false;

#if defined(CLIENT_DLL)
		if (IsMultiplayer())
			if (gHUD.m_iGameFlags & GAME_FLAG_NOSHOOTING)
#else
		if (g_pGameRules && g_pGameRules->GetCombatMode() == GAME_COMBATMODE_NOSHOOTING)
#endif
			return false;
	}
	else// XDM3035a: if monsters will be able to hold weapons, this code should be revisited
		return false;

	return (attack_time <= UTIL_WeaponTimeBase());
}

//-----------------------------------------------------------------------------
// Purpose: called each frame by the player PostThink
// XDM3035: I'm amazed it works
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::ItemPostFrame(void)
{
	//conprintf(1, "CBasePlayerWeapon(%d id %d)::ItemPostFrame()\n", entindex(), GetID());
	ASSERT(m_pPlayer != NULL);
	if (GetHost() == NULL)
	{
		conprintf(1, "CBasePlayerWeapon(%d)::ItemPostFrame(): weapon %d %s has no owner!\n", entindex(), GetID(), GetWeaponName());
		DBG_FORCEBREAK
		return;
	}
	ASSERT(GetID() != WEAPON_NONE);
	if (IsRemoving())
	{
		conprintf(1, "CBasePlayerWeapon(%d)::ItemPostFrame(): weapon %d %s was removed!\n", entindex(), GetID(), GetWeaponName());
		DBG_FORCEBREAK
		return;
	}
#if defined (_DEBUG_ITEMS)
	if (m_pPlayer->m_pActiveItem != this)
		conprintf(1, "CBasePlayerWeapon(%d)::ItemPostFrame(): weapon %d %s is not active!\n", entindex(), GetID(), GetWeaponName());
#endif

	if (IsHolstered())// XDM3037: this requires m_pTank check (done in another place)
	{
		/* XDM3038a: cmd->weaponselect prevents us from doing this because Holster() will be called during deploy
		if (FBitSet(m_pPlayer->pev->button, BUTTONS_FIRE))// XDM3035b: if the weapon is holstered, player may try to draw it again
		{
			if (m_pPlayer->CanDeployItem(this) && CanDeploy())// XDM3038
				Deploy();
		}*/
		return;
	}
	if (!IsCarried())// XDM3038a
	{
		DBG_FORCEBREAK
		return;
	}

	if (m_fInReload > 0 && (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
	{
		if (iMaxClip() != WEAPON_NOCLIP)// complete the reload
		{
			int loadammo = min(iMaxClip() - m_iClip, m_pPlayer->AmmoInventory(PrimaryAmmoIndex()));
			// Add them to the clip
			m_iClip += loadammo;
			m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] -= loadammo;
			m_pPlayer->TabulateAmmo();
		}
		m_fInReload = 0;
	}

	//if ( !FBitSet(m_pPlayer->pev->button, IN_ATTACK ) )
	//XDM: this ruins XDM weapon idle system		m_flLastFireTime = 0.0f;

	/* useless	if (FBitSet(m_pPlayer->pev->button, IN_SCORE) || FBitSet(m_pPlayer->m_afButtonLast, IN_SCORE))
	{
		// XDM3037a: do nothing
	}
	else */if (FBitSet(m_pPlayer->pev->button, IN_ATTACK2) && CanAttack(m_flNextSecondaryAttack))// Player can do SecondaryAttack
	{
		//if (pszAmmo2() && m_pPlayer->AmmoInventory(SecondaryAmmoIndex()) <= 0)// dont' !HasAmmo(AMMO_SECONDARY)
		//	m_fFireOnEmpty = TRUE;

		m_pPlayer->TabulateAmmo();
		SecondaryAttack();
		//UTIL_DebugAngles(m_pPlayer->pev->origin, m_pPlayer->pev->angles, 1.0, 64.0f);
		m_flLastAttackTime = UTIL_WeaponTimeBase();// XDM3037
		//if (GetHost())// XDM3035a: m_pPlayer may be NULL at this point!
		//	ClearBits(m_pPlayer->pev->button, IN_ATTACK2);// XDM3035: TODO CHECK (do we need this line?)

		// BAD	if ((PrimaryAmmoIndex() != AMMOINDEX_NONE || SecondaryAmmoIndex() != AMMOINDEX_NONE) && !HasAmmo(AMMO_ANYTYPE))
		//		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
	else if (FBitSet(m_pPlayer->pev->button, IN_ATTACK) && CanAttack(m_flNextPrimaryAttack))// Player can do PrimaryAttack
	{
		//if ((m_iClip == 0 && pszAmmo1()) || (iMaxClip() == WEAPON_NOCLIP && m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) <= 0))
		//	m_fFireOnEmpty = TRUE;
		//UTIL_DebugAngles(m_pPlayer->pev->origin, m_pPlayer->pev->v_angle, 1.0, 64.0f);
		m_pPlayer->TabulateAmmo();
		PrimaryAttack();
		m_flLastAttackTime = UTIL_WeaponTimeBase();// XDM3037

		//if ((PrimaryAmmoIndex() != AMMOINDEX_NONE || SecondaryAmmoIndex() != AMMOINDEX_NONE) && !HasAmmo(AMMO_ANYTYPE))
		//	m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
	else if (FBitSet(m_pPlayer->pev->button, IN_RELOAD) && iMaxClip() != WEAPON_NOCLIP && m_fInReload == 0)// Player can do Reload
	{
		Reload();// reload when reload is pressed, or if no buttons are down and weapon is empty.
	}
	else if (!FBitSet(m_pPlayer->pev->button, BUTTONS_FIRE))// XDM3035: can omit this condition, but it's better this way
	{
		//m_fFireOnEmpty = FALSE;// no fire buttons down
		if (m_fInReload == FALSE)// XDM3035
		{
			if (!IsUseable() && m_flNextPrimaryAttack < UTIL_WeaponTimeBase())// weapon isn't useable, switch.
			{
				if (!FBitSet(iFlags(), ITEM_FLAG_NOAUTOSWITCHEMPTY) && m_pPlayer->SelectNextBestItem(this))
				{
					m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
					return;
				}
			}
			else// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			{
				if (iMaxClip() != WEAPON_NOCLIP)// XDM3035b
				{
					if (m_iClip == 0 && !FBitSet(iFlags(), ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
					{
						Reload();
						return;
					}
				}
			}
		}

		if (m_flTimeWeaponIdle != -1 && m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())// XDM3038a: TESTME: test this check
			ResetEmptySound();

		WeaponIdle();
		return;
	}
	// catch all
	if (GetHost() && ShouldWeaponIdle())
	{
		ResetEmptySound();
		WeaponIdle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Individual client-side state processing function
// Input  : *from - 
//			*to - 
//			*cmd - 
//			&time - 
//			&random_seed - 
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::ClientPostFrame(local_state_s *from, local_state_s *to, usercmd_t *cmd, const double &time, const unsigned int &random_seed)
{
#if defined(CLIENT_DLL)
	int iID = GetID();
	weapon_data_t *pfrom = NULL;
	for (int i = 0; i < MAX_WEAPONS; ++i)// Find weapon with this ID in the structure WARNING: i != ID!!!
		if (from->weapondata[i].m_iId == iID)
			pfrom = &from->weapondata[i];

	if (pfrom == NULL)
	{
		conprintf(0, "CBasePlayerWeapon::ClientPostFrame() ERROR: ID %d not found in incoming data!\n", iID);
		return;
	}
	m_fInReload				= pfrom->m_fInReload;
	m_fInSpecialReload		= pfrom->m_fInSpecialReload;
	m_flPumpTime			= pfrom->m_flPumpTime;
	m_iClip					= pfrom->m_iClip;
	m_flNextPrimaryAttack	= pfrom->m_flNextPrimaryAttack;
	m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
	m_flTimeWeaponIdle		= pfrom->m_flTimeWeaponIdle;
	pev->fuser1				= pfrom->fuser1;
	//m_flStartThrow			= pfrom->fuser2;
	//m_flReleaseThrow		= pfrom->fuser3;
	//m_chargeReady			= pfrom->iuser1;
	//m_fInAttack				= pfrom->iuser2;
	//m_fireState				= pfrom->iuser3;
	m_iPrimaryAmmoType		= (int)from->client.vuser4[0];
	m_iSecondaryAmmoType	= (int)from->client.vuser3[2];

	if (m_iPrimaryAmmoType >= 0)
		g_ClientPlayer.m_rgAmmo[m_iPrimaryAmmoType] = (int)from->client.vuser4[1];

	if (m_iSecondaryAmmoType >= 0)
		g_ClientPlayer.m_rgAmmo[m_iSecondaryAmmoType] = (int)from->client.vuser4[2];
	
	m_flNextAmmoBurn		= from->client.fuser2;
	//m_flStartCharge			= from->client.fuser3;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Server-side only. Pack data.
// Warning: Derived weapon classes MUST CALL this!
// Input  : *player - receiver
//			*weapondata - pack data into this structure
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
//#if !defined(CLIENT_DLL)
//	SERVER_PRINTF("CBasePlayerWeapon(%d id %d)::ClientPackData(%d)\n", entindex(), GetID(), ENTINDEX(player));
//#endif
	weapondata->m_iId					= GetID();
	weapondata->m_iClip					= m_iClip;
	weapondata->m_flNextPrimaryAttack	= max(m_flNextPrimaryAttack, -0.001);
	weapondata->m_flNextSecondaryAttack	= max(m_flNextSecondaryAttack, -0.001);
	weapondata->m_flTimeWeaponIdle		= max(m_flTimeWeaponIdle, -0.001);
	weapondata->m_fInReload				= m_fInReload;
	weapondata->m_fInSpecialReload		= m_fInSpecialReload;
	// m_flNextReload
	weapondata->m_flPumpTime			= max(m_flPumpTime, -0.001);
	// m_fReloadTime
	// m_fAimedDamage
	// m_fNextAimBonus
	// m_fInZoom
	weapondata->m_iWeaponState			= GetWeaponState();
	//weapondata->iuser1					= max(pev->iuser1, -0.001);
	//weapondata->fuser1					= max(pev->fuser1, -0.001);
}

//-----------------------------------------------------------------------------
// Purpose: Sends hud info to client dll, if things have changed
// WARNING: If protocol ever changes, change buffer size in CBasePlayer::SendWeaponsUpdate()!
// Note   : This is old method, now we use weapon_data_t. Used when m_iLocalWeapons == 0.
// Format : byte - state: 0, 1=active, 2=ontarget
//			byte - ID
//			byte - clip (255 == no clip)
// Input  : *pBuffer - common buffer for all updating weapons
// Output : int - buffer length generated by this function
//-----------------------------------------------------------------------------
uint32 CBasePlayerWeapon::UpdateClientData(char *pBuffer)
{
	if (m_pPlayer == NULL)
	{
		DBG_ITM_PRINT("CBasePlayerWeapon(%d)::UpdateClientData(%s) (id %d) has no owner!\n", entindex(), pBuffer, GetID());
		return 0;
	}
	if (pBuffer == NULL)// XDM3035: new method: single buffer for all updates
	{
		conprintf(1, "CBasePlayerWeapon::UpdateClientData(): ERROR: NO BUFFER!!!\n");
		return 0;
	}
	uint32 iSize = 0;
	int state = GetWeaponState();// XDM3038a
	bool bSend = false;

	// This is the current or last weapon, so the state will need to be updated
	/* XDM3038c: state is enough if (m_pPlayer->m_pActiveItem != m_pPlayer->m_pClientActiveItem)
	{
		if (this == m_pPlayer->m_pActiveItem || this == m_pPlayer->m_pClientActiveItem)
			bSend = true;
	}*/

	// If the ammo, clip, state, or other client-related data changed, write the update
	if (m_iClip != m_iClientClip || state != m_iClientWeaponState)// XDM3037a: ? || m_pPlayer->m_iFOV != m_pPlayer->m_iClientFOV)
		bSend = true;

	if (bSend)
	{
		byte value = state;
		memcpy(pBuffer+iSize, &value, sizeof(byte)); iSize+=sizeof(byte);
		value = m_iId;
		memcpy(pBuffer+iSize, &value, sizeof(byte)); iSize+=sizeof(byte);

#if defined(DOUBLE_MAX_AMMO)
		short value2;
		if (iMaxClip() == WEAPON_NOCLIP && m_iClip == 0)// XDM3034: HACK?
			value2 = USHRT_MAX;//-1;?
		else
			value2 = m_iClip;

		memcpy(pBuffer+iSize, &value2, sizeof(uint16)); iSize+=sizeof(uint16);
#else
		if (iMaxClip() == WEAPON_NOCLIP && m_iClip == 0)// XDM3034: HACK?
			value = UCHAR_MAX;//-1; this limits max clip to 254!
		else
			value = m_iClip;

		memcpy(pBuffer+iSize, &value, sizeof(byte)); iSize+=sizeof(byte);
#endif
		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
	}
	//DBG_ITM_PRINT("CBasePlayerWeapon(%d)::UpdateClientData(%s) (id %d) state %d, send %d, size %d\n", entindex(), pBuffer, GetID(), state, bSend, iSize);
	return iSize;
}

//-----------------------------------------------------------------------------
// Purpose: Send weapon animation to client
// Input  : &iAnim - 
//			&body - 
//			skiplocal - 1 if client is predicting weapon animations
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::SendWeaponAnim(const int &iAnim, const int &body, bool skiplocal)
{
	if (m_pPlayer == NULL)// XDM3035
	{
		DBG_FORCEBREAK
		return;
	}
	if (UseDecrement())
		skiplocal = 1;
	else
		skiplocal = 0;

	//m_pPlayer->pev->weaponanim = iAnim;// TODO: use this for something better

#if defined(CLIENT_WEAPONS)
	if (skiplocal && ENGINE_CANSKIP(m_pPlayer->edict()))
		return;
#endif

#if defined(CLIENT_DLL)
	HUD_SendWeaponAnim(iAnim, body, 0);
#else
	MESSAGE_BEGIN(MSG_ONE, svc_weaponanim, NULL, m_pPlayer->edict());
		WRITE_BYTE(iAnim);// sequence number
		WRITE_BYTE(pev->body);
	MESSAGE_END();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Add primary ammo to me/my owner (primary ammo has clip)
// Input  : &iCount - 
//			&iAmmoIndex - 
// Output : Returns the amount of ammo actually added.
//-----------------------------------------------------------------------------
int CBasePlayerWeapon::AddPrimaryAmmo(const int &iCount, const int &iAmmoIndex)//, const int &iMaxClip, const int &iMaxCarry)
{
	DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::AddPrimaryAmmo(%d %d %d)\n", entindex(), GetID(), iCount, iAmmoIndex, iMaxClip());

	if (GetHost() == NULL)
		return 0;
	if (iAmmoIndex != PrimaryAmmoIndex())
	{
#if defined (_DEBUG_ITEMS)
		conprintf(1, "CBasePlayerWeapon(%d id %d)::AddPrimaryAmmo() ammmo ID mismatch! %d %d\n", entindex(), GetID(), iAmmoIndex, PrimaryAmmoIndex());
#endif
		return 0;
	}

	int added = 0;
	int addtoinventory = iCount;
	// fill the clip first, if it's empty or the weapon is not in hands
	if ((iMaxClip() != WEAPON_NOCLIP) && (m_iClip == 0 || (m_iClip < iMaxClip() && pev->impulse == ITEM_STATE_INVENTORY)))
	{
		if (!FBitSet(m_pPlayer->pev->button, IN_ATTACK|IN_ATTACK2))// XDM3035b: things get ugly on weapons that use clip for charging
		{
			int addtoclip = min(iMaxClip() - m_iClip, addtoinventory);// fill the clip to MAX or take what iCount can offer
			if (addtoclip > 0)
			{
				m_iClip += addtoclip;
				added += addtoclip;
				addtoinventory -= addtoclip;
			}
		}
	}
	// now fill player's inventory
#if defined(OLD_WEAPON_AMMO_INFO)
	added += m_pPlayer->GiveAmmo(addtoinventory, iAmmoIndex, iMaxAmmo1());
#else
	added += m_pPlayer->GiveAmmo(addtoinventory, iAmmoIndex);
#endif
	return added;
}

//-----------------------------------------------------------------------------
// Purpose: Add primary ammo to me/my owner
// Input  : &iCount - 
//			&iAmmoIndex - 
// Output : Returns the amount of ammo actually added
//-----------------------------------------------------------------------------
int CBasePlayerWeapon::AddSecondaryAmmo(const int &iCount, const int &iAmmoIndex)//, const int &iMaxCarry)
{
	DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::AddSecondaryAmmo(%d %d)\n", entindex(), GetID(), iCount, iAmmoIndex);

	if (GetHost() == NULL)// XDM3035
		return 0;
	if (iAmmoIndex != SecondaryAmmoIndex())
	{
#if defined (_DEBUG_ITEMS)
		conprintf(1, "CBasePlayerWeapon(%d id %d)::AddSecondaryAmmo() ammmo ID mismatch! %d %d\n", entindex(), GetID(), iAmmoIndex, SecondaryAmmoIndex());
#endif
		return 0;
	}

	if (iCount <= 0)// XDM: prevent player from picking up MP5 when he has full primary and SOME SECONDARY ammo
		return 0;

#if defined(OLD_WEAPON_AMMO_INFO)
	int added = m_pPlayer->GiveAmmo(iCount, iAmmoIndex, iMaxAmmo2());
#else
	int added = m_pPlayer->GiveAmmo(iCount, iAmmoIndex);
#endif
	return added;// XDM3035a: can be 0!
}

//-----------------------------------------------------------------------------
// Purpose: Pack ammo into this gun. When dropped.
// Note   : Can only contain ammo of local types
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::PackAmmo(const int &iCount, bool bSecondary)//, bool bPackClip)
{
	if (bSecondary)
		m_iAmmoContained2 += iCount;
	else
	{
		m_iAmmoContained1 += iCount;
		//if (bPackClip)
			m_iAmmoContained1 += m_iClip;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Can be used, hypotethically
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::IsUseable(void) const// XDM
{
	if (pszAmmo1() == NULL && pszAmmo2() == NULL)
		return true;// this weapon doesn't use ammo, can always deploy.

	return HasAmmo(AMMO_ANYTYPE);// now it is safe and usable
}

//-----------------------------------------------------------------------------
// Purpose: Can be deployed at this moment
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::CanDeploy(void) const
{
	if (m_pPlayer == NULL)
		return false;

	// XDM3038: player should check his own abilities
	//if (!m_pPlayer->CanDeployItem(this))
	//	return false;

	return IsUseable();// is this a good thing to do?
}

//-----------------------------------------------------------------------------
// Purpose: DefaultDeploy, used by all weapons everytime they are selected
// Input  : iViewAnim - view model animation index
//			*szAnimExt - player animation extension string
//			*szSound - optional sound
//			skiplocal - 1 if client is predicting weapon animations
//			body - optional view model body
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::DefaultDeploy(int iViewAnim, const char *szAnimExt, const char *szSound, int skiplocal /* = 0 */, int body)
{
	if (!CanDeploy())
		return false;
	// must be checked outside	if (!m_pPlayer->CanDeployItem(this))// XDM3038a
	//	return false;
	if (!CBasePlayerItem::Deploy())// XDM3038a
		return false;

	// remember models for future use
	m_iModelIndexView = MODEL_INDEX(STRING(pev->viewmodel));
	m_iModelIndexWorld = MODEL_INDEX(STRING(pev->model));

	// XDM3038a: these get lost during transition (and we don't want to save them)
	m_iPrimaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo1());
	m_iSecondaryAmmoType = GetAmmoIndexFromRegistry(pszAmmo2());

	m_pPlayer->TabulateAmmo();
#if defined(CLIENT_DLL)
	SetViewModel(STRING(pev->viewmodel), m_pPlayer);// XDM3038a: we're pretty corked here!!
#endif
	m_pPlayer->pev->viewmodel = pev->viewmodel;
#if defined (WEAPON_MODELATTACH)
	m_pPlayer->pev->weaponmodel = pev->model;
#endif
	m_pPlayer->SetWeaponAnimType(szAnimExt);
	//m_pPlayer->SetAnimation(PLAYER_ARM);// unused, let it be
	SendWeaponAnim(iViewAnim, body);//, skiplocal>0);

	if (szSound && *szSound)
		EMIT_SOUND(edict(), CHAN_WEAPON, szSound, VOL_NORM, ATTN_STATIC);
		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, szSound, VOL_NORM, ATTN_STATIC);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	m_flLastAttackTime = UTIL_WeaponTimeBase() + 0.1f;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Start reload process
// Input  : &iAnim - view model animation
//			&fDelay - after which the reload process is complete
//			*szSound - optional sound
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::DefaultReload(const int &iAnim, const float &fDelay, const char *szSound)//, const int &body = 0)
{
	if (m_pPlayer == NULL)// XDM3035
	{
		DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::DefaultReload(%d %g): NO PLAYER!!!\n", entindex(), GetID(), iAnim, fDelay);
		DBG_FORCEBREAK
		return false;
	}
	int hasammo = m_pPlayer->AmmoInventory(PrimaryAmmoIndex());
	if (hasammo <= 0 || iMaxClip() == WEAPON_NOCLIP)// XDM3034: TESTME
		return false;

	int loadammo = min(iMaxClip() - m_iClip, hasammo);
	if (loadammo <= 0)
		return false;

	m_pPlayer->SetAnimation(PLAYER_RELOAD);// XDM
	SendWeaponAnim(iAnim, pev->body/*, UseDecrement()*/);// why separate body?
	if (szSound && *szSound)
		EMIT_SOUND(edict(), CHAN_WEAPON, szSound, VOL_NORM, ATTN_STATIC);// XDM3038a
		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, szSound, VOL_NORM, ATTN_STATIC);

#if !defined(CLIENT_DLL)
	if (fDelay > 0.0f)// not a virtual call, but a real animation
		CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, WEAPON_ACTIVITY_VOLUME, 1.0f);// XDM3038c
#endif
	m_fInReload = TRUE;
	m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + fDelay;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: ?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::PlayEmptySound(void)
{
	if (m_iPlayEmptySound)
	{
		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/dryfire1.wav", 0.8, ATTN_NORM);
//#if defined(CLIENT_DLL) do we really need to do it this way?
//		HUD_PlaySound("weapons/dryfire1.wav", 0.8);
//#else
		EMIT_SOUND(edict(), CHAN_BODY, "weapons/dryfire1.wav", VOL_NORM, ATTN_STATIC);// XDM
//#endif
		m_iPlayEmptySound = 0;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: ?
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::ResetEmptySound(void)
{
	m_iPlayEmptySound = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Overloaded access to ammo index (faster than in CBasePlayerItem)
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayerWeapon::PrimaryAmmoIndex(void) const
{
	return m_iPrimaryAmmoType;
}

//-----------------------------------------------------------------------------
// Purpose: Overloaded access to ammo index (faster than in CBasePlayerItem)
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayerWeapon::SecondaryAmmoIndex(void) const
{
	return m_iSecondaryAmmoType;
}

//-----------------------------------------------------------------------------
// Purpose: Holster, put away. Also called when selecting another weapon.
// Warning: call this last! (from derived classes)
// Input  : skiplocal - 1 if client is predicting weapon animations
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::Holster(int skiplocal /* = 0 */)
{
	m_flNextAmmoBurn = 0.0;
	m_flPumpTime = 0.0;
	// m_flLastAttackTime
	m_fInSpecialReload = FALSE;

	if (m_pPlayer)
	{
		m_flNextPrimaryAttack = m_pPlayer->m_flNextAttack;
		m_flNextSecondaryAttack = m_pPlayer->m_flNextAttack;
		m_flTimeWeaponIdle = m_pPlayer->m_flNextAttack + 1.0f;
	}
	else
	{
		m_flNextPrimaryAttack = 0;
		m_flNextSecondaryAttack = 0;
		m_flTimeWeaponIdle = 0;
	}
	// m_flPrevPrimaryAttack?
	// m_flLastFireTime?
	m_fInReload = FALSE;// cancel any reload in progress.
	m_iPlayEmptySound = 0;// XDM3035

	CBasePlayerItem::Holster(skiplocal);
	/*if (m_pPlayer)
	{
		//if (m_pPlayer->pev->deadflag == DEAD_NO)
		//	m_pPlayer->SetAnimation(PLAYER_DISARM);// unused, let it be

		// allow the animation to finish	m_pPlayer->pev->viewmodel = 0;
		m_pPlayer->pev->weaponmodel = iStringNull;// XDM3035: we use this to determine weapon IsHolstered state
	}*/
	/* TODO: TESTME
	if (FBitSet(iFlags(), ITEM_FLAG_EXHAUSTIBLE))
		if (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) == 0)
				Destroy();*/
}

//-----------------------------------------------------------------------------
// Purpose: Extract ammo from 'this' to pDestWeapon
// Note   : Called by the new item with the existing item as parameter
// Input  : *pDestWeapon - extract ammo to (can be == this)
// Output : int - total amount of ammo extracted
//-----------------------------------------------------------------------------
int CBasePlayerWeapon::ExtractAmmo(CBasePlayerWeapon *pDestWeapon)
{
	int iReturn = 0;
	if (pDestWeapon)
	{
		DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::ExtractAmmo(%d id %d)\n", entindex(), GetID(), pDestWeapon->entindex(), pDestWeapon->GetID());
		int iAdded;
		if (pszAmmo1())
		{
			if (m_iClip > 0)// extrtact clip (should not be used by now)
			{
				iAdded = pDestWeapon->AddPrimaryAmmo(m_iClip, PrimaryAmmoIndex());// depends on Initialize()!
				m_iClip -= iAdded;
				iReturn += iAdded;
				DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::ExtractAmmo(%d id %d) extracted %d clip ammo\n", entindex(), GetID(), pDestWeapon->entindex(), pDestWeapon->GetID(), iAdded);
			}
			iAdded = pDestWeapon->AddPrimaryAmmo(m_iAmmoContained1, PrimaryAmmoIndex());// depends on Initialize()!
			m_iAmmoContained1 -= iAdded;
			iReturn += iAdded;
			DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::ExtractAmmo(%d id %d) extracted %d primary ammo\n", entindex(), GetID(), pDestWeapon->entindex(), pDestWeapon->GetID(), iAdded);
		}
		if (pszAmmo2())// since THIS weapon has no ammo2 in it, why is this needed anyway?
		{
			iAdded = pDestWeapon->AddSecondaryAmmo(m_iAmmoContained2, SecondaryAmmoIndex());// depends on Initialize()!
			m_iAmmoContained2 -= iAdded;
			iReturn += iAdded;
			DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::ExtractAmmo(%d id %d) extracted %d secondary ammo\n", entindex(), GetID(), pDestWeapon->entindex(), pDestWeapon->GetID(), iAdded);
		}
		if (iReturn > 0)
			EMIT_SOUND(edict(), CHAN_ITEM, DEFAULT_PICKUP_SOUND_AMMO, VOL_NORM, ATTN_IDLE);
	}
#if defined (_DEBUG_ITEMS)
	else
		conprintf(1, "CBasePlayerWeapon(%d id %d)::ExtractAmmo(NULL) error!\n", entindex(), GetID());
#endif
	return iReturn;
}

//-----------------------------------------------------------------------------
// TODO: merge these two functions ^v
// Purpose: Extract ammo from 'this' to pDestWeapon
// Input  : *pDestWeapon - extract ammo to (can be == this)
// Output : int amount of ammo extracted
//-----------------------------------------------------------------------------
/*int CBasePlayerWeapon::ExtractClipAmmo(CBasePlayerWeapon *pDestWeapon)
{
	int iReturn = 0;
	if (pDestWeapon)
	{
		DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::ExtractClipAmmo(%d id %d)\n", entindex(), GetID(), pDestWeapon->entindex(), pDestWeapon->GetID());

		if (m_iClip <= 0)
			return 1;// don't flood the world, pick up this useless weapon

#if defined(OLD_WEAPON_AMMO_INFO)
		if (iMaxAmmo1() <= 0)// pszAmmo1() // weapon actually (theoretically) uses ammo
#else
		if (MaxAmmoCarry(PrimaryAmmoIndex()) <= 0)// XDM3037
#endif
		{
			return 1;// don't flood the world, pick up this useless weapon
		}

		int iAmmoIndex = GetAmmoIndexFromRegistry(pszAmmo1());// XDM3037
		if (iAmmoIndex >= 0)
#if defined(OLD_WEAPON_AMMO_INFO)
			iReturn = pDestWeapon->m_pPlayer->GiveAmmo(m_iClip, iAmmoIndex, iMaxAmmo1());
#else
			iReturn = pDestWeapon->AddPrimaryAmmo(m_iClip, iAmmoIndex);// XDM3038c //iReturn = pDestWeapon->m_pPlayer->GiveAmmo(m_iClip, iAmmoIndex);
#endif
		if (iReturn > 0)
		{
			m_iClip -= iReturn;// just leave what wasn't picked up
			/ *if (iReturn < m_iClip)// inventory accepted less ammo than we offered
			{
				if (iMaxClip() != WEAPON_NOCLIP)// the "m_iClip" could be used only to store packed ammo! Try to add rest into the clip.
					m_iClip += min(iMaxClip(), iAmmo - iReturn);
			}* /
			EMIT_SOUND(edict(), CHAN_ITEM, DEFAULT_PICKUP_SOUND_AMMO, VOL_NORM, ATTN_IDLE);
		}
	}
#if defined (_DEBUG_ITEMS)
	else
		conprintf(1, "CBasePlayerWeapon(%d id %d)::ExtractClipAmmo(NULL) error!\n", entindex(), GetID());
#endif
	return iReturn;
}*/

//-----------------------------------------------------------------------------
// Purpose: no more ammo for this gun, put it away.
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::RetireWeapon(void)
{
	DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::RetireWeapon()\n", entindex(), GetID());
	Holster();// XDM!!! IMPORTANT: CWeaponPlasma depends on this

	// first, no viewmodel at all.
	if (m_pPlayer)
	{
		m_pPlayer->pev->viewmodel = iStringNull;
#if defined (WEAPON_MODELATTACH)
		m_pPlayer->pev->weaponmodel = iStringNull;
#else
		SetBits(pev->effects, EF_NODRAW);
#endif
		m_pPlayer->SelectNextBestItem(this);
	}

	if (FBitSet(iFlags(), ITEM_FLAG_EXHAUSTIBLE))
		Destroy();// this clears m_pPlayer, so call it after
}

//-----------------------------------------------------------------------------
// Purpose: Caller may explicitly specify:
// Input  : type - primary/secondary/any type to check (flags)
//			count - 0 means any amount
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::HasAmmo(short type, short count) const
{
	// TESTME: Moved outside m_pPlayer check
	if (type == AMMO_ANYTYPE || (type & AMMO_CLIP))// AMMO_ANYTYPE is 0!
	{
		if (iMaxClip() != WEAPON_NOCLIP && m_iClip >= max(1,count))// m_iClip may indicate something else!
			return true;
	}
	if (m_pPlayer)// important
	{
		// AMMO_ANYTYPE is 0!
		if (type == AMMO_ANYTYPE || (type & AMMO_PRIMARY))
		{
			if (pszAmmo1() && m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) >= max(1,count))// abstract PrimaryAmmoIndex() allows to call THIS HasAmmo() from CBasePlayerWeapon code
				return true;
		}
		if (type == AMMO_ANYTYPE || (type & AMMO_SECONDARY))
		{
			if (pszAmmo2() && m_pPlayer->AmmoInventory(SecondaryAmmoIndex()) >= max(1,count))
				return true;
		}
	}
	return false;//return CBasePlayerItem::HasAmmo(type, count);// XDM3038c: FIXED!
}

//-----------------------------------------------------------------------------
// Purpose: Use some ammo
// Input  : type - AMMO_PRIMARY, AMMO_SECONDARY or their combination only!
//			count - amount (if both pri|sec specified, same will be used from both)
// Output : int - amount of ammo used
//-----------------------------------------------------------------------------
int CBasePlayerWeapon::UseAmmo(byte type, int count)
{
	if (m_pPlayer == NULL || type == 0)// important
		return 0;

	// XDM3038a: boolean math in action
	bool bCanUse = ((type & AMMO_PRIMARY)!=0 ? (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) >= count):true) &
					((type & AMMO_SECONDARY)!=0 ? (m_pPlayer->AmmoInventory(SecondaryAmmoIndex()) >= count):true) &
					((type & AMMO_CLIP)!=0 ? (m_iClip >= count):true);

	if (bCanUse)
	{
		if (type & AMMO_PRIMARY)
			m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] -= count;

		if (type & AMMO_SECONDARY)
			m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] -= count;

		if (type & AMMO_CLIP)// XDM3038a
			m_iClip -= count;

		return count;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038: distance at which a projectile must be created
// Output : float - forward distance
//-----------------------------------------------------------------------------
float CBasePlayerWeapon::GetBarrelLength(void) const
{
	return (HULL_RADIUS-1.0f);// this should NEVER be far enough to stick through a thin wall!
}

//-----------------------------------------------------------------------------
// Purpose: XDM3037: hopefully, this will add stability to the system
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::Destroy(void)
{
	DBG_ITM_PRINT("CBasePlayerWeapon(%d id %d)::Destroy()\n", entindex(), GetID());
	// Don't clear ID yet!!
	m_flNextAmmoBurn = 0.0;
	m_flPumpTime = 0.0;
	//m_flLastAttackTime = 0;
	m_fInSpecialReload = 0;
	m_flNextPrimaryAttack = -1;
	m_flNextSecondaryAttack = -1;
	m_flTimeWeaponIdle = -1;
	m_iClip = 0;
	m_iClientClip = 0;
	m_iClientWeaponState = wstate_error;
	m_fInReload = 0;
	m_iDefaultAmmo = 0;
	m_iAmmoContained1 = 0;
	m_iAmmoContained2 = 0;
	CBasePlayerItem::Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Overloaded for weapons
// Output : Returns valid attacker for TakeDamage
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayerWeapon::GetDamageAttacker(void)
{
	if (GetHost())
		return GetHost();

	return CBasePlayerItem::GetDamageAttacker();
}

//-----------------------------------------------------------------------------
// Purpose: XDM3038c: Print current important state parameters.
// Warning: Should be accumulative across subclasses.
// Warning: Each subclass should first call MyParent::ReportState()
//-----------------------------------------------------------------------------
void CBasePlayerWeapon::ReportState(int printlevel)
{
	CBasePlayerItem::ReportState(printlevel);
	float wtb = UTIL_WeaponTimeBase();
	conprintf(printlevel, "WeaponState: %d, InReload: %d, Ammo1(%d): %d, Ammo2(%d): %d, Clip: %d, DefAmmo: %d\n NextAttack: %g pri: %g sec: %g, widle: %g\n",
		GetWeaponState(), m_fInReload,
		PrimaryAmmoIndex(), m_pPlayer->AmmoInventory(PrimaryAmmoIndex()),
		SecondaryAmmoIndex(), m_pPlayer->AmmoInventory(SecondaryAmmoIndex()),
		m_iClip, m_iDefaultAmmo,
		m_pPlayer->m_flNextAttack-wtb, m_flNextPrimaryAttack-wtb, m_flNextSecondaryAttack-wtb, m_flTimeWeaponIdle-wtb);
}

//-----------------------------------------------------------------------------
// Purpose: CLIENT WEAPONS
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerWeapon::UseDecrement(void) const
{
#if defined(CLIENT_WEAPONS)// XDM
	return true;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// UNDONE: TODO: FIXME: this had variable names conflict when SDK was updated, no time to check now
// Purpose: An accurate way of calcualting the next attack time.
// Input  : delay - 
// Output : float
//-----------------------------------------------------------------------------
float CBasePlayerWeapon::GetNextAttackDelay(float delay)
{
	// XDM: (gpGlobals->time - m_flLastAttackTime > delay)?
	/* UNDONE: conflicts with current system
	if (m_flLastAttackTime == 0 || m_flNextPrimaryAttack == -1) 
	{
		// At this point, we are assuming that the client has stopped firing and we are going to reset our book keeping variables.
		m_flLastAttackTime = gpGlobals->time;
		m_flPrevPrimaryAttack = delay;
	}
	// calculate the time between this shot and the previous
	float flTimeBetweenFires = gpGlobals->time - m_flLastFireTime;
	float flCreep = 0.0f;
	if (flTimeBetweenFires > 0)
		flCreep = flTimeBetweenFires - m_flPrevPrimaryAttack;// postive or negative

	// save the last fire time
	m_flLastFireTime = gpGlobals->time;		

	float flNextAttack = UTIL_WeaponTimeBase() + delay - flCreep;
	// we need to remember what the m_flNextPrimaryAttack time is set to for each shot, 
	// store it as m_flPrevPrimaryAttack.
	m_flPrevPrimaryAttack = flNextAttack - UTIL_WeaponTimeBase();
	//char szMsg[256];
	//_snprintf(szMsg, 256, "next attack time: %0.4f\n", gpGlobals->time + flNextAttack);
	//OutputDebugString(szMsg);
	return flNextAttack;*/
	return UTIL_WeaponTimeBase() + delay;
}

//-----------------------------------------------------------------------------
// Purpose: Weapon state as it should be sent to client side
// Warning: Return value must differ from initial m_iClientWeaponState!
// Output : weapon_state - MUST NEVER BE wstate_undefined
//-----------------------------------------------------------------------------
int CBasePlayerWeapon::GetWeaponState(void) const
{
	if (m_pPlayer)
	{
		if (!IsUseable())// check this first!
			return wstate_unusable;

		if (IsHolstered())
			return wstate_holstered;

		if (m_fInReload > 0)
			return wstate_reloading;

		if (m_pPlayer->m_pActiveItem == this)
		{
			if (m_pPlayer->m_fOnTarget)
				return wstate_current_ontarget;
			else
				return wstate_current;
		}
		return wstate_inventory;
	}
	DBG_FORCEBREAK
	return wstate_error;
}
