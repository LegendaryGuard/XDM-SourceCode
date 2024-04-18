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

enum vanim_strtarget_e
{
	STRTARGET_DRAW = 0,
	STRTARGET_IDLE,
	STRTARGET_FIRE,
	STRTARGET_HOLSTER
};

LINK_ENTITY_TO_CLASS(weapon_strtarget, CWeaponStrikeTarget);

void CWeaponStrikeTarget::Precache(void)
{
	m_iId = WEAPON_STRTARGET;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = STRTARGET_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_target.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_target.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgStrikeTarget;

	PRECACHE_SOUND("weapons/strtarget_select.wav");
	//UTIL_PrecacheOther("strtarget");
	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponStrikeTarget::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_STRTARGET;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE | ITEM_FLAG_SUPERWEAPON;// XDM3035
	p->iMaxClip = STRTARGET_MAX_CLIP;
	p->iWeight = STRTARGET_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_STRTARGET;
	p->iPosition = POSITION_STRTARGET;
#endif
	p->pszAmmo1 = "strtarget";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = STRTARGET_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponStrikeTarget::Deploy(void)
{
	pev->button = 0;// XDM3038: pev->impulse conflicts with CBasePlayerWeapon
	m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + STRTARGET_ATTACK_INTERVAL1;
	return DefaultDeploy(STRTARGET_DRAW, "trip", "weapons/strtarget_select.wav");
}

void CWeaponStrikeTarget::Holster(int skiplocal /* = 0 */)
{
	pev->button = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if (HasAmmo(AMMO_PRIMARY))//if (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) > 0)
		SendWeaponAnim(STRTARGET_HOLSTER);

	//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", 0.1, ATTN_NORM);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponStrikeTarget::PrimaryAttack(void)
{
	if (UseAmmo(AMMO_PRIMARY, 1) < 1)
		return;

	SendWeaponAnim(STRTARGET_FIRE);
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	CBaseEntity *pMine = Create("strtarget", m_pPlayer->GetGunPosition() + gpGlobals->v_forward * GetBarrelLength(), vecAng, m_pPlayer->pev->velocity + gpGlobals->v_forward * 300.0f, GetDamageAttacker()->edict(), SF_NORESPAWN);
	if (pMine)
	{
		pMine->pev->scale = pev->scale;
		pMine->pev->owner = GetDamageAttacker()->edict();// XDM3037: collision hack
		pMine->pev->dmg = pev->dmg;// XDM3038c
	}
#endif
	m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(STRTARGET_ATTACK_INTERVAL1);
	pev->button = 1;// play "draw" animaiton instead of "idle" for the first time

	if (!HasAmmo(AMMO_PRIMARY))
		RetireWeapon();
}

void CWeaponStrikeTarget::SecondaryAttack(void)
{
	// TODO: attach to a grenade launcher :D
}

void CWeaponStrikeTarget::WeaponIdle(void)
{
	if ((m_flTimeWeaponIdle > UTIL_WeaponTimeBase()) || !HasAmmo(AMMO_PRIMARY))
		return;

	if (pev->button == 1)
	{
		SendWeaponAnim(STRTARGET_DRAW);
		pev->button = 0;
	}
	else
		SendWeaponAnim(STRTARGET_IDLE);

	m_pPlayer->SetWeaponAnimType("trip");
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}
