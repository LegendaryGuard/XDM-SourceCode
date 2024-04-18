#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "globals.h"
#if !defined (CLIENT_DLL)
#include "hornet.h"
#endif

enum vanim_hgun_e
{
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

LINK_ENTITY_TO_CLASS(weapon_hornetgun, CWeaponHornetGun);

void CWeaponHornetGun::Precache(void)
{
	m_iId = WEAPON_HORNETGUN;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = HIVEHAND_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_hgun.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_hgun.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgHornet;

	PRECACHE_SOUND("weapons/hgun_select.wav");
	PRECACHE_SOUND("weapons/hgun_reload.wav");
	PRECACHE_SOUND("weapons/hgun_fire.wav");

	m_usHornetFire = PRECACHE_EVENT(1, "events/weapons/firehornet.sc");

	//UTIL_PrecacheOther("hornet");
	CBasePlayerWeapon::Precache();// XDM3038
}

bool CWeaponHornetGun::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
#if !defined (CLIENT_DLL)
		pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = MaxAmmoCarry(PrimaryAmmoIndex());//HORNET_MAX_CARRY;
#endif
		return true;
	}
	return false;
}

int CWeaponHornetGun::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_HORNETGUN;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iMaxClip = HORNETGUN_MAX_CLIP;
	p->iWeight = HORNETGUN_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_HORNETGUN;
	p->iPosition = POSITION_HORNETGUN;
#endif
	p->pszAmmo1 = "Hornets";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = HORNET_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponHornetGun::Deploy(void)
{
	return DefaultDeploy(HGUN_UP, "hive", "weapons/hgun_select.wav");
}

void CWeaponHornetGun::Holster(int skiplocal /* = 0 */)
{
	if (m_pPlayer)
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		SendWeaponAnim(HGUN_DOWN);
		m_pPlayer->GiveAmmo(1, PrimaryAmmoIndex());// XDM3038c: a little hack to make this weapon permanently selectable even on client side
	}
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

bool CWeaponHornetGun::IsUseable(void) const
{
	return true;
}

void CWeaponHornetGun::PrimaryAttack(void)
{
	Reload();
	if (!HasAmmo(AMMO_PRIMARY))
		return;

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usHornetFire, 0.0, g_vecZero, g_vecZero, 0.0f, 0.0f, HGUN_SHOOT, 0, pev->body, 0);

	int num = 1;
#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 2 + gpGlobals->v_up * -6.0f;
	if (sv_overdrive.value > 0.0f)
	{
		num = min(3, m_pPlayer->AmmoInventory(m_iPrimaryAmmoType));// fire 3 or everything left
		for (int i=0; i<num; ++i)
			CHornet::CreateHornet(vecSrc+gpGlobals->v_right*(((float)i-(float)num/2.0f)*2.0f), vecAng,
				m_pPlayer->pev->velocity*0.5f + gpGlobals->v_forward*HORNET_VELOCITY1 + gpGlobals->v_up*(((float)i-(float)num/2.0f)*10.0f), GetDamageAttacker(), GetDamageAttacker(), pev->dmg, true);
	}
	else
		CHornet::CreateHornet(vecSrc, vecAng, m_pPlayer->pev->velocity*0.5f + gpGlobals->v_forward*300.0f, GetDamageAttacker(), GetDamageAttacker(), pev->dmg, true);// XDM
#endif
	UseAmmo(AMMO_PRIMARY, num);
	m_flRechargeTime = gpGlobals->time + HGUN_RECHARGE_DELAY;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(HGUN_ATTACK_INTERVAL1);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CWeaponHornetGun::SecondaryAttack(void)
{
	Reload();

	if (!HasAmmo(AMMO_PRIMARY))
		return;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usHornetFire, 0.0, g_vecZero, g_vecZero, 0.0f, 0.0f, HGUN_SHOOT, 0, pev->body, 1);

#if !defined (CLIENT_DLL)
	Vector vecAng(m_pPlayer->pev->v_angle); vecAng += m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(vecAng);
	Vector vecSrc(m_pPlayer->GetGunPosition()); vecSrc += gpGlobals->v_forward * 16.0f + gpGlobals->v_right * 2.0f + gpGlobals->v_up * -8.0f;
	CHornet::CreateHornet(vecSrc, vecAng, m_pPlayer->pev->velocity * 0.5f + gpGlobals->v_forward * HORNET_VELOCITY2, GetDamageAttacker(), GetDamageAttacker(), pev->dmg, false);
#endif
	UseAmmo(AMMO_PRIMARY, 1);
	m_flRechargeTime = gpGlobals->time + HGUN_RECHARGE_DELAY;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(HGUN_ATTACK_INTERVAL2);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CWeaponHornetGun::Reload(void)
{
	if (m_pPlayer)
	{
		while (m_pPlayer->AmmoInventory(PrimaryAmmoIndex()) < MaxAmmoCarry(PrimaryAmmoIndex()) && m_flRechargeTime < gpGlobals->time)
		{
			m_pPlayer->GiveAmmo(1, PrimaryAmmoIndex());// XDM3038c
			EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/hgun_reload.wav", 0.5, ATTN_NORM);
			m_flRechargeTime += HGUN_RECHARGE_DELAY;
		}
	}
}

void CWeaponHornetGun::WeaponIdle(void)
{
	Reload();

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int rmax;
	if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
		rmax = 2;
	else
		rmax = 1;

	int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
	if (r == 2)
		SendWeaponAnim(HGUN_FIDGETSWAY, pev->body);
	else if (r == 1)
		SendWeaponAnim(HGUN_FIDGETSHAKE, pev->body);
	else// if (r == 0)
		SendWeaponAnim(HGUN_IDLE1, pev->body);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 2, 4);
}
