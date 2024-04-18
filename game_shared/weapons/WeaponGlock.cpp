#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "globals.h"
#include "skill.h"

enum vanim_glock_e
{
	GLOCK_IDLE1_LONG = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS(weapon_glock, CWeaponGlock);
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CWeaponGlock);

void CWeaponGlock::Spawn(void)
{
	SetClassName("weapon_9mmhandgun");// hack to allow for old names
	CBasePlayerWeapon::Spawn();// XDM3038a
}

void CWeaponGlock::Precache(void)
{
	m_iId = WEAPON_GLOCK;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_9mmhandgun.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_9mmhandgun.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.Dmg9MM;

	m_iShell = PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/pl_gun3.wav");
	PRECACHE_SOUND("weapons/glock_reload.wav");
	PRECACHE_SOUND("weapons/glock_select.wav");
	m_usFire = PRECACHE_EVENT(1, "events/weapons/glock.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponGlock::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_GLOCK;
	p->iFlags = 0;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iWeight = GLOCK_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_GLOCK;
	p->iPosition = POSITION_GLOCK;
#endif
	p->pszAmmo1 = "9mm";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponGlock::Deploy(void)
{
	return DefaultDeploy(GLOCK_DRAW, "onehanded", "weapons/glock_select.wav");
}

void CWeaponGlock::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
	SendWeaponAnim(GLOCK_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponGlock::PrimaryAttack(void)
{
	Fire(0.01f*m_pPlayer->GetShootSpreadFactor(), GLOCK_ATTACK_INTERVAL1, TRUE);
}

void CWeaponGlock::SecondaryAttack(void)
{
	Fire(0.1f*m_pPlayer->GetShootSpreadFactor(), GLOCK_ATTACK_INTERVAL2, FALSE);
}

void CWeaponGlock::Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		if (!HasAmmo(AMMO_ANYTYPE))
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;
		}
		return;
	}
	if (UseAmmo(AMMO_CLIP, 1) == 0)
		return;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (pev->body == 1)// silenced
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire, 0.0, g_vecZero, g_vecZero, 0,0, (m_iClip == 0?GLOCK_SHOOT_EMPTY:GLOCK_SHOOT), m_iShell, pev->body, fUseAutoAim);// XDM
	/*Vector vecDir = */FireBullets(1, m_pPlayer->GetGunPosition(), m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES), Vector(flSpread, flSpread, flSpread), NULL, GetDefaultBulletDistance(), BULLET_9MM, pev->dmg, this, GetDamageAttacker(), m_pPlayer->random_seed);
	if (m_pPlayer == NULL)// XDM3038c: happened
		return;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5, 15);
}

void CWeaponGlock::Reload(void)
{
	DefaultReload((m_iClip == 0)?GLOCK_RELOAD:GLOCK_RELOAD_NOT_EMPTY, GLOCK_RELOAD_DELAY, "weapons/glock_reload.wav");
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CWeaponGlock::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iClip > 0)// only idle if the slid isn't back
	{
		int rmax;
		if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
			rmax = 2;
		else
			rmax = 1;

		int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
		if (r == 2)
			SendWeaponAnim(GLOCK_IDLE1_LONG, pev->body, UseDecrement());
		else if (r == 1)
			SendWeaponAnim(GLOCK_IDLE3, pev->body, UseDecrement());
		else // if (r == 0)
			SendWeaponAnim(GLOCK_IDLE2, pev->body, UseDecrement());

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	}
}
