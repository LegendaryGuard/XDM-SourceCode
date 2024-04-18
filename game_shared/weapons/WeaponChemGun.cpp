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
#if !defined (CLIENT_DLL)
#include "lightp.h"
#endif

enum vanim_chemgun_e
{
	CHEMGUN_DRAW = 0,
	CHEMGUN_HOLSTER,
	CHEMGUN_IDLE,
	CHEMGUN_FIDGET,
	CHEMGUN_FIREMODE,
	CHEMGUN_SHOOT,
};

LINK_ENTITY_TO_CLASS(weapon_chemgun, CWeaponChemGun);

TYPEDESCRIPTION	CWeaponChemGun::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponChemGun, m_fireMode, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CWeaponChemGun, CBasePlayerWeapon);

void CWeaponChemGun::Precache(void)
{
	m_iId = WEAPON_CHEMGUN;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = CHEMGUN_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_chemgun.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_chemgun.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgChemgun;

	PRECACHE_SOUND("weapons/chemgun_fire1.wav");
	PRECACHE_SOUND("weapons/chemgun_fire2.wav");
	PRECACHE_SOUND("weapons/chemgun_select.wav");
	PRECACHE_SOUND("weapons/chemgun_switch.wav");

	m_usFire = PRECACHE_EVENT(1, "events/weapons/firechemgun.sc");

	//UTIL_PrecacheOther("lightp");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponChemGun::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_CHEMGUN;
	p->iFlags = 0;
	p->iMaxClip = CHEMGUN_MAX_CLIP;
	p->iWeight = CHEMGUN_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_CHEMGUN;
	p->iPosition = POSITION_CHEMGUN;
#endif
	p->pszAmmo1 = "lightp";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = LP_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponChemGun::Deploy(void)
{
	//return DefaultDeploy("models/v_chemgun.mdl", "models/p_chemgun.mdl", CHEMGUN_DRAW, "hive", "weapons/chemgun_select.wav");
	return DefaultDeploy(CHEMGUN_DRAW, "hive", "weapons/chemgun_select.wav");
}

void CWeaponChemGun::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(CHEMGUN_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponChemGun::PrimaryAttack(void)
{
	// don't fire underwater
	if ((m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD) || !HasAmmo(AMMO_ANYTYPE))
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + CHEMGUN_ATTACK_INTERVAL1;
		return;
	}

	m_pPlayer->SetWeaponAnimType("hive");
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	/*m_pPlayer->pev->weaponanim = CHEMGUN_SHOOT;
	SetBits(m_pPlayer->pev->effects, EF_MUZZLEFLASH);*/
	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, 0.0f, 0.0f, CHEMGUN_SHOOT, 0, pev->body, m_fireMode);

#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward + gpGlobals->v_right * 2.0f;
	CLightProjectile::CreateLP(vecSrc, vecAng, gpGlobals->v_forward, GetDamageAttacker(), GetDamageAttacker(), pev->dmg, m_fireMode);
#endif // !CLIENT_DLL
	UseAmmo(AMMO_PRIMARY, 1);

	if (m_fireMode == FIRE_PRI)
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(CHEMGUN_ATTACK_INTERVAL1);
	else
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(CHEMGUN_ATTACK_INTERVAL2);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
}

void CWeaponChemGun::SecondaryAttack(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/chemgun_switch.wav", VOL_NORM, ATTN_NORM, 0, 100);
	SendWeaponAnim(CHEMGUN_FIREMODE);

	if (m_fireMode == FIRE_PRI)
		m_fireMode = FIRE_SEC;
	else
		m_fireMode = FIRE_PRI;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + CHEMGUN_SWITCH_INTERVAL;
}

void CWeaponChemGun::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 1;
	else
		rmax = 0;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0, rmax);
	if (r == 1)
		SendWeaponAnim(CHEMGUN_FIDGET, pev->body);
	else // if (r == 0)
		SendWeaponAnim(CHEMGUN_IDLE, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CWeaponChemGun::ClientPackData(struct edict_s *player, struct weapon_data_s *weapondata)
{
	CBasePlayerWeapon::ClientPackData(player, weapondata);
	weapondata->iuser1 = m_fireMode;
	//weapondata->iuser2 = m_fireState;
}
