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
#include "game.h"
#include "gamerules.h"
#include "skill.h"
#include "soundent.h"

enum vanim_grenadelauncher_e
{
	GLAUNCHER_IDLE = 0,
	GLAUNCHER_FIDGET,
	GLAUNCHER_RELOAD,
	GLAUNCHER_FIRE,
	GLAUNCHER_HOLSTER,
	GLAUNCHER_DRAW
};

LINK_ENTITY_TO_CLASS(weapon_glauncher, CWeaponGrenadeLauncher);

void CWeaponGrenadeLauncher::Precache(void)
{
	m_iId = WEAPON_GLAUNCHER;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = GLAUNCHER_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_glauncher.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_glauncher.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgGrenadeLauncher;

	m_iShell = PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/g_launch1.wav");
	PRECACHE_SOUND("weapons/g_launch2.wav");
	PRECACHE_SOUND("weapons/glauncher_select.wav");
	m_usFire = PRECACHE_EVENT(1, "events/weapons/glauncher.sc");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponGrenadeLauncher::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_GLAUNCHER;
	p->iFlags = ITEM_FLAG_SUPERWEAPON;// XDM3035
	p->iMaxClip = GLAUNCHER_MAX_CLIP;
	p->iWeight = GLAUNCHER_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_GLAUNCHER;
	p->iPosition = POSITION_GLAUNCHER;
#endif
	p->pszAmmo1 = "lgrenades";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = GLAUNCHER_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponGrenadeLauncher::Deploy(void)
{
	return DefaultDeploy(GLAUNCHER_DRAW, "shotgun", "weapons/glauncher_select.wav");
}

void CWeaponGrenadeLauncher::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(GLAUNCHER_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponGrenadeLauncher::PrimaryAttack(void)
{
	Fire(FIRE_PRI);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(GLAUNCHER_ATTACK_INTERVAL1);
}

void CWeaponGrenadeLauncher::SecondaryAttack(void)
{
	Fire(FIRE_SEC);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(GLAUNCHER_ATTACK_INTERVAL2);
}

void CWeaponGrenadeLauncher::Fire(int firemode)
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
		return;
	}

#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward * GetBarrelLength() + gpGlobals->v_right * 2.0f;
	if (POINT_CONTENTS(vecSrc) == CONTENTS_SOLID)
	{
		PlayEmptySound();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}
#endif
	m_pPlayer->SetWeaponAnimType("shotgun");
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2f;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0f, g_vecZero, g_vecZero, 0.0, 0.0, GLAUNCHER_FIRE, m_iShell, pev->body, firemode);// SHELL_GLAUNCHER

#if !defined (CLIENT_DLL)
	Vector vecVel(gpGlobals->v_forward); vecVel *= GLAUNCHER_GRENADE_VELOCITY;
	if (sv_overdrive.value > 0.0f)
		vecVel *= 8.0f;

	CLGrenade::CreateGrenade(vecSrc, vecAng, vecVel, GetDamageAttacker(), GetDamageAttacker(), pev->dmg, GLAUNCHER_GRENADE_TIME, firemode == FIRE_PRI);
#endif
	--m_iClip;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
}

void CWeaponGrenadeLauncher::Reload(void)
{
	if (m_iClip > 0)
		return;

	DefaultReload(GLAUNCHER_RELOAD, GLAUNCHER_RELOAD_DELAY);
}

void CWeaponGrenadeLauncher::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 1;
	else
		rmax = 0;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
	if (r == 1)
		SendWeaponAnim(GLAUNCHER_FIDGET);
	else
		SendWeaponAnim(GLAUNCHER_IDLE);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 8, 12);
}

bool CWeaponGrenadeLauncher::CanAttack(const float &attack_time)
{
	if (m_iClip == 0)
		return false;
	if (m_fInReload)
		return false;

	return CBasePlayerWeapon::CanAttack(attack_time);
}
