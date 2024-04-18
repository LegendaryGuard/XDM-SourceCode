//-----------------------------------------------------------------------------
// X-Half-Life code
// Copyright (c) 2001-2017
//-----------------------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "soundent.h"
#include "skill.h"
#if !defined (CLIENT_DLL)
#include "razordisk.h"
#endif

enum vanim_disklauncher_e
{
	DLAUNCHER_IDLE = 0,
	DLAUNCHER_DRAW,
	DLAUNCHER_FIRE1,
	DLAUNCHER_FIRE2,
	DLAUNCHER_HOLSTER
};

LINK_ENTITY_TO_CLASS(weapon_dlauncher, CWeaponDiskLauncher);

void CWeaponDiskLauncher::Precache(void)
{
	m_iId = WEAPON_DLAUNCHER;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = DLAUNCHER_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_razordisk.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_razordisk.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgRazorDisk;

	m_usFire = PRECACHE_EVENT(1, "events/weapons/disklauncher.sc");

	PRECACHE_SOUND("weapons/razordisk_fire.wav");
	PRECACHE_SOUND("weapons/razordisk_select.wav");
	//UTIL_PrecacheOther("razordisk");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponDiskLauncher::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();
	p->iFlags = ITEM_FLAG_NOAUTORELOAD;
	p->iMaxClip = DLAUNCHER_MAX_CLIP;
	p->iWeight = DLAUNCHER_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_DLAUNCHER;
	p->iPosition = POSITION_DLAUNCHER;
#endif
	p->pszAmmo1 = "blades";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = BLADES_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponDiskLauncher::Deploy(void)
{
	pev->button = 0;// XDM3038: pev->impulse conflicts with CBasePlayerWeapon
	m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + DLAUNCHER_ATTACK_INTERVAL1;
	return DefaultDeploy(DLAUNCHER_DRAW, "hive", "weapons/razordisk_select.wav");
}

void CWeaponDiskLauncher::Holster(int skiplocal /* = 0 */)
{
	pev->button = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	pev->body = HasAmmo(AMMO_PRIMARY)?0:1;
	SendWeaponAnim(DLAUNCHER_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);
}

void CWeaponDiskLauncher::PrimaryAttack(void)
{
	Fire(RAZORDISK_REFLECT, DLAUNCHER_USE_AMMO1, DLAUNCHER_FIRE1, DLAUNCHER_ATTACK_INTERVAL1);
}

void CWeaponDiskLauncher::SecondaryAttack(void)
{
	Fire(RAZORDISK_EXPLODE, DLAUNCHER_USE_AMMO2, DLAUNCHER_FIRE2, DLAUNCHER_ATTACK_INTERVAL2);
}

void CWeaponDiskLauncher::Fire(int diskmode, int ammouse, int animation, float delay)
{
	if (!HasAmmo(AMMO_PRIMARY, ammouse))
	{
		PlayEmptySound();
		return;
	}
#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward * 12.0f + gpGlobals->v_right * 2.0f + gpGlobals->v_up * -8.0f;
	if (POINT_CONTENTS(vecSrc) == CONTENTS_SOLID)
	{
		PlayEmptySound();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}
#endif

	//SendWeaponAnim(animation);
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	if (UseAmmo(AMMO_PRIMARY, ammouse) == ammouse)
	{
#if !defined (CLIENT_DLL)
		CRazorDisk::DiskCreate(vecSrc, vecAng, gpGlobals->v_forward, GetDamageAttacker(), GetDamageAttacker(), pev->dmg, diskmode);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_FLASH, 0.1);
#endif
		int evflags;
#if defined(CLIENT_WEAPONS)
		evflags = FEV_NOTHOST;
#else
		evflags = 0;
#endif
		pev->body = HasAmmo(AMMO_PRIMARY)?0:1;// body without disk
		PLAYBACK_EVENT_FULL(evflags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, delay, 0.0, animation, diskmode==0?RANDOM_LONG(PITCH_NORM,PITCH_NORM+4):RANDOM_LONG(PITCH_NORM-6,PITCH_NORM-4), pev->body, diskmode);
		m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(delay);
	}
	if (!HasAmmo(AMMO_PRIMARY))
		RetireWeapon();
}

void CWeaponDiskLauncher::WeaponIdle(void)
{
	if ((m_flTimeWeaponIdle > UTIL_WeaponTimeBase()) || !HasAmmo(AMMO_PRIMARY))
		return;

	pev->body = HasAmmo(AMMO_PRIMARY)?0:1;
	SendWeaponAnim(DLAUNCHER_IDLE);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}
