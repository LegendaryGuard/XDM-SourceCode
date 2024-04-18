#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "weaponslots.h"
#include "player.h"
#include "gamerules.h"

enum vanim_shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

LINK_ENTITY_TO_CLASS(weapon_shotgun, CWeaponShotgun);

TYPEDESCRIPTION	CWeaponShotgun::m_SaveData[] =
{
	DEFINE_FIELD(CWeaponShotgun, m_flNextReload, FIELD_TIME),
	DEFINE_FIELD(CWeaponShotgun, m_flPumpTime, FIELD_TIME),
};
IMPLEMENT_SAVERESTORE(CWeaponShotgun, CBasePlayerWeapon);

void CWeaponShotgun::Precache(void)
{
	m_iId = WEAPON_SHOTGUN;
	if (m_iDefaultAmmo == 0)
		m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;
	if (FStringNull(pev->model))// XDM3038a
		pev->model = MAKE_STRING("models/p_shotgun.mdl");
	if (FStringNull(pev->viewmodel))// XDM3038a
		pev->viewmodel = MAKE_STRING("models/v_shotgun.mdl");
	if (pev->dmg == 0)// XDM3038c: custom damage
		pev->dmg = gSkillData.DmgBuckshot;

	PRECACHE_SOUND("weapons/reload1.wav");// shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav");// shotgun reload
	PRECACHE_SOUND("weapons/shotgun_fire1.wav");
	PRECACHE_SOUND("weapons/shotgun_fire2.wav");
	PRECACHE_SOUND("weapons/shotgun_pump.wav");
	PRECACHE_SOUND("weapons/shotgun_select.wav");

	//m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell
	m_iShell = PRECACHE_MODEL("models/shell.mdl");// XDM3035a: model limit
	m_usFire1 = PRECACHE_EVENT(1, "events/weapons/shotgun1.sc");
	m_usFire2 = PRECACHE_EVENT(1, "events/weapons/shotgun2.sc");

	CBasePlayerWeapon::Precache();// XDM3038
}

int CWeaponShotgun::GetItemInfo(ItemInfo *p)
{
	p->iId = GetID();// XDM3038a: was m_iId = WEAPON_SHOTGUN;
	p->iFlags = 0;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iWeight = SHOTGUN_WEIGHT;
#if defined (SERVER_WEAPON_SLOTS)
	p->iSlot = SLOT_SHOTGUN;
	p->iPosition = POSITION_SHOTGUN;
#endif
	p->pszAmmo1 = "buckshot";
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
#endif
	p->pszAmmo2 = NULL;
#if defined (OLD_WEAPON_AMMO_INFO)
	p->iMaxAmmo2 = -1;
#endif
	return 1;
}

bool CWeaponShotgun::Deploy(void)
{
	return DefaultDeploy(SHOTGUN_DRAW, "shotgun", "weapons/shotgun_select.wav");
}

void CWeaponShotgun::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(SHOTGUN_HOLSTER);
	CBasePlayerWeapon::Holster(skiplocal);// XDM3035
}

void CWeaponShotgun::PrimaryAttack(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		if (m_iClip == 0)
			PlayEmptySound();

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + DEFAULT_ATTACK_INTERVAL;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	Vector vecAiming(m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire1, 0.0, g_vecZero, g_vecZero, 0,0/*vecDir.x, vecDir.y*/, SHOTGUN_FIRE, m_iShell, pev->body, SHELL_BUCKSHOT/*shell body*/);
	m_iClip -= SHOTGUN_USE_AMMO1;
	/*Vector vecDir(*/FireBullets(SHOTGUN_USE_AMMO1*SHOTGUN_PELLETS_PER_AMMO, m_pPlayer->GetGunPosition(), vecAiming, VECTOR_CONE_3DEGREES, NULL, GetDefaultBulletDistance()/2, BULLET_BUCKSHOT, pev->dmg, this, GetDamageAttacker(), m_pPlayer->random_seed);
	//if (m_pPlayer == NULL)// XDM3038c
	//	return;

	if (m_iClip > 0)
	{
		m_flPumpTime = UTIL_WeaponTimeBase() + 0.35;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
	}
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;

	m_fInSpecialReload = 0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SHOTGUN_PUMP_TIME1);
}

void CWeaponShotgun::SecondaryAttack(void)
{
	if (m_pPlayer->pev->waterlevel >= WATERLEVEL_HEAD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}

	if (m_iClip <= 1)
	{
		Reload();
		if (m_iClip == 0)
			PlayEmptySound();

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	Vector vecAiming(m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFire2, 0.0, g_vecZero, g_vecZero, 0,0/*vecDir.x, vecDir.y*/, SHOTGUN_FIRE2, m_iShell, pev->body, SHELL_BUCKSHOT/*shell body*/);
	m_iClip -= SHOTGUN_USE_AMMO2;
	m_pPlayer->pev->velocity -= gpGlobals->v_forward * SHOTGUN_RECOIL2;// XDM
	/*Vector vecDir = */FireBullets(SHOTGUN_USE_AMMO2*SHOTGUN_PELLETS_PER_AMMO, m_pPlayer->GetGunPosition(), vecAiming, VECTOR_CONE_5DEGREES, NULL, GetDefaultBulletDistance()/2, BULLET_BUCKSHOT, pev->dmg, this, GetDamageAttacker(), m_pPlayer->random_seed);
	//if (m_pPlayer == NULL)// XDM3038c
	//	return;

	if (m_iClip > 0)
	{
		m_flPumpTime = UTIL_WeaponTimeBase() + 0.6f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0f;
	}
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;

	m_fInSpecialReload = 0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(SHOTGUN_PUMP_TIME2);
}

void CWeaponShotgun::Reload(void)
{
	if (m_flNextReload > UTIL_WeaponTimeBase())
		return;

	if (!HasAmmo(AMMO_PRIMARY) || m_iClip >= SHOTGUN_MAX_CLIP)//if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip >= SHOTGUN_MAX_CLIP)
		return;

	if (m_fInSpecialReload == 0)// check to see if we're ready to reload
	{
		if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())// don't reload until recoil is done
			return;

		SendWeaponAnim(SHOTGUN_START_RELOAD);
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flNextReload = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.4;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/reload1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(90,105));
		else
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/reload3.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(90,105));

		SendWeaponAnim(SHOTGUN_RELOAD);
		m_fInSpecialReload = 2;
		m_flNextReload = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + SHOTGUN_RELOAD_DELAY;
		m_pPlayer->m_flNextAttack = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + SHOTGUN_RELOAD_DELAY+0.05f;//0.3;
	}
	else
	{
		m_iClip++;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
		m_flNextReload = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + SHOTGUN_RELOAD_DELAY;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6f;
}

void CWeaponShotgun::WeaponIdle(void)
{
	if (m_flPumpTime > 0 && m_flPumpTime <= UTIL_WeaponTimeBase())
	{
		SendWeaponAnim(SHOTGUN_PUMP);
		//EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/shotgun_pump.wav", VOL_NORM, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
		return;
	}

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_iClip == 0 && m_fInSpecialReload == 0 && HasAmmo(AMMO_PRIMARY))//m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		Reload();
	}
	else if (m_fInSpecialReload != 0)
	{
		if (m_iClip != SHOTGUN_MAX_CLIP && HasAmmo(AMMO_PRIMARY))//m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
		{
			Reload();
		}
		else
		{
			SendWeaponAnim(SHOTGUN_PUMP);
			//EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/shotgun_pump.wav", VOL_NORM, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;
		}
	}
	else if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase())
	{
		int rmax;
		if (UTIL_WeaponTimeBase() - m_flLastAttackTime >= WEAPON_LONG_IDLE_TIME)
			rmax = 2;
		else
			rmax = 1;

		int r = UTIL_SharedRandomLong(m_pPlayer->random_seed, 0,rmax);
		if (r == 2)
			SendWeaponAnim(SHOTGUN_IDLE_DEEP, pev->body);
		else if (r == 1)
			SendWeaponAnim(SHOTGUN_IDLE4, pev->body);
		else // if (r == 0)
			SendWeaponAnim(SHOTGUN_IDLE, pev->body);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}
}
