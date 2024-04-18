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
#include "globals.h"
#include "skill.h"
#if !defined (CLIENT_DLL)
#include "teleporter.h"
#endif

enum vanim_displacer_e
{
	DISPLACER_IDLE1 = 0,
	DISPLACER_IDLE2,
	DISPLACER_SPINUP,
	DISPLACER_SPIN,
	DISPLACER_FIRE,
	DISPLACER_DRAW,
	DISPLACER_HOLSTER
};

LINK_ENTITY_TO_CLASS(weapon_displacer, CWeaponDisplacer);

TYPEDESCRIPTION	CWeaponDisplacer::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponDisplacer, m_fireMode, FIELD_INTEGER),
	DEFINE_FIELD(CWeaponDisplacer, m_fireState, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponDisplacer, CBasePlayerWeapon);

void CWeaponDisplacer::Precache(void)
{
	m_iId = WEAPON_DISPLACER;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = DISPLACER_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_displacer.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_displacer.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgDisplacerBlast;

	//m_iSprite = PRECACHE_MODEL("sprites/eshock.spr");
	PRECACHE_SOUND("weapons/displacer_fire.wav");
	PRECACHE_SOUND("weapons/displacer_select.wav");
	PRECACHE_SOUND("weapons/displacer_spinup.wav");
	PRECACHE_SOUND("weapons/electro5.wav");// XDM3038
	m_usFire = PRECACHE_EVENT(1, "events/weapons/displacer.sc");
	//UTIL_PrecacheOther("teleporter");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponDisplacer::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_DISPLACER;
	p->iFlags = ITEM_FLAG_NOAUTORELOAD|ITEM_FLAG_SUPERWEAPON;// XDM3038c
	p->iMaxClip = DISPLACER_MAX_CLIP;
	p->iWeight = DISPLACER_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_DISPLACER;
	p->iPosition = POSITION_DISPLACER;
#endif
	p->pszAmmo1 = "uranium";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponDisplacer::Deploy(void)
{
	m_fireState = FIRE_OFF;
	return DefaultDeploy(DISPLACER_DRAW, "gauss", "weapons/displacer_select.wav");
}

void CWeaponDisplacer::Holster(int skiplocal /* = 0 */)
{
	KillFX();
	m_fireState = FIRE_OFF;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(DISPLACER_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

bool CWeaponDisplacer::CanHolster(void) const
{
	if (m_fireState != FIRE_OFF)
		return false;

	return true;
}

bool CWeaponDisplacer::IsUseable(void) const
{
	if (m_fireState != FIRE_OFF)// XDM3038a: don't put away while firing!
		return true;

	if (m_pPlayer->AmmoInventory(m_iPrimaryAmmoType) < min(DISPLACER_AMMO_USE1, DISPLACER_AMMO_USE2))// minimum required amount
		return false;

	return CBasePlayerWeapon::IsUseable();
}

void CWeaponDisplacer::PrimaryAttack(void)
{
	if (m_fireState != FIRE_OFF)
		return;

	m_fireMode = FIRE_PRI;
	StartFire();
}

void CWeaponDisplacer::SecondaryAttack(void)
{
	if (m_fireState != FIRE_OFF)
		return;

	m_fireMode = FIRE_SEC;
	StartFire();
}

void CWeaponDisplacer::StartFire(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD || !HasAmmo(AMMO_PRIMARY, (m_fireMode == FIRE_PRI)?DISPLACER_AMMO_USE1:DISPLACER_AMMO_USE2))
	{
		m_fireState = FIRE_OFF;
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
		return;
	}
	m_fireState = FIRE_CHARGE;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	//origin, angles, DISPLACER_ANIM_TIME, scale, m_fireMode, m_fireState, spriteIndex, fps
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, DISPLACER_ANIM_TIME, 0.3, DISPLACER_SPINUP, g_iModelIndexLaser, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));
	// All of these will be overwritten by ShootBall():
	pev->teleport_time = UTIL_WeaponTimeBase() + DISPLACER_ANIM_TIME;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(DISPLACER_ANIM_TIME + 0.25f);
	m_pPlayer->m_flNextAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;// WeaponIdle() must be called for autoaim to work!
	//m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.05f;
}

void CWeaponDisplacer::ShootBall(void)
{
	pev->teleport_time = -1;
	m_fireState = FIRE_ON;
	bool success = false;
	if (m_fireMode == FIRE_PRI)
		success = (UseAmmo(AMMO_PRIMARY, DISPLACER_AMMO_USE1) == DISPLACER_AMMO_USE1);
	else
		success = (UseAmmo(AMMO_PRIMARY, DISPLACER_AMMO_USE2) == DISPLACER_AMMO_USE2);

#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle);
	Vector vecSrc(m_pPlayer->GetGunPosition());
	if (success)
	{
		vecAng += m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(vecAng);
		vecSrc += gpGlobals->v_forward * GetBarrelLength() + gpGlobals->v_right * 2.0f + gpGlobals->v_up * -8.0f;
		if (POINT_CONTENTS(vecSrc) == CONTENTS_SOLID)
			success = false;
	}
#endif
	if (!success)
	{
		//STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spinup.wav");
		KillFX();
		PlayEmptySound();
		m_fireState = FIRE_OFF;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		return;
	}
	m_pPlayer->SetWeaponAnimType("gauss");
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_COMBAT;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2f;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_fire.wav", VOL_NORM, ATTN_NORM);
	//SendWeaponAnim(DISPLACER_FIRE);
	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, DISPLACER_ATTACK_INTERVAL, 0.0f, DISPLACER_FIRE, 0, pev->body, BitMerge2x4bit(m_fireMode, m_fireState));

#if !defined (CLIENT_DLL)
	Vector vecVel;
	float k = 1.0f;
	if (m_fireMode == FIRE_PRI)
	{
		k = TELEPORTER_FLY_SPEED1;
		vecVel = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		k = TELEPORTER_FLY_SPEED2;
		vecVel = gpGlobals->v_forward;
		m_pPlayer->m_hAutoaimTarget = NULL;// is it safe?
	}
	vecVel *= k;// .SetLength(k);?
	CTeleporter::CreateTeleporter(vecSrc, vecVel, GetDamageAttacker(), GetDamageAttacker(), m_pPlayer->m_hAutoaimTarget, pev->dmg, TELEPORTER_LIFE, m_fireMode);// XDM3038c
#endif
	m_pPlayer->ResetAutoaim();
	m_pPlayer->m_flNextAttack = m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(DISPLACER_ATTACK_INTERVAL);
}

void CWeaponDisplacer::WeaponIdle(void)
{
	if (m_fireState == FIRE_CHARGE)
	{
		if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)// Charging: got into water
		{
			//STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spinup.wav");
			KillFX();
			//PlayEmptySound();
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/electro5.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(95,105));
			//SendWeaponAnim(DISPLACER_IDLE1);
#if !defined (CLIENT_DLL)
			RadiusDamage(pev->origin, this, GetDamageAttacker(), pev->dmg, 128, CLASS_NONE, DMG_SHOCK | DMG_NEVERGIB);
			UTIL_ScreenFade(m_pPlayer, Vector(160,160,255), 2.0, 0.5, 128, 0);// FFADE_IN
#endif
			m_fireState = FIRE_OFF;
			pev->teleport_time = -1;
			m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		}
		else if (pev->teleport_time < UTIL_WeaponTimeBase())// Fully charged
		{
			ShootBall();
		}
		else// Charging: update target!
		{
			if (m_fireMode == FIRE_PRI)// primary fire turns autoaim on while charging
				m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.25f;
		}
		return;
	}
	else if (m_fireState == FIRE_ON)
	{
		m_pPlayer->ResetAutoaim();
		m_fireState = FIRE_OFF;
	}
	//else
	//	KillFX();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,1) == 0)
		SendWeaponAnim(DISPLACER_IDLE1, pev->body);
	else
		SendWeaponAnim(DISPLACER_IDLE2, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CWeaponDisplacer::KillFX(void)
{
	STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spinup.wav");// XDM3037
}

//-----------------------------------------------------------------------------
// Purpose: Server-side only. Pack data.
// Warning: Weapon classes must call their parent's functions!
// Input  : *player - receiver
//			*weapondata - pack data into this structure
//-----------------------------------------------------------------------------
void CWeaponDisplacer::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	weapondata->iuser2 = m_fireState;
}
