#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"
#include "globals.h"
#include "explode.h"
#include "pm_shared.h"
#include "soundent.h"

int giAmmoIndex = 0;
DLL_GLOBAL AmmoInfo g_AmmoInfoArray[MAX_AMMO_SLOTS] =
{
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" },
	{ 0, "" }
};
const char ammo_registry_file[] = "scripts/ammotypes.txt\0";

// Ammo parameters are no longer hard-coded, but loaded from a text file
// Ammo is not added to inventory as an entity, just value

//-----------------------------------------------------------------------------
// Purpose: Initialize ammo registry from a text file
// UNDONE : Use regular expressions to parse lines
// Output : int - number of ammo types registered
//-----------------------------------------------------------------------------
int RegisterAmmoTypes(const char *szAmmoRegistryFile)
{
	bool bCritical;
	if (szAmmoRegistryFile == NULL)
	{
		szAmmoRegistryFile = ammo_registry_file;
		bCritical = true;// this is the main registry
	}
	else
		bCritical = false;// custom registry, may not exist

	FILE *pFile = LoadFile(szAmmoRegistryFile, "rt");
	if (pFile == NULL)
	{
		conprintf(0, "RegisterAmmoTypes()%s Unable to load \"%s\"!\n", bCritical?" ERROR:":"", szAmmoRegistryFile);
		return 0;
	}

	char str[256];
	char ammo_name[32];
	int ammo_max;
	while (!feof(pFile))
	{
		if (fgets(str, 256, pFile) == NULL)
			break;
		if ((str[0] == '/' && str[1] == '/') || str[0] == '\n')
			continue;

		ammo_max = 0;
		ammo_name[0] = 0;
		//memset(ammo_name, NULL, sizeof(ammo_name));
		// this is fail		token = strtok(str, " \"");
		// this doesn't help		if (sscanf(str, "'%s' %d", ammo_name, &ammo_max) != 2)
		// parse line:
		if (str[0] == '\"')// quoted name
		{
			size_t ofs = 1;// !
			// read quoted name
			while (str[ofs] != '"')
			{
				if (str[ofs] == '\n')
					break;

				ammo_name[ofs-1] = str[ofs];
				++ofs;
			}
			ammo_name[ofs-1] = '\0';
			++ofs;// it's the \"
			// skip space
			while (isspace(str[ofs]))
				++ofs;
			// read amount
			/*char tmpval[8];
			for (int j=0; j<8; ++j)
			{
				if (!isdigit(str[ofs]))
				{
					tmpval[j] = '\0';
					break;
				}
				tmpval[j] = str[ofs];
				++ofs;
			}
			if (tmpval[0])
				ammo_max = atoi(tmpval);*/
			const char *valuestart = &str[ofs];
			while (isdigit(str[ofs]))
				++ofs;

			str[ofs] = '\0';// we're not afraid modify this string here
			ammo_max = atoi(valuestart);
		}
		else if (sscanf(str, "%s %d", ammo_name, &ammo_max) != 2)// easy stuff
		{
			conprintf(0, "RegisterAmmoTypes() ERROR while parsing \"%s\": invalid line: \"%s\"!\n", szAmmoRegistryFile, str);
			continue;
		}
		if (AddAmmoToRegistry(ammo_name, ammo_max) <= AMMOINDEX_NONE)
		{
			conprintf(0, "RegisterAmmoTypes() ERROR: AddAmmoToRegistry(%s, %d) failed!\n", ammo_name, ammo_max);
			break;
		}
		//else
		//	conprintf(1, "RegisterAmmoTypes(): registered %s %d\n", ammo_name, ammo_max);
	}
	fclose(pFile);
	conprintf(0, "RegisterAmmoTypes() registered %d ammo types.\n", giAmmoIndex);
	return giAmmoIndex;

}

//-----------------------------------------------------------------------------
// Purpose: Precaches the ammo and queues the ammo info for sending to clients
// Input  : *szAmmoname - 
// Output : int index or AMMOINDEX_NONE
//-----------------------------------------------------------------------------
int AddAmmoToRegistry(const char *szAmmoName, short iMaxCarry)
{
	if (szAmmoName == NULL)
		return AMMOINDEX_NONE;

	// make sure it's not already in the registry
	int i = GetAmmoIndexFromRegistry(szAmmoName);
	if (i != AMMOINDEX_NONE)
	{
		if (iMaxCarry > g_AmmoInfoArray[i].iMaxCarry)// somebody else says player should carry more ammo of this type
		{
			conprintf(0, "Warning: AddAmmoToRegistry(%s) updating iMaxCarry from %d to %d!\n", szAmmoName, g_AmmoInfoArray[i].iMaxCarry, iMaxCarry);
			g_AmmoInfoArray[i].iMaxCarry = iMaxCarry;
		}
		return i;// found
	}

	// XDM?	giAmmoIndex++;
	ASSERT(giAmmoIndex < MAX_AMMO_SLOTS);

	if (giAmmoIndex < MAX_AMMO_SLOTS)
	{
		strncpy(g_AmmoInfoArray[giAmmoIndex].name, szAmmoName, MAX_AMMO_NAME_LEN);
		g_AmmoInfoArray[giAmmoIndex].iMaxCarry = iMaxCarry;
		//g_AmmoInfoArray[giAmmoIndex].pszName = szAmmoname;
		//g_AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;   // yes, this info is redundant
		++giAmmoIndex;
		return giAmmoIndex-1;
	}
	//giAmmoIndex = 0;
	return AMMOINDEX_NONE;// 0?
}

//-----------------------------------------------------------------------------
// Purpose: GetAmmoIndex
// Input  : *psz - 
// Output : int
//-----------------------------------------------------------------------------
int GetAmmoIndexFromRegistry(const char *szAmmoName)
{
	if (szAmmoName == NULL || *szAmmoName == '\0')
		return AMMOINDEX_NONE;

	for (int i = 0; i < giAmmoIndex; ++i)// XDM3037: array should never be fragmented // < MAX_AMMO_SLOTS;
	{
		if (g_AmmoInfoArray[i].name[0] == 0)// 1st char is empty
			continue;

		if (_stricmp(szAmmoName, g_AmmoInfoArray[i].name) == 0)// case-insensitive for stupid mappers
			return i;
	}
	return AMMOINDEX_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Maximum amount of that type of ammunition that a player can carry.
// Input  : ammoID - 
// Output : int - returns -1 if unlimited, 0 on error
//-----------------------------------------------------------------------------
short MaxAmmoCarry(int ammoID)
{
	if (ammoID >= 0 && ammoID < MAX_AMMO_SLOTS)// != AMMOINDEX_NONE
		return g_AmmoInfoArray[ammoID].iMaxCarry;

	return 0;// XDM3035c
}

//-----------------------------------------------------------------------------
// Purpose: MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a player can carry.
// Input  : *szName - 
// Output : int - returns -1 if unlimited, 0 on error
//-----------------------------------------------------------------------------
short MaxAmmoCarry(const char *szName)
{
	return MaxAmmoCarry(GetAmmoIndexFromRegistry(szName));
}

//-----------------------------------------------------------------------------
// Purpose: Send ammo static data, e.g. type, max amount, etc. but NOT the inventory!
// Warning: Track message size limit! Also, this needs to be sent on EVERY map
// Input  : *pPlayer - 
// Output : int - number of messages generated
//-----------------------------------------------------------------------------
size_t SendAmmoRegistry(CBaseEntity *pPlayer)
{
	if (pPlayer == NULL)
		return 0;

	size_t msg_size = 0;
	size_t last_size = 0;
	int nummessages = 1;
	MESSAGE_BEGIN(MSG_ONE, gmsgAmmoList, NULL, ENT(pPlayer->pev));// XDM3037: right thing to do
	for (int i=0; i<giAmmoIndex; ++i)
	{
		// + 2 bytes and 1 nullterm \0
		msg_size += strlen(g_AmmoInfoArray[i].name) +
#if defined (DOUBLE_MAX_AMMO)
			4;
#else
			3;
#endif
		if (msg_size >= MAX_USER_MSG_DATA)// split message if too large
		{
			MESSAGE_END();
			msg_size -= last_size;// discard sent packet size
			++nummessages;
			MESSAGE_BEGIN(MSG_ONE, gmsgAmmoList, NULL, ENT(pPlayer->pev));
		}
		WRITE_BYTE(i);
#if defined (DOUBLE_MAX_AMMO)
		WRITE_SHORT(g_AmmoInfoArray[i].iMaxCarry);
#else
		WRITE_BYTE(g_AmmoInfoArray[i].iMaxCarry);
#endif
		WRITE_STRING(g_AmmoInfoArray[i].name);
		last_size = msg_size;
	}
	MESSAGE_END();
	//conprintf(2, "SendAmmoRegistry([%d]): generated %u messages\n", pPlayer->entindex(), nummessages);
	return nummessages;
}




//=========================================================
// Base ammo class
// UNDONE: use global respawn mechanism!
//=========================================================

TYPEDESCRIPTION	CBasePlayerAmmo::m_SaveData[] =
{
	DEFINE_FIELD(CBasePlayerAmmo, m_iAmmoGive, FIELD_UINT32),
	DEFINE_FIELD(CBasePlayerAmmo, m_iAmmoContained, FIELD_UINT32),
#if defined(OLD_WEAPON_AMMO_INFO)
	DEFINE_FIELD(CBasePlayerAmmo, m_iAmmoMax, FIELD_UINT32),
#endif
};

IMPLEMENT_SAVERESTORE(CBasePlayerAmmo, CBaseAnimating);

LINK_ENTITY_TO_CLASS(ammo, CBasePlayerAmmo);

//-----------------------------------------------------------------------------
// Purpose: Can be used by custom "ammo" entities for customization
// Input  : *pkvd - 
// NOTE   : Ammo name is in pev->message
//-----------------------------------------------------------------------------
void CBasePlayerAmmo::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "iAmmoGive"))
	{
		m_iAmmoGive = strtoul(pkvd->szValue, NULL, 10);
		pkvd->fHandled = TRUE;
	}
#if defined(OLD_WEAPON_AMMO_INFO)
	else if (FStrEq(pkvd->szKeyName, "iAmmoMax"))
	{
		m_iAmmoMax = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
#endif
	else
		CBaseAnimating::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: Overload this in subclasses to customize ammo, but call afterwards!
//-----------------------------------------------------------------------------
void CBasePlayerAmmo::Precache(void)
{
	/* this was ok for public CBaseEntity
	if (g_SilentItemPickup || ((pev->spawnflags & (SF_NOREFERENCE|SF_NORESPAWN|SF_ITEM_NOFALL)) == (SF_NOREFERENCE|SF_NORESPAWN|SF_ITEM_NOFALL)))// XDM3038c: a little hack to disable models on items created by GiveNamedItem
		pev->modelindex = g_iModelIndexTestSphere;// don't assign sprites!
	else if (!FStringNull(pev->model))// XDM3038c
		pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));*/

	if (FStringNull(pev->message))
	{
		conprintf(1, "%s[%d] \"%s\": ERROR: ammo type is undefined!\n", STRING(pev->classname), entindex(), STRING(pev->targetname));
		return;
	}

	// check first
	if (g_SilentItemPickup || ((pev->spawnflags & (SF_NOREFERENCE|SF_NORESPAWN|SF_ITEM_NOFALL)) == (SF_NOREFERENCE|SF_NORESPAWN|SF_ITEM_NOFALL)))// XDM3038c: a little hack to disable models on items created by GiveNamedItem
	{
		//conprintf(0, "%s is virtual: using default model\n", STRING(pev->classname));
		pev->model = MAKE_STRING(g_szDefaultStudioModel);
	}
	else if (FStringNull(pev->model))// now here it is illegal to have no model
	{
		conprintf(0, "Design error: %s[%d] \"%s\" at (%g %g %g) without model!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		pev->model = MAKE_STRING(g_szDefaultStudioModel);
		//pev->modelindex = g_iModelIndexTestSphere;
	}

	CBaseAnimating::Precache();

	if (FStringNull(pev->noise))// XDM3038a
		pev->noise = MAKE_STRING(DEFAULT_PICKUP_SOUND_AMMO);

	PRECACHE_SOUND(STRINGV(pev->noise));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Warning: Ammo models are no longer precached if the ammo was not placed on the map.
// Warning: When using AddDefault() or cheats, ammo never appears, so it's safe to set some fake model.
//-----------------------------------------------------------------------------
void CBasePlayerAmmo::Spawn(void)
{
	if (GetExistenceState() != ENTITY_EXSTATE_CARRIED)
	{
		if (FBitSet(pev->spawnflags, SF_ITEM_NOFALL))// XDM3038c
			pev->movetype = MOVETYPE_NONE;
		else
			pev->movetype = MOVETYPE_TOSS;

		if (pev->scale <= 0.0f)
			pev->scale = UTIL_GetWeaponWorldScale();// XDM3035b
	}

	CBaseAnimating::Spawn();// CBaseDelay::Spawn(); -> CMyAmmo::Precache() -> CBasePlayerAmmo::InitAmmo(!!!) -> CBasePlayerAmmo::Precache() -> CBaseAnimating::Precache() -> CBaseAnimating::Spawn()
	//SET_MODEL(edict(), STRING(pev->model));// XDM3038: this modifies pev->mins/maxs!

	if (FStringNull(pev->message))// XDM3038c: set by InitAmmo()
	{
		SetBits(pev->flags, FL_KILLME);
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0);
		return;
	}

	/*pev->health*/m_iAmmoContained = m_iAmmoGive;// current ammo amount

	if (IsMultiplayer())
		SetBits(pev->flags, FL_HIGHLIGHT);// XDM3037

	if (GetExistenceState() != ENTITY_EXSTATE_CARRIED)
	{
		// keep pev->sequence set in the editor
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
			SetThink(&CBasePlayerAmmo::FallThink);// XDM3035c: reuse more code
			SetNextThink(0.25);
			//SetTouch(&CBasePlayerAmmo::DefaultTouch);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called during [re]spawning
//-----------------------------------------------------------------------------
void CBasePlayerAmmo::Materialize(void)
{
	if (GetExistenceState() == ENTITY_EXSTATE_CARRIED)
		return;

	//if (pev->movetype == MOVETYPE_NONE)// not falling
	{
		pev->solid = SOLID_TRIGGER;
		pev->owner = NULL;// XDM3038c: TESTME! enable collisions
		UTIL_SetSize(this, VEC_HULL_ITEM_MIN, VEC_HULL_ITEM_MAX);// XDM3038c
		SetTouch(&CBasePlayerAmmo::DefaultTouch);
		//pev->groupinfo = 0;// XDM3038c: TESTME! enable collisions
		//UTIL_UnsetGroupTrace();// XDM3038c: TESTME!
		//SetThinkNull();
		//DontThink();
	}
	/*else if (pev->movetype == MOVETYPE_TOSS)
	{
		ClearBits(pev->flags, FL_ONGROUND);
		pev->solid = SOLID_BBOX;
		UTIL_SetSize(this, g_vecZero, g_vecZero);// pointsize until it lands on the ground
	}*/
	CBaseAnimating::Materialize();// 	UTIL_SetOrigin(this, pev->origin);
}

//-----------------------------------------------------------------------------
// Purpose: THINK Items check landing and materialize here
// Note   : Item should not stop thinking in case it gets pushed or falls from breakable, etc.
//-----------------------------------------------------------------------------
void CBasePlayerAmmo::FallThink(void)
{
#if defined (_DEBUG)
	if (GetExistenceState() == ENTITY_EXSTATE_CARRIED)
	{
		conprintf(1, "%s[%d]::FallThink() called while IsCarried()!\n", STRING(pev->classname), entindex());
		SetThinkNull();
		return;
	}
#endif

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (pev->solid != SOLID_TRIGGER)// m_pfnTouch != &CBasePlayerAmmo::DefaultTouch // if (!pev->vuser3.IsZero())// is not materialized yet
		{
			if (/*pev->impulse == ITEM_STATE_DROPPED && */FBitSet(pev->spawnflags, SF_NORESPAWN))// && !(iFlags() & ITEM_FLAG_IMPORTANT))
			{
				int pc = POINT_CONTENTS(pev->origin);// Remove items that fall into unreachable locations
				if (pc == CONTENTS_SLIME || pc == CONTENTS_LAVA || pc == CONTENTS_SKY)
				{
					Destroy();
					return;
				}
				else
				{
					EMIT_SOUND_DYN(edict(), CHAN_VOICE, DEFAULT_DROP_SOUND_AMMO, VOL_NORM, ATTN_STATIC, 0, RANDOM_LONG(115,120));// since the sound is the same, use different pitch
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
// Purpose: Overload: XDM3038c
// Warning: CBasePlayerAmmo does not create duplicates or gets destroyed, just hides!
// Output : CBaseEntity * - entity that will appear later
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayerAmmo::StartRespawn(void)
{
	m_vecSpawnSpot = pev->origin;// g_pGameRules->GetAmmoRespawnSpot(this);
	if (m_iAmmoContained <= 0)// don't hide partially picked up item
	{
		SetBits(pev->effects, EF_NODRAW);
		SetTouchNull();
		UTIL_SetSize(this, g_vecZero, g_vecZero);
	}
	ScheduleRespawn(g_pGameRules->GetAmmoRespawnDelay(this));
	return this;
}

//-----------------------------------------------------------------------------
// Purpose: Overload: XDM3038c: Called after respawning successfully
//-----------------------------------------------------------------------------
void CBasePlayerAmmo::OnRespawn(void)
{
	SetBits(pev->effects, EF_MUZZLEFLASH);
	ClearBits(pev->flags, FL_NOTARGET);// clear "don't pickup" flags for AI
	if (IsMultiplayer())
		SetBits(pev->flags, FL_HIGHLIGHT);

#if !defined(CLIENT_DLL)
	MESSAGE_BEGIN(MSG_PVS, gmsgItemSpawn, pev->origin+Vector(0,0,4));
		WRITE_BYTE(EV_ITEMSPAWN_AMMO);
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
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Overload
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerAmmo::ShouldRespawn(void) const
{
	if (FBitSet(pev->spawnflags, /*SF_NOREFERENCE|*/SF_NORESPAWN))
		return false;
	if (g_pGameRules)
	{
		if (!g_pGameRules->FAmmoShouldRespawn(this))
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Ammo touched by someone
// Warning: No duplicated are created
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBasePlayerAmmo::DefaultTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;
	if (!pOther->IsAlive())// XDM3038: why check inside overloads?
		return;
	if (g_pGameRules && g_pGameRules->IsGameOver())// XDM3037
		return;

	int iAdded = AddAmmo(pOther);
	if (iAdded > 0)
	{
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		if (!g_SilentItemPickup)// XDM3038c
		{
			if (!FBitSet(pev->spawnflags, SF_NOREFERENCE))// XDM3038c
				CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, ITEM_PICKUP_VOLUME, 1.0f);// XDM3038a

			if (!FStringNull(pev->noise))// XDM3038c
				EMIT_SOUND(edict(), CHAN_ITEM, STRING(pev->noise), VOL_NORM, ATTN_IDLE);
		}
		m_iAmmoContained -= min(m_iAmmoContained, iAdded);// don't bo below 0

		if (sv_partialpickup.value > 0.0f && m_iAmmoContained > 0.0f)
		{
			if (ShouldRespawn())
				StartRespawn();// don't disappear

			ClientPrint(pOther->pev, HUD_PRINTCENTER, "#CONTAINER_REMAIN", UTIL_dtos1(m_iAmmoContained));
		}
		else// remove or respawn normally
		{
			SetTouchNull();
			SetThink(&CBaseEntity::SUB_Remove);// XDM3038: unsafe! Destroy();
			SetNextThink(0.001);// XDM3038a
		}
	}
	else if (g_SilentItemPickup)
	{
		// XDM3038c pev->owner = pOther->edict();
		SetTouchNull();
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(0.001);// XDM3038a
		// XDM3038: unsafe	UTIL_Remove(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ammo touched by someone
// Input  : *pOther - 
// Output : int - amount actually added
//-----------------------------------------------------------------------------
int CBasePlayerAmmo::AddAmmo(CBaseEntity *pOther)// XDM
{
	int iAmmoIndex = GetAmmoIndexFromRegistry(GetAmmoName());// XDM3037
	if (iAmmoIndex >= 0)
	{
		ASSERT(m_iAmmoContained > 0);
#if defined(OLD_WEAPON_AMMO_INFO)
		return pOther->GiveAmmo(m_iAmmoContained, iAmmoIndex, m_iAmmoMax);
#else
		return pOther->GiveAmmo(m_iAmmoContained, iAmmoIndex);
#endif
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A better way to initialize an ammo pickup
// Warning: Call before Precache()!
// Warning: NEVER CALLED FOR GENERIC ammo ENTITY!!!
// Input  : ammo_give - 
//			*name - pointer to a constant NON-TEMPORARY string!
//			ammo_max - 
//			*model - pointer to a constant NON-TEMPORARY string!
//-----------------------------------------------------------------------------
#if defined(OLD_WEAPON_AMMO_INFO)
void CBasePlayerAmmo::InitAmmo(const uint32 &ammo_give, const char *name, const uint32 &ammo_max, const char *model)
#else
void CBasePlayerAmmo::InitAmmo(const uint32 &ammo_give, const char *name, const char *model)
#endif
{
	/* too strict, done in later code if (model == NULL || *model == '\0')//if (FStringNull(pev->model))
	{
		conprintf(0, "Design error: %s[%d] \"%s\" at (%g %g %g) without model!\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		pev->flags = FL_KILLME;//UTIL_Remove(this);?
		return;
	}*/

	if (m_iAmmoGive == 0)
		m_iAmmoGive = ammo_give;
	//else
	//also during respawn	conprintf(2, "%s[%d] has custom capacity: %u\n", STRING(pev->classname), entindex(), m_iAmmoGive);

#if defined(OLD_WEAPON_AMMO_INFO)
	m_iAmmoMax = ammo_max;
#endif
	pev->message = MAKE_STRING(name);// must be safe string // NOTE: is may be not safe to try to GetAmmoIndexFromRegistry() here
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING(model);
}

//-----------------------------------------------------------------------------
// Purpose: Allow pushing dropped ammo
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayerAmmo::IsPushable(void)
{
	if (FBitSet(pev->spawnflags, SF_NORESPAWN))
		return true;

	return false;
}




//=========================================================
// ammo classes
//=========================================================

class CAmmoALauncher : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_ALAUNCHER_GIVE, "agrenades", "models/w_alammo.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_alauncher, CAmmoALauncher);


class CAmmoChemGun : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_LPBOX_GIVE, "lightp", "models/w_chemammo.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_chemgun, CAmmoChemGun);// old name, obsolete
LINK_ENTITY_TO_CLASS(ammo_lightp, CAmmoChemGun);

class CAmmoCrossbow : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_CROSSBOWCLIP_GIVE, "bolts", "models/w_crossbow_clip.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_crossbow, CAmmoCrossbow);

class CAmmoFuel : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_FUELTANK_GIVE, "fuel", "models/w_fuelammo.mdl");
		CBasePlayerAmmo::Precache();
		pev->takedamage = DAMAGE_YES;
		///pev->solid = SOLID_BBOX;
		///UTIL_SetSize(this, Vector(-4, -4, 0), Vector(4, 4, 4));
	}
	virtual int TakeDamage(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
	{
		if (!FBitSet(pev->effects, EF_NODRAW) && FBitSet(bitsDamageType, DMGM_BREAK))
		{
#if !defined(CLIENT_DLL)
			pev->takedamage = DAMAGE_NO;// ! stack overflow prevention
			pev->solid = SOLID_NOT;
			ExplosionCreate(pev->origin, pev->angles, NULL, this, AMMO_FUELTANK_GIVE*2, 0, 0.0f);

			if (!FBitSet(pev->spawnflags, /*SF_NOREFERENCE|*/SF_NORESPAWN) && g_pGameRules && g_pGameRules->FAmmoShouldRespawn(this))
			{
				StartRespawn();
				pev->takedamage = DAMAGE_YES;
			}
			else
			{
				SetTouchNull();
				SetThink(&CBaseEntity::SUB_Remove);
				SetNextThink(0.1);// XDM3038a
			}
#endif // CLIENT_DLL
			return 1;
		}
		return 0;
	}
};
LINK_ENTITY_TO_CLASS(ammo_fueltank, CAmmoFuel);

class CAmmoGauss : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_URANIUMBOX_GIVE, "uranium", "models/w_gaussammo.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_gaussclip, CAmmoGauss);
LINK_ENTITY_TO_CLASS(ammo_egonclip, CAmmoGauss);
// they should've called it LINK_ENTITY_TO_CLASS(ammo_uranium, CAmmoGauss);

class CAmmoGLauncher : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_GLAUNCHER_GIVE, "lgrenades", "models/w_glammo.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_glauncher, CAmmoGLauncher);

class CAmmo9mmClip : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_GLOCKCLIP_GIVE, "9mm", "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_glockclip, CAmmo9mmClip);
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CAmmo9mmClip);


class CAmmoMP5Clip : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_MP5CLIP_GIVE, "9mm", "models/w_9mmarclip.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5clip, CAmmoMP5Clip);
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CAmmoMP5Clip);
LINK_ENTITY_TO_CLASS(ammo_9mmARclip, CAmmoMP5Clip);

class CAmmoMP5Chain : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_CHAINBOX_GIVE, "9mm", "models/w_chainammo.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_9mmbox, CAmmoMP5Chain);

class CAmmoMP5Grenade : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_M203BOX_GIVE, "ARgrenades", "models/w_argrenade.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5grenades, CAmmoMP5Grenade);
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CAmmoMP5Grenade);

class CAmmoPython : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_357BOX_GIVE, "357", "models/w_357ammobox.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_357, CAmmoPython);

class CAmmoRpg : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_RPGCLIP_GIVE, "rockets", "models/w_rpgammo.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_rpgclip, CAmmoRpg);

class CAmmoShotgun : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_BUCKSHOTBOX_GIVE, "buckshot", "models/w_shotbox.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_buckshot, CAmmoShotgun);

class CAmmoRazorBlades : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_RAZORBLADES_GIVE, "blades", "models/w_blades.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_razorblades, CAmmoRazorBlades);

class CAmmoSniper : public CBasePlayerAmmo
{
	virtual void Precache(void)
	{
		InitAmmo(AMMO_SNIPERBOX_GIVE, "sniper", "models/w_sniperammo.mdl");
		CBasePlayerAmmo::Precache();
	}
};
LINK_ENTITY_TO_CLASS(ammo_sniper, CAmmoSniper);
