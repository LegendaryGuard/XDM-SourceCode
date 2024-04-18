#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "pm_shared.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "game.h"
#include "animation.h"
#include "soundent.h"
#if !defined (CLIENT_DLL)
#include "customprojectile.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: XHL Universal custom weapon
// Note   : Not very complete, but works
//-----------------------------------------------------------------------------

#define SF_WEAPONCUSTOM_NOFIRE1		(1<<0)// 0x0001 // 1
#define SF_WEAPONCUSTOM_NOFIRE2		(1<<1)// 0x0002 // 2
#define SF_WEAPONCUSTOM_NOFIREUW1	(1<<2)// 0x0004 // 4
#define SF_WEAPONCUSTOM_NOFIREUW2	(1<<3)// 0x0008 // 8
#define SF_WEAPONCUSTOM_SILENT1		(1<<4)// 0x0010 // 16
#define SF_WEAPONCUSTOM_SILENT2		(1<<5)// 0x0020 // 32
//#define SF_WEAPONCUSTOM_			(1<<6)// 0x0040 // 64
//#define SF_WEAPONCUSTOM_			(1<<7)// 0x0080 // 128
#define SF_WEAPONCUSTOM_DUPLICATE	(1<<8)// 0x0100 // 256// XDM3038b: used to place copy of the same custom weapon on the map: don't occupy new ItemInfo slot, find original by targetname and become a copy of it
// SF 512 occupied!!!

//LINK_ENTITY_TO_CLASS(cycler_weapon, CWeaponCustom);// disable HL compatibility: some mappers used this in a very wrong way! (unearth.bsp)
//LINK_ENTITY_TO_CLASS(weapon_custom, CWeaponCustom);
LINK_ENTITY_TO_CLASS(weapon, CWeaponCustom);

TYPEDESCRIPTION	CWeaponCustom::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponCustom, m_iszModelWorld, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszModelView, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszWeaponName, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszAmmoName1, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszAmmoName2, FIELD_STRING),
	//DEFINE_ARRAY(CWeaponCustom, m_szAmmoName1, FIELD_CHARACTER, MAX_AMMO_NAME_LEN),
	//DEFINE_ARRAY(CWeaponCustom, m_szAmmoName2, FIELD_CHARACTER, MAX_AMMO_NAME_LEN),
	DEFINE_FIELD(CWeaponCustom, m_iszProjectile1, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszProjectile2, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszCustomProjectile1, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszCustomProjectile2, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszFireTarget1, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszFireTarget2, FIELD_STRING),
	DEFINE_FIELD(CWeaponCustom, m_iszPlayerAnimExt, FIELD_STRING),
	//DEFINE_FIELD(CWeaponCustom, m_bCanFireUnderWater1, FIELD_CHARACTER),
	//DEFINE_FIELD(CWeaponCustom, m_bCanFireUnderWater2, FIELD_CHARACTER),
	DEFINE_FIELD(CWeaponCustom, m_fAttackDelay1, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_fAttackDelay2, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_fAmmoRechargeDelay1, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_fAmmoRechargeDelay2, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_fReloadDelay, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_iUseAmmo1, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponCustom, m_iUseAmmo2, FIELD_INTEGER),
	//DEFINE_FIELD(CWeaponCustom, m_iMaxAmmo1, FIELD_INTEGER),
	//DEFINE_FIELD(CWeaponCustom, m_iMaxAmmo2, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponCustom, m_iMaxClip, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponCustom, m_iItemFlags, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponCustom, m_iWeight, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponCustom, m_iAnimationDeploy, FIELD_SHORT),
	DEFINE_FIELD(CWeaponCustom, m_iAnimationHolster, FIELD_SHORT),
	DEFINE_FIELD(CWeaponCustom, m_iAnimationFire1, FIELD_SHORT),
	DEFINE_FIELD(CWeaponCustom, m_iAnimationFire2, FIELD_SHORT),
	DEFINE_FIELD(CWeaponCustom, m_iAnimationReload, FIELD_SHORT),
	DEFINE_FIELD(CWeaponCustom, m_fProjectileSpeed1, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_fProjectileSpeed2, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_fDamage1, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_fDamage2, FIELD_FLOAT),
	DEFINE_FIELD(CWeaponCustom, m_iNumBullets1, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponCustom, m_iNumBullets2, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponCustom, CBasePlayerWeapon);

//-----------------------------------------------------------------------------
// Purpose: Fill provided structure with weapon info
// Input  : *p - 
// Output : Returns 1 on success, 0 on failure.
//-----------------------------------------------------------------------------
int CWeaponCustom::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was GetID();// = WEAPON_CUSTOM;
	p->iFlags = m_iItemFlags;// ITEM_FLAG_SELECTONEMPTY
	p->iMaxClip = m_iMaxClip;
	p->iWeight = m_iWeight;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = 7;
	p->iPosition = 0;
#endif
	//p->pszName = STRING(m_iszWeaponName);//STRING(pev->classname);
	if (FStringNull(m_iszAmmoName1))
	{
		p->pszAmmo1 = NULL;// must be NULL, not ""
#if defined (OLD_WEAPON_AMMO_INFO)
		p->iMaxAmmo1 = -1;
#endif
	}
	else
	{
		p->pszAmmo1 = STRING(m_iszAmmoName1);//m_szAmmoName1;
#if defined (OLD_WEAPON_AMMO_INFO)
		p->iMaxAmmo1 = m_iMaxAmmo1;
#endif
	}

	if (FStringNull(m_iszAmmoName2))
	{
		p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
		p->iMaxAmmo2 = -1;
#endif
	}
	else
	{
		p->pszAmmo2 = STRING(m_iszAmmoName2);//m_szAmmoName2;
#if defined (OLD_WEAPON_AMMO_INFO)
		p->iMaxAmmo2 = m_iMaxAmmo2;
#endif
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Get weapon name as it should be in the database
// Output : const char	* - overridden for weapon_custom
//-----------------------------------------------------------------------------
const char *CWeaponCustom::GetWeaponName(void) const
{
	return STRING(m_iszWeaponName);
}

//-----------------------------------------------------------------------------
// Purpose: Customization done by the map designer is processed here
// Output : pkvd->fHandled
//-----------------------------------------------------------------------------
void CWeaponCustom::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "attackdelay1"))
	{
		m_fAttackDelay1 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "attackdelay2"))
	{
		m_fAttackDelay2 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "AmmoRechargeDelay1"))
	{
		m_fAmmoRechargeDelay1 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "AmmoRechargeDelay2"))
	{
		m_fAmmoRechargeDelay2 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "reloaddelay"))
	{
		m_fReloadDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "useammo1"))
	{
		m_iUseAmmo1 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "useammo2"))
	{
		m_iUseAmmo2 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
#if defined (OLD_WEAPON_AMMO_INFO)
	else if (FStrEq(pkvd->szKeyName, "maxammo1"))
	{
		m_iMaxAmmo1 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "maxammo2"))
	{
		m_iMaxAmmo2 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
#endif
	else if (FStrEq(pkvd->szKeyName, "maxclip"))
	{
		m_iMaxClip = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weaponflags"))
	{
		m_iItemFlags = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weight"))
	{
		m_iWeight = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weaponname"))
	{
		m_iszWeaponName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ammo1"))
	{
		m_iszAmmoName1 = ALLOC_STRING(pkvd->szValue);
		//strcpy(m_szAmmoName1, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ammo2"))
	{
		m_iszAmmoName2 = ALLOC_STRING(pkvd->szValue);
		//strcpy(m_szAmmoName2, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "anim_deploy"))
	{
		m_iAnimationDeploy = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "anim_holster"))
	{
		m_iAnimationHolster = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "anim_fire1"))
	{
		m_iAnimationFire1 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "anim_fire2"))
	{
		m_iAnimationFire2 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "anim_reload"))
	{
		m_iAnimationReload = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "animext"))
	{
		m_iszPlayerAnimExt = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	/*else if (FStrEq(pkvd->szKeyName, "viewmodel"))// entvars_t
	{
		m_iszModelView = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}*/
	else if (FStrEq(pkvd->szKeyName, "projectile1"))
	{
		m_iszProjectile1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "projectile2"))
	{
		m_iszProjectile2 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "customprojectile1"))
	{
		m_iszCustomProjectile1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "customprojectile2"))
	{
		m_iszCustomProjectile2 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firetarget1"))
	{
		m_iszFireTarget1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firetarget2"))
	{
		m_iszFireTarget2 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "projectilespeed1"))
	{
		m_fProjectileSpeed1 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "projectilespeed2"))
	{
		m_fProjectileSpeed2 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage1"))
	{
		m_fDamage1 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage2"))
	{
		m_fDamage2 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "numbullets1"))
	{
		m_iNumBullets1 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "numbullets2"))
	{
		m_iNumBullets2 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBasePlayerWeapon::KeyValue(pkvd);
}

//-----------------------------------------------------------------------------
// Purpose: Verify all parameters, register in database, abort if something wrong
// Warning: classname is the same for all instances! GetWeaponName() fixes that.
//-----------------------------------------------------------------------------
void CWeaponCustom::Precache(void)
{
#if !defined (CLIENT_DLL)
	if (FBitSet(pev->spawnflags, SF_WEAPONCUSTOM_DUPLICATE))// XDM3038b: used to place copy of the same custom weapon on the map
	{
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->targetname))) != NULL)// find weapon_custom with the same targetname
		{
			if (pEntity == this)
				continue;
			if (FBitSet(pEntity->pev->spawnflags, SF_WEAPONCUSTOM_DUPLICATE))// Only copy data from ORIGINAL, not from another copy!
				continue;
			if (FClassnameIs(pEntity->pev, STRING(pev->classname)))// it IS a weapon_custom
			{
				conprintf(1, "CWeaponCustom::Precache() %s[%d] %s at (%g %g %g) is a copy, replacement found, removing.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
				CreateCopy(STRING(pEntity->pev->classname), pEntity, pEntity->pev->spawnflags, true);
				SetThink(&CBaseEntity::SUB_Remove);
				SetNextThink(0.0f);
				return;//break;
			}
		}
	}
#endif

	if (FStringNull(pev->model))
	{
		conprintf(0, "Error: %s[%d] %s at (%g %g %g) has no model! Removing.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		m_iId = WEAPON_NONE;
		SetBits(pev->flags, FL_KILLME);
		return;
	}

	if (FStringNull(pev->viewmodel))
	{
		conprintf(0, "Error: %s[%d] %s at (%g %g %g) has no view model! Removing.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
		m_iId = WEAPON_NONE;
		SetBits(pev->flags, FL_KILLME);
		return;
	}

	if (m_iId != WEAPON_CUSTOM1 && m_iId != WEAPON_CUSTOM2)// invalid ID was assigned
	{
		if (g_ItemInfoArray[WEAPON_CUSTOM1].iId == 0)// this slot is free
			m_iId = WEAPON_CUSTOM1;
		else if (g_ItemInfoArray[WEAPON_CUSTOM2].iId == 0)// this slot is free
			m_iId = WEAPON_CUSTOM2;
		else
		{
			conprintf(0, "Error: %s[d] %s at (%g %g %g) cannot assign ID! Removing.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
			m_iId = WEAPON_NONE;
			SetBits(pev->flags, FL_KILLME);
			return;
		}
	}

	if (FStringNull(m_iszWeaponName))
	{
		m_iszWeaponName = pev->classname;// reallocate?
	}
	else// validate!
	{
		for (size_t i=1; i<MAX_WEAPONS; ++i)
		{
			if (g_ItemInfoArray[i].szName[0] && strcmp(STRING(m_iszWeaponName), g_ItemInfoArray[i].szName) == 0)// this name is already taken
			{
				if (m_iId == WEAPON_CUSTOM1)
					m_iszWeaponName = MAKE_STRING("weapon_custom1");
				else if (m_iId == WEAPON_CUSTOM2)
					m_iszWeaponName = MAKE_STRING("weapon_custom2");
				else// should never get here
				{
					conprintf(0, "Error: %s[%d] %s at (%g %g %g) has wrong ID! Removing.\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z);
					m_iId = WEAPON_NONE;
					SetBits(pev->flags, FL_KILLME);
					return;
				}

				conprintf(0, "Error: %s[%d] %s at (%g %g %g) has non-unique weapon name! Renamed to %s\n", STRING(pev->classname), entindex(), STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z, STRING(m_iszWeaponName));
				break;
			}
		}
	}

	m_iszModelView = pev->viewmodel;// use this standard key
	m_iModelIndexWorld = pev->modelindex = PRECACHE_MODEL(STRINGV(pev->model));
	m_iModelIndexView = PRECACHE_MODEL(STRINGV(pev->viewmodel));

	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(STRINGV(pev->noise));
	if (!FStringNull(pev->noise1))
		PRECACHE_SOUND(STRINGV(pev->noise1));
	if (!FStringNull(pev->noise2))
		PRECACHE_SOUND(STRINGV(pev->noise2));
	if (!FStringNull(pev->noise3))
		PRECACHE_SOUND(STRINGV(pev->noise3));

	//	if (!FStringNull(m_iszCustomProjectile1))
	// This one precaches itself. BTW, this is a targetname

	if (!FStringNull(m_iszProjectile1))
		UTIL_PrecacheOther(STRING(m_iszProjectile1));
	if (!FStringNull(m_iszProjectile2))
		UTIL_PrecacheOther(STRING(m_iszProjectile2));

	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = m_fDamage1;

	CBasePlayerWeapon::Precache();// XDM3038
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustom::Spawn(void)
{
	SetClassName("weapon_custom");//pev->classname = MAKE_STRING("weapon_custom");// hack to allow old names

	CBasePlayerWeapon::Spawn();
	// XDM3038c: Precache();
	if (m_iId != WEAPON_CUSTOM1 && m_iId != WEAPON_CUSTOM2)
		SetBits(pev->flags, FL_KILLME);

	if (FBitSet(pev->flags, FL_KILLME))
		return;

	//if (FBitSet(pev->spawnflags, SF_NORESPAWN))// detect this flag set by level designer?
	// better expect it to be set explicitly 	SetBits(m_iItemFlags, ITEM_FLAG_IMPORTANT);

	SetBits(pev->spawnflags, SF_NORESPAWN);// UNDONE: CWeaponCustom cannot respawn right now (major problems with Create())

	// XDM3038c: SET_MODEL(edict(), STRING(pev->model));
	m_iszModelWorld = pev->model;
	// XDM3038c: UTIL_SetOrigin(this, pev->origin);
	// XDM3038c: UTIL_SetSize(this, VEC_HULL_ITEM_MIN, VEC_HULL_ITEM_MAX);

	//if (m_iDefaultAmmo <= 0)
	//	m_iDefaultAmmo = 10;
	if (m_fAttackDelay1 <= 0.0f)
		m_fAttackDelay1 = 0.5f;
	if (m_fAttackDelay2 <= 0.0f)
		m_fAttackDelay2 = 0.5f;

	if (FStringNull(m_iszPlayerAnimExt))
		m_iszPlayerAnimExt = MAKE_STRING("shotgun");

	//if (m_fProjectileSpeed1 <= 0.0f)// allow
	//	m_fProjectileSpeed1 = 1000.0f;

	// XDM3038c: Initialize();
	/*if (m_iAnimationDeploy == 0)// cannot be checked because 0 is valid index too
	m_iAnimationHolster
	m_iAnimationFire1
	m_iAnimationFire2
	m_iAnimationReload*/
}

//-----------------------------------------------------------------------------
// Purpose: Deploy the weapon
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCustom::Deploy(void)
{
	//m_pPlayer->pev->viewmodel = m_iszModelView;
	//m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	//return DefaultDeploy(STRING(m_iszModelView), STRING(m_iszModelWorld), m_iAnimationDeploy, STRING(m_iszPlayerAnimExt), STRING(pev->noise));
	return DefaultDeploy(m_iAnimationDeploy, STRING(m_iszPlayerAnimExt), STRING(pev->noise));
}

//-----------------------------------------------------------------------------
// Purpose: Put away the weapon
//-----------------------------------------------------------------------------
void CWeaponCustom::Holster(int skiplocal /* = 0 */)
{
	SendWeaponAnim(m_iAnimationHolster);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustom::PrimaryAttack(void)
{
	if (FBitSet(pev->spawnflags, SF_WEAPONCUSTOM_NOFIRE1))
		return;

	m_flNextPrimaryAttack = GetNextAttackDelay(m_fAttackDelay1);
	if ((m_iUseAmmo1 > 0 && !HasAmmo(AMMO_PRIMARY|AMMO_CLIP)) ||
		(FBitSet(pev->spawnflags, SF_WEAPONCUSTOM_NOFIREUW1) && (m_pPlayer->pev->waterlevel == WATERLEVEL_HEAD)))
	{
		PlayEmptySound();
		return;
	}

	UseAmmo(AMMO_PRIMARY, m_iUseAmmo1);
	SendWeaponAnim(m_iAnimationFire1);
	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, STRING(pev->noise1), VOL_NORM, ATTN_NORM);

	if (UseBullets(FIRE_PRI))
	{
		FireBullets(m_iNumBullets1, m_pPlayer->GetGunPosition(), m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES), VECTOR_CONE_5DEGREES, NULL, GetDefaultBulletDistance(), BULLET_NONE, m_fDamage1, this, GetDamageAttacker(), m_pPlayer->random_seed);
	}
	else if (!FStringNull(m_iszProjectile1) || !FStringNull(m_iszCustomProjectile1))
	{
		if (!FBitSet(pev->spawnflags, SF_WEAPONCUSTOM_SILENT1))
		{
#if !defined (CLIENT_DLL)
			CSoundEnt::InsertSound(bits_SOUND_COMBAT | bits_SOUND_DANGER, pev->origin, NORMAL_GUN_FLASH, 0.1);
#endif
			m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
			m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		}
		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 20.0f + gpGlobals->v_right * 2.0f;
		Vector vecVel(gpGlobals->v_forward);
		vecVel *= m_fProjectileSpeed1;

#if !defined (CLIENT_DLL)
		if (sv_overdrive.value > 0.0f)
			vecVel *= 10.0f;

		Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(vecAng);
		CBaseEntity *pProjectile;
		CBaseEntity *pAttacker = GetDamageAttacker();
		if (!FStringNull(m_iszCustomProjectile1))
			pProjectile = CreateProjectile(STRING(m_iszCustomProjectile1), vecSrc, vecAng, vecVel, pAttacker, pAttacker);
		else
			pProjectile = CBaseEntity::Create(STRING(m_iszProjectile1), vecSrc, vecAng, vecVel, pAttacker?pAttacker->edict():edict(), SF_NORESPAWN);// XDM3038: force no respawn

		if (pProjectile && m_fDamage1 > 0.0f)
			pProjectile->pev->dmg = m_fDamage1;// XDM3038
#endif
	}
#if !defined (CLIENT_DLL)
	if (!FStringNull(m_iszFireTarget1))
		FireTargets(STRING(m_iszFireTarget1), GetHost(), GetHost(), USE_TOGGLE, 1.0f);
#endif
	// player may be NULL now!
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + m_fAttackDelay1 + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCustom::SecondaryAttack(void)
{
	if (FBitSet(pev->spawnflags, SF_WEAPONCUSTOM_NOFIRE2))
		return;

	m_flNextSecondaryAttack = GetNextAttackDelay(m_fAttackDelay2);
	if ((m_iUseAmmo2 > 0 && !HasAmmo(AMMO_SECONDARY|AMMO_CLIP)) ||
		(FBitSet(pev->spawnflags, SF_WEAPONCUSTOM_SILENT1) && (m_pPlayer->pev->waterlevel == WATERLEVEL_HEAD)))
	{
		PlayEmptySound();
		return;
	}

	UseAmmo(AMMO_SECONDARY, m_iUseAmmo2);
	SendWeaponAnim(m_iAnimationFire2);
	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, STRING(pev->noise2), VOL_NORM, ATTN_NORM);

	if (UseBullets(FIRE_PRI))
	{
		FireBullets(m_iNumBullets2, m_pPlayer->GetGunPosition(), m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES), VECTOR_CONE_5DEGREES, NULL, GetDefaultBulletDistance(), BULLET_NONE, m_fDamage2, this, GetDamageAttacker(), m_pPlayer->random_seed);
	}
	else if (!FStringNull(m_iszProjectile2) || !FStringNull(m_iszCustomProjectile2))
	{
		if (!FBitSet(pev->spawnflags, SF_WEAPONCUSTOM_SILENT2))
		{
#if !defined (CLIENT_DLL)
			CSoundEnt::InsertSound(bits_SOUND_COMBAT | bits_SOUND_DANGER, pev->origin, NORMAL_GUN_FLASH, 0.1);
#endif
			m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
			m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		}
		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 20.0f + gpGlobals->v_right * 2.0f;
		Vector vecVel(gpGlobals->v_forward);
		vecVel *= m_fProjectileSpeed2;

#if !defined (CLIENT_DLL)
		if (sv_overdrive.value > 0.0f)
			vecVel *= 10.0f;

		Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(vecAng);
		CBaseEntity *pProjectile;
		CBaseEntity *pAttacker = GetDamageAttacker();
		if (!FStringNull(m_iszCustomProjectile2))
			pProjectile = CreateProjectile(STRING(m_iszCustomProjectile2), vecSrc, vecAng, vecVel, pAttacker, pAttacker);
		else
			pProjectile = CBaseEntity::Create(STRING(m_iszProjectile2), vecSrc, vecAng, vecVel, pAttacker?pAttacker->edict():edict(), SF_NORESPAWN);// XDM3038: force no respawn

		if (pProjectile && m_fDamage2 > 0.0f)
			pProjectile->pev->dmg = m_fDamage2;// XDM3038
#endif
	}
#if !defined (CLIENT_DLL)
	if (!FStringNull(m_iszFireTarget2))
		FireTargets(STRING(m_iszFireTarget2), GetHost(), GetHost(), USE_TOGGLE, 1.0f);
#endif
	// player may be NULL now!
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + m_fAttackDelay2 + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Reload
//-----------------------------------------------------------------------------
void CWeaponCustom::Reload(void)
{
	if (!HasAmmo(AMMO_PRIMARY))
		return;

	DefaultReload(m_iAnimationReload, m_fReloadDelay, STRING(pev->noise3));
}

//-----------------------------------------------------------------------------
// Purpose: Nothing is pressed
//-----------------------------------------------------------------------------
void CWeaponCustom::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	// Time to regenerate some ammo
	if (m_fAmmoRechargeDelay1 > 0 && gpGlobals->time >= m_flRechargeTime1)
	{
		m_pPlayer->GiveAmmo(1, PrimaryAmmoIndex());
		m_flRechargeTime1 = gpGlobals->time + m_fAmmoRechargeDelay1;
	}
	if (m_fAmmoRechargeDelay2 > 0 && gpGlobals->time >= m_flRechargeTime2)
	{
		m_pPlayer->GiveAmmo(1, SecondaryAmmoIndex());
		m_flRechargeTime2 = gpGlobals->time + m_fAmmoRechargeDelay2;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	pev->modelindex = m_iModelIndexView;// hack to get v_ model informaiton
	// UNDONE: TODO: LookupActivity(ACT_IDLE); and other activities
	// HACK: sequences
	void *pmodel = GET_MODEL_PTR(edict());// get viewmodel data
	int iAnim = -1;
	if (pmodel)
	{
		for (int i=0; i<GetSequenceCount(pmodel); ++i)
		{
			if (i != m_iAnimationDeploy &&
				i != m_iAnimationHolster &&
				i != m_iAnimationFire1 &&
				i != m_iAnimationFire2 &&
				i != m_iAnimationReload)
			{
				iAnim = i;// UNDONE: first unused sequence counts as idle
			}
		}
	}
	pev->modelindex = m_iModelIndexWorld;// restore normal model index ASAP

	if (iAnim >= 0)
		SendWeaponAnim(iAnim, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);// XDM3038a: fix typo
}

//-----------------------------------------------------------------------------
// Purpose: Does this fire mode use bullets instead of projectiles?
// Input  : *fireMode - FIRE_PRI
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCustom::UseBullets(const int &fireMode)
{
	if (fireMode == FIRE_PRI && m_iNumBullets1 > 0 && m_fDamage1 > 0.0f)
		return true;

	if (fireMode == FIRE_SEC && m_iNumBullets2 > 0 && m_fDamage2 > 0.0f)
		return true;

	return false;
}
