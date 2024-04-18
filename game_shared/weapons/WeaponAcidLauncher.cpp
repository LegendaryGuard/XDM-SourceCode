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
#include "skill.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"

enum vanim_acidlauncher_e
{
	ALAUNCHER_DRAW = 0,
	ALAUNCHER_IDLE,
	ALAUNCHER_FIDGET,
	ALAUNCHER_FIRE,
	ALAUNCHER_CHARGE,
	ALAUNCHER_HOLSTER
};

LINK_ENTITY_TO_CLASS(weapon_alauncher, CWeaponAcidLauncher);

TYPEDESCRIPTION	CWeaponAcidLauncher::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponAcidLauncher, m_fireState, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponAcidLauncher, CBasePlayerWeapon);

void CWeaponAcidLauncher::Precache(void)
{
	m_iId = WEAPON_ALAUNCHER;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = ALAUNCHER_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_alauncher.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_alauncher.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgAcidGrenade;

	PRECACHE_SOUND("weapons/a_launch1.wav");
	PRECACHE_SOUND("weapons/a_launch2.wav");
	PRECACHE_SOUND("weapons/alauncher_select.wav");
	PRECACHE_SOUND("weapons/alauncher_charge.wav");
	m_usFire = PRECACHE_EVENT(1, "events/weapons/alauncher.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponAcidLauncher::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_ALAUNCHER;
	p->iFlags = 0;
	p->iMaxClip = ALAUNCHER_MAX_CLIP;
	p->iWeight = ALAUNCHER_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_ALAUNCHER;
	p->iPosition = POSITION_ALAUNCHER;
#endif
	p->pszAmmo1 = "agrenades";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = ALAUNCHER_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponAcidLauncher::CanHolster(void) const
{
	if (m_iClip > 0)
		return false;

	return true;
}

void CWeaponAcidLauncher::Holster(int skiplocal /* = 0 */)
{
	if (m_iClip > 0)
		Fire();

	m_iClip = 0;
	m_fireState = FIRE_OFF;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(ALAUNCHER_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

bool CWeaponAcidLauncher::Deploy(void)
{
	//bool ret = DefaultDeploy("models/v_alauncher.mdl", "models/p_alauncher.mdl", ALAUNCHER_DRAW, "gauss", "weapons/alauncher_select.wav");
	bool ret = DefaultDeploy(ALAUNCHER_DRAW, "gauss", "weapons/alauncher_select.wav");
	if (ret)
	{
		m_flNextAmmoBurn = 0.0;
		if (m_iClip > 0)
		{
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] += m_iClip;
			m_iClip = 0;
		}
	}
	return ret;
}

void CWeaponAcidLauncher::PrimaryAttack(void)
{
	if (m_fireState == FIRE_OFF)// XDM3035: don't interrupt charging
	{
		if (HasAmmo(AMMO_PRIMARY))//if (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) > 0)
		{
			m_iClip = 1;
			UseAmmo(AMMO_PRIMARY, 1);
			Fire();
		}
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_pPlayer->m_flNextAttack = GetNextAttackDelay(ALAUNCHER_ATTACK_INTERVAL1);
		}
	}
}

void CWeaponAcidLauncher::SecondaryAttack(void)
{
	if (m_fireState == FIRE_OFF)
	{
		if (HasAmmo(AMMO_PRIMARY))//if (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) > 0)
		{
			m_fireState = FIRE_CHARGE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		}
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + ALAUNCHER_ATTACK_INTERVAL2;
		}
	}
	else if (m_fireState == FIRE_CHARGE)
	{
		if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
		{
			EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/alauncher_charge.wav", VOL_NORM, ATTN_NORM);
			SendWeaponAnim(ALAUNCHER_CHARGE);
			m_fireState = FIRE_ON;
			m_flNextAmmoBurn = UTIL_WeaponTimeBase();// + 0.05; single alt-clik
		}
	}
	else if (m_fireState == FIRE_ON)
	{
		if (m_flNextAmmoBurn > 0.0 && m_flNextAmmoBurn <= UTIL_WeaponTimeBase())
		{
			UseAmmo(AMMO_PRIMARY, 1);
			m_iClip++;

			if (m_iClip >= iMaxClip() || !HasAmmo(AMMO_PRIMARY))//m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) <= 0)
				m_flNextAmmoBurn = 0.0f;
			else
				m_flNextAmmoBurn = UTIL_WeaponTimeBase() + ALAUNCHER_AMMO_USE_DELAY;
		}
	}
}

void CWeaponAcidLauncher::Fire(void)
{
	if (m_iClip <= 0)
	{
		m_fireState = FIRE_OFF;
		return;
	}
	m_pPlayer->SetWeaponAnimType("gauss");
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	//m_pPlayer->pev->weaponanim = ALAUNCHER_FIRE;// XDM3037: does this do anything?
	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags | FEV_RELIABLE, m_pPlayer->edict(), m_usFire, 0.0f, g_vecZero, g_vecZero, 0.0, 0.0, ALAUNCHER_FIRE, g_iModelIndexAcidDrip, m_iClip, RANDOM_LONG(0,1));

#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward*10.0f + gpGlobals->v_right*4.0f - gpGlobals->v_up*12.0f;
	CSoundEnt::InsertSound(bits_SOUND_COMBAT | bits_SOUND_DANGER, pev->origin, NORMAL_GUN_FLASH, 0.1);
	/*if (m_iClip <= 1 && sv_overdrive.value > 0.0f)
	{
		for (int i=0; i<2; ++i)
		{
		oh, screw this	CAGrenade::ShootTimed(vecSrc + gpGlobals->v_right*RANDOM_FLOAT(-3,3) + RandomVector()*3.0f, m_pPlayer->pev->v_angle,
				gpGlobals->v_forward*1090.0f + gpGlobals->v_right*RANDOM_FLOAT(-10,10) + gpGlobals->v_up*RANDOM_FLOAT(4,6),
				m_pPlayer, 5, 0);
		}
	}*/
	CAGrenade::ShootTimed(vecSrc, vecAng, gpGlobals->v_forward*ALAUNCHER_GRENADE_VELOCITY+gpGlobals->v_up*5.0f, GetDamageAttacker(), GetDamageAttacker(), ALAUNCHER_GRENADE_TIME+sv_overdrive.value, pev->dmg, (m_iClip-1));
#endif
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2f;

	m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(((m_iClip > 1)?ALAUNCHER_ATTACK_INTERVAL2:ALAUNCHER_ATTACK_INTERVAL1));
	m_iClip = 0;
	m_fireState = FIRE_OFF;
	m_flNextAmmoBurn = 0.0f;
}

void CWeaponAcidLauncher::WeaponIdle(void)
{
	if (m_fireState != FIRE_OFF)
	{
		Fire();
		return;
	}

	if (m_iClip > 0 && m_fireState != FIRE_ON)// this may happen
		m_iClip = 0;// ExtractClipAmmo(this) can cause unforeseen consequences

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)// Getting the REAL animation delay requires GET_MODEL_PTR() which cannot be used for viewmodel
		rmax = 1;
	else
		rmax = 0;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
	if (r == 1)
		SendWeaponAnim(ALAUNCHER_FIDGET, pev->body);
	else
		SendWeaponAnim(ALAUNCHER_IDLE, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 4, 8);
}

void CWeaponAcidLauncher::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	//weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fireState;
}
